/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2026 Felix Hammer, Florian Thauer, Lothar May          *
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

// pokerth_globalnotice - admin tool: logs in briefly with an admin account
// on the server, sends an AdminGlobalNoticeMessage and disconnects
// again. The server distributes the announcement as a global chat to all clients.
//
// The flow corresponds to the GUI client (see net/clientstate.cpp):
//   TCP (+ TLS) -> AnnounceMessage -> InitMessage (authenticatedLogin,
//   password in plain text inside the TLS tunnel) -> InitAckMessage
//   -> AdminGlobalNoticeMessage -> AdminGlobalNoticeAckMessage.

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/program_options.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include <third_party/protobuf/pokerth.pb.h>
#include <net/netpacket.h>
#include <net/downloadhelper.h>
#include <net/tlspinning.h>
#include <serverdata.h>
#include <game_defs.h>

#include <QCoreApplication>
#include <QDomDocument>
#include <QFile>
#include <QTemporaryDir>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>
#ifndef _WIN32
#include <termios.h>
#endif

using namespace std;
namespace po = boost::program_options;
namespace ssl = boost::asio::ssl;
using boost::asio::ip::tcp;

namespace
{

// Default from the ConfigFile ("InternetServerListAddress"), so that this tool
// uses the same server list as the client.
const char *const DEFAULT_SERVERLIST_URL = "pokerth.net/serverlist.xml.z";
const unsigned DEFAULT_PROTOBUF_PORT = 7236;
const size_t RECV_BUF_SIZE = 4096;

// Exit-Codes.
const int EXIT_USAGE = 1;
const int EXIT_NETWORK = 2;
const int EXIT_REJECTED = 3;

string
errorReasonToString(ErrorMessage::ErrorReason reason)
{
	switch (reason) {
	case ErrorMessage::initVersionNotSupported:
		return "client version not supported by the server";
	case ErrorMessage::initServerFull:
		return "server is full";
	case ErrorMessage::initAuthFailure:
		return "authentication failed (wrong user name or password)";
	case ErrorMessage::initPlayerNameInUse:
		return "player name already in use";
	case ErrorMessage::initInvalidPlayerName:
		return "invalid player name";
	case ErrorMessage::initServerMaintenance:
		return "server is in maintenance mode";
	case ErrorMessage::initBlocked:
		return "login blocked (rate limit / brute force protection)";
	case ErrorMessage::avatarTooLarge:
		return "avatar too large";
	case ErrorMessage::invalidPacket:
		return "invalid packet";
	case ErrorMessage::invalidState:
		return "invalid state";
	case ErrorMessage::kickedFromServer:
		return "kicked from server";
	case ErrorMessage::bannedFromServer:
		return "banned from server";
	case ErrorMessage::blockedByServer:
		return "blocked by server";
	case ErrorMessage::sessionTimeout:
		return "session timeout";
	default:
		return "unknown error";
	}
}

bool
parseOnOff(const string &value, bool &result)
{
	string v;
	for (string::const_iterator i = value.begin(); i != value.end(); ++i)
		v += static_cast<char>(tolower(*i));

	if (v == "on" || v == "1" || v == "true" || v == "yes") {
		result = true;
		return true;
	}
	if (v == "off" || v == "0" || v == "false" || v == "no") {
		result = false;
		return true;
	}
	return false;
}

// Password entry without an echo: the admin password should end up neither in the
// shell history nor (via --password) in the process list.
string
promptPassword(const string &prompt)
{
	string password;
	cout << prompt << flush;
#ifndef _WIN32
	termios oldTerm;
	bool restore = false;
	if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &oldTerm) == 0) {
		termios newTerm = oldTerm;
		newTerm.c_lflag &= ~static_cast<tcflag_t>(ECHO);
		restore = tcsetattr(STDIN_FILENO, TCSAFLUSH, &newTerm) == 0;
	}
#endif
	getline(cin, password);
#ifndef _WIN32
	if (restore)
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldTerm);
#endif
	cout << endl;
	return password;
}

