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

// Load test program for PokerTH

#include <third_party/asn1/PokerTHMessage.h>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <gsasl.h>

#include <iostream>
#include <sstream>

#define STL_STRING_FROM_OCTET_STRING(_a) (string((const char *)(_a).buf, (_a).size))

using namespace std;
using boost::asio::ip::tcp;
namespace po = boost::program_options;


#define BUF_SIZE 1024

struct NetSession {
	NetSession(boost::asio::io_service &ioService) : recBufPos(0), authSession(NULL), socket(ioService) {}
	boost::array<char, BUF_SIZE> recBuf;
	size_t recBufPos;
	boost::array<char, BUF_SIZE> sendBuf;
	Gsasl_session *authSession;
	tcp::socket socket;
	string name;
};

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
receiveMessage(NetSession *session)
{
	PokerTHMessage_t *msg = NULL;
	do {
		asn_dec_rval_t retVal = ber_decode(0, &asn_DEF_PokerTHMessage, (void **)&msg, session->recBuf.data(), session->recBufPos);
		if(retVal.code == RC_OK && msg != NULL) {
			if (retVal.consumed < session->recBufPos) {
				session->recBufPos -= retVal.consumed;
				memmove(session->recBuf.c_array(), session->recBuf.c_array() + retVal.consumed, session->recBufPos);
			} else {
				session->recBufPos = 0;
			}
			if (asn_check_constraints(&asn_DEF_PokerTHMessage, msg, NULL, NULL) != 0) {
				cerr << "Invalid packet received:" << endl;
				string packetString;
				xer_encode(&asn_DEF_PokerTHMessage, msg, XER_F_BASIC, &net_packet_print_to_string, &packetString);
				cout << packetString << endl;
			}
		} else {
			// Free the partially decoded message (if applicable).
			ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
			msg = NULL;
			session->recBufPos += session->socket.receive(boost::asio::buffer(session->recBuf.c_array() + session->recBufPos, BUF_SIZE - session->recBufPos));
		}
	} while (msg == NULL);
	return msg;
}

