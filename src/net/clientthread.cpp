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

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <net/socket_helper.h>
#include <net/clientthread.h>
#include <net/clientstate.h>
#include <net/clientcontext.h>
#include <net/senderhelper.h>
#include <net/downloaderthread.h>
#include <net/clientexception.h>
#include <net/socket_msg.h>
#include <net/net_helper.h>
#include <net/asioreceivebuffer.h>
#include <core/avatarmanager.h>
#include <core/loghelper.h>
#include <vector>
#include <clientenginefactory.h>
#include <game.h>
#include <log.h>
#include <qttoolsinterface.h>

#include <boost/lambda/lambda.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include <QDir>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QUrl>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <memory>
#include <cassert>
#include <typeinfo>
#include <cctype>
#include <ctime>
#include <openssl/ssl.h>

#define TEMP_AVATAR_FILENAME	"avatar.tmp"
#define TEMP_GUID_FILENAME		"guid.tmp"
#define CLIENT_GUID_SIZE		16
#define CLIENT_AVATAR_LOOP_MSEC	100
#define CLIENT_SEND_LOOP_MSEC	50

// Qt message handler: writes qDebug output to stdout with [YYYY-MM-DD HH:MM:SS] prefix.
// Installed once at bot startup (in bot_downloadfiles). Works regardless of whether
// stdout is a terminal or piped through an external logger.
static void bbcbot_msg_handler(QtMsgType, const QMessageLogContext&, const QString& msg)
{
	time_t now = time(NULL);
	char ts[32];
	strftime(ts, sizeof(ts), "[%Y-%m-%d %H:%M:%S] ", localtime(&now));
	fprintf(stdout, "%s%s\n", ts, msg.toLocal8Bit().constData());
	fflush(stdout);
}

using namespace std;
using namespace boost::filesystem;
using boost::asio::ip::tcp;

#ifdef BOOST_ASIO_HAS_STD_CHRONO
using namespace std::chrono;
#else
using namespace boost::chrono;
#endif

ClientThread::ClientThread(GuiInterface &gui, AvatarManager &avatarManager, boost::shared_ptr<Log> myLog)
	: m_ioService(new boost::asio::io_context), m_clientLog(myLog), m_curState(NULL), m_gui(gui),
	  m_avatarManager(avatarManager), m_isServerSelected(false),
	  m_curGameId(0), m_curGameNum(1), m_guiPlayerId(0), m_spectating(false),
	  m_sessionEstablished(false),
	  m_stateTimer(*m_ioService), m_avatarTimer(*m_ioService), m_bbcbotTimer(*m_ioService), botdb(this)
{
	m_context.reset(new ClientContext);
	myQtToolsInterface.reset(CreateQtToolsWrapper());
	m_senderHelper.reset(new SenderHelper(m_ioService));
}

ClientThread::~ClientThread()
{
}

void
ClientThread::Init(
	const string &serverAddress, const string &serverListUrl,
	const string &serverPassword,
	bool useServerList, unsigned serverPort, 
	bool ipv6, bool sctp, bool tls,
	const string &avatarServerAddress, const string &playerName,
	const string &avatarFile, const string &cacheDir)
{
	if (IsRunning()) {
		assert(false);
		return;
	}

	ClientContext &context = GetContext();

	context.SetSctp(sctp);
	context.SetTls(tls);
	context.SetAddrFamily(ipv6 ? AF_INET6 : AF_INET);
	context.SetServerAddr(serverAddress);
	context.SetServerListUrl(serverListUrl);
	context.SetServerPassword(serverPassword);
	context.SetUseServerList(useServerList);
	context.SetServerPort(serverPort);
	context.SetAvatarServerAddr(avatarServerAddress);
	context.SetPlayerName(playerName);
	context.SetAvatarFile(avatarFile);
	context.SetCacheDir(cacheDir);

	ReadSessionGuidFromFile();
}

void
ClientThread::SetClientType(unsigned clientType)
{
	GetContext().SetClientType(clientType);
}

void
ClientThread::SignalTermination()
{
	Thread::SignalTermination();
	m_ioService->stop();
}

void
ClientThread::CloseSocket()
{
	try {
		if (m_context && m_context->GetSessionData()) {
			m_context->GetSessionData()->CloseSocketHandle();
		}
	} catch (...) {
		// Ignore errors during cleanup.
	}
}

void
ClientThread::SendKickPlayer(unsigned playerId)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_KickPlayerRequestMessage);
	KickPlayerRequestMessage *netKick = packet->GetMsg()->mutable_kickplayerrequestmessage();
	netKick->set_gameid(GetGameId());
	netKick->set_playerid(playerId);
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendLeaveCurrentGame()
{
	// NOTE: This runs in the GUI thread. Flushing the SQLite log synchronously
	// here can block the GUI on the SQLite file lock if the network thread is
	// flushing the same log at the same moment (e.g. an end-of-hand flush at
	// round end) -> the whole GUI freezes. So we POST the flush onto the
	// io_service instead: it then runs on the network/client thread, the same
	// thread that performs all regular logging, and the GUI thread never blocks.
	// The leave packet is posted afterwards; since the io_service is processed
	// in order on a single thread, the flush still completes before we leave.
	LOG_MSG("SendLeaveCurrentGame: ENTER gameId=" << GetGameId()
	        << " - posting log flush + leave to io_service");
	// Flush log before leaving game to ensure all data is written to SQLite.
	// Bind the shared_ptr so the Log stays alive until the handler runs.
	if (m_clientLog) {
		boost::asio::post(*m_ioService, boost::bind(&Log::flushLog, m_clientLog));
	}
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_LeaveGameRequestMessage);
	LeaveGameRequestMessage *netLeave = packet->GetMsg()->mutable_leavegamerequestmessage();
	netLeave->set_gameid(GetGameId());
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
	LOG_MSG("SendLeaveCurrentGame: flush + leave packet posted, returning to GUI");
}

void
ClientThread::SendStartEvent(bool fillUpWithCpuPlayers)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet for the server start event.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_StartEventMessage);
	StartEventMessage *netStartEvent = packet->GetMsg()->mutable_starteventmessage();
	netStartEvent->set_starteventtype(StartEventMessage::startEvent);
	netStartEvent->set_gameid(GetGameId());
	netStartEvent->set_fillwithcomputerplayers(fillUpWithCpuPlayers);
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendPlayerAction()
{
	// Called on the GUI thread. Reading GetGame()->getCurrentHand() *here* races
	// with the network thread, which mutates the engine Hand in clientstate (hand
	// transitions). A transient null read used to make us silently drop the action
	// -> the packet was never sent and the game froze (server kept waiting for an
	// action the client thought it had made). Fix: only POST to the io_service;
	// the actual read+build+send happens in DoSendPlayerAction() on that thread,
	// i.e. the SAME thread that mutates the Hand -> a consistent, race-free view.
	// The local player's action was already set on the engine (setMyAction) before
	// this call, and asio::post establishes the happens-before so the io thread
	// sees it.
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::DoSendPlayerAction, shared_from_this()));
}

void
ClientThread::DoSendPlayerAction()
{
	// Runs on the io_service (network) thread.
	boost::shared_ptr<Game> curGame = GetGame();
	if (!curGame || !curGame->getCurrentHand()) {
		// Reached only if the hand/game is genuinely gone on the network thread
		// (the game really ended/we left). Dropping is correct here; this is no
		// longer the transient GUI-thread race that caused the freeze.
		LOG_ERROR("[SENDACTDROP] DoSendPlayerAction DROPPED - "
		          << (curGame ? "currentHand==null" : "game==null")
		          << " gameId=" << GetGameId()
		          << " -> game ended/transitioned, action not sent");
		return;
	}
	// Create a network packet containing the current player action.
	{
		boost::mutex::scoped_lock lock(m_pingDataMutex);
		m_pingData.StartPing();
	}
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_MyActionRequestMessage);
	MyActionRequestMessage *netMyAction = packet->GetMsg()->mutable_myactionrequestmessage();
	netMyAction->set_gameid(GetGameId());
	boost::shared_ptr<PlayerInterface> myPlayer = curGame->getSeatsList()->front();
	netMyAction->set_handnum(curGame->getCurrentHandID());
	netMyAction->set_gamestate(static_cast<NetGameState>(curGame->getCurrentHand()->getCurrentRound()));
	netMyAction->set_myaction(static_cast<NetPlayerAction>(myPlayer->getMyAction()));
	// Only send last bet if not fold/checked.
	if (myPlayer->getMyAction() != PLAYER_ACTION_FOLD && myPlayer->getMyAction() != PLAYER_ACTION_CHECK)
		netMyAction->set_myrelativebet(myPlayer->getMyLastRelativeSet());
	else
		netMyAction->set_myrelativebet(0);
	qDebug() << "[SENDACT] MyActionRequest -> server"
	         << "handnum=" << netMyAction->handnum()
	         << "gamestate=" << (int)netMyAction->gamestate()
	         << "(0=Pre,1=F,2=T,3=R)"
	         << "myaction=" << (int)netMyAction->myaction()
	         << "(1=FOLD,2=CHK,3=CALL,4=BET,5=RAISE,6=ALLIN)"
	         << "myrelativebet=" << (int)netMyAction->myrelativebet()
	         << "| local mySet=" << myPlayer->getMySet()
	         << "myCash=" << myPlayer->getMyCash()
	         << "myButton=" << myPlayer->getMyButton();
	// Already on the io_service thread -> send directly.
	SendSessionPacket(packet);
}

