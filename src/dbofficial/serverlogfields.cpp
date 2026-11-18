/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *****************************************************************************/
/* Filter columns of server_log, taken from the text of a log line. */

#include <dbofficial/serverlogfields.h>
#include <boost/asio/ip/address.hpp>
#include <regex>

using namespace std;

// The patterns follow the wording of the LOG_* calls on the server; a line
// which does not use it simply leaves the column NULL. They run in the
// database thread, so their cost never reaches a network handler.
namespace
{
// Player "X", player="X", player "X", the start ACK lines of a game, which
// name the player as ACK from "X" / ACK from: "X", and rejected guest names.
const regex g_playerNameRe("(?:[Pp]layer=? ?|ACK from:? |name )\"([^\"]*)\"");
// Admin actions name the account without quotes, followed by its player_id:
// "Player sp0ck(36) removes game ...", "Global notice by sp0ck(36): ...". For a
// ban the line is about the banned account - its address is the one in the
// line - so that one is taken instead of the admin.
const regex g_bannedPlayerRe("bans player (.+)\\(([0-9]+)\\) who has IP ");
const regex g_adminPlayerRe("^(?:Player|Global notice by) ([^\"()]+)\\(([0-9]+)\\)[: ]");
const regex g_noticeRejectedRe("^Global notice rejected: player (\\S+) \\(dbId ([0-9]+)");
// "session #12", "Session 12 (", "Session 12 -"; not "Sessions".
const regex g_sessionRe("[Ss]ession #?([0-9]+)");
// "game 41", "Game 41 -", "game=41".
const regex g_gameRe("\\b[Gg]ame[ =]([0-9]+)");
// "(id:382" of the connect line, and the lines which name a player only by
// that number: "Player 382 offline", "kicks player id 382".
const regex g_playerUidRe("\\(id:([0-9]+)|\\b[Pp]layer (?:id )?([0-9]+)\\b");
const regex g_playerIdRe("dbId:([0-9]+)");
// Candidates only, every one is checked by the address parser.
const regex g_ipCandidateRe("[0-9A-Fa-f:.]{3,}");

// Player names are chosen by players: "Game 42" or "1.2.3.4" as a name must
// not be read as a game number or an address. The numbers are searched in a
// copy with the quoted names blanked out. Only the names, not every quoted
// part: the server does not escape a quote inside a name, and pairing all
// quotes of the line would then blank the rest of it.
string
BlankNames(const string &text)
{
	string out(text);
	for (sregex_iterator i(text.begin(), text.end(), g_playerNameRe), end; i != end; ++i) {
		const ssub_match &name = (*i)[1];
		for (auto pos = name.first - text.begin(); pos < name.second - text.begin(); ++pos)
			out[pos] = ' ';
	}
	return out;
}

string
FirstGroup(const string &text, const regex &re)
{
	smatch m;
	if (!regex_search(text, m, re))
		return string();
	for (size_t i = 1; i < m.size(); ++i) {
		if (m[i].matched)
			return m[i].str();
	}
	return string();
}

// Numbers outside the column range are worth less than no value at all.
string
UnsignedColumn(const string &digits)
{
	if (digits.empty() || digits.size() > 9)
		return string();
	return to_string(stoul(digits));
}

string
FirstIp(const string &text)
{
	for (sregex_iterator i(text.begin(), text.end(), g_ipCandidateRe), end; i != end; ++i) {
		string candidate = i->str();
		// The end of a sentence or a "host:port" list separator.
		while (!candidate.empty() && (candidate.back() == '.' || candidate.back() == ':'))
			candidate.pop_back();
		if (candidate.find_first_of(".:") == string::npos)
			continue;
		boost::system::error_code ec;
		boost::asio::ip::address addr = boost::asio::ip::make_address(candidate, ec);
		if (ec || addr.is_unspecified())
			continue;
		// Dual stack sockets report IPv4 clients as ::ffff:a.b.c.d; stored as
		// plain IPv4, one filter finds the address in either spelling.
		if (addr.is_v6() && addr.to_v6().is_v4_mapped())
			addr = boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped, addr.to_v6());
		return addr.to_string();
	}
	return string();
}
} // namespace

ServerLogFields
ParseServerLogFields(const string &message)
{
	ServerLogFields fields;
	const string blanked = BlankNames(message);

	fields.sessionNo = UnsignedColumn(FirstGroup(blanked, g_sessionRe));
	fields.gameId = UnsignedColumn(FirstGroup(blanked, g_gameRe));
	fields.playerUid = UnsignedColumn(FirstGroup(blanked, g_playerUidRe));
	fields.playerId = UnsignedColumn(FirstGroup(blanked, g_playerIdRe));
	// dbId 0 is a guest - no account.
	if (fields.playerId == "0")
		fields.playerId.clear();
	fields.ip = FirstIp(blanked);

	// The name from the original text, of course.
	string name = FirstGroup(message, g_playerNameRe);
	if (name.empty()) {
		smatch m;
		if (regex_search(message, m, g_bannedPlayerRe) || regex_search(message, m, g_adminPlayerRe)
				|| regex_search(message, m, g_noticeRejectedRe)) {
			name = m[1].str();
			if (fields.playerId.empty())
				fields.playerId = UnsignedColumn(m[2].str());
			if (fields.playerId == "0")
				fields.playerId.clear();
		}
	}
	fields.playerName = SanitizeServerLogText(name, 64);
	if (fields.playerName == "(unknown)")
		fields.playerName.clear();
	return fields;
}

string
SanitizeServerLogText(const string &text, size_t maxChars)
{
	static const char replacement[] = "\xEF\xBF\xBD";
	string out;
	out.reserve(text.size());
	size_t chars = 0;
	size_t i = 0;
	while (i < text.size() && chars < maxChars) {
		const unsigned char c = static_cast<unsigned char>(text[i]);
		size_t len = 0;
		unsigned min = 0;
		unsigned cp = 0;
		if (c < 0x80) {
			out.push_back((c < 0x20 && c != '\t' && c != '\n') || c == 0x7F ? ' ' : static_cast<char>(c));
			++i;
			++chars;
			continue;
		} else if ((c & 0xE0) == 0xC0) {
			len = 2;
			min = 0x80;
			cp = c & 0x1F;
		} else if ((c & 0xF0) == 0xE0) {
			len = 3;
			min = 0x800;
			cp = c & 0x0F;
		} else if ((c & 0xF8) == 0xF0) {
			len = 4;
			min = 0x10000;
			cp = c & 0x07;
		}
		bool valid = len != 0 && i + len <= text.size();
		for (size_t k = 1; valid && k < len; ++k) {
			const unsigned char cc = static_cast<unsigned char>(text[i + k]);
			if ((cc & 0xC0) != 0x80)
				valid = false;
			else
				cp = (cp << 6) | (cc & 0x3F);
		}
		// Overlong forms, surrogates and values beyond Unicode are invalid too.
		if (valid && (cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)))
			valid = false;
		if (valid) {
			out.append(text, i, len);
			i += len;
		} else {
			out.append(replacement);
			++i;
		}
		++chars;
	}
	return out;
}