bool
confirm(const string &question)
{
	if (!isatty(STDIN_FILENO))
		return true; // Non-interactive (script/cron): no confirmation possible.
	cout << question << " [y/N] " << flush;
	string answer;
	getline(cin, answer);
	return answer == "y" || answer == "Y" || answer == "yes";
}

//-----------------------------------------------------------------------------
// Serverliste
//-----------------------------------------------------------------------------

// Loads the server list via the same DownloadHelper as the client (it sets the
// user agent "PokerTH/2.0 (Qt Network)" that the attack filter of pokerth.net
// lets through) and unpacks it with zlib - .z is a normal zlib stream,
// produced by zlib_compress or create_serverlist.sh.
vector<ServerInfo>
downloadServerList(const string &url)
{
	QTemporaryDir tmpDir;
	if (!tmpDir.isValid())
		throw runtime_error("could not create a temporary directory for the server list download");

	string fileName;
	const size_t slashPos = url.find_last_of('/');
	if (slashPos != string::npos && slashPos + 1 < url.length())
		fileName = url.substr(slashPos + 1);
	if (fileName.empty())
		fileName = "serverlist.xml.z";

	const QString targetPath = tmpDir.filePath(QString::fromStdString(fileName));

	DownloadHelper downloader;
	downloader.Init(url, targetPath.toStdString());
	// Process() waits internally in a Qt event loop (safety timeout 30s) and
	// only returns true once the transfer is finished. The counter limits the
	// total waiting time in case the finished signal fails to appear.
	int rounds = 0;
	while (!downloader.Process()) {
		if (++rounds > 3)
			throw runtime_error("timeout while downloading the server list from " + url);
	}

	QFile downloadedFile(targetPath);
	if (!downloadedFile.open(QIODevice::ReadOnly))
		throw runtime_error("could not read the downloaded server list");
	const QByteArray rawData = downloadedFile.readAll();
	downloadedFile.close();

	QByteArray xmlData;
	if (fileName.size() > 2 && fileName.compare(fileName.size() - 2, 2, ".z") == 0) {
		try {
			istringstream inStream(string(rawData.constData(), rawData.size()));
			ostringstream outStream;
			boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
			in.push(boost::iostreams::zlib_decompressor());
			in.push(inStream);
			boost::iostreams::copy(in, outStream);
			const string decompressed(outStream.str());
			xmlData = QByteArray(decompressed.data(), static_cast<int>(decompressed.size()));
		} catch (const std::exception &e) {
			throw runtime_error(string("could not decompress the server list: ") + e.what());
		}
	} else {
		xmlData = rawData;
	}

	QDomDocument xmlDoc;
	if (!xmlDoc.setContent(xmlData))
		throw runtime_error("could not parse the server list XML");

	// Field names and defaults as in ClientStateReadingServerList::Enter().
	vector<ServerInfo> servers;
	QDomElement nextServer = xmlDoc.documentElement().firstChildElement("Server");
	while (!nextServer.isNull()) {
		ServerInfo serverInfo;
		serverInfo.id = static_cast<unsigned>(nextServer.attribute("id").toInt());

		const QDomElement nameNode = nextServer.firstChildElement("Name");
		const QDomElement countryNode = nextServer.firstChildElement("Country");
		const QDomElement addr4Node = nextServer.firstChildElement("IPv4Address");
		const QDomElement tlsNode = nextServer.firstChildElement("TLS");
		const QDomElement portNode = nextServer.firstChildElement("ProtobufPort");

		if (!addr4Node.isNull() && !portNode.isNull()) {
			serverInfo.name = nameNode.isNull() ? string() : nameNode.attribute("value").toStdString();
			serverInfo.ipv4addr = addr4Node.attribute("value").toStdString();
			serverInfo.port = portNode.attribute("value").toInt();
			if (!countryNode.isNull())
				serverInfo.country = countryNode.attribute("value").toStdString();
			// A missing TLS entry means TLS on (the default of ServerInfo).
			if (!tlsNode.isNull()) {
				bool useTls = true;
				if (parseOnOff(tlsNode.attribute("value").toStdString(), useTls))
					serverInfo.useTLS = useTls;
			}
			// Pinned server keys as in the GUI client, so that a
			// key change via the server list arrives here as well.
			for (QDomElement pinNode = nextServer.firstChildElement("TLSPin");
					!pinNode.isNull();
					pinNode = pinNode.nextSiblingElement("TLSPin")) {
				const string tlsPin = pinNode.attribute("value").toStdString();
				if (!tlsPin.empty())
					serverInfo.tlsPins.push_back(tlsPin);
			}
			servers.push_back(serverInfo);
		}
		nextServer = nextServer.nextSiblingElement("Server");
	}

	if (servers.empty())
		throw runtime_error("the server list does not contain any usable server entry");

	return servers;
}

