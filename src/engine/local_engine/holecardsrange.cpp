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
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/

#include "holecardsrange.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace
{

// Rank character -> value 0..12 (0 = two, 12 = ace), matching the card encoding
// of the rest of the engine code.
int rankFromChar(char c)
{
	switch(std::toupper(static_cast<unsigned char>(c))) {
	case '2':
		return 0;
	case '3':
		return 1;
	case '4':
		return 2;
	case '5':
		return 3;
	case '6':
		return 4;
	case '7':
		return 5;
	case '8':
		return 6;
	case '9':
		return 7;
	case 'T':
		return 8;
	case 'J':
		return 9;
	case 'Q':
		return 10;
	case 'K':
		return 11;
	case 'A':
		return 12;
	default:
		return -1;
	}
}

enum Suitedness { SUITED_BOTH, SUITED_ONLY, OFFSUIT_ONLY };

struct HandSpec {
	int high;
	int low;
	Suitedness suitedness;

	bool isPair() const
	{
		return high == low;
	}
};

// Reads "AA", "AKs", "AKo" or "AK" (then both variants).
bool parseHand(const std::string& text, HandSpec& spec)
{
	if(text.size() < 2 || text.size() > 3) return false;

	const int a = rankFromChar(text[0]);
	const int b = rankFromChar(text[1]);
	if(a < 0 || b < 0) return false;

	spec.high = std::max(a, b);
	spec.low = std::min(a, b);
	spec.suitedness = SUITED_BOTH;

	if(text.size() == 3) {
		const char s = static_cast<char>(std::toupper(static_cast<unsigned char>(text[2])));
		if(s == 'S') spec.suitedness = SUITED_ONLY;
		else if(s == 'O') spec.suitedness = OFFSUIT_ONLY;
		else return false;
		// A pair can be restricted to neither suited nor offsuit.
		if(spec.isPair()) return false;
	}

	return true;
}

std::string trim(const std::string& text)
{
	std::string::size_type begin = 0;
	std::string::size_type end = text.size();
	while(begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
	while(end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
	return text.substr(begin, end - begin);
}

}

HoleCardsRange::HoleCardsRange()
{
	clear();
}

HoleCardsRange::HoleCardsRange(const std::string& notation)
{
	clear();
	add(notation);
}

void HoleCardsRange::clear()
{
	std::memset(myTypes, 0, sizeof(myTypes));
}

bool HoleCardsRange::empty() const
{
	for(int i = 0; i < 169; ++i) {
		if(myTypes[i]) return false;
	}
	return true;
}

int HoleCardsRange::typeIndex(int cardA, int cardB)
{
	if(cardA < 0 || cardA > 51 || cardB < 0 || cardB > 51 || cardA == cardB) return -1;

	const int rankA = cardA % 13;
	const int rankB = cardB % 13;
	const bool suited = (cardA / 13) == (cardB / 13);

	if(rankA == rankB) return rankA;

	const int high = std::max(rankA, rankB);
	const int low = std::min(rankA, rankB);
	const int offset = high * (high - 1) / 2 + low;

	return suited ? 13 + offset : 13 + 78 + offset;
}

bool HoleCardsRange::contains(int cardA, int cardB) const
{
	const int index = typeIndex(cardA, cardB);
	return index >= 0 && myTypes[index];
}

double HoleCardsRange::combinationShare() const
{
	int combinations = 0;
	for(int index = 0; index < 169; ++index) {
		if(!myTypes[index]) continue;
		if(index < 13) combinations += 6;            // Paar
		else if(index < 13 + 78) combinations += 4;  // gleichfarbig
		else combinations += 12;                     // ungleichfarbig
	}
	return combinations / 1326.0;
}

bool HoleCardsRange::add(const std::string& notation)
{
	// Marks one concrete starting hand type.
	auto mark = [this](int high, int low, Suitedness suitedness) {
		if(high == low) {
			myTypes[high] = true;
			return;
		}
		const int offset = high * (high - 1) / 2 + low;
		if(suitedness != OFFSUIT_ONLY) myTypes[13 + offset] = true;
		if(suitedness != SUITED_ONLY) myTypes[13 + 78 + offset] = true;
	};

	bool allParsed = true;

	std::string::size_type pos = 0;
	while(pos <= notation.size()) {
		const std::string::size_type comma = notation.find(',', pos);
		const std::string token = trim(notation.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos));
		pos = (comma == std::string::npos) ? notation.size() + 1 : comma + 1;

		if(token.empty()) continue;

		// "77+", "A9s+" -- everything from this hand upwards
		if(token[token.size() - 1] == '+') {
			HandSpec spec;
			if(!parseHand(token.substr(0, token.size() - 1), spec)) {
				allParsed = false;
				continue;
			}
			if(spec.isPair()) {
				for(int rank = spec.high; rank <= 12; ++rank) mark(rank, rank, SUITED_BOTH);
			} else {
				// The high card stays put, the low card grows towards it.
				for(int low = spec.low; low < spec.high; ++low) mark(spec.high, low, spec.suitedness);
			}
			continue;
		}

		// "22-55", "KTo-K7o" -- Spanne
		const std::string::size_type dash = token.find('-', 1);
		if(dash != std::string::npos) {
			HandSpec from, to;
			if(!parseHand(trim(token.substr(0, dash)), from) || !parseHand(trim(token.substr(dash + 1)), to)) {
				allParsed = false;
				continue;
			}
			if(from.isPair() != to.isPair()) {
				allParsed = false;
				continue;
			}
			if(from.isPair()) {
				const int lowEnd = std::min(from.high, to.high);
				const int highEnd = std::max(from.high, to.high);
				for(int rank = lowEnd; rank <= highEnd; ++rank) mark(rank, rank, SUITED_BOTH);
			} else {
				// Only meaningful if the high card and the suitedness stay the same.
				if(from.high != to.high || from.suitedness != to.suitedness) {
					allParsed = false;
					continue;
				}
				const int lowEnd = std::min(from.low, to.low);
				const int highEnd = std::max(from.low, to.low);
				for(int low = lowEnd; low <= highEnd; ++low) mark(from.high, low, from.suitedness);
			}
			continue;
		}

		// Einzelne Hand
		HandSpec spec;
		if(!parseHand(token, spec)) {
			allParsed = false;
			continue;
		}
		mark(spec.high, spec.low, spec.suitedness);
	}

	return allParsed;
}