bool
sendMessage(NetSession *session, PokerTHMessage_t *msg)
{
	bool retVal = false;
	if (msg) {
		asn_enc_rval_t e = der_encode_to_buffer(&asn_DEF_PokerTHMessage, msg, session->sendBuf.data(), BUF_SIZE);
		if (e.encoded != -1) {
			session->socket.send(boost::asio::buffer(session->sendBuf.data(), e.encoded));
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
		("numGames,n", po::value<unsigned>(), "Number of games to open")
		("firstId,f", po::value<int>(), "First id of username testx")
		;

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			cout << desc << endl;
			return 1;
		}
		if (!vm.count("server") || !vm.count("port") || !vm.count("numGames") || !vm.count("firstId")) {
			cout << "Missing option!" << endl << desc << endl;
			return 1;
		}

		string server(vm["server"].as<string>());
		string port = vm["port"].as<string>();
		unsigned numGames = vm["numGames"].as<unsigned>();
		int firstId = vm["firstId"].as<int>();

		// Initialise gsasl.
		Gsasl *authContext;
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
		boost::asio::io_service ioService;
		tcp::resolver resolver(ioService);
		tcp::resolver::query query(server, port);
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		tcp::resolver::iterator end;
		boost::system::error_code error = boost::asio::error::host_not_found;
		tcp::resolver::iterator curEndpoint;
		tcp::socket tmpSocket(ioService);
		while (error && endpoint_iterator != end) {
			curEndpoint = endpoint_iterator++;
			tmpSocket.connect(*curEndpoint, error);
			tmpSocket.close();
		}

		if (error) {
			cout << "Connect failed" << endl;
			return 1;
		}

		PokerTHMessage_t *msg = NULL;
		int errorCode;
		char *tmpOut;
		size_t tmpOutSize;
		string nextGsaslMsg;
		NetSession **sessionArray = new NetSession *[numGames * 10];
		unsigned *gameId = new unsigned[numGames];
		const int LoginsPerLoop = 50;
		for (int t = 0; t < (numGames * 10) / LoginsPerLoop + 1; t++) {
			int startNum = t * LoginsPerLoop;
			int endNum = (t + 1) * LoginsPerLoop;
			if (endNum > numGames * 10) {
				endNum = numGames * 10;
			}
			for (int i = startNum; i < endNum; i++) {
				sessionArray[i] = new NetSession(ioService);
				NetSession *session = sessionArray[i];

				session->socket.connect(*curEndpoint, error);

				// Receive server information
				msg = receiveMessage(session);
				if (!msg || msg->present != PokerTHMessage_PR_announceMessage) {
					cout << "Announce failed" << endl;
					return 1;
				}
				ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);

				// Send init
				msg = (PokerTHMessage_t *)calloc(1, sizeof(PokerTHMessage_t));
				msg->present = PokerTHMessage_PR_initMessage;
				InitMessage_t *netInit = &msg->choice.initMessage;
				netInit->requestedVersion.major = 2;
				netInit->requestedVersion.minor = 0;
				errorCode = gsasl_client_start(authContext, "SCRAM-SHA-1", &session->authSession);
				if (errorCode != GSASL_OK) {
					cout << "Auth start error." << endl;
					return 1;
				}
				ostringstream param;
				param << "test" << i + firstId;
				cout << "User " << param.str() << " logging in." << endl;
				session->name = param.str();
				gsasl_property_set(session->authSession, GSASL_AUTHID, session->name.c_str());
				gsasl_property_set(session->authSession, GSASL_PASSWORD, session->name.c_str());

				netInit->login.present = login_PR_authenticatedLogin;
				AuthenticatedLogin_t *authLogin = &netInit->login.choice.authenticatedLogin;

				errorCode = gsasl_step(session->authSession, NULL, 0, &tmpOut, &tmpOutSize);
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
				if (!sendMessage(session, msg)) {
					cout << "Init auth request failed" << endl;
					return 1;
				}
			}

			for (int i = startNum; i < endNum; i++) {
				NetSession *session = sessionArray[i];
				msg = receiveMessage(session);
				if (!msg || msg->present != PokerTHMessage_PR_authMessage) {
					cout << "Auth request failed" << endl;
					return 1;
				}

				AuthMessage_t *netAuth = &msg->choice.authMessage;
				AuthServerChallenge_t *netChallenge = &netAuth->choice.authServerChallenge;
				string challengeStr = STL_STRING_FROM_OCTET_STRING(netChallenge->serverChallenge);
				errorCode = gsasl_step(session->authSession, challengeStr.c_str(), challengeStr.size(), &tmpOut, &tmpOutSize);
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
				if (!sendMessage(session, msg)) {
					cout << "Init auth response failed" << endl;
					return 1;
				}
			}

			for (int i = startNum; i < endNum; i++) {
				NetSession *session = sessionArray[i];
				msg = receiveMessage(session);
				if (!msg || msg->present != PokerTHMessage_PR_authMessage) {
					cout << "Auth response failed" << endl;
					return 1;
				}
				ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);

				// Receive init ack
				msg = receiveMessage(session);
				if (!msg || msg->present != PokerTHMessage_PR_initAckMessage) {
					cout << "Init ack failed" << endl;
					return 1;
				}
				ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);

				for (int j = i; j >= 0; j--) {
					size_t bytes_readable = sessionArray[j]->socket.available();
					while (bytes_readable > 0) {
						msg = receiveMessage(sessionArray[j]);
						ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
						bytes_readable = sessionArray[j]->socket.available();
					}
				}
			}
		}

		for (int g = 0; g < numGames; g++) {
			NetSession *session = sessionArray[g * 10];
			// Send create game
			cout << "Player " << session->name << " creating game " << g+1 << endl;
			msg = (PokerTHMessage_t *)calloc(1, sizeof(PokerTHMessage_t));
			msg->present = PokerTHMessage_PR_joinGameRequestMessage;
			JoinGameRequestMessage_t *netJoinGame = &msg->choice.joinGameRequestMessage;
			string tmpGamePassword("blah123");
			netJoinGame->password = OCTET_STRING_new_fromBuf(
										&asn_DEF_UTF8String,
										tmpGamePassword.c_str(),
										tmpGamePassword.length());
			netJoinGame->joinGameAction.present = joinGameAction_PR_joinNewGame;
			JoinNewGame_t *joinNew = &netJoinGame->joinGameAction.choice.joinNewGame;
			string tmpGameName("_loadtest_do_not_join_" + session->name);
			joinNew->gameInfo.netGameType		= netGameType_normalGame;
			joinNew->gameInfo.maxNumPlayers		= 10;
			joinNew->gameInfo.raiseIntervalMode.present	= raiseIntervalMode_PR_raiseEveryHands;
			joinNew->gameInfo.raiseIntervalMode.choice.raiseEveryHands = 1;
			joinNew->gameInfo.endRaiseMode		= endRaiseMode_keepLastBlind;
			joinNew->gameInfo.proposedGuiSpeed			= 5;
			joinNew->gameInfo.delayBetweenHands			= 5;
			joinNew->gameInfo.playerActionTimeout		= 5;
			joinNew->gameInfo.endRaiseSmallBlindValue	= 0;
			joinNew->gameInfo.firstSmallBlind			= 200;
			joinNew->gameInfo.startMoney				= 10000;
			OCTET_STRING_fromBuf(&joinNew->gameInfo.gameName,
								 tmpGameName.c_str(),
								 tmpGameName.length());
			if (!sendMessage(session, msg)) {
				cout << "Create game failed" << endl;
				return 1;
			}
			msg = NULL;
			// Receive join game ack
			do {
				ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
				msg = receiveMessage(session);
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
			gameId[g] = msg->choice.joinGameReplyMessage.gameId;
			ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
		}

		for (int t = 0; t < (numGames * 10) / LoginsPerLoop + 1; t++) {
			int startNum = t * LoginsPerLoop;
			int endNum = (t + 1) * LoginsPerLoop;
			if (endNum > numGames * 10) {
				endNum = numGames * 10;
			}
			for (int i = startNum; i < endNum; i++) {
				if (i % 10 == 0) {
					continue;
				}
				NetSession *session = sessionArray[i];

				cout << "Player " << session->name << " joining game " << (i / 10)+1 << endl;
				msg = (PokerTHMessage_t *)calloc(1, sizeof(PokerTHMessage_t));
				msg->present = PokerTHMessage_PR_joinGameRequestMessage;
				JoinGameRequestMessage_t *netJoinGame = &msg->choice.joinGameRequestMessage;
				string tmpGamePassword("blah123");
				netJoinGame->password = OCTET_STRING_new_fromBuf(
											&asn_DEF_UTF8String,
											tmpGamePassword.c_str(),
											tmpGamePassword.length());
				netJoinGame->joinGameAction.present = joinGameAction_PR_joinExistingGame;
				JoinExistingGame_t *joinExisting = &netJoinGame->joinGameAction.choice.joinExistingGame;
				joinExisting->gameId = gameId[i / 10];
				if (!sendMessage(session, msg)) {
					cout << "Join game failed" << endl;
					return 1;
				}
				msg = NULL;
			}
			for (int i = startNum; i < endNum; i++) {
				if (i % 10 == 0) {
					continue;
				}
				NetSession *session = sessionArray[i];
				// Receive join game ack
				do {
					ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
					msg = receiveMessage(session);
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
				msg = NULL;
			}
		}
		bool terminated = false;
		while (!terminated) {
			for (int i = 0; i < numGames * 10; i++) {
				NetSession *session = sessionArray[i];

				size_t bytes_readable = session->socket.available();
				while (bytes_readable > 0) {
					msg = receiveMessage(session);
					if (msg->present == PokerTHMessage_PR_endOfGameMessage) {
						cout << "One game was ended." << endl;
						terminated = true;
					}
					ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
					bytes_readable = session->socket.available();
				}
			}
			boost::this_thread::sleep(boost::posix_time::milliseconds(100));
		}
		gsasl_done(authContext);

	} catch (const exception &e) {
		cout << "Exception caught " << e.what() << endl;
		return 1;
	}

	return 0;
}