//-----------------------------------------------------------------------------
// Verbindung
//-----------------------------------------------------------------------------

class NoticeClient
{
public:
	NoticeClient(bool useTls, unsigned timeoutSec)
		: m_sslCtx(ssl::context::sslv23_client),
		  m_stream(m_io, m_sslCtx),
		  m_useTls(useTls),
		  m_timeout(chrono::seconds(timeoutSec)),
		  m_recvBuf(RECV_BUF_SIZE, 0),
		  m_recvBufUsed(0)
	{
		// As in the GUI client: the server certificates are self-signed. The check is
		// therefore not against a CA but against the built-in pin
		// of the server - see Connect(), where the target host is known.
		m_sslCtx.set_verify_mode(ssl::verify_none);
		m_sslCtx.set_options(
			ssl::context::default_workarounds |
			ssl::context::no_sslv2 |
			ssl::context::no_sslv3 |
			ssl::context::no_tlsv1 |
			ssl::context::no_tlsv1_1);
	}

	~NoticeClient()
	{
		Disconnect();
	}

	void Connect(const string &host, unsigned port, const vector<string> &serverListPins = vector<string>())
	{
		tcp::resolver resolver(m_io);
		boost::system::error_code ec;
		const tcp::resolver::results_type endpoints = resolver.resolve(host, to_string(port), ec);
		if (ec)
			throw runtime_error("could not resolve " + host + ": " + ec.message());

		bool done = false;
		boost::system::error_code opEc;
		boost::asio::async_connect(
			m_stream.lowest_layer(), endpoints,
		[&done, &opEc](const boost::system::error_code &e, const tcp::endpoint &) {
			done = true;
			opEc = e;
		});
		if (!RunWithTimeout(done))
			throw runtime_error("timeout while connecting to " + host);
		if (opEc)
			throw runtime_error("could not connect to " + host + ": " + opEc.message());

		m_stream.lowest_layer().set_option(tcp::no_delay(true), ec);

		if (m_useTls) {
			// This tool logs in with an admin password, an
			// unauthenticated channel would be particularly expensive here.
			vector<string> pins(TlsPinning::GetBuiltinPins(host));
			for (const string &listPin : serverListPins) {
				if (find(pins.begin(), pins.end(), listPin) == pins.end())
					pins.push_back(listPin);
			}
			const TlsPinning::ReportFunc reportMismatch = [](const string &msg) {
				cerr << msg << endl;
			};
			if (!TlsPinning::ApplyPins(m_stream, pins, reportMismatch))
				cerr << "warning: no pinned key for " << host
					 << " - the connection is encrypted, but the server is not authenticated." << endl;

			done = false;
			m_stream.async_handshake(
				ssl::stream_base::client,
			[&done, &opEc](const boost::system::error_code &e) {
				done = true;
				opEc = e;
			});
			if (!RunWithTimeout(done))
				throw runtime_error("timeout during TLS handshake");
			if (opEc)
				throw runtime_error("TLS handshake failed: " + opEc.message());
		}
	}