void
ClientThread::SendGameChatMessage(const std::string &msg)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet containing the chat message.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ChatRequestMessage);
	ChatRequestMessage *netChat = packet->GetMsg()->mutable_chatrequestmessage();
	netChat->set_targetgameid(GetGameId());
	netChat->set_chattext(msg);

	// Just dump the packet.
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendLobbyChatMessage(const std::string &msg)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet containing the chat message.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ChatRequestMessage);
	ChatRequestMessage *netChat = packet->GetMsg()->mutable_chatrequestmessage();
	netChat->set_chattext(msg);

	// Just dump the packet.
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendPrivateChatMessage(unsigned targetPlayerId, const std::string &msg)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet containing the chat message.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ChatRequestMessage);
	ChatRequestMessage *netChat = packet->GetMsg()->mutable_chatrequestmessage();
	netChat->set_targetplayerid(targetPlayerId);
	netChat->set_chattext(msg);

	// Just dump the packet.
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendJoinFirstGame(const std::string &password, bool autoLeave)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet to request joining a game.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_JoinExistingGameMessage);
	JoinExistingGameMessage *netJoinGame = packet->GetMsg()->mutable_joinexistinggamemessage();
	netJoinGame->set_gameid(1);
	netJoinGame->set_autoleave(autoLeave);

	if (!password.empty()) {
		netJoinGame->set_password(password);
	}
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendJoinGame(unsigned gameId, const std::string &password, bool autoLeave, bool spectateOnly)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet to request joining a game.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_JoinExistingGameMessage);
	JoinExistingGameMessage *netJoinGame = packet->GetMsg()->mutable_joinexistinggamemessage();
	netJoinGame->set_gameid(gameId);
	netJoinGame->set_autoleave(autoLeave);
	if (spectateOnly) {
		netJoinGame->set_spectateonly(true);
	}

	if (!password.empty()) {
		netJoinGame->set_password(password);
	}
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendRejoinGame(unsigned gameId, bool autoLeave)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet to request rejoining a running game.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_RejoinExistingGameMessage);
	RejoinExistingGameMessage *netJoinGame = packet->GetMsg()->mutable_rejoinexistinggamemessage();
	netJoinGame->set_gameid(gameId);
	netJoinGame->set_autoleave(autoLeave);

	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendCreateGame(const GameData &gameData, const std::string &name, const std::string &password, bool autoLeave)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet to request creating a new game.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_JoinNewGameMessage);
	JoinNewGameMessage *netJoinGame = packet->GetMsg()->mutable_joinnewgamemessage();
	netJoinGame->set_autoleave(autoLeave);
	NetGameInfo *gameInfo = netJoinGame->mutable_gameinfo();
	NetPacket::SetGameData(gameData, *gameInfo);
	gameInfo->set_gamename(name);

	if (!password.empty()) {
		netJoinGame->set_password(password);
	}
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendResetTimeout()
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ResetTimeoutMessage);
	packet->GetMsg()->mutable_resettimeoutmessage();
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendAskKickPlayer(unsigned playerId)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AskKickPlayerMessage);
	AskKickPlayerMessage *netAsk = packet->GetMsg()->mutable_askkickplayermessage();
	netAsk->set_gameid(GetGameId());
	netAsk->set_playerid(playerId);
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendVoteKick(bool doKick)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_VoteKickRequestMessage);
	VoteKickRequestMessage *netVote = packet->GetMsg()->mutable_votekickrequestmessage();
	netVote->set_gameid(GetGameId());
	{
		boost::mutex::scoped_lock lock(m_curPetitionIdMutex);
		netVote->set_petitionid(m_curPetitionId);
	}
	netVote->set_votekick(doKick);
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendShowMyCards()
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ShowMyCardsRequestMessage);
	packet->GetMsg()->mutable_showmycardsrequestmessage();
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendInvitePlayerToCurrentGame(unsigned playerId)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_InvitePlayerToGameMessage);
	InvitePlayerToGameMessage *netInvite = packet->GetMsg()->mutable_inviteplayertogamemessage();
	netInvite->set_gameid(GetGameId());
	netInvite->set_playerid(playerId);
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendRejectGameInvitation(unsigned gameId, DenyGameInvitationReason reason)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_RejectGameInvitationMessage);
	RejectGameInvitationMessage *netReject = packet->GetMsg()->mutable_rejectgameinvitationmessage();
	netReject->set_gameid(gameId);
	netReject->set_myrejectreason(static_cast<RejectGameInvitationMessage::RejectGameInvReason>(reason));
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendReportAvatar(unsigned reportedPlayerId, const std::string &avatarHash)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ReportAvatarMessage);
	ReportAvatarMessage *netReport = packet->GetMsg()->mutable_reportavatarmessage();
	netReport->set_reportedplayerid(reportedPlayerId);
	MD5Buf tmpMD5;
	if (tmpMD5.FromString(avatarHash) && !tmpMD5.IsZero()) {
		netReport->set_reportedavatarhash(tmpMD5.GetData(), MD5_DATA_SIZE);

		boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
	}
}

void
ClientThread::SendReportGameName(unsigned reportedGameId)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ReportGameMessage);
	ReportGameMessage *netReport = packet->GetMsg()->mutable_reportgamemessage();
	netReport->set_reportedgameid(reportedGameId);
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendAdminRemoveGame(unsigned removeGameId)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AdminRemoveGameMessage);
	AdminRemoveGameMessage *netRemove = packet->GetMsg()->mutable_adminremovegamemessage();
	netRemove->set_removegameid(removeGameId);
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::SendAdminBanPlayer(unsigned playerId)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AdminBanPlayerMessage);
	AdminBanPlayerMessage *netBan = packet->GetMsg()->mutable_adminbanplayermessage();
	netBan->set_banplayerid(playerId);
	boost::asio::post(*m_ioService, boost::bind(&ClientThread::SendSessionPacket, shared_from_this(), packet));
}

void
ClientThread::StartAsyncRead()
{
	GetContext().GetSessionData()->GetReceiveBuffer().StartAsyncRead(GetContext().GetSessionData());
}

void
ClientThread::CloseSession(boost::shared_ptr<SessionData> /*session*/)
{
	throw NetException(__FILE__, __LINE__, ERR_SOCK_CONN_RESET, 0);
}

void
ClientThread::HandlePacket(boost::shared_ptr<SessionData> /*session*/, boost::shared_ptr<NetPacket> packet)
{
	GetState().HandlePacket(shared_from_this(), packet);
}

void
ClientThread::SelectServer(unsigned serverId)
{
	boost::mutex::scoped_lock lock(m_selectServerMutex);
	m_isServerSelected = true;
	m_selectedServerId = serverId;
}

void
ClientThread::SetLogin(const std::string &userName, const std::string &password, bool isGuest)
{
	boost::mutex::scoped_lock lock(m_loginDataMutex);
	m_loginData.userName = userName;
	m_loginData.password = password;
	m_loginData.isGuest = isGuest;
}

ServerInfo
ClientThread::GetServerInfo(unsigned serverId) const
{
	ServerInfo tmpInfo;
	boost::mutex::scoped_lock lock(m_serverInfoMapMutex);
	ServerInfoMap::const_iterator pos = m_serverInfoMap.find(serverId);
	if (pos != m_serverInfoMap.end()) {
		tmpInfo = pos->second;
	}
	return tmpInfo;
}

GameInfo
ClientThread::GetGameInfo(unsigned gameId) const
{
	GameInfo tmpInfo;
	boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
	GameInfoMap::const_iterator pos = m_gameInfoMap.find(gameId);
	if (pos != m_gameInfoMap.end()) {
		tmpInfo = pos->second;
	}
	return tmpInfo;
}

PlayerInfo
ClientThread::GetPlayerInfo(unsigned playerId) const
{
	PlayerInfo info;
	if (!GetCachedPlayerInfo(playerId, info)) {
		ostringstream name;
		name << "#" << playerId;

		info.playerName = name.str();
	}
	return info;
}

bool
ClientThread::GetPlayerIdFromName(const string &playerName, unsigned &playerId) const
{
	bool retVal = false;

	boost::mutex::scoped_lock lock(m_playerInfoMapMutex);
	PlayerInfoMap::const_reverse_iterator i = m_playerInfoMap.rbegin();
	PlayerInfoMap::const_reverse_iterator end = m_playerInfoMap.rend();

	while (i != end) {
		if (i->second.playerName == playerName) {
			playerId = i->first;
			retVal = true;
			break;
		}
		++i;
	}
	return retVal;
}

unsigned
ClientThread::GetGameIdOfPlayer(unsigned playerId) const
{
	unsigned gameId = 0; // Default: no game (invalid id).

	// Iterate through all games to find the player.
	boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
	GameInfoMap::const_iterator i = m_gameInfoMap.begin();
	GameInfoMap::const_iterator i_end = m_gameInfoMap.end();
	while (i != i_end) {
		PlayerIdList::const_iterator j = (*i).second.players.begin();
		PlayerIdList::const_iterator j_end = (*i).second.players.end();
		while (j != j_end) {
			if (playerId == *j) {
				gameId = (*i).first;
				break;
			}
			++j;
		}
		if (gameId)
			break;
		++i;
	}
	return gameId;
}

ClientCallback &
ClientThread::GetCallback()
{
	return m_gui;
}

GuiInterface &
ClientThread::GetGui()
{
	return m_gui;
}

boost::shared_ptr<Log>
ClientThread::GetClientLog()
{
	return m_clientLog;
}

AvatarManager &
ClientThread::GetAvatarManager()
{
	return m_avatarManager;
}

void
ClientThread::Main()
{
	// Main loop.
	try {
		InitAuthContext();
		// Start sub-threads.
		m_avatarDownloader.reset(new DownloaderThread);
		m_avatarDownloader->Run();
		SetState(CLIENT_INITIAL_STATE::Instance());
		RegisterTimers();

		m_ioService->run(); // Will only be aborted asynchronously.

	} catch (const PokerTHException &e) {
		// Flush any pending log data before handling the error
		// This ensures the current hand's data is written to SQLite
		// even during unexpected disconnects
		if (m_clientLog) {
			m_clientLog->flushLog();
		}

		// Close the session completely before handling the error
		if (GetContext().GetSessionData()) {
			try {
				GetContext().GetSessionData()->Close();
			} catch (...) {
				// Ignore any errors during cleanup
			}
		}
		
		// Delete the cached server list, as it may be outdated.
		path tmpServerListPath(GetCacheServerListFileName());
		if (exists(tmpServerListPath)) {
			remove(tmpServerListPath);
		}
		GetCallback().SignalNetClientError(e.GetErrorId(), e.GetOsErrorCode());
	}

	// Immediately close the socket so the server detects the disconnect
	// via its pending async_read (TCP FIN / RST).  Without this, the
	// socket would only be closed when the ClientThread shared_ptrs are
	// finally released, which may be delayed or may never happen if
	// Join() times out in terminateNetworkClient().
	CloseSocket();

	// Set a state which does not do anything.
	SetState(CLIENT_FINAL_STATE::Instance());
	// Cancel timers.
	GetStateTimer().cancel();
	CancelTimers();
	// Terminate sub-threads.
	m_avatarDownloader->SignalTermination();
	m_avatarDownloader->Join(DOWNLOADER_THREAD_TERMINATE_TIMEOUT);

	ClearAuthContext();
}

void
ClientThread::RegisterTimers()
{
	m_avatarTimer.expires_after(milliseconds(CLIENT_AVATAR_LOOP_MSEC));
	m_avatarTimer.async_wait(
		boost::bind(
			&ClientThread::TimerCheckAvatarDownloads, shared_from_this(), boost::asio::placeholders::error));
	
	// Start BBCBot timer (1 second interval)
	m_bbcbotTimer.expires_after(seconds(1));
	m_bbcbotTimer.async_wait(
		boost::bind(
			&ClientThread::bbcbotTimerCallback, shared_from_this(), boost::asio::placeholders::error));
}

void
ClientThread::CancelTimers()
{
	m_avatarTimer.cancel();
	m_bbcbotTimer.cancel();
}

void
ClientThread::InitAuthContext()
{
    m_authContext = NULL;
}

void
ClientThread::ClearAuthContext()
{
    // GSASL entfernt: nichts zu räumen.
    m_authContext = NULL;
}

void
ClientThread::InitGame()
{
	// Store current session guid, in case we need to rejoin the game.
	WriteSessionGuidToFile();

	// EngineFactory erstellen
	boost::shared_ptr<EngineFactory> factory(new ClientEngineFactory); // LocalEngine erstellen

	MapPlayerDataList();
	m_startData.numberOfPlayers = (int)GetPlayerDataList().size();
	m_game.reset(new Game(&m_gui, factory, GetPlayerDataList(), GetGameData(), GetStartData(), m_curGameNum++, m_clientLog.get()));
	// Initialize Minimum GUI speed.
	int minimumGuiSpeed = 1;
	if(GetGameData().delayBetweenHandsSec < 11) {
		minimumGuiSpeed = 12-GetGameData().delayBetweenHandsSec;
	}
	GetGui().initGui(minimumGuiSpeed);
	// Signal start of game to GUI.
	GetCallback().SignalNetClientGameStart(m_game);
}

void
ClientThread::SendSessionPacket(boost::shared_ptr<NetPacket> packet)
{
	// Put packets in a buffer until the session is established.
	if (IsSessionEstablished())
		GetSender().Send(GetContext().GetSessionData(), packet);
	else
		m_outPacketList.push_back(packet);
}

void
ClientThread::SendQueuedPackets()
{
	if (!m_outPacketList.empty()) {
		NetPacketList::iterator i = m_outPacketList.begin();
		NetPacketList::iterator end = m_outPacketList.end();

		while (i != end) {
			GetSender().Send(GetContext().GetSessionData(), *i);
			++i;
		}
		m_outPacketList.clear();
	}
}

void
ClientThread::RemoveCachedPlayerInfo(unsigned id)
{
	boost::mutex::scoped_lock lock(m_playerInfoMapMutex);
	m_playerInfoMap.erase(id);
}

