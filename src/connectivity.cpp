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

#include <third_party/asn1/PokerTHMessage.h>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <third_party/boost/timers.hpp>
#include <gsasl.h>

#include <iostream>

#define STL_STRING_FROM_OCTET_STRING(_a) (string((const char *)(_a).buf, (_a).size))

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

/*string packetString;
xer_encode(&asn_DEF_PokerTHMessage, msg, XER_F_BASIC, &net_packet_print_to_string, &packetString);
cout << packetString << endl;*/

PokerTHMessage_t *
receiveMessage(tcp::socket &socket)
{
	PokerTHMessage_t *msg = NULL;
	do {
		asn_dec_rval_t retVal = ber_decode(0, &asn_DEF_PokerTHMessage, (void **)&msg, recBuf.data(), recBufPos);
		if(retVal.code == RC_OK && msg != NULL) {
			if (retVal.consumed < recBufPos) {
				recBufPos -= retVal.consumed;
				memmove(recBuf.c_array(), recBuf.c_array() + retVal.consumed, recBufPos);
			} else {
				recBufPos = 0;
			}
		} else {
			// Free the partially decoded message (if applicable).
			ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
			msg = NULL;
			recBufPos += socket.receive(boost::asio::buffer(recBuf.c_array() + recBufPos, BUF_SIZE - recBufPos));
		}
	} while (msg == NULL);
	return msg;
}

