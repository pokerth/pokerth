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

// Connectivity test program for PokerTH

#include <boost/asio.hpp>
#include <third_party/protobuf/pokerth.pb.h>
#include <net/netpacket.h>
#include <boost/program_options.hpp>
#include <boost/array.hpp>
#include <third_party/boost/timers.hpp>
#include <gsasl.h>

#include <iostream>

using namespace std;
using boost::asio::ip::tcp;
namespace po = boost::program_options;

#define BUF_SIZE 1024
// Global receive buffer
boost::array<char, BUF_SIZE> recBuf;
size_t recBufPos = 0;
boost::array<char, BUF_SIZE> sendBuf;

static int
net_packet_print_to_string(const void *buffer, size_t size, void *packetStr)
{
	string *tmpString = (string *)packetStr;
	*tmpString += string((const char *)buffer, size);
	return 0;
}

boost::shared_ptr<NetPacket>
receiveMessage(tcp::socket &socket)
{
	boost::shared_ptr<NetPacket> tmpPacket;

	do {
		// This is necessary, because we use TCP.
		// Packets may be received in multiple chunks or
		// several packets may be received at once.
		if (recBufPos >= NET_HEADER_SIZE) {
			// Read the size of the packet (first 4 bytes in network byte order).
			uint32_t nativeVal;
			memcpy(&nativeVal, recBuf.c_array(), sizeof(uint32_t));
			size_t packetSize = ntohl(nativeVal);
			if (packetSize > MAX_PACKET_SIZE) {
				recBufPos = 0;
				cout << "Packet too large" << endl;
				return boost::shared_ptr<NetPacket>();
			} else if (recBufPos >= packetSize + NET_HEADER_SIZE) {
				try {
					tmpPacket = NetPacket::Create(&recBuf.c_array()[NET_HEADER_SIZE], packetSize);
					if (tmpPacket) {
						recBufPos -= (packetSize + NET_HEADER_SIZE);
						if (recBufPos) {
							memmove(recBuf.c_array(), recBuf.c_array() + packetSize + NET_HEADER_SIZE, recBufPos);
						}
					}
				} catch (const exception &) {
					// Reset buffer on error.
					recBufPos = 0;
					cout << "Packet creation failed" << endl;
					return boost::shared_ptr<NetPacket>();
				}
			}
		}

		if (!tmpPacket) {
			recBufPos += socket.receive(boost::asio::buffer(recBuf.c_array() + recBufPos, BUF_SIZE - recBufPos));
			if (recBufPos == 0) {
				cout << "Receive failed" << endl;
				return boost::shared_ptr<NetPacket>();
			}
		}
	} while (!tmpPacket);

	return tmpPacket;
}

bool
sendMessage(tcp::socket &socket, boost::shared_ptr<NetPacket> packet)
{
	bool retVal = false;
	if (packet) {
		uint32_t packetSize = packet->GetMsg()->ByteSize();
		google::protobuf::uint8 *buf = new google::protobuf::uint8[packetSize + NET_HEADER_SIZE];
		*((uint32_t *)buf) = htonl(packetSize);
		packet->GetMsg()->SerializeWithCachedSizesToArray(&buf[NET_HEADER_SIZE]);
		retVal = socket.send(boost::asio::buffer(buf, packetSize + NET_HEADER_SIZE)) != 0;
		delete[] buf;
	}
	return retVal;
}