bool
ClientThread::GetCachedPlayerInfo(unsigned id, PlayerInfo &info) const
{
	bool retVal = false;

	boost::mutex::scoped_lock lock(m_playerInfoMapMutex);
	PlayerInfoMap::const_iterator pos = m_playerInfoMap.find(id);
	if (pos != m_playerInfoMap.end()) {
		info = pos->second;
		retVal = true;
	}
	return retVal;
}

void
ClientThread::RequestPlayerInfo(unsigned id, bool requestAvatar)
{
	list<unsigned> idList;
	idList.push_back(id);
	RequestPlayerInfo(idList, requestAvatar);
}

void
ClientThread::RequestPlayerInfo(const list<unsigned> &idList, bool requestAvatar)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_PlayerInfoRequestMessage);
	PlayerInfoRequestMessage *netPlayerInfo = packet->GetMsg()->mutable_playerinforequestmessage();
	BOOST_FOREACH(unsigned playerId, idList) {
		if (find(m_playerInfoRequestList.begin(), m_playerInfoRequestList.end(), playerId) == m_playerInfoRequestList.end()) {
			netPlayerInfo->add_playerid(playerId);
			m_playerInfoRequestList.push_back(playerId);
		}
		// Remember that we have to request an avatar.
		if (requestAvatar) {
			m_avatarShouldRequestList.push_back(playerId);
		}
	}
	if (netPlayerInfo->playerid_size() > 0) {
		GetSender().Send(GetContext().GetSessionData(), packet);
	}
}

void
ClientThread::SetPlayerInfo(unsigned id, const PlayerInfo &info)
{
	{
		boost::mutex::scoped_lock lock(m_playerInfoMapMutex);
		// Remove previous player entry with different id
		// for the same player name if it exists.
		// This can only be one entry, since every time a duplicate
		// name is added one is removed.
		// Only erase non computer player entries.
		if (info.playerName.substr(0, sizeof(SERVER_COMPUTER_PLAYER_NAME) - 1) != SERVER_COMPUTER_PLAYER_NAME) {
			PlayerInfoMap::iterator i = m_playerInfoMap.begin();
			PlayerInfoMap::iterator end = m_playerInfoMap.end();
			while (i != end) {
				if (i->first != id && i->second.playerName == info.playerName) {
					m_playerInfoMap.erase(i);
					break;
				}
				++i;
			}
		}
		m_playerInfoMap[id] = info;
	}

	// Update player data for current game.
	boost::shared_ptr<PlayerData> playerData(GetPlayerDataByUniqueId(id));
	if (playerData) {
		playerData->SetName(info.playerName);
		playerData->SetType(info.ptype);
		if (info.hasAvatar) {
			string avatarFile;
			if (GetAvatarManager().GetAvatarFileName(info.avatar, avatarFile)) {
				string utf8 = GetQtToolsInterface().stringToUtf8(avatarFile);
				playerData->SetAvatarFile(utf8);
			} else {
			}
		}
	}
	if (GetGame()) {
		boost::shared_ptr<PlayerInterface> clientPlayer(GetGame()->getPlayerByUniqueId(id));
		if (clientPlayer) {
			clientPlayer->setMyName(info.playerName);
			if (info.hasAvatar) {
				string avatarFile;
				if (GetAvatarManager().GetAvatarFileName(info.avatar, avatarFile)) {
					string utf8File = GetQtToolsInterface().stringToUtf8(avatarFile);
					clientPlayer->setMyAvatar(utf8File);
					GetGui().setPlayerAvatar(id, utf8File);
				} else {
				}
			}
		}
	}

	if (find(m_avatarShouldRequestList.begin(), m_avatarShouldRequestList.end(), id) != m_avatarShouldRequestList.end()) {
		m_avatarShouldRequestList.remove(id);
		// Retrieve avatar if needed.
		RetrieveAvatarIfNeeded(id, info);
	}

	// Remove it from the request list.
	m_playerInfoRequestList.remove(id);

	// Notify GUI
	GetCallback().SignalNetClientPlayerChanged(id, info.playerName);

}

void
ClientThread::SetUnknownPlayer(unsigned id)
{
	// Just remove it from the request list.
	m_playerInfoRequestList.remove(id);
	m_avatarShouldRequestList.remove(id);
	LOG_ERROR("Server reported unknown player id: " << id);
}

void
ClientThread::SetNewGameAdmin(unsigned id)
{
	// Update player data for current game.
	boost::shared_ptr<PlayerData> playerData = GetPlayerDataByUniqueId(id);
	if (playerData.get()) {
		playerData->SetGameAdmin(true);
		GetCallback().SignalNetClientNewGameAdmin(id, playerData->GetName());
		if(m_game) {
			m_clientLog->logPlayerAction(playerData->GetName(),LOG_ACTION_ADMIN);
		}
	}
}

void
ClientThread::RetrieveAvatarIfNeeded(unsigned id, const PlayerInfo &info)
{
	if (find(m_avatarHasRequestedList.begin(), m_avatarHasRequestedList.end(), id) == m_avatarHasRequestedList.end()) {
		if (info.hasAvatar && !info.avatar.IsZero() && !GetAvatarManager().HasAvatar(info.avatar)) {
			m_avatarHasRequestedList.push_back(id); // Never remove from this list. Only request once.

			// Download from avatar server if applicable.
			string avatarServerAddress(GetContext().GetAvatarServerAddr());
			if (!avatarServerAddress.empty() && m_avatarDownloader) {
				string serverFileName(info.avatar.ToString() + AvatarManager::GetAvatarFileExtension(info.avatarType));
				m_avatarDownloader->QueueDownload(
					id, avatarServerAddress + serverFileName, GetContext().GetCacheDir() + TEMP_AVATAR_FILENAME);
			} else {
				boost::shared_ptr<NetPacket> packet(new NetPacket);
				packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AvatarRequestMessage);
				AvatarRequestMessage *netAvatar = packet->GetMsg()->mutable_avatarrequestmessage();
				netAvatar->set_requestid(id);
				netAvatar->set_avatarhash(info.avatar.GetData(), MD5_DATA_SIZE);
				GetSender().Send(GetContext().GetSessionData(), packet);
			}
		}
	}
}

std::string
ClientThread::GetPlayerName(unsigned id)
{
	PlayerInfo info;
	if (!GetCachedPlayerInfo(id, info)) {
		// Request player info.
		ostringstream name;
		name << "#" << id;
		info.playerName = name.str();
		RequestPlayerInfo(id);
	}
	return info.playerName;
}

void ClientThread::AddAllLobbyPlayersToIdle()
{
        boost::mutex::scoped_lock lock(m_playerInfoMapMutex);
        for (const auto& p : m_playerInfoMap) {
                unsigned playerId = p.first;
                const PlayerInfo& info = p.second;
                if (GetGameIdOfPlayer(playerId) == 0 && info.playerName.substr(0, 5) != "Guest") {
                        botdb.addidleplayer(playerId);
                }
        }
}
void
ClientThread::StoreInTempAvatarFile(unsigned playerId, const vector<unsigned char> &data)
{
	AvatarFileMap::iterator pos = m_tempAvatarMap.find(playerId);
	if (pos == m_tempAvatarMap.end())
		throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_REQUEST_ID, 0);
	// We trust the server (concerning size of the data).
	std::copy(data.begin(), data.end(), back_inserter(pos->second->fileData));
}

void
ClientThread::CompleteTempAvatarFile(unsigned playerId)
{
	AvatarFileMap::iterator pos = m_tempAvatarMap.find(playerId);
	if (pos == m_tempAvatarMap.end())
		throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_REQUEST_ID, 0);
	boost::shared_ptr<AvatarFile> tmpAvatar = pos->second;
	unsigned avatarSize = (unsigned)tmpAvatar->fileData.size();
	if (avatarSize != tmpAvatar->reportedSize)
		LOG_ERROR("Client received invalid avatar file size!");
	else
		PassAvatarFileToManager(playerId, tmpAvatar);

	// Free memory.
	m_tempAvatarMap.erase(pos);
}

void
ClientThread::AddTempAvatarFile(unsigned requestId, unsigned avatarSize, AvatarFileType type)
{
    boost::shared_ptr<AvatarFile> tmpAvatar(new AvatarFile);
    tmpAvatar->reportedSize = avatarSize;
    tmpAvatar->fileType = type;
    m_tempAvatarMap[requestId] = tmpAvatar;
}

void
ClientThread::PassAvatarFileToManager(unsigned playerId, boost::shared_ptr<AvatarFile> AvatarFile)
{
	PlayerInfo tmpPlayerInfo;
	if (!GetCachedPlayerInfo(playerId, tmpPlayerInfo))
		LOG_ERROR("Client received invalid player id!");
	else {
		if (AvatarFile->fileType == AVATAR_FILE_TYPE_UNKNOWN)
			AvatarFile->fileType = tmpPlayerInfo.avatarType;
		if (!GetAvatarManager().StoreAvatarInCache(tmpPlayerInfo.avatar, AvatarFile->fileType, &AvatarFile->fileData[0], AvatarFile->reportedSize, false))
			LOG_ERROR("Failed to store avatar in cache directory.");

		// Update player info, but never re-request avatar.
		SetPlayerInfo(playerId, tmpPlayerInfo);

		string fileName;
		if (GetAvatarManager().GetAvatarFileName(tmpPlayerInfo.avatar, fileName)) {
			// Dynamically update avatar in GUI.
			GetGui().setPlayerAvatar(playerId, GetQtToolsInterface().stringToUtf8(fileName));
		}
	}
}

void
ClientThread::SetUnknownAvatar(unsigned playerId)
{
	m_tempAvatarMap.erase(playerId);
	LOG_ERROR("Server reported unknown avatar for player: " << playerId);
}

void
ClientThread::TimerCheckAvatarDownloads(const boost::system::error_code& ec)
{
	if (!ec) {
		if (m_avatarDownloader && m_avatarDownloader->HasDownloadResult()) {
			unsigned playerId;
			boost::shared_ptr<AvatarFile> tmpAvatar(new AvatarFile);
			m_avatarDownloader->GetDownloadResult(playerId, tmpAvatar->fileData);
			tmpAvatar->reportedSize = tmpAvatar->fileData.size();
			PassAvatarFileToManager(playerId, tmpAvatar);
		}
		m_avatarTimer.expires_after(milliseconds(CLIENT_AVATAR_LOOP_MSEC));
		m_avatarTimer.async_wait(
			boost::bind(
				&ClientThread::TimerCheckAvatarDownloads, shared_from_this(), boost::asio::placeholders::error));
	}
}

void
ClientThread::UnsubscribeLobbyMsg()
{
	if (GetContext().GetSubscribeLobbyMsg()) {
		// Send unsubscribe request.
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_SubscriptionRequestMessage);
		SubscriptionRequestMessage *netRequest = packet->GetMsg()->mutable_subscriptionrequestmessage();
		netRequest->set_subscriptionaction(SubscriptionRequestMessage::unsubscribeGameList);
		GetSender().Send(GetContext().GetSessionData(), packet);
		GetContext().SetSubscribeLobbyMsg(false);
	}
}

void
ClientThread::ResubscribeLobbyMsg()
{
	if (!GetContext().GetSubscribeLobbyMsg()) {
		// Clear game info map as it is outdated.
		ClearGameInfoMap();
		// Send resubscribe request.
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_SubscriptionRequestMessage);
		SubscriptionRequestMessage *netRequest = packet->GetMsg()->mutable_subscriptionrequestmessage();
		netRequest->set_subscriptionaction(SubscriptionRequestMessage::resubscribeGameList);
		GetSender().Send(GetContext().GetSessionData(), packet);
		GetContext().SetSubscribeLobbyMsg(true);
	}
}

const ClientContext &
ClientThread::GetContext() const
{
	assert(m_context.get());
	return *m_context;
}

ClientContext &
ClientThread::GetContext()
{
	assert(m_context.get());
	return *m_context;
}