bool
sendMessage(tcp::socket &socket, PokerTHMessage_t *msg)
{
	bool retVal = false;
	if (msg) {
		asn_enc_rval_t e = der_encode_to_buffer(&asn_DEF_PokerTHMessage, msg, sendBuf.data(), BUF_SIZE);
		if (e.encoded != -1) {
			socket.send(boost::asio::buffer(sendBuf.data(), e.encoded));
			retVal = true;
		}
		ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
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
		PokerTHMessage_t *msg = receiveMessage(socket);
		if (!msg || msg->present != PokerTHMessage_PR_announceMessage) {
			cout << "Announce failed" << endl;
			return 1;
		}
		ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);

		// Send init
		msg = (PokerTHMessage_t *)calloc(1, sizeof(PokerTHMessage_t));
		msg->present = PokerTHMessage_PR_initMessage;
		InitMessage_t *netInit = &msg->choice.initMessage;
		netInit->requestedVersion.major = 5;
		netInit->requestedVersion.minor = 0;
		if (password.empty()) {
			netInit->login.present = login_PR_guestLogin;
			GuestLogin_t *guestLogin = &netInit->login.choice.guestLogin;
			OCTET_STRING_fromBuf(&guestLogin->nickName,
								 username.c_str(),
								 username.length());
			if (!sendMessage(socket, msg)) {
				cout << "Init guest failed" << endl;
				return 1;
			}
		} else {
			int errorCode = gsasl_client_start(authContext, "SCRAM-SHA-1", &authSession);
			if (errorCode == GSASL_OK) {
				gsasl_property_set(authSession, GSASL_AUTHID, username.c_str());
				gsasl_property_set(authSession, GSASL_PASSWORD, password.c_str());

				netInit->login.present = login_PR_authenticatedLogin;
				AuthenticatedLogin_t *authLogin = &netInit->login.choice.authenticatedLogin;

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

				OCTET_STRING_fromBuf(&authLogin->clientUserData,
									 nextGsaslMsg.c_str(),
									 nextGsaslMsg.length());
				if (!sendMessage(socket, msg)) {
					cout << "Init auth request failed" << endl;
					return 1;
				}

				msg = receiveMessage(socket);
				if (!msg || msg->present != PokerTHMessage_PR_authMessage) {
					cout << "Auth request failed" << endl;
					return 1;
				}

				AuthMessage_t *netAuth = &msg->choice.authMessage;
				AuthServerChallenge_t *netChallenge = &netAuth->choice.authServerChallenge;
				string challengeStr = STL_STRING_FROM_OCTET_STRING(netChallenge->serverChallenge);
				errorCode = gsasl_step(authSession, challengeStr.c_str(), challengeStr.size(), &tmpOut, &tmpOutSize);
				if (errorCode == GSASL_NEEDS_MORE) {
					nextGsaslMsg = string(tmpOut, tmpOutSize);
				} else {
					cout << "gsasl step 2 failed" << endl;
					return 1;
				}
				gsasl_free(tmpOut);
				ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
				msg = (PokerTHMessage_t *)calloc(1, sizeof(PokerTHMessage_t));
				msg->present = PokerTHMessage_PR_authMessage;
				AuthMessage_t *outAuth = &msg->choice.authMessage;
				outAuth->present = AuthMessage_PR_authClientResponse;
				AuthClientResponse_t *outResponse = &outAuth->choice.authClientResponse;

				OCTET_STRING_fromBuf(&outResponse->clientResponse,
									 nextGsaslMsg.c_str(),
									 nextGsaslMsg.length());
				if (!sendMessage(socket, msg)) {
					cout << "Init auth response failed" << endl;
					return 1;
				}
				msg = receiveMessage(socket);
				if (!msg || msg->present != PokerTHMessage_PR_authMessage) {
					cout << "Auth response failed" << endl;
					return 1;
				}
			}
		}

		// Receive init ack
		msg = receiveMessage(socket);
		if (!msg || msg->present != PokerTHMessage_PR_initAckMessage) {
			cout << "Init ack failed" << endl;
			return 1;
		}
		ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);

		if (mode == 1) {
			cout << "Init.value " << perfTimer.elapsed().total_milliseconds() << endl;
		}
		perfTimer.restart();

		// Send create game
		msg = (PokerTHMessage_t *)calloc(1, sizeof(PokerTHMessage_t));
		msg->present = PokerTHMessage_PR_joinGameRequestMessage;
		JoinGameRequestMessage_t *netJoinGame = &msg->choice.joinGameRequestMessage;
		netJoinGame->autoLeave = 0;
		netJoinGame->joinGameAction.present = joinGameAction_PR_joinNewGame;
		JoinNewGame_t *joinNew = &netJoinGame->joinGameAction.choice.joinNewGame;
		string tmpGameName("_perftest_do_not_join_" + username);
		joinNew->gameInfo.netGameType		= netGameType_normalGame;
		joinNew->gameInfo.maxNumPlayers		= 10;
		joinNew->gameInfo.raiseIntervalMode.present	= raiseIntervalMode_PR_raiseEveryHands;
		joinNew->gameInfo.raiseIntervalMode.choice.raiseEveryHands = 5;
		joinNew->gameInfo.endRaiseMode		= endRaiseMode_keepLastBlind;
		joinNew->gameInfo.proposedGuiSpeed			= 5;
		joinNew->gameInfo.delayBetweenHands			= 6;
		joinNew->gameInfo.playerActionTimeout		= 10;
		joinNew->gameInfo.endRaiseSmallBlindValue	= 0;
		joinNew->gameInfo.firstSmallBlind			= 50;
		joinNew->gameInfo.startMoney				= 2000;
		OCTET_STRING_fromBuf(&joinNew->gameInfo.gameName,
							 tmpGameName.c_str(),
							 tmpGameName.length());
		string tmpGamePassword("blah123");
		joinNew->password = OCTET_STRING_new_fromBuf(
								&asn_DEF_UTF8String,
								tmpGamePassword.c_str(),
								tmpGamePassword.length());
		if (!sendMessage(socket, msg)) {
			cout << "Create game failed" << endl;
			return 1;
		}
		msg = NULL;
		// Receive join game ack
		do {
			ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
			msg = receiveMessage(socket);
			if (!msg) {
				cout << "Receive in lobby failed" << endl;
				return 1;
			}
			if (msg->present == PokerTHMessage_PR_errorMessage) {
				cout << "Received error" << endl;
				return 1;
			}
		} while (msg->present != PokerTHMessage_PR_joinGameReplyMessage);
		if (msg->choice.joinGameReplyMessage.joinGameResult.present != joinGameResult_PR_joinGameAck) {
			cout << "Join game ack failed" << endl;
			return 1;
		}
		ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);

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