	void Disconnect()
	{
		// No SSL shutdown: that waits synchronously for the close_notify of the
		// peer and can hang. For a short-lived tool
		// closing the TCP socket is enough.
		boost::system::error_code ec;
		if (m_stream.lowest_layer().is_open()) {
			m_stream.lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
			m_stream.lowest_layer().close(ec);
		}
	}

	void Send(const boost::shared_ptr<NetPacket> &packet)
	{
		// Framing as in AsioSendBuffer::InternalStorePacket: 4 bytes of length
		// (network byte order) followed by the serialized message.
		const uint32_t packetSize = static_cast<uint32_t>(packet->GetMsg()->ByteSizeLong());
		vector<google::protobuf::uint8> buf(packetSize + NET_HEADER_SIZE);
		const uint32_t netSize = htonl(packetSize);
		memcpy(buf.data(), &netSize, sizeof(netSize));
		packet->GetMsg()->SerializeWithCachedSizesToArray(&buf[NET_HEADER_SIZE]);

		boost::system::error_code ec;
		if (m_useTls)
			boost::asio::write(m_stream, boost::asio::buffer(buf), ec);
		else
			boost::asio::write(m_stream.next_layer(), boost::asio::buffer(buf), ec);
		if (ec)
			throw runtime_error("send failed: " + ec.message());
	}

	// Returns the next complete packet or throws on a timeout/error.
	boost::shared_ptr<NetPacket> Receive()
	{
		const chrono::steady_clock::time_point deadline = chrono::steady_clock::now() + m_timeout;

		for (;;) {
			boost::shared_ptr<NetPacket> packet = ExtractPacket();
			if (packet)
				return packet;

			const chrono::milliseconds remaining =
				chrono::duration_cast<chrono::milliseconds>(deadline - chrono::steady_clock::now());
			if (remaining.count() <= 0)
				throw runtime_error("timeout while waiting for a server response");

			if (m_recvBufUsed >= m_recvBuf.size())
				throw runtime_error("receive buffer overflow (protocol error)");

			bool done = false;
			boost::system::error_code opEc;
			size_t bytesRead = 0;
			auto handler = [&done, &opEc, &bytesRead](const boost::system::error_code &e, size_t bytes) {
				done = true;
				opEc = e;
				bytesRead = bytes;
			};
			const boost::asio::mutable_buffer readBuf =
				boost::asio::buffer(&m_recvBuf[m_recvBufUsed], m_recvBuf.size() - m_recvBufUsed);
			if (m_useTls)
				m_stream.async_read_some(readBuf, handler);
			else
				m_stream.next_layer().async_read_some(readBuf, handler);

			if (!RunWithTimeout(done, remaining))
				throw runtime_error("timeout while waiting for a server response");
			if (opEc == boost::asio::error::eof || opEc == ssl::error::stream_truncated)
				throw runtime_error("connection closed by the server");
			if (opEc)
				throw runtime_error("receive failed: " + opEc.message());

			m_recvBufUsed += bytesRead;
		}
	}

private:
	// Waits at most the given time for an asynchronous operation to finish.
	// On a timeout the operation is cancelled and the handler is still
	// processed - otherwise the local variables captured by reference would be
	// used further after the return.
	bool RunWithTimeout(const bool &done, chrono::milliseconds duration)
	{
		m_io.restart();
		m_io.run_for(duration);
		const bool completed = done;
		if (!completed) {
			boost::system::error_code ec;
			m_stream.lowest_layer().cancel(ec);
			m_io.restart();
			m_io.run();
		}
		return completed;
	}

	bool RunWithTimeout(const bool &done)
	{
		return RunWithTimeout(done, chrono::duration_cast<chrono::milliseconds>(m_timeout));
	}