string
ClientThread::GetCacheServerListFileName()
{
	string fileName;
	path tmpServerListPath(GetContext().GetCacheDir());
	string serverListUrl(GetContext().GetServerListUrl());
	// Retrieve the file name from the URL.
	size_t pos = serverListUrl.find_last_of('/');
	if (!GetContext().GetCacheDir().empty() && !serverListUrl.empty() && pos != string::npos && ++pos < serverListUrl.length()) {
		tmpServerListPath /= serverListUrl.substr(pos);
		fileName = tmpServerListPath.string();
	}
	return fileName;
}

// Implementierung der statischen SslInfoCallback Methode
void
ClientThread::SslInfoCallback(const SSL *ssl, int where, int ret)
{
    const char *state = SSL_state_string_long((SSL*)ssl);
    
    if (where & SSL_CB_LOOP) {
    }
    else if (where & SSL_CB_ALERT) {
        const char *alert_type = (where & SSL_CB_READ) ? "read" : "write";
    }
    else if (where & SSL_CB_EXIT) {
        if (ret == 0) {
        }
        else if (ret < 0) {
        }
    }
    else if (where & SSL_CB_HANDSHAKE_START) {
    }
    else if (where & SSL_CB_HANDSHAKE_DONE) {
    }
}

void
ClientThread::CreateContextSession()
{
    ClientContext &context = GetContext();


    boost::shared_ptr<boost::asio::ip::tcp::resolver> resolver(new boost::asio::ip::tcp::resolver(*m_ioService));
    context.SetResolver(resolver);

    if (context.GetTls()) {
        boost::shared_ptr<boost::asio::ssl::context> sslCtx(
            new boost::asio::ssl::context(boost::asio::ssl::context::sslv23_client));
        
        sslCtx->set_verify_mode(boost::asio::ssl::verify_none);

        SSL_CTX_set_info_callback(sslCtx->native_handle(), &ClientThread::SslInfoCallback);

        boost::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> sslStream(
            new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(*m_ioService, *sslCtx));

        SSL_set_info_callback(sslStream->native_handle(), &ClientThread::SslInfoCallback);

        boost::shared_ptr<SessionData> session(new SessionData(sslStream, SESSION_ID_GENERIC, *this, *m_ioService, 0));
        context.SetSessionData(session);
    } else {
        boost::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(*m_ioService));
        boost::shared_ptr<SessionData> session(new SessionData(sock, SESSION_ID_GENERIC, *this, *m_ioService));
        context.SetSessionData(session);
    }
    
}

ClientState &
ClientThread::GetState()
{
	assert(m_curState);
	return *m_curState;
}

void
ClientThread::SetState(ClientState &newState)
{
	if (m_curState) {
		m_curState->Exit(shared_from_this());
	}
	m_curState = &newState;
	m_curState->Enter(shared_from_this());
}

boost::asio::steady_timer &
ClientThread::GetStateTimer()
{
	return m_stateTimer;
}

SenderHelper &
ClientThread::GetSender()
{
	assert(m_senderHelper);
	return *m_senderHelper;
}

unsigned
ClientThread::GetGameId() const
{
	boost::mutex::scoped_lock lock(m_curGameIdMutex);
	return m_curGameId;
}

void
ClientThread::SetGameId(unsigned id)
{
	boost::mutex::scoped_lock lock(m_curGameIdMutex);
	m_curGameId = id;
}

Gsasl *
ClientThread::GetAuthContext()
{
	assert(m_authContext);
	return m_authContext;
}

const GameData &
ClientThread::GetGameData() const
{
	return m_gameData;
}

void
ClientThread::SetGameData(const GameData &gameData)
{
	m_gameData = gameData;
}

const StartData &
ClientThread::GetStartData() const
{
	return m_startData;
}

void
ClientThread::SetStartData(const StartData &startData)
{
	m_startData = startData;
}

unsigned
ClientThread::GetGuiPlayerId() const
{
	boost::mutex::scoped_lock lock(m_guiPlayerIdMutex);
	return m_guiPlayerId;
}

int
ClientThread::GetOrigGuiPlayerNum() const
{
	return m_origGuiPlayerNum;
}

bool
ClientThread::IsSpectating() const
{
	return m_spectating;
}

void
ClientThread::SetSpectating(bool spectating)
{
	m_spectating = spectating;
	// Ein Zuschauer spielt nicht mit -> es wird auch kein Logfile geschrieben.
	if (m_clientLog) {
		m_clientLog->setRecordingSuspended(spectating);
	}
}

void
ClientThread::SetGuiPlayerId(unsigned guiPlayerId)
{
	boost::mutex::scoped_lock lock(m_guiPlayerIdMutex);
	m_guiPlayerId = guiPlayerId;
}

boost::shared_ptr<Game>
ClientThread::GetGame()
{
	return m_game;
}

QtToolsInterface &
ClientThread::GetQtToolsInterface()
{
	assert(myQtToolsInterface.get());
	return *myQtToolsInterface;
}

boost::shared_ptr<PlayerData>
ClientThread::CreatePlayerData(unsigned playerId, bool isGameAdmin)
{
	boost::shared_ptr<PlayerData> playerData;
	PlayerInfo info;
	if (GetCachedPlayerInfo(playerId, info)) {
		playerData.reset(
			new PlayerData(playerId, 0, info.ptype,
						   info.isGuest ? PLAYER_RIGHTS_GUEST : PLAYER_RIGHTS_NORMAL, isGameAdmin));
		playerData->SetName(info.playerName);
		if (info.hasAvatar) {
			string avatarFile;
			if (GetAvatarManager().GetAvatarFileName(info.avatar, avatarFile))
				playerData->SetAvatarFile(GetQtToolsInterface().stringToUtf8(avatarFile));
			else
				RetrieveAvatarIfNeeded(playerId, info);
		}
	} else {
		ostringstream name;
		name << "#" << playerId;

		// Request player info.
		RequestPlayerInfo(playerId, true);
		// Use temporary data until the PlayerInfo request is completed.
		playerData.reset(
			new PlayerData(playerId, 0, PLAYER_TYPE_HUMAN, PLAYER_RIGHTS_NORMAL, isGameAdmin));
		playerData->SetName(name.str());
	}
	return playerData;
}

void
ClientThread::AddPlayerData(boost::shared_ptr<PlayerData> playerData)
{
	if (playerData.get() && !playerData->GetName().empty()) {
		m_playerDataList.push_back(playerData);
		if (playerData->GetUniqueId() == GetGuiPlayerId())
			GetCallback().SignalNetClientSelfJoined(playerData->GetUniqueId(), playerData->GetName(), playerData->IsGameAdmin());
		else {
			GetCallback().SignalNetClientPlayerJoined(playerData->GetUniqueId(), playerData->GetName(), playerData->IsGameAdmin());
//			if(m_game) {
//				m_clientLog->logPlayerAction(playerData->GetName(),LOG_ACTION_JOIN);
//			}
		}
	}
}

void
ClientThread::RemovePlayerData(unsigned playerId, int removeReason)
{
	boost::shared_ptr<PlayerData> tmpData;

	PlayerDataList::iterator i = m_playerDataList.begin();
	PlayerDataList::iterator end = m_playerDataList.end();
	while (i != end) {
		if ((*i)->GetUniqueId() == playerId) {
			tmpData = *i;
			m_playerDataList.erase(i);
			break;
		}
		++i;
	}

	if (tmpData.get()) {
		// Remove player from gui.
		if (GetGame()) {
			boost::shared_ptr<PlayerInterface> tmpPlayer(GetGame()->getPlayerByUniqueId(tmpData->GetUniqueId()));
			if (tmpPlayer) {
				tmpPlayer->setMyStayOnTableStatus(false);
			}
		}
		GetCallback().SignalNetClientPlayerLeft(tmpData->GetUniqueId(), tmpData->GetName(), removeReason);

		if(m_game) {
			if(removeReason == NTF_NET_REMOVED_KICKED) {
				m_clientLog->logPlayerAction(tmpData->GetName(),LOG_ACTION_KICKED);
			} else {
				m_clientLog->logPlayerAction(tmpData->GetName(),LOG_ACTION_LEFT);
			}
		}

	}
}

void
ClientThread::ClearPlayerDataList()
{
	m_playerDataList.clear();
}

void
ClientThread::MapPlayerDataList()
{
	// A spectator has no seat of his own, so there is nothing to rotate: keep
	// the server's seat numbering. m_origGuiPlayerNum = 0 makes every
	// (number + numberOfPlayers) % numberOfPlayers mapping in the GUI an
	// identity, so seat N of the server stays seat N on the table.
	if (IsSpectating()) {
		m_origGuiPlayerNum = 0;
		m_playerDataList.sort(*boost::lambda::_1 < *boost::lambda::_2);
		return;
	}

	// Retrieve the GUI player.
	boost::shared_ptr<PlayerData> guiPlayer = GetPlayerDataByUniqueId(GetGuiPlayerId());
	assert(guiPlayer.get());
	m_origGuiPlayerNum = guiPlayer->GetNumber();

	// Create a copy of the player list so that the GUI player
	// is player 0. This is mapped because the GUI depends on it.
	PlayerDataList mappedList;

	PlayerDataList::const_iterator i = m_playerDataList.begin();
	PlayerDataList::const_iterator end = m_playerDataList.end();
	int numPlayers = GetStartData().numberOfPlayers;

	while (i != end) {
		boost::shared_ptr<PlayerData> tmpData(new PlayerData(*(*i)));
		int numberDiff = numPlayers - m_origGuiPlayerNum;
		tmpData->SetNumber((tmpData->GetNumber() + numberDiff) % numPlayers);
		mappedList.push_back(tmpData);
		++i;
	}

	// Sort the list by player number.
	mappedList.sort(*boost::lambda::_1 < *boost::lambda::_2);

	m_playerDataList = mappedList;
}

const PlayerDataList &
ClientThread::GetPlayerDataList() const
{
	return m_playerDataList;
}

boost::shared_ptr<PlayerData>
ClientThread::GetPlayerDataByUniqueId(unsigned id)
{
	boost::shared_ptr<PlayerData> tmpPlayer;

	PlayerDataList::const_iterator i = m_playerDataList.begin();
	PlayerDataList::const_iterator end = m_playerDataList.end();

	while (i != end) {
		if ((*i)->GetUniqueId() == id) {
			tmpPlayer = *i;
			break;
		}
		++i;
	}
	return tmpPlayer;
}

boost::shared_ptr<PlayerData>
ClientThread::GetPlayerDataByName(const std::string &name)
{
	boost::shared_ptr<PlayerData> tmpPlayer;

	if (!name.empty()) {
		PlayerDataList::const_iterator i = m_playerDataList.begin();
		PlayerDataList::const_iterator end = m_playerDataList.end();

		while (i != end) {
			if ((*i)->GetName() == name) {
				tmpPlayer = *i;
				break;
			}
			++i;
		}
	}
	return tmpPlayer;
}

void
ClientThread::AddServerInfo(unsigned serverId, const ServerInfo &info)
{
	{
		boost::mutex::scoped_lock lock(m_serverInfoMapMutex);
		m_serverInfoMap.insert(ServerInfoMap::value_type(serverId, info));
	}
	GetCallback().SignalNetClientServerListAdd(serverId);
}

void
ClientThread::ClearServerInfoMap()
{
	{
		boost::mutex::scoped_lock lock(m_serverInfoMapMutex);
		m_serverInfoMap.clear();
	}
	GetCallback().SignalNetClientServerListClear();
}

bool
ClientThread::GetSelectedServer(unsigned &serverId) const
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_selectServerMutex);
	if (m_isServerSelected) {
		retVal = true;
		serverId = m_selectedServerId;
	}
	return retVal;
}