int
main(int argc, char *argv[])
{
	try {
		// Check command line options.
		po::options_description desc("Allowed options");
		desc.add_options()
		("help,h", "produce help message")
		("server,s", po::value<string>(), "PokerTH server name")
		("port,P", po::value<string>(), "PokerTH server port")
		("mode,m", po::value<int>(), "set mode (0=connection test, 1=lag test)")
		("username,u", po::value<string>(), "user name used for test")
		("password,p", po::value<string>(), "password used for test")
		;

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			cout << desc << endl;
			return 1;
		}
		if (!vm.count("server") || !vm.count("port") || !vm.count("mode") || !vm.count("username")) {
			cout << "Missing option!" << endl << desc << endl;
			return 1;
		}

		string server(vm["server"].as<string>());
		string port = vm["port"].as<string>();
		int mode = vm["mode"].as<int>();
		string username(vm["username"].as<string>());
		string password;
		if (vm.count("password")) {
			password = vm["password"].as<string>();
		}
		// Initialise gsasl.
		Gsasl *authContext;
		Gsasl_session *authSession;
		int res = gsasl_init(&authContext);
		if (res != GSASL_OK) {
			cout << "gsasl init failed" << endl;
			return 1;
		}

		if (!gsasl_client_support_p(authContext, "SCRAM-SHA-1")) {
			gsasl_done(authContext);
			cout << "This version of gsasl does not support SCRAM-SHA-1" << endl;
			return 1;
		}

		// Connect to the PokerTH server.
		boost::timers::portable::microsec_timer perfTimer;
		boost::asio::io_service io_service;
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(server, port);
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		tcp::resolver::iterator end;
		tcp::socket socket(io_service);
		boost::system::error_code error = boost::asio::error::host_not_found;
		while (error && endpoint_iterator != end) {
			socket.close();
			socket.connect(*endpoint_iterator++, error);
		}
		if (error) {
			cout << "Connect failed" << endl;
			return 1;
		}
		if (mode == 1) {
			cout << "Connect.value " << perfTimer.elapsed().total_milliseconds() << endl;
		}
		perfTimer.restart();

		// Receive server information
		boost::shared_ptr<NetPacket> msg = receiveMessage(socket);
		if (!msg || msg->GetMsg()->messagetype() != PokerTHMessage_PokerTHMessageType_Type_AnnounceMessage) {
			cout << "Announce failed" << endl;
			return 1;
		}

		// Send init
		msg.reset(new NetPacket);
		msg->GetMsg()->set_messagetype(PokerTHMessage_PokerTHMessageType_Type_InitMessage);
		InitMessage *netInit = msg->GetMsg()->mutable_initmessage();
		netInit->mutable_requestedversion()->set_majorversion(NET_VERSION_MAJOR);
		netInit->mutable_requestedversion()->set_minorversion(NET_VERSION_MINOR);
		netInit->set_buildid(0);
		if (password.empty()) {
			netInit->set_login(InitMessage_LoginType_guestLogin);
			netInit->set_nickname(username);
			if (!sendMessage(socket, msg)) {
				cout << "Init guest failed" << endl;
				return 1;
			}
		} else {
			int errorCode = gsasl_client_start(authContext, "SCRAM-SHA-1", &authSession);
			if (errorCode == GSASL_OK) {
				gsasl_property_set(authSession, GSASL_AUTHID, username.c_str());
				gsasl_property_set(authSession, GSASL_PASSWORD, password.c_str());

				netInit->set_login(InitMessage_LoginType_authenticatedLogin);

				char *tmpOut;
				size_t tmpOutSize;
				string nextGsaslMsg;
				errorCode = gsasl_step(authSession, NULL, 0, &tmpOut, &tmpOutSize);
				if (errorCode == GSASL_NEEDS_MORE) {
					nextGsaslMsg = string(tmpOut, tmpOutSize);
				} else {
					cout << "gsasl step 1 failed" << endl;
					return 1;
				}
				gsasl_free(tmpOut);

				netInit->set_clientuserdata(nextGsaslMsg);
				if (!sendMessage(socket, msg)) {
					cout << "Init auth request failed" << endl;
					return 1;
				}

				msg = receiveMessage(socket);
				if (!msg || msg->GetMsg()->messagetype() != PokerTHMessage_PokerTHMessageType_Type_AuthServerChallengeMessage) {
					cout << "Auth request failed" << endl;
					return 1;
				}

				const AuthServerChallengeMessage &netAuth = msg->GetMsg()->authserverchallengemessage();
				string challengeStr(netAuth.serverchallenge());
				errorCode = gsasl_step(authSession, challengeStr.c_str(), challengeStr.size(), &tmpOut, &tmpOutSize);
				if (errorCode == GSASL_NEEDS_MORE) {
					nextGsaslMsg = string(tmpOut, tmpOutSize);
				} else {
					cout << "gsasl step 2 failed" << endl;
					return 1;
				}
				gsasl_free(tmpOut);
				msg.reset(new NetPacket);
				msg->GetMsg()->set_messagetype(PokerTHMessage_PokerTHMessageType_Type_AuthClientResponseMessage);
				AuthClientResponseMessage *outAuth = msg->GetMsg()->mutable_authclientresponsemessage();
				outAuth->set_clientresponse(nextGsaslMsg);
				if (!sendMessage(socket, msg)) {
					cout << "Init auth response failed" << endl;
					return 1;
				}
				msg = receiveMessage(socket);
				if (!msg || msg->GetMsg()->messagetype() != PokerTHMessage_PokerTHMessageType_Type_AuthServerVerificationMessage) {
					cout << "Auth response failed" << endl;
					return 1;
				}
			}
		}

		// Receive init ack
		msg = receiveMessage(socket);
		if (!msg || msg->GetMsg()->messagetype() != PokerTHMessage_PokerTHMessageType_Type_InitAckMessage) {
			cout << "Init ack failed" << endl;
			return 1;
		}

		if (mode == 1) {
			cout << "Init.value " << perfTimer.elapsed().total_milliseconds() << endl;
		}
		perfTimer.restart();

		// Send create game
		msg.reset(new NetPacket);
		msg->GetMsg()->set_messagetype(PokerTHMessage_PokerTHMessageType_Type_JoinNewGameMessage);
		JoinNewGameMessage *joinNew = msg->GetMsg()->mutable_joinnewgamemessage();
		joinNew->set_autoleave(false);
		NetGameInfo *tmpGameInfo = joinNew->mutable_gameinfo();
		string tmpGameName("_perftest_do_not_join_" + username);
		tmpGameInfo->set_netgametype(NetGameInfo_NetGameType_normalGame);
		tmpGameInfo->set_maxnumplayers(10);
		tmpGameInfo->set_raiseintervalmode(NetGameInfo_RaiseIntervalMode_raiseOnHandNum);
		tmpGameInfo->set_raiseeveryhands(5);
		tmpGameInfo->set_endraisemode(NetGameInfo_EndRaiseMode_keepLastBlind);
		tmpGameInfo->set_proposedguispeed(5);
		tmpGameInfo->set_delaybetweenhands(6);
		tmpGameInfo->set_playeractiontimeout(10);
		tmpGameInfo->set_endraisesmallblindvalue(0);
		tmpGameInfo->set_firstsmallblind(50);
		tmpGameInfo->set_startmoney(2000);
		tmpGameInfo->set_gamename(tmpGameName);
		string tmpGamePassword("blah123");
		joinNew->set_password(tmpGamePassword);
		if (!sendMessage(socket, msg)) {
			cout << "Create game failed" << endl;
			return 1;
		}
		// Receive join game ack
		do {
			msg = receiveMessage(socket);
			if (!msg) {
				cout << "Receive in lobby failed" << endl;
				return 1;
			}
			if (msg->GetMsg()->messagetype() == PokerTHMessage_PokerTHMessageType_Type_ErrorMessage) {
				cout << "Received error" << endl;
				return 1;
			} else if (msg->GetMsg()->messagetype() == PokerTHMessage_PokerTHMessageType_Type_JoinGameFailedMessage) {
				cout << "Join game ack failed" << endl;
				return 1;
			}
		} while (msg->GetMsg()->messagetype() != PokerTHMessage_PokerTHMessageType_Type_JoinGameAckMessage);

		if (mode == 1) {
			cout << "CreateGame.value " << perfTimer.elapsed().total_milliseconds() << endl;
		} else {
			cout << "Success" << endl;
		}
		perfTimer.restart();
		gsasl_done(authContext);
	} catch (...) {
		cout << "Exception caught" << endl;
		return 1;
	}

	return 0;
}