	// Takes a complete packet out of the receive buffer (framing as in
	// AsioReceiveBuffer::HandleRead).
	boost::shared_ptr<NetPacket> ExtractPacket()
	{
		boost::shared_ptr<NetPacket> packet;
		if (m_recvBufUsed >= NET_HEADER_SIZE) {
			uint32_t nativeVal = 0;
			memcpy(&nativeVal, m_recvBuf.data(), sizeof(nativeVal));
			const size_t packetSize = ntohl(nativeVal);
			if (packetSize > MAX_PACKET_SIZE)
				throw runtime_error("received an oversized packet (protocol error)");

			if (m_recvBufUsed >= packetSize + NET_HEADER_SIZE) {
				packet = NetPacket::Create(&m_recvBuf[NET_HEADER_SIZE], packetSize);
				m_recvBufUsed -= (packetSize + NET_HEADER_SIZE);
				if (m_recvBufUsed)
					memmove(&m_recvBuf[0], &m_recvBuf[packetSize + NET_HEADER_SIZE], m_recvBufUsed);
				if (!packet)
					throw runtime_error("could not parse a packet received from the server");
			}
		}
		return packet;
	}

	boost::asio::io_context m_io;
	ssl::context m_sslCtx;
	ssl::stream<tcp::socket> m_stream;
	bool m_useTls;
	chrono::steady_clock::duration m_timeout;
	vector<char> m_recvBuf;
	size_t m_recvBufUsed;
};

// Logs in and issues the announcement. Throws on network/login errors,
// returns false if the server rejects the announcement (missing admin rights).
bool
sendGlobalNotice(NoticeClient &client, const string &userName, const string &password, const string &noticeText)
{
	// 1. AnnounceMessage abwarten.
	for (;;) {
		boost::shared_ptr<NetPacket> packet = client.Receive();
		const PokerTHMessage *msg = packet->GetMsg();
		if (msg->messagetype() == PokerTHMessage::Type_AnnounceMessage) {
			const AnnounceMessage &announce = msg->announcemessage();
			cout << "Server announce: protocol " << announce.protocolversion().majorversion()
				 << "." << announce.protocolversion().minorversion()
				 << ", game version " << announce.latestgameversion().majorversion()
				 << "." << announce.latestgameversion().minorversion()
				 << ", " << announce.numplayersonserver() << " player(s) online." << endl;
			if (announce.servertype() != AnnounceMessage::serverTypeInternetAuth)
				throw runtime_error("this server does not use authenticated logins - global notices require an admin account");
			break;
		}
		if (msg->messagetype() == PokerTHMessage::Type_ErrorMessage)
			throw runtime_error("server error: " + errorReasonToString(msg->errormessage().errorreason()));
	}

	// 2. Login. The password goes - as with the GUI client - in plain text through
	// the TLS tunnel (see ServerLobbyThread::UserValid).
	{
		boost::shared_ptr<NetPacket> init(new NetPacket);
		init->GetMsg()->set_messagetype(PokerTHMessage::Type_InitMessage);
		InitMessage *netInit = init->GetMsg()->mutable_initmessage();
		netInit->mutable_requestedversion()->set_majorversion(NET_VERSION_MAJOR);
		netInit->mutable_requestedversion()->set_minorversion(NET_VERSION_MINOR);
		netInit->set_buildid(POKERTH_BUILD_ID);
		netInit->set_login(InitMessage::authenticatedLogin);
		netInit->set_nickname(userName);
		netInit->set_clientuserdata(password);
		client.Send(init);
	}

	// 3. Wait for InitAck (the server may send further packets before it).
	for (;;) {
		boost::shared_ptr<NetPacket> packet = client.Receive();
		const PokerTHMessage *msg = packet->GetMsg();
		if (msg->messagetype() == PokerTHMessage::Type_InitAckMessage) {
			cout << "Logged in as \"" << userName << "\" (player id "
				 << msg->initackmessage().yourplayerid() << ")." << endl;
			break;
		}
		if (msg->messagetype() == PokerTHMessage::Type_ErrorMessage)
			throw runtime_error("login failed: " + errorReasonToString(msg->errormessage().errorreason()));
	}

	// 4. Durchsage senden.
	{
		boost::shared_ptr<NetPacket> notice(new NetPacket);
		notice->GetMsg()->set_messagetype(PokerTHMessage::Type_AdminGlobalNoticeMessage);
		notice->GetMsg()->mutable_adminglobalnoticemessage()->set_noticetext(noticeText);
		client.Send(notice);
	}

	// 5. Wait for the confirmation. In between player/game lists arrive
	// that are of no interest here.
	for (;;) {
		boost::shared_ptr<NetPacket> packet = client.Receive();
		const PokerTHMessage *msg = packet->GetMsg();
		if (msg->messagetype() == PokerTHMessage::Type_AdminGlobalNoticeAckMessage) {
			return msg->adminglobalnoticeackmessage().globalnoticeresult()
				   == AdminGlobalNoticeAckMessage::globalNoticeAccepted;
		}
		if (msg->messagetype() == PokerTHMessage::Type_ErrorMessage)
			throw runtime_error("server error: " + errorReasonToString(msg->errormessage().errorreason()));
	}
}

} // namespace