void
ClientThread::UseServer(unsigned serverId)
{
	ClientContext &context = GetContext();
	ServerInfo useInfo(GetServerInfo(serverId));

	if (context.GetAddrFamily() == AF_INET6)
		context.SetServerAddr(useInfo.ipv6addr);
	else
		context.SetServerAddr(useInfo.ipv4addr);

	context.SetServerPort((unsigned)useInfo.port);
	context.SetAvatarServerAddr(useInfo.avatarServerAddr);
	context.SetTls(useInfo.useTLS);  // Use TLS setting from serverlist
}

bool
ClientThread::GetLoginData(LoginData &loginData) const
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_loginDataMutex);
	if (!m_loginData.userName.empty()) {
		loginData = m_loginData;
		retVal = true;
	}
	return retVal;
}

void
ClientThread::AddGameInfo(unsigned gameId, const GameInfo &info)
{
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		m_gameInfoMap.insert(GameInfoMap::value_type(gameId, info));
	}
	GetCallback().SignalNetClientGameListNew(gameId);

	// If bbcbot requested a create and this new game belongs to this client, move to CREATED
	if (bot.creategamestate == GS_GOTCOMMAND) {
		unsigned myId = GetGuiPlayerId();
		if (info.adminPlayerId == myId) {
			BBCLOG("[BBCBot] Detected created game id " << gameId << " by bot; scheduling invite.");
			bot.creategamestate = GS_CREATED;
			bot.countdowninvite = 2; // small delay to allow client join to settle
		}
	}
}

void
ClientThread::UpdateGameInfoMode(unsigned gameId, GameMode mode)
{
	bool found = false;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		GameInfoMap::iterator pos = m_gameInfoMap.find(gameId);
		if (pos != m_gameInfoMap.end()) {
			found = true;
			(*pos).second.mode = mode;
		}
	}
	if (found)
		GetCallback().SignalNetClientGameListUpdateMode(gameId, mode);
}

void
ClientThread::UpdateGameInfoAdmin(unsigned gameId, unsigned adminPlayerId)
{
	bool found = false;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		GameInfoMap::iterator pos = m_gameInfoMap.find(gameId);
		if (pos != m_gameInfoMap.end()) {
			found = true;
			(*pos).second.adminPlayerId = adminPlayerId;
		}
	}
	if (found)
		GetCallback().SignalNetClientGameListUpdateAdmin(gameId, adminPlayerId);
}

void
ClientThread::RemoveGameInfo(unsigned gameId)
{
	bool found = false;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		GameInfoMap::iterator pos = m_gameInfoMap.find(gameId);
		if (pos != m_gameInfoMap.end()) {
			found = true;
			m_gameInfoMap.erase(pos);
		}
	}
	if (found)
		GetCallback().SignalNetClientGameListRemove(gameId);
}

void
ClientThread::ModifyGameInfoAddPlayer(unsigned gameId, unsigned playerId)
{
	bool playerAdded = false;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		GameInfoMap::iterator pos = m_gameInfoMap.find(gameId);
		if (pos != m_gameInfoMap.end()) {
			pos->second.players.push_back(playerId);
			playerAdded = true;
		}
	}
	if (playerAdded)
		GetCallback().SignalNetClientGameListPlayerJoined(gameId, playerId);
}

void
ClientThread::ModifyGameInfoRemovePlayer(unsigned gameId, unsigned playerId)
{
	bool playerRemoved = false;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		GameInfoMap::iterator pos = m_gameInfoMap.find(gameId);
		if (pos != m_gameInfoMap.end()) {
			pos->second.players.remove(playerId);
			playerRemoved = true;
		}
	}
	if (playerRemoved)
		GetCallback().SignalNetClientGameListPlayerLeft(gameId, playerId);
}

void
ClientThread::ModifyGameInfoAddSpectator(unsigned gameId, unsigned playerId)
{
	bool playerAdded = false;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		GameInfoMap::iterator pos = m_gameInfoMap.find(gameId);
		if (pos != m_gameInfoMap.end()) {
			pos->second.spectators.push_back(playerId);
			playerAdded = true;
		}
	}
	if (playerAdded)
		GetCallback().SignalNetClientGameListSpectatorJoined(gameId, playerId);
}

void
ClientThread::ModifyGameInfoRemoveSpectator(unsigned gameId, unsigned playerId)
{
	bool playerRemoved = false;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		GameInfoMap::iterator pos = m_gameInfoMap.find(gameId);
		if (pos != m_gameInfoMap.end()) {
			pos->second.spectators.remove(playerId);
			playerRemoved = true;
		}
	}
	if (playerRemoved)
		GetCallback().SignalNetClientGameListSpectatorLeft(gameId, playerId);
}

void
ClientThread::ModifyGameInfoClearSpectatorsDuringGame()
{
	boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
	GameInfoMap::iterator pos = m_gameInfoMap.find(GetGameId());
	if (pos != m_gameInfoMap.end()) {
		pos->second.spectatorsDuringGame.clear();
	}
}

void
ClientThread::ModifyGameInfoAddSpectatorDuringGame(unsigned playerId)
{
	bool spectatorAdded = false;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		GameInfoMap::iterator pos = m_gameInfoMap.find(GetGameId());
		if (pos != m_gameInfoMap.end()) {
			pos->second.spectatorsDuringGame.push_back(playerId);
			spectatorAdded = true;
		}
	}
	if (spectatorAdded)
		GetCallback().SignalNetClientSpectatorJoined(playerId, GetPlayerName(playerId));
}

void
ClientThread::ModifyGameInfoRemoveSpectatorDuringGame(unsigned playerId, int removeReason)
{
	bool spectatorRemoved = false;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		GameInfoMap::iterator pos = m_gameInfoMap.find(GetGameId());
		if (pos != m_gameInfoMap.end()) {
			pos->second.spectatorsDuringGame.remove(playerId);
			spectatorRemoved = true;
		}
	}
	if (spectatorRemoved)
		GetCallback().SignalNetClientSpectatorLeft(playerId, GetPlayerName(playerId), removeReason);
}

void
ClientThread::ClearGameInfoMap()
{
	std::vector<unsigned> removedGameIds;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		removedGameIds.reserve(m_gameInfoMap.size());
		for (GameInfoMap::const_iterator it = m_gameInfoMap.begin(); it != m_gameInfoMap.end(); ++it) {
			removedGameIds.push_back(it->first);
		}
		m_gameInfoMap.clear();
	}

	for (size_t i = 0; i < removedGameIds.size(); ++i) {
		GetCallback().SignalNetClientGameListRemove(removedGameIds[i]);
	}
}

void
ClientThread::StartPetition(unsigned petitionId, unsigned proposingPlayerId, unsigned kickPlayerId, int timeoutSec, int numVotesToKick)
{
	{
		boost::mutex::scoped_lock lock(m_curPetitionIdMutex);
		m_curPetitionId = petitionId;
	}
	GetGui().startVoteOnKick(kickPlayerId, proposingPlayerId, timeoutSec, numVotesToKick);
	if (GetGuiPlayerId() != kickPlayerId
			&& GetGuiPlayerId() != proposingPlayerId) {
		GetGui().changeVoteOnKickButtonsState(true);
	}
}

void
ClientThread::UpdatePetition(unsigned petitionId, int /*numVotesAgainstKicking*/, int numVotesInFavourOfKicking, int numVotesToKick)
{
	bool isCurPetition;
	{
		boost::mutex::scoped_lock lock(m_curPetitionIdMutex);
		isCurPetition = m_curPetitionId == petitionId;
	}
	if (isCurPetition) {
		GetGui().refreshVotesMonitor(numVotesInFavourOfKicking, numVotesToKick);
	}
}

void
ClientThread::EndPetition(unsigned petitionId)
{
	bool isCurPetition;
	{
		boost::mutex::scoped_lock lock(m_curPetitionIdMutex);
		isCurPetition = m_curPetitionId == petitionId;
	}
	if (isCurPetition)
		GetGui().endVoteOnKick();
}

void
ClientThread::UpdateStatData(const ServerStats &stats)
{
	boost::mutex::scoped_lock lock(m_curStatsMutex);
	if (stats.numberOfPlayersOnServer)
		m_curStats.numberOfPlayersOnServer = stats.numberOfPlayersOnServer;

	if (stats.totalPlayersEverLoggedIn)
		m_curStats.totalPlayersEverLoggedIn = stats.totalPlayersEverLoggedIn;

	if (stats.totalGamesEverCreated)
		m_curStats.totalGamesEverCreated = stats.totalGamesEverCreated;

	GetCallback().SignalNetClientStatsUpdate(m_curStats);
}

void
ClientThread::EndPing()
{
	boost::mutex::scoped_lock lock(m_pingDataMutex);
	if (m_pingData.EndPing()) {
		GetCallback().SignalNetClientPingUpdate(m_pingData.MinPing(), m_pingData.AveragePing(), m_pingData.MaxPing());
	}
}

ServerStats
ClientThread::GetStatData() const
{
	boost::mutex::scoped_lock lock(m_curStatsMutex);
	return m_curStats;
}

bool
ClientThread::IsSessionEstablished() const
{
	return m_sessionEstablished;
}

void
ClientThread::SetSessionEstablished(bool flag)
{
	if (m_sessionEstablished != flag) {
		m_sessionEstablished = flag;
		if (flag)
			SendQueuedPackets();
	}
}

bool
ClientThread::IsSynchronized() const
{
	return m_playerInfoRequestList.empty();
}

void
ClientThread::ReadSessionGuidFromFile()
{
	string guidFileName(GetContext().GetCacheDir() + TEMP_GUID_FILENAME);
	std::ifstream guidStream(guidFileName.c_str(), ios::in | ios::binary);
	if (guidStream.good()) {
		std::vector<char> tmpGuid(CLIENT_GUID_SIZE);
		guidStream.read(&tmpGuid[0], CLIENT_GUID_SIZE);
		GetContext().SetSessionGuid(string(tmpGuid.begin(), tmpGuid.end()));
	}
}

void
ClientThread::WriteSessionGuidToFile() const
{
	string guidFileName(GetContext().GetCacheDir() + TEMP_GUID_FILENAME);
	std::ofstream guidStream(guidFileName.c_str(), ios::out | ios::trunc | ios::binary);
	if (guidStream.good()) {
		guidStream.write(GetContext().GetSessionGuid().c_str(), GetContext().GetSessionGuid().size());
	}
}



// bbcbot code - Bot implementation


void
ClientThread::bot_loadfiles()
{
	BBCLOG("[BBCBot] bot_loadfiles() called");
	BBCLOG("[BBCBot] Loading bot files...");
	
	// Initialize bot as enabled
	bot.enabled = true;
	// Keep idle players list to avoid losing lobby players on file reloads.
	// Disable interactive info popups for bot runs
	qputenv("POKERTH_BBCBOT_NOPOPUPS", QByteArray("1"));
	// Note: do NOT reset creategamestate/creatorid/countdowns here –
	// bot_loadfiles() is also called during periodic 10-minute reloads,
	// and resetting these would abort an in-progress game-creation flow.
	// They are only initialised once at first startup (see bot_init below).
	
	// Clear all reloadable data to avoid accumulation on 10-minute reloads
	botdb.clear();
	bot.fixedcommands.clear();
	bot.fixedreply.clear();

	// Load fixed commands from file if available
	// Format: lines with "command=reply"
	QFile fixedFile("botfiles/fixedcommands.txt");
	if (fixedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&fixedFile);
		while (!in.atEnd()) {
			QString line = in.readLine().trimmed();
			if (!line.isEmpty() && !line.startsWith("#")) {
				int eqPos = line.indexOf('=');
				if (eqPos > 0) {
					QString cmd = line.left(eqPos).trimmed();
					QString reply = line.mid(eqPos + 1).trimmed();
					bot.fixedcommands.push_back(cmd.toStdString());
					bot.fixedreply.push_back(reply.toStdString());
				}
			}
		}
		fixedFile.close();
		BBCLOG("[BBCBot] Loaded " << bot.fixedcommands.size() << " fixed commands");
	} else {
		BBCLOG("[BBCBot] No botfiles/fixedcommands.txt found, using defaults");
	}
	
	// Sort fixed commands for binary search
	if (!bot.fixedcommands.empty()) {
		// Create paired vector for sorting
		std::vector<std::pair<std::string, std::string>> paired;
		for (size_t i = 0; i < bot.fixedcommands.size(); i++) {
			paired.push_back(std::make_pair(bot.fixedcommands[i], bot.fixedreply[i]));
		}
		std::sort(paired.begin(), paired.end());
		bot.fixedcommands.clear();
		bot.fixedreply.clear();
		for (const auto& p : paired) {
			bot.fixedcommands.push_back(p.first);
			bot.fixedreply.push_back(p.second);
		}
	}
	
	// Load game templates from file if available
	// Format: INI-style sections for each game template
	// [game:commandname]
	// name=Game Name Prefix
	// players=10
	// startcash=5000
	// smallblind=10
	// raisehands=8 (optional, default 8)
	// raiseminutes=5 (optional, default 5)
	// raisemode=double (optional: double, manual, always)
	// manualblindlist=10,20,30,50,100 (optional, for manual mode)
	// gametype=normal (optional: normal, ranking - default normal)
	// permgroup=groupname (optional)
	//
	// [permissions:groupname]
	// type=whitelist (or blacklist)
	// players=Alice,Bob,Charlie
	
	bot.gdata.clear();
	bot.permgroups.clear();
	
	// Helper: resolve botfiles path in several likely locations (cwd, build/bin, parent)
	auto resolveBotFile = [](const QString &name) -> QString {
		QStringList candidates = {QString("botfiles/") + name,
			QString("build/bin/botfiles/") + name,
			QString("./build/bin/botfiles/") + name,
			QString("../build/bin/botfiles/") + name};
		for (const QString &c : candidates) {
			if (QFile::exists(c)) return c;
		}
		return QString();
	};

	// First try legacy format: botfiles/gameslist.txt and botfiles/permissions.txt
	QString gamesListPath = resolveBotFile("gameslist.txt");
	QFile gamesListFile(gamesListPath.isEmpty() ? QString("botfiles/gameslist.txt") : gamesListPath);
	if (gamesListFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&gamesListFile);
		BBCLOG("[BBCBot] Parsing gameslist.txt (legacy format)...");

		// Load permissions first if present
		QString permPath = resolveBotFile("permissions.txt");
		QFile permFile(permPath.isEmpty() ? QString("botfiles/permissions.txt") : permPath);
		if (permFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream pin(&permFile);
			bbcbotpermissiongroup* currentPerm = nullptr;
			while (!pin.atEnd()) {
				QString line = pin.readLine().trimmed();
				if (line.isEmpty()) continue;
				if (line.startsWith("//")) continue;
				if (line.startsWith("#")) {
					QStringList parts = line.split('#');
					if (parts.size() >= 3) {
						QString groupname = parts[2].trimmed();
						if (!groupname.isEmpty()) {
							bot.permgroups.push_back(bbcbotpermissiongroup());
							currentPerm = &bot.permgroups.back();
							currentPerm->name = groupname.toStdString();
							currentPerm->isblacklist = false; // default to whitelist
							BBCLOG("[BBCBot] Found permission group: " << groupname.toStdString());
						} else {
							currentPerm = nullptr;
						}
					}
					continue;
				}
				if (line.startsWith("+")) {
					if (currentPerm) {
						QString player = line.mid(1).trimmed();
						if (!player.isEmpty()) currentPerm->players.push_back(player.toStdString());
					}
				}
			}
			permFile.close();
		}

		// Parse gameslist entries
		while (!in.atEnd()) {
			QString line = in.readLine().trimmed();
			if (line.isEmpty()) continue;
			if (line.startsWith("//")) continue;
			// Legacy format uses lines like: #command#permgroup#Game Title Prefix#
			int hashCount = line.count('#');
			if (hashCount < 4) continue;
			QStringList parts = line.split('#');
			if (parts.size() >= 4) {
				QString cmd = parts[1].trimmed();
				QString permname = parts[2].trimmed();
				QString title = parts[3].trimmed();
				if (cmd.isEmpty()) continue;
				bot.gdata.push_back(bbcbotgamedata());
				bbcbotgamedata &g = bot.gdata.back();
				g.commandname = cmd.toStdString();
				g.gamenameprefix = title.toStdString();

				// attempt to link permission group by name
				if (!permname.isEmpty()) {
					for (auto &pg : bot.permgroups) {
						if (pg.name == permname.toStdString()) {
							g.pgroup = &pg;
							break;
						}
					}
				}

					// Try to load per-command settings file: <cmd>_settings.txt
					QString settingsName = cmd + QString("_settings.txt");
					QString settingsPath = resolveBotFile(settingsName);
					if (settingsPath.isEmpty()) settingsPath = QString("botfiles/") + settingsName;
					QFile sfile(settingsPath);
					if (sfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
						QTextStream sin(&sfile);
						while (!sin.atEnd()) {
							QString line2 = sin.readLine().trimmed();
							if (line2.isEmpty() || line2.startsWith("#")) continue;
							int eq = line2.indexOf('=');
							if (eq <= 0) continue;
							QString key2 = line2.left(eq).trimmed().toLower();
							QString val2 = line2.mid(eq + 1).trimmed();

							// Legacy keys mapping
							if (key2 == "players" || key2 == "numberofplayers") {
								g.gdata.maxNumberOfPlayers = val2.toInt();
							} else if (key2 == "startcash" || key2 == "startcash") {
								g.gdata.startMoney = val2.toInt();
							} else if (key2 == "smallblind" || key2 == "firstsmallblind") {
								g.gdata.firstSmallBlind = val2.toInt();
							} else if (key2 == "raiseblindsathands") {
								// Boolean flag: RaiseBlindsAtHands=1 means use RAISE_ON_HANDNUMBER
								if (val2 == "1" || val2.toLower() == "true") g.gdata.raiseIntervalMode = RAISE_ON_HANDNUMBER;
							} else if (key2 == "raiseblindsatminutes") {
								// Boolean flag: RaiseBlindsAtMinutes=1 means use RAISE_ON_MINUTES
								if (val2 == "1" || val2.toLower() == "true") g.gdata.raiseIntervalMode = RAISE_ON_MINUTES;
							} else if (key2 == "raisehands" || key2 == "raisesmallblindeveryhands") {
								g.gdata.raiseSmallBlindEveryHandsValue = val2.toInt();
								// Only set mode if RaiseBlindsAt* keys were not used (fallback for simple configs)
							} else if (key2 == "raiseminutes" || key2 == "raisesmallblindeveryminutes") {
								g.gdata.raiseSmallBlindEveryMinutesValue = val2.toInt();
								// Only set mode if RaiseBlindsAt* keys were not used (fallback for simple configs)
							} else if (key2 == "raisemode" || key2 == "alwaysdoubleblinds" || key2 == "manualblindsorder") {
								// Handle multiple representations
								if (key2 == "raisemode") {
									if (val2 == "double") g.gdata.raiseMode = DOUBLE_BLINDS;
									else if (val2 == "manual") g.gdata.raiseMode = MANUAL_BLINDS_ORDER;
									else if (val2 == "always") g.gdata.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
								} else {
									// boolean flags
									if (key2 == "alwaysdoubleblinds") {
										if (val2 == "1" || val2.toLower() == "true") g.gdata.raiseMode = DOUBLE_BLINDS;
									} else if (key2 == "manualblindsorder") {
										if (val2 == "1" || val2.toLower() == "true") g.gdata.raiseMode = MANUAL_BLINDS_ORDER;
									}
								}
							} else if (key2 == "aftermbalwaysdoubleblinds") {
								if (val2 == "1" || val2.toLower() == "true") g.gdata.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS;
							} else if (key2 == "aftermbalwaysraiseabout") {
								if (val2 == "1" || val2.toLower() == "true") g.gdata.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
							} else if (key2 == "aftermbalwaysraisevalue") {
								g.gdata.afterMBAlwaysRaiseValue = val2.toInt();
							} else if (key2 == "aftermbstayatlastblind") {
								if (val2 == "1" || val2.toLower() == "true") g.gdata.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND;
							} else if (key2 == "manualblindlist" || key2 == "listblind") {
								// 'ListBlind' can appear multiple times
								if (key2 == "manualblindlist") {
									QStringList blinds = val2.split(',');
									for (const QString &b : blinds) g.gdata.manualBlindsList.push_back(b.trimmed().toInt());
								} else {
									g.gdata.manualBlindsList.push_back(val2.toInt());
								}
							} else if (key2.startsWith("gametypenormal") || key2.startsWith("gametyperegisteredonly") || key2.startsWith("gametypeinviteonly") || key2.startsWith("gametyperanking")) {
								// keys like GameTypeInviteOnly=1
								if ((key2.contains("inviteonly") && (val2 == "1" || val2.toLower() == "true"))) g.gdata.gameType = GAME_TYPE_INVITE_ONLY;
								if ((key2.contains("registeredonly") && (val2 == "1" || val2.toLower() == "true"))) g.gdata.gameType = GAME_TYPE_REGISTERED_ONLY;
								if ((key2.contains("ranking") && (val2 == "1" || val2.toLower() == "true"))) g.gdata.gameType = GAME_TYPE_RANKING;
								// if GameTypeNormal==1 or none set, default remains
							} else if (key2 == "gamespeed") {
								g.gdata.guiSpeed = val2.toInt();
							} else if (key2 == "delaybetweenhands") {
								g.gdata.delayBetweenHandsSec = val2.toInt();
							} else if (key2 == "timeoutplayeraction") {
								g.gdata.playerActionTimeoutSec = val2.toInt();
							}
						}
						sfile.close();
						BBCLOG("[BBCBot] Loaded settings for: " << g.commandname);
					}

				BBCLOG("[BBCBot] Found legacy game entry: " << g.commandname << " -> " << g.gamenameprefix);
			}
		}

		gamesListFile.close();
		BBCLOG("[BBCBot] Loaded " << bot.gdata.size() << " game(s) and " << bot.permgroups.size() << " permission group(s) from legacy files");
	} else {
		// Fallback: existing INI-style gametemplates.txt loader
		QString gtPath = resolveBotFile("gametemplates.txt");
		QFile gameFile(gtPath.isEmpty() ? QString("botfiles/gametemplates.txt") : gtPath);
		if (gameFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream in(&gameFile);
			BBCLOG("[BBCBot] Parsing gametemplates.txt...");
            
			bbcbotgamedata* currentGame = nullptr;
			bbcbotpermissiongroup* currentPerm = nullptr;
			int lineNum = 0;
            
			while (!in.atEnd()) {
				QString line = in.readLine().trimmed();
				lineNum++;
                
				// Skip empty lines and comments
				if (line.isEmpty() || line.startsWith("#")) continue;
                
				// Section headers
				if (line.startsWith("[") && line.endsWith("]")) {
					QString section = line.mid(1, line.length() - 2);
                    
					if (section.startsWith("game:")) {
						QString cmdname = section.mid(5).trimmed();
						if (cmdname.isEmpty()) {
							BBCLOG("[BBCBot] Warning: Empty game command name at line " << lineNum);
							continue;
						}
						bot.gdata.push_back(bbcbotgamedata());
						currentGame = &bot.gdata.back();
						currentGame->commandname = cmdname.toStdString();
						currentPerm = nullptr;
						BBCLOG("[BBCBot] Found game template: " << cmdname.toStdString());
					}
					else if (section.startsWith("permissions:")) {
						QString groupname = section.mid(12).trimmed();
						if (groupname.isEmpty()) {
							BBCLOG("[BBCBot] Warning: Empty permission group name at line " << lineNum);
							continue;
						}
						bot.permgroups.push_back(bbcbotpermissiongroup());
						currentPerm = &bot.permgroups.back();
						currentPerm->name = groupname.toStdString();
						currentGame = nullptr;
						BBCLOG("[BBCBot] Found permission group: " << groupname.toStdString());
					}
					continue;
				}
                
				// Key=Value pairs
				int eqPos = line.indexOf('=');
				if (eqPos <= 0) continue;
                
				QString key = line.left(eqPos).trimmed().toLower();
				QString value = line.mid(eqPos + 1).trimmed();
                
				if (currentGame) {
					// Parse game settings
					if (key == "name") {
						currentGame->gamenameprefix = value.toStdString();
					}
					else if (key == "players") {
						currentGame->gdata.maxNumberOfPlayers = value.toInt();
					}
					else if (key == "startcash") {
						currentGame->gdata.startMoney = value.toInt();
					}
					else if (key == "smallblind") {
						currentGame->gdata.firstSmallBlind = value.toInt();
					}
					else if (key == "raisehands") {
						currentGame->gdata.raiseSmallBlindEveryHandsValue = value.toInt();
						currentGame->gdata.raiseIntervalMode = RAISE_ON_HANDNUMBER;
					}
					else if (key == "raiseminutes") {
						currentGame->gdata.raiseSmallBlindEveryMinutesValue = value.toInt();
						currentGame->gdata.raiseIntervalMode = RAISE_ON_MINUTES;
					}
					else if (key == "raisemode") {
						if (value == "double") {
							currentGame->gdata.raiseMode = DOUBLE_BLINDS;
						} else if (value == "manual") {
							currentGame->gdata.raiseMode = MANUAL_BLINDS_ORDER;
						} else if (value == "always") {
							currentGame->gdata.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
						}
					}
					else if (key == "manualblindlist") {
						QStringList blinds = value.split(',');
						for (const QString& blind : blinds) {
							currentGame->gdata.manualBlindsList.push_back(blind.trimmed().toInt());
						}
					}
					else if (key == "gametype") {
						if (value == "ranking") {
							currentGame->gdata.gameType = GAME_TYPE_RANKING;
						} else {
							currentGame->gdata.gameType = GAME_TYPE_NORMAL;
						}
					}
					else if (key == "permgroup") {
						// Link to permission group (will be resolved after loading)
						currentGame->pgroup = nullptr; // Will be set later
						for (auto& pg : bot.permgroups) {
							if (pg.name == value.toStdString()) {
								currentGame->pgroup = &pg;
								break;
							}
						}
					}
				}
				else if (currentPerm) {
					// Parse permission settings
					if (key == "type") {
						currentPerm->isblacklist = (value.toLower() == "blacklist");
					}
					else if (key == "players") {
						QStringList playerList = value.split(',');
						for (const QString& player : playerList) {
							QString trimmed = player.trimmed();
							if (!trimmed.isEmpty()) {
								currentPerm->players.push_back(trimmed.toStdString());
							}
						}
					}
				}
			}

			// Resolve permission group pointers after all data is loaded
			for (auto& game : bot.gdata) {
				if (game.pgroup == nullptr) {
					// nothing to do here for legacy format; groups were linked above
				}
			}

			gameFile.close();
			BBCLOG("[BBCBot] Loaded " << bot.gdata.size() << " game template(s) and " << bot.permgroups.size() << " permission group(s)");
		} else {
			BBCLOG("[BBCBot] No botfiles/gametemplates.txt found");
		}
	}
	
	// Load player database
	QString minidbPath = resolveBotFile("minidb.txt");
	if (minidbPath.isEmpty()) minidbPath = QString("botfiles/minidb.txt");
	if (botdb.loadfile(minidbPath.toStdString())) {
		BBCLOG("[BBCBot] Player database loaded successfully");
	} else {
		BBCLOG("[BBCBot] Warning: Could not load player database");
	}
	
	// Load WEC player list
	QString wecPath = resolveBotFile("weclist.txt");
	if (wecPath.isEmpty()) wecPath = QString("botfiles/weclist.txt");
	if (botdb.loadwecfile(wecPath.toStdString())) {
		BBCLOG("[BBCBot] WEC player list loaded");
	} else {
		BBCLOG("[BBCBot] Warning: Could not load WEC list");
	}
	
	BBCLOG("[BBCBot] Bot initialization complete");
}