int
main(int argc, char *argv[])
{
	// QCoreApplication is needed for the Qt network download of the server list
	// (DownloadHelper) and its event loop.
	QCoreApplication app(argc, argv);

	po::options_description desc("Options");
	desc.add_options()
	("help,h", "show this help message")
	("user,u", po::value<string>(), "PokerTH account name of a server admin")
	("password,p", po::value<string>(), "account password (default: $POKERTH_ADMIN_PASSWORD, otherwise asked interactively)")
	("message,m", po::value<string>(), "notice text (max. 128 bytes UTF-8, may also be given as positional argument)")
	("host,H", po::value<string>(), "server host name or address; skips the server list download (e.g. a test server)")
	("port", po::value<unsigned>()->default_value(DEFAULT_PROTOBUF_PORT), "server port (only with --host)")
	("tls", po::value<string>()->default_value("on"), "use TLS: on|off (only with --host)")
	("serverlist", po::value<string>()->default_value(DEFAULT_SERVERLIST_URL), "URL of the server list")
	("server-id", po::value<unsigned>(), "id of the server list entry to use (default: first entry)")
	("list-servers", po::bool_switch(), "print the server list and exit")
	("timeout", po::value<unsigned>()->default_value(20), "network timeout in seconds")
	("yes,y", po::bool_switch(), "do not ask for confirmation before sending");

	po::positional_options_description positional;
	positional.add("message", 1);

	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).options(desc).positional(positional).run(), vm);
		po::notify(vm);
	} catch (const std::exception &e) {
		cerr << "Error: " << e.what() << endl;
		return EXIT_USAGE;
	}

	if (vm.count("help")) {
		cout << "pokerth_globalnotice - send a global notice to a PokerTH server." << endl
			 << "The account must have server admin rights." << endl << endl
			 << "Usage: pokerth_globalnotice -u <admin> [-p <password>] \"<notice text>\"" << endl
			 << "       pokerth_globalnotice -u <admin> -H testserver.example.com --tls off -m \"...\"" << endl
			 << endl << desc << endl;
		return 0;
	}

	bool useTls = true;
	if (!parseOnOff(vm["tls"].as<string>(), useTls)) {
		cerr << "Error: --tls expects on or off." << endl;
		return EXIT_USAGE;
	}

	const bool listServers = vm["list-servers"].as<bool>();
	if (listServers && vm.count("host")) {
		cerr << "Error: --list-servers cannot be combined with --host." << endl;
		return EXIT_USAGE;
	}
	if (!listServers) {
		if (!vm.count("user")) {
			cerr << "Error: --user is required." << endl;
			return EXIT_USAGE;
		}
		if (!vm.count("message")) {
			cerr << "Error: no notice text given (--message or positional argument)." << endl;
			return EXIT_USAGE;
		}
	}

	string host;
	unsigned port = vm["port"].as<unsigned>();
	string serverName;
	vector<string> serverListPins;

	try {
		if (vm.count("host")) {
			host = vm["host"].as<string>();
			serverName = host;
		} else {
			const vector<ServerInfo> servers = downloadServerList(vm["serverlist"].as<string>());

			if (listServers) {
				cout << "Servers from " << vm["serverlist"].as<string>() << ":" << endl;
				for (vector<ServerInfo>::const_iterator i = servers.begin(); i != servers.end(); ++i) {
					cout << "  id " << i->id << ": " << i->name << " - " << i->ipv4addr
						 << ":" << i->port << " (TLS " << (i->useTLS ? "on" : "off") << ")" << endl;
				}
				return 0;
			}

			const ServerInfo *chosen = NULL;
			if (vm.count("server-id")) {
				const unsigned wantedId = vm["server-id"].as<unsigned>();
				for (vector<ServerInfo>::const_iterator i = servers.begin(); i != servers.end(); ++i) {
					if (i->id == wantedId) {
						chosen = &(*i);
						break;
					}
				}
				if (!chosen) {
					cerr << "Error: no server with id " << wantedId << " in the server list." << endl;
					return EXIT_USAGE;
				}
			} else {
				chosen = &servers.front();
			}

			host = chosen->ipv4addr;
			port = static_cast<unsigned>(chosen->port);
			useTls = chosen->useTLS;
			serverListPins = chosen->tlsPins;
			serverName = chosen->name.empty() ? chosen->ipv4addr : chosen->name;
			cout << "Using server \"" << serverName << "\" (" << host << ":" << port
				 << ", TLS " << (useTls ? "on" : "off") << ")." << endl;
		}
	} catch (const std::exception &e) {
		cerr << "Error: " << e.what() << endl;
		return EXIT_NETWORK;
	}

	const string userName = vm["user"].as<string>();

	// The server distributes the announcement as a chat message, hence the same
	// 128 byte limit as for the chat (otherwise the packet validator discards it).
	QString noticeText = QString::fromStdString(vm["message"].as<string>()).trimmed();
	if (noticeText.isEmpty()) {
		cerr << "Error: the notice text is empty." << endl;
		return EXIT_USAGE;
	}
	if (noticeText.toUtf8().size() > MAX_CHAT_TEXT_SIZE) {
		while (!noticeText.isEmpty() && noticeText.toUtf8().size() > MAX_CHAT_TEXT_SIZE)
			noticeText.chop(1);
		cerr << "Warning: the notice text was truncated to " << MAX_CHAT_TEXT_SIZE
			 << " bytes: \"" << noticeText.toStdString() << "\"" << endl;
	}

	string password;
	if (vm.count("password")) {
		password = vm["password"].as<string>();
	} else {
		const char *envPassword = getenv("POKERTH_ADMIN_PASSWORD");
		if (envPassword) {
			password = envPassword;
		} else {
			password = promptPassword("Password for \"" + userName + "\": ");
		}
	}
	if (password.empty()) {
		cerr << "Error: no password given." << endl;
		return EXIT_USAGE;
	}

	// The announcement goes to all connected players - have it confirmed
	// before sending (except with -y or a non-interactive call).
	if (!vm["yes"].as<bool>()) {
		cout << endl << "Notice to all players on \"" << serverName << "\":" << endl
			 << "  " << noticeText.toStdString() << endl;
		if (!confirm("Send it?")) {
			cout << "Aborted." << endl;
			return 0;
		}
	}

	try {
		NoticeClient client(useTls, vm["timeout"].as<unsigned>());
		cout << "Connecting to " << host << ":" << port << "..." << endl;
		client.Connect(host, port, serverListPins);

		const bool accepted = sendGlobalNotice(client, userName, password, noticeText.toStdString());
		client.Disconnect();

		if (!accepted) {
			cerr << "The server rejected the global notice - \"" << userName
				 << "\" is probably not a server admin." << endl;
			return EXIT_REJECTED;
		}
		cout << "The global notice was sent to all players." << endl;
	} catch (const std::exception &e) {
		cerr << "Error: " << e.what() << endl;
		return EXIT_NETWORK;
	}

	return 0;
}