void
ClientThread::bbcbotTimerCallback(const boost::system::error_code& ec)
{
	if (ec) return; // timer cancelled

	// Always reschedule first so the chain never breaks, even if bot is not yet enabled
	m_bbcbotTimer.expires_after(seconds(1));
	m_bbcbotTimer.async_wait(
		boost::bind(
			&ClientThread::bbcbotTimerCallback, shared_from_this(), boost::asio::placeholders::error));

	if (!bot.enabled) return;

	// Increment uptime counter
	bot.stdcount++;

	// Log game state periodically for debugging
	if (bot.creategamestate != GS_NORMAL && bot.stdcount % 5 == 0) {
		BBCLOG("[BBCBot] Timer tick: state=" << bot.creategamestate << " creatorid=" << bot.creatorid);
	}

	// Handle game creation states
	if (bot.creategamestate == GS_CREATED) {
		bot.countdowninvite--;
		if (bot.countdowninvite <= 0) {
			bot_invite();
		}
	} else if (bot.creategamestate == GS_SENDINV) {
		bot.countdowninvitetimeout--;
		if (bot.countdowninvitetimeout <= 0) {
			bot_invitetimeout();
		}
	} else if (bot.creategamestate == GS_ACCEPTED) {
		bot.countdownleave--;
		if (bot.countdownleave <= 0) {
			bot_leave();
		}
	}

	// Heartbeat: log uptime every 60 seconds for diagnostic purposes
	if (bot.stdcount % 60 == 0) {
		BBCLOG("[BBCBot] Timer alive: uptime=" << bot.stdcount << "s");
	}

	// Periodic actions every 10 minutes (600 seconds)
	if (bot.stdcount % 600 == 0) {
		try {
			bot_every10min();
		} catch (const std::exception& e) {
			BBCLOG("[BBCBot] ERROR in bot_every10min: " << e.what());
		} catch (...) {
			BBCLOG("[BBCBot] ERROR in bot_every10min: unknown exception");
		}
	}
}

void
ClientThread::bot_invite()
{
	if (bot.creatorid > 0) {
		BBCLOG("[BBCBot] Inviting player " << bot.creatorid << " to game");
		SendInvitePlayerToCurrentGame(bot.creatorid);
		bot.creategamestate = GS_SENDINV;
		bot.countdowninvitetimeout = 30;
	}
}

void
ClientThread::bot_invitetimeout()
{
	BBCLOG("[BBCBot] Invitation timeout - leaving game");
	SendPrivateChatMessage(bot.creatorid, "ERROR: you didn't accept the game invitation in time");
	SendLeaveCurrentGame();
	bot.creategamestate = GS_NORMAL;
	bot.creatorid = 0;
}

void
ClientThread::bot_leave()
{
	BBCLOG("[BBCBot] Leaving game after player joined");
	SendLeaveCurrentGame();
	bot.creategamestate = GS_NORMAL;
	bot.creatorid = 0;
}

void
ClientThread::bot_every10min()
{
	BBCLOG("[BBCBot] Running 10-minute maintenance tasks");
	// Download updated bot files from server
	bot_downloadfiles();
}

void
ClientThread::bot_downloadfiles()
{
	// Install custom Qt message handler once at bot startup.
	static bool s_handlerInstalled = false;
	if (!s_handlerInstalled) {
		s_handlerInstalled = true;
		qInstallMessageHandler(bbcbot_msg_handler);
	}
	BBCLOG("[BBCBot] Downloading updated bot files from server...");
	
	// Create botfiles directory if it doesn't exist
	QDir botfilesDir("botfiles");
	if (!botfilesDir.exists()) {
		botfilesDir.mkpath(".");
		BBCLOG("[BBCBot] Created botfiles directory: " << botfilesDir.absolutePath().toStdString());
	} else {
		BBCLOG("[BBCBot] Using existing botfiles directory: " << botfilesDir.absolutePath().toStdString());
	}
	
	// List of all files to download
	QStringList files;
	files << "bbcupfinal_settings.txt" << "gameslist.txt" << "husctest2_settings.txt"
	      << "mcupfinal_settings.txt" << "newweclist.txt" << "step2_settings.txt"
	      << "wecgfinal_settings.txt" << "weclist.old3.txt" << "weclist.txt"
	      << "bbcup_settings.txt" << "hash2.txt" << "manual_fixedcommands.txt"
	      << "mcup_settings.txt" << "permissions.txt" << "step3_settings.txt"
	      << "wecmfinal_settings.txt" << "fixedcommands.txt" << "husc_settings.txt"
	      << "manual_permissions.txt" << "minidb.txt" << "step1_settings.txt"
	      << "step4_settings.txt" << "wec_settings.txt" << "duckscup_settings.txt";
	
	const QString baseUrl = "https://bbc.pokerth.net/exp3/bbcbot/";
	BBCLOG("[BBCBot] Starting synchronous download of " << files.size() << " files from " << baseUrl.toStdString());
	
	// Use QNetworkAccessManager for downloads (no parent since ClientThread is not a QObject)
	QNetworkAccessManager *manager = new QNetworkAccessManager();
	
	int successCount = 0;
	int errorCount = 0;
	
	// Download each file synchronously using QEventLoop
	for (const QString &filename : files) {
		QUrl url(baseUrl + filename);
		QNetworkRequest request(url);
		
		// Set timeout to 30 seconds
		request.setTransferTimeout(30000);
		
		// Disable SSL verification (like curl -k)
		QSslConfiguration sslConfig = request.sslConfiguration();
		sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
		request.setSslConfiguration(sslConfig);
		
		QNetworkReply *reply = manager->get(request);
		
		// Create a local event loop to wait for this download
		QEventLoop loop;
		QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
		loop.exec(); // Wait for download to finish
		
		BBCLOG("[BBCBot] Download finished for: " << filename.toStdString());
		
		if (reply->error() == QNetworkReply::NoError) {
			QByteArray data = reply->readAll();
			QString filepath = "botfiles/" + filename;
			QFile file(filepath);
			
			if (file.open(QIODevice::WriteOnly)) {
				file.write(data);
				file.close();
				BBCLOG("[BBCBot] Successfully saved: " << filename.toStdString() << " (" << data.size() << " bytes)");
				successCount++;
			} else {
				BBCLOG("[BBCBot] Error: Could not save file: " << filename.toStdString());
				errorCount++;
			}
		} else {
			BBCLOG("[BBCBot] Error downloading " << filename.toStdString() << ": " << reply->errorString().toStdString());
			errorCount++;
		}
		
		reply->deleteLater();
	}
	
	delete manager;
	
	BBCLOG("[BBCBot] Download complete: " << successCount << " successful, " << errorCount << " errors");
	
	// Reload bot files after successful downloads
	if (successCount > 0) {
		BBCLOG("[BBCBot] Reloading bot files...");
		bot_loadfiles();
	}
}

// bbcbotplayerdb implementation
namespace {

std::string trim_ascii(const std::string &input)
{
	size_t start = 0;
	size_t end = input.size();
	while (start < end && std::isspace(static_cast<unsigned char>(input[start]))) {
		++start;
	}
	while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
		--end;
	}
	return input.substr(start, end - start);
}

std::string tolower_ascii(const std::string &input)
{
	std::string out;
	out.reserve(input.size());
	for (char ch : input) {
		out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
	}
	return out;
}

bool equals_case_insensitive(const std::string &left, const std::string &right)
{
	return tolower_ascii(left) == tolower_ascii(right);
}
}

bbcbotplayerdb::bbcbotplayerdb(ClientThread*p)
{
	issorted=true;
	size=0;
	parent=p;
}

bbcbotplayerdb::~bbcbotplayerdb()
{
}

void bbcbotplayerdb::clear()
{
	pname.clear();
	ts2.clear();
	ts3.clear();
	ts4.clear();
	games.clear();
	rating.clear();
	size=0;
	issorted=true;
}

bool bbcbotplayerdb::checkcontent()
{
	bool sizecheck=(size==pname.size() && size==ts2.size() && size==ts3.size() && size==ts4.size() && size==games.size() && size==rating.size());
	if(!sizecheck) clear();
	if(!sizecheck) return false;
	issorted=true;
	for(size_t i=1;i<size;i++)
	{
		if(pname[i-1].compare(pname[i])<0) continue;
		BBCLOG("[BBCBot] ERROR: player database not sorted");
		issorted=false;
		return true;
	}
	return true;
}

bool bbcbotplayerdb::loadline(std::string line)
{
	size_t pos1,pos2=0;
	int data1[8];
	std::string tempname;
	pos1=line.find('\t',0);
	if(pos1==std::string::npos) return false;
	tempname=line.substr(0,pos1);
	pos2=line.find('\t',pos1+1);
	if(pos2==std::string::npos) return false;
	data1[1]=strtol(line.substr(pos1+1,pos2-pos1-1).c_str(),NULL,10);
	pos1=line.find('\t',pos2+1);
	if(pos1==std::string::npos) return false;
	data1[2]=strtol(line.substr(pos2+1,pos1-pos2-1).c_str(),NULL,10);
	pos2=line.find('\t',pos1+1);
	if(pos2==std::string::npos) return false;
	data1[3]=strtol(line.substr(pos1+1,pos2-pos1-1).c_str(),NULL,10);
	pos1=line.find('\t',pos2+1);
	if(pos1==std::string::npos) return false;
	data1[4]=strtol(line.substr(pos2+1,pos1-pos2-1).c_str(),NULL,10);
	data1[5]=strtol(line.substr(pos1+1).c_str(),NULL,10);

	if(data1[4]<=0) return false;
	pname.push_back(tempname);
	ts2.push_back(data1[1]);
	ts3.push_back(data1[2]);
	ts4.push_back(data1[3]);
	rating.push_back(data1[4]);
	games.push_back(data1[5]);
	size++;
	return true;
}

bool bbcbotplayerdb::loadfile(std::string filename)
{
	BBCLOG("[BBCBot DEBUG] Attempting to load file: " << filename);
	std::ifstream permissionfile(filename.c_str());
	if (!permissionfile.is_open()) {
		BBCLOG("[BBCBot DEBUG] ERROR: Could not open file: " << filename);
		BBCLOG("[BBCBot DEBUG] Current working directory might be: " << std::filesystem::current_path());
		return false;
	}
	
	BBCLOG("[BBCBot DEBUG] File opened successfully: " << filename);
	int lineCount = 0;
	std::string line;
	while(std::getline(permissionfile,line))
	{
		loadline(line);
		lineCount++;
	}
	BBCLOG("[BBCBot DEBUG] Loaded " << lineCount << " lines from " << filename);
	bool checkResult = checkcontent();
	BBCLOG("[BBCBot DEBUG] Content check result: " << (checkResult ? "OK" : "FAILED"));
	return checkResult;
}

bool bbcbotplayerdb::loadwecfile(std::string filename)
{
	BBCLOG("[BBCBot DEBUG] Attempting to load WEC file: " << filename);
	std::ifstream wecfile(filename.c_str());
	if (!wecfile.is_open()) {
		BBCLOG("[BBCBot DEBUG] ERROR: Could not open WEC file: " << filename);
		BBCLOG("[BBCBot DEBUG] Current working directory might be: " << std::filesystem::current_path());
		return false;
	}
	
	BBCLOG("[BBCBot DEBUG] WEC file opened successfully: " << filename);
	int lineCount = 0;
	std::string line;
	while(std::getline(wecfile,line))
	{
		std::string trimmed = trim_ascii(line);
		if (trimmed.empty()) {
			continue;
		}
		wecpeople.push_back(trimmed);
		lineCount++;
	}
	BBCLOG("[BBCBot DEBUG] Loaded " << lineCount << " WEC players from " << filename);
	return true;
}

int bbcbotplayerdb::suggestionscore2(int ratingpoints,int tickets,int gamescount)
{
	if(tickets<=0) return 0;
	return (tickets<<11)+(gamescount<<4)+ratingpoints;
}

int bbcbotplayerdb::suggestionscore1(int index,int step)
{
	if(index==-1) return 0;
	if(step==1) return suggestionscore2(rating[index],1,games[index]);
	if(step==2) return suggestionscore2(rating[index],ts2[index],games[index]);
	if(step==3) return suggestionscore2(rating[index],ts3[index],games[index]);
	if(step==4) return suggestionscore2(rating[index],ts4[index],games[index]);
	return 0;
}

int bbcbotplayerdb::getindex(std::string name)
{
	if(issorted)
	{
		// Binary search
		int left,right,center,eval;
		left=0;
		right=size;
		for(size_t i=0;i<size;i++)
		{
			if(right <= left) break;
			center=(left+right)/2;
			eval=name.compare(pname[center]);
			if(eval==0) return center;
			if(eval<0) right=center;
			if(eval>0) left=center+1;
		}
	}
	else
	{
		for(size_t i=0;i<size;i++)
		{
			if(name==pname[i]) return i;
		}
	}
	for(size_t i=0;i<size;i++)
	{
		if(equals_case_insensitive(name, pname[i])) return i;
	}
	return -1;
}

void bbcbotplayerdb::addidleplayer(unsigned pid)
{
	if(pid==0) return;
	idleplayers.insert(pid);
}

void bbcbotplayerdb::removeidleplayer(unsigned pid)
{
	if(pid==0) return;
	idleplayers.erase(pid);
}

void bbcbotplayerdb::printidledebug()
{
	BBCLOG("[BBCBot] idle player count: " << idleplayers.size());
	for(auto pid : idleplayers)
	{
		if(parent->GetGameIdOfPlayer(pid)) continue;
		if(parent->GetPlayerName(pid).substr(0,5)=="Guest") continue;
		BBCLOG("[BBCBot] idle player: "<<parent->GetPlayerName(pid)<<" (ID: "<<pid<<")");
	}
}

void bbcbotplayerdb::clear_idleplayers()
{
	idleplayers.clear();
}

std::string bbcbotplayerdb::int2string(int a)
{
	char buffer[16];
	sprintf(buffer,"%d",a);
	return std::string(buffer);
}

std::string bbcbotplayerdb::printrating(std::string name)
{
	int i=getindex(name);
	if(i==-1) return "ERROR: player "+name+" not found";
	return name+" has "+int2string(rating[i])+" rating points";
}

std::string bbcbotplayerdb::printtickets(std::string name)
{
	int i=getindex(name);
	if(i==-1) return "ERROR: player "+name+" not found";
	std::string retval=name+" has ";
	if(ts2[i]==1) retval+= "1 ticket";
	else if(ts2[i]==0) retval+="no ticket";
	else retval+= int2string(ts2[i])+" tickets";
	retval+=" for step 2, ";
	if(ts3[i]==1) retval+= "1 ticket";
	else if(ts3[i]==0) retval+="no ticket";
	else retval+= int2string(ts3[i])+" tickets";
	retval+=" for step 3, and ";
	if(ts4[i]==1) retval+= "1 ticket";
	else if(ts4[i]==0) retval+="no ticket";
	else retval+= int2string(ts4[i])+" tickets";
	retval+=" for step 4.";
	return retval;
}

std::string bbcbotplayerdb::printgamescount(std::string name)
{
	int i=getindex(name);
	if(i==-1) return "ERROR: player "+name+" not found";
	return name+" has played "+int2string(games[i])+" BBC games";
}

std::string bbcbotplayerdb::printsuggest(int step)
{
	return printsuggest(step,12);
}

std::string bbcbotplayerdb::printsuggest(int step,unsigned limit)
{
	std::vector<int> sindex;
	std::vector<int> sscore;

	BBCLOG("[BBCBot DEBUG] printsuggest() called for step " << step);
	int idleCount = 0;
	int validCount = 0;
	int dbCount = 0;
	int scoreCount = 0;

	std::string tempname="";
	int tempindex=-1;
	int tempscore=0;
	std::vector<int>::iterator it1,it2,it3;
	for(auto pid : idleplayers)
	{
		idleCount++;
		if(parent->GetGameIdOfPlayer(pid)) continue;
		validCount++;
		tempname=parent->GetPlayerName(pid);
		if(tempname.substr(0,5)=="Guest") continue;
		tempindex=getindex(tempname);
		if(tempindex==-1) continue;
		dbCount++;
		tempscore=suggestionscore1(tempindex,step);
		if(tempscore<=10) continue;
		scoreCount++;
		it1=sindex.begin();
		it2=sindex.end();
		it3=sscore.begin();
		while(it1!=it2)
		{
			if(tempscore >= *it3) 
			{
				sindex.insert(it1,tempindex);
				sscore.insert(it3,tempscore);
				break;
			}
			it1++;
			it3++;
		}
		if(it1==it2)
		{
			sindex.push_back(tempindex);
			sscore.push_back(tempscore);
		}
	}
	BBCLOG("[BBCBot DEBUG] Idle players: " << idleCount << ", not in game: " << validCount << ", in DB: " << dbCount << ", with score: " << scoreCount);
	if(sindex.size()==0) return "Sorry, no player found to suggest";
	tempname="I suggest the following players for step "+int2string(step)+": ";
	for(unsigned i=0;i<sindex.size() && i<limit; i++)
	{
		if(i!=0) tempname+=", ";
		tempname+=pname[sindex[i]];
	}
	return tempname;
}

std::string bbcbotplayerdb::wecsuggest()
{
	BBCLOG("[BBCBot DEBUG] wecsuggest() called");
	BBCLOG("[BBCBot DEBUG] wecpeople list has " << wecpeople.size() << " entries");
	
	unsigned limit=10;
	std::vector<int> sindex;
	std::vector<int> sscore;

	std::string tempname="";
	int tempindex=-1;
	int tempscore=0;
	std::vector<int>::iterator it1,it2,it3;
	
	int idleCount = 0;
	int wecMatchCount = 0;
	
	for(auto pid : idleplayers)
	{
		idleCount++;
		if(parent->GetGameIdOfPlayer(pid)) continue;
		tempname=parent->GetPlayerName(pid);
		if(tempname.substr(0,5)=="Guest") continue;
		tempindex=-1;
		for(unsigned i2=0;i2<wecpeople.size();i2++)
		{
			if(equals_case_insensitive(tempname, wecpeople[i2]))
			{
				tempindex=i2;
				wecMatchCount++;
				BBCLOG("[BBCBot DEBUG] Found WEC player in lobby: " << tempname);
				break;
			}
		}
		if(tempindex==-1) continue;
		tempscore=(rand()&0xefbd)|0x42;
		if(tempscore<=10) continue;
		it1=sindex.begin();
		it2=sindex.end();
		it3=sscore.begin();
		while(it1!=it2)
		{
			if(tempscore >= *it3) 
			{
				sindex.insert(it1,tempindex);
				sscore.insert(it3,tempscore);
				break;
			}
			it1++;
			it3++;
		}
		if(it1==it2)
		{
			sindex.push_back(tempindex);
			sscore.push_back(tempscore);
		}
	}
	
	BBCLOG("[BBCBot DEBUG] Total idle players: " << idleCount);
	BBCLOG("[BBCBot DEBUG] WEC players found in lobby: " << wecMatchCount);
	BBCLOG("[BBCBot DEBUG] Suggested players: " << sindex.size());
	
	if(sindex.size()==0) return "Sorry, no wec player found to suggest";
	tempname="I suggest the following players for wec: ";
	for(unsigned i=0;i<sindex.size() && i<limit; i++)
	{
		if(i!=0) tempname+=", ";
		tempname += wecpeople[sindex[i]];
	}
	return tempname;
}

// end bbcbot code
