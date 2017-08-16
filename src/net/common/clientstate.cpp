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

#include <net/clientstate.h>
#include <net/clientthread.h>
#include <net/clientcontext.h>
#include <net/senderhelper.h>
#include <net/netpacket.h>
#include <net/clientexception.h>
#include <net/socket_helper.h>
#include <net/socket_msg.h>
#include <net/downloadhelper.h>
#include <core/avatarmanager.h>
#include <core/crypthelper.h>
#include <qttoolsinterface.h>

#include <game.h>
#include <playerinterface.h>

#include <tinyxml.h>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
using namespace boost::filesystem;

#ifdef BOOST_ASIO_HAS_STD_CHRONO
using namespace std::chrono;
#else
using namespace boost::chrono;
#endif

#define CLIENT_WAIT_TIMEOUT_MSEC	50
#define CLIENT_CONNECT_TIMEOUT_SEC	10


ClientState::~ClientState()
{
}

//-----------------------------------------------------------------------------

ClientStateInit &
ClientStateInit::Instance()
{
	static ClientStateInit state;
	return state;
}

ClientStateInit::ClientStateInit()
{
}

ClientStateInit::~ClientStateInit()
{
}

void
ClientStateInit::Enter(boost::shared_ptr<ClientThread> client)
{
	ClientContext &context = client->GetContext();

	if (context.GetServerAddr().empty())
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_SERVERADDR_NOT_SET, 0);

	if (context.GetServerPort() < 1024)
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_PORT, 0);

	client->CreateContextSession();
	client->GetCallback().SignalNetClientConnect(MSG_SOCK_INIT_DONE);

	if (context.GetUseServerList())
		client->SetState(ClientStateStartServerListDownload::Instance());
	else
		client->SetState(ClientStateStartResolve::Instance());
}

void
ClientStateInit::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

ClientStateStartResolve &
ClientStateStartResolve::Instance()
{
	static ClientStateStartResolve state;
	return state;
}

ClientStateStartResolve::ClientStateStartResolve()
{
}

ClientStateStartResolve::~ClientStateStartResolve()
{
}

void
ClientStateStartResolve::Enter(boost::shared_ptr<ClientThread> client)
{
	ClientContext &context = client->GetContext();
	ostringstream portStr;
	portStr << context.GetServerPort();
	boost::asio::ip::tcp::resolver::query q(context.GetServerAddr(), portStr.str());

	context.GetResolver()->async_resolve(
		q,
		boost::bind(&ClientStateStartResolve::HandleResolve,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::iterator,
					client));
}

void
ClientStateStartResolve::Exit(boost::shared_ptr<ClientThread> client)
{
	client->GetContext().GetResolver()->cancel();
}

void
ClientStateStartResolve::HandleResolve(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
									   boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this) {
		client->GetCallback().SignalNetClientConnect(MSG_SOCK_RESOLVE_DONE);
		// Use the first resolver result.
		ClientStateStartConnect::Instance().SetRemoteEndpoint(endpoint_iterator);
		client->SetState(ClientStateStartConnect::Instance());
	} else {
		if (ec != boost::asio::error::operation_aborted)
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_RESOLVE_FAILED, 0);
	}
}

//-----------------------------------------------------------------------------

ClientStateStartServerListDownload &
ClientStateStartServerListDownload::Instance()
{
	static ClientStateStartServerListDownload state;
	return state;
}

ClientStateStartServerListDownload::ClientStateStartServerListDownload()
{
}

ClientStateStartServerListDownload::~ClientStateStartServerListDownload()
{
}

void
ClientStateStartServerListDownload::Enter(boost::shared_ptr<ClientThread> client)
{
	path tmpServerListPath(client->GetCacheServerListFileName());
	if (tmpServerListPath.empty())
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_URL, 0);

	if (exists(tmpServerListPath)) {
		// Download the current server list once a day.
		// If the previous file is older than one day, delete it.
		// Also delete the file if it is empty.
		if (file_size(tmpServerListPath) == 0 || (last_write_time(tmpServerListPath) + 86400 < time(NULL))) {
			remove(tmpServerListPath);
		}
	}

	if (exists(tmpServerListPath)) {
		// Use the existing server list.
		client->SetState(ClientStateReadingServerList::Instance());
	} else {
		// Download the server list.
		boost::shared_ptr<DownloadHelper> downloader(new DownloadHelper);
		downloader->Init(client->GetContext().GetServerListUrl(), tmpServerListPath.directory_string());
		ClientStateDownloadingServerList::Instance().SetDownloadHelper(downloader);
		client->SetState(ClientStateDownloadingServerList::Instance());
	}
}

void
ClientStateStartServerListDownload::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

ClientStateDownloadingServerList &
ClientStateDownloadingServerList::Instance()
{
	static ClientStateDownloadingServerList state;
	return state;
}

ClientStateDownloadingServerList::ClientStateDownloadingServerList()
{
}

ClientStateDownloadingServerList::~ClientStateDownloadingServerList()
{
}

void
ClientStateDownloadingServerList::Enter(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().expires_from_now(
		milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
	client->GetStateTimer().async_wait(
		boost::bind(
			&ClientStateDownloadingServerList::TimerLoop, this, boost::asio::placeholders::error, client));
}

void
ClientStateDownloadingServerList::Exit(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().cancel();
}

void
ClientStateDownloadingServerList::SetDownloadHelper(boost::shared_ptr<DownloadHelper> helper)
{
	m_downloadHelper = helper;
}

void
ClientStateDownloadingServerList::TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this) {
		if (m_downloadHelper->Process()) {
			m_downloadHelper.reset();
			client->SetState(ClientStateReadingServerList::Instance());
		} else {
			client->GetStateTimer().expires_from_now(
				milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
			client->GetStateTimer().async_wait(
				boost::bind(
					&ClientStateDownloadingServerList::TimerLoop, this, boost::asio::placeholders::error, client));
		}
	}
}

//-----------------------------------------------------------------------------

ClientStateReadingServerList &
ClientStateReadingServerList::Instance()
{
	static ClientStateReadingServerList state;
	return state;
}

ClientStateReadingServerList::ClientStateReadingServerList()
{
}

ClientStateReadingServerList::~ClientStateReadingServerList()
{
}

void
ClientStateReadingServerList::Enter(boost::shared_ptr<ClientThread> client)
{
	ClientContext &context = client->GetContext();
	path zippedServerListPath(context.GetCacheDir());
	zippedServerListPath /= context.GetServerListUrl().substr(context.GetServerListUrl().find_last_of('/') + 1);
	path xmlServerListPath;
	if (extension(zippedServerListPath) == ".z") {
		xmlServerListPath = change_extension(zippedServerListPath, "");

		// Unzip the file using zlib.
		try {
			std::ifstream inFile(zippedServerListPath.directory_string().c_str(), ios_base::in | ios_base::binary);
			std::ofstream outFile(xmlServerListPath.directory_string().c_str(), ios_base::out | ios_base::trunc);
			boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
			in.push(boost::iostreams::zlib_decompressor());
			in.push(inFile);
			boost::iostreams::copy(in, outFile);
		} catch (...) {
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_UNZIP_FAILED, 0);
		}
	} else
		xmlServerListPath = zippedServerListPath;

	// Parse the server address.
	TiXmlDocument doc(xmlServerListPath.directory_string());

	if (doc.LoadFile()) {
		client->ClearServerInfoMap();
		int serverCount = 0;
		unsigned lastServerInfoId = 0;
		TiXmlHandle docHandle(&doc);
		const TiXmlElement *nextServer = docHandle.FirstChild("ServerList" ).FirstChild("Server").ToElement();
		while (nextServer) {
			ServerInfo serverInfo;
			{
				int tmpId;
				nextServer->QueryIntAttribute("id", &tmpId);
				serverInfo.id = (unsigned)tmpId;
			}
			const TiXmlNode *nameNode = nextServer->FirstChild("Name");
			const TiXmlNode *sponsorNode = nextServer->FirstChild("Sponsor");
			const TiXmlNode *countryNode = nextServer->FirstChild("Country");
			const TiXmlNode *addr4Node = nextServer->FirstChild("IPv4Address");
			const TiXmlNode *addr6Node = nextServer->FirstChild("IPv6Address");
			const TiXmlNode *sctpNode = nextServer->FirstChild("SCTP");
			const TiXmlNode *portNode = nextServer->FirstChild("ProtobufPort");

			// IPv6 support for avatar servers depends on this address and on libcurl.
			const TiXmlNode *avatarNode = nextServer->FirstChild("AvatarServerAddress");

			if (!nameNode || !nameNode->ToElement() || !addr4Node || !addr4Node->ToElement()
					|| !addr6Node || !addr6Node->ToElement() || !portNode || !portNode->ToElement())
				throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_XML, 0);

			serverInfo.name = nameNode->ToElement()->Attribute("value");
			serverInfo.ipv4addr = addr4Node->ToElement()->Attribute("value");
			serverInfo.ipv6addr = addr6Node->ToElement()->Attribute("value");
			portNode->ToElement()->QueryIntAttribute("value", &serverInfo.port);

			// Optional parameters:
			if (sponsorNode && sponsorNode->ToElement())
				serverInfo.sponsor = sponsorNode->ToElement()->Attribute("value");
			if (countryNode && countryNode->ToElement())
				serverInfo.country = countryNode->ToElement()->Attribute("value");
			if (sctpNode && sctpNode->ToElement()) {
				int tmpSctp;
				sctpNode->ToElement()->QueryIntAttribute("value", &tmpSctp);
				serverInfo.supportsSctp = tmpSctp == 1 ? true : false;
			}
			if (avatarNode && avatarNode->ToElement())
				serverInfo.avatarServerAddr = avatarNode->ToElement()->Attribute("value");

			client->AddServerInfo(serverInfo.id, serverInfo);
			nextServer = nextServer->NextSiblingElement();
			lastServerInfoId = serverInfo.id;
			serverCount++;
		}

		if (serverCount == 1) {
			client->UseServer(lastServerInfoId);
			client->GetCallback().SignalNetClientConnect(MSG_SOCK_SERVER_LIST_DONE);
			client->SetState(ClientStateStartResolve::Instance());
		} else if (serverCount > 1) {
			client->GetCallback().SignalNetClientServerListShow();
			client->SetState(ClientStateWaitChooseServer::Instance());
		} else
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_XML, 0);
	} else
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_XML, 0);
}

void
ClientStateReadingServerList::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

ClientStateWaitChooseServer &
ClientStateWaitChooseServer::Instance()
{
	static ClientStateWaitChooseServer state;
	return state;
}

ClientStateWaitChooseServer::ClientStateWaitChooseServer()
{
}

ClientStateWaitChooseServer::~ClientStateWaitChooseServer()
{
}

void
ClientStateWaitChooseServer::Enter(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().expires_from_now(
		milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
	client->GetStateTimer().async_wait(
		boost::bind(
			&ClientStateWaitChooseServer::TimerLoop, this, boost::asio::placeholders::error, client));
}

void
ClientStateWaitChooseServer::Exit(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().cancel();
}

void
ClientStateWaitChooseServer::TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this) {
		unsigned serverId;
		if (client->GetSelectedServer(serverId)) {
			client->UseServer(serverId);
			client->GetCallback().SignalNetClientConnect(MSG_SOCK_SERVER_LIST_DONE);
			client->SetState(ClientStateStartResolve::Instance());
		} else {
			client->GetStateTimer().expires_from_now(
				milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
			client->GetStateTimer().async_wait(
				boost::bind(
					&ClientStateWaitChooseServer::TimerLoop, this, boost::asio::placeholders::error, client));
		}
	}
}

//-----------------------------------------------------------------------------

ClientStateStartConnect &
ClientStateStartConnect::Instance()
{
	static ClientStateStartConnect state;
	return state;
}

ClientStateStartConnect::ClientStateStartConnect()
{
}

ClientStateStartConnect::~ClientStateStartConnect()
{
}

void
ClientStateStartConnect::Enter(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().expires_from_now(
		seconds(CLIENT_CONNECT_TIMEOUT_SEC));
	client->GetStateTimer().async_wait(
		boost::bind(
			&ClientStateStartConnect::TimerTimeout, this, boost::asio::placeholders::error, client));

	boost::asio::ip::tcp::endpoint endpoint = *m_remoteEndpointIterator;
	client->GetContext().GetSessionData()->GetAsioSocket()->async_connect(
		endpoint,
		boost::bind(&ClientStateStartConnect::HandleConnect,
					this,
					boost::asio::placeholders::error,
					++m_remoteEndpointIterator,
					client));
}

void
ClientStateStartConnect::Exit(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().cancel();
}

void
ClientStateStartConnect::SetRemoteEndpoint(boost::asio::ip::tcp::resolver::iterator endpointIterator)
{
	m_remoteEndpointIterator = endpointIterator;
}

void
ClientStateStartConnect::HandleConnect(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
									   boost::shared_ptr<ClientThread> client)
{
	if (&client->GetState() == this) {
		if (!ec) {
			client->GetCallback().SignalNetClientConnect(MSG_SOCK_CONNECT_DONE);
			client->SetState(ClientStateStartSession::Instance());
		} else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator()) {
			// Try next resolve entry.
			ClientContext &context = client->GetContext();
			boost::system::error_code ec;
			context.GetSessionData()->GetAsioSocket()->close(ec);
			boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
			context.GetSessionData()->GetAsioSocket()->async_connect(
				endpoint,
				boost::bind(&ClientStateStartConnect::HandleConnect,
							this,
							boost::asio::placeholders::error,
							++m_remoteEndpointIterator,
							client));
		} else {
			if (ec != boost::asio::error::operation_aborted) {
				if (client->GetContext().GetAddrFamily() == AF_INET6) {
					throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_IPV6_FAILED, ec.value());
				} else {
					throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_FAILED, ec.value());
				}
			}
		}
	}
}

void
ClientStateStartConnect::TimerTimeout(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this) {
		boost::system::error_code ec;
		client->GetContext().GetSessionData()->GetAsioSocket()->close(ec);
		if (client->GetContext().GetAddrFamily() == AF_INET6) {
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_IPV6_TIMEOUT, 0);
		} else {
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_TIMEOUT, 0);
		}
	}
}

//-----------------------------------------------------------------------------

AbstractClientStateReceiving::AbstractClientStateReceiving()
{
}

AbstractClientStateReceiving::~AbstractClientStateReceiving()
{
}

void
AbstractClientStateReceiving::HandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_PlayerInfoReplyMessage) {
		const PlayerInfoReplyMessage &infoReply = tmpPacket->GetMsg()->playerinforeplymessage();
		unsigned playerId = infoReply.playerid();
		if (infoReply.has_playerinfodata()) {
			PlayerInfo tmpInfo;
			const PlayerInfoReplyMessage::PlayerInfoData &netInfo = infoReply.playerinfodata();
			tmpInfo.playerName = netInfo.playername();
			tmpInfo.ptype = netInfo.ishuman() ? PLAYER_TYPE_HUMAN : PLAYER_TYPE_COMPUTER;
			tmpInfo.isGuest = netInfo.playerrights() == netPlayerRightsGuest;
			tmpInfo.isAdmin = netInfo.playerrights() == netPlayerRightsAdmin;
			if (netInfo.has_countrycode()) {
				tmpInfo.countryCode = netInfo.countrycode();
			}
			if (netInfo.has_avatardata()) {
				tmpInfo.hasAvatar = true;
				memcpy(tmpInfo.avatar.GetData(), netInfo.avatardata().avatarhash().data(), MD5_DATA_SIZE);
				tmpInfo.avatarType = static_cast<AvatarFileType>(netInfo.avatardata().avatartype());
			}
			client->SetPlayerInfo(
				playerId,
				tmpInfo);
		} else {
			client->SetUnknownPlayer(playerId);
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_RemovedFromGameMessage) {
		const RemovedFromGameMessage &netRemoved = tmpPacket->GetMsg()->removedfromgamemessage();

		client->ClearPlayerDataList();
		// Resubscribe Lobby messages.
		client->ResubscribeLobbyMsg();
		// Show Lobby.
		client->GetCallback().SignalNetClientWaitDialog();
		int removeReason;
		switch (netRemoved.removedfromgamereason()) {
		case RemovedFromGameMessage::kickedFromGame :
			removeReason = NTF_NET_REMOVED_KICKED;
			break;
		case RemovedFromGameMessage::gameIsFull :
			removeReason = NTF_NET_REMOVED_GAME_FULL;
			break;
		case RemovedFromGameMessage::gameIsRunning :
			removeReason = NTF_NET_REMOVED_ALREADY_RUNNING;
			break;
		case RemovedFromGameMessage::gameTimeout :
			removeReason = NTF_NET_REMOVED_TIMEOUT;
			break;
		case RemovedFromGameMessage::removedStartFailed :
			removeReason = NTF_NET_REMOVED_START_FAILED;
			break;
		case RemovedFromGameMessage::gameClosed :
			removeReason = NTF_NET_REMOVED_GAME_CLOSED;
			break;
		default :
			removeReason = NTF_NET_REMOVED_ON_REQUEST;
			break;
		}
		client->GetCallback().SignalNetClientRemovedFromGame(removeReason);
		client->SetState(ClientStateWaitJoin::Instance());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GamePlayerLeftMessage) {
		// A player left the game.
		const GamePlayerLeftMessage &netLeft = tmpPacket->GetMsg()->gameplayerleftmessage();

		if (client->GetGame()) {
			boost::shared_ptr<PlayerInterface> tmpPlayer = client->GetGame()->getPlayerByUniqueId(netLeft.playerid());
			if (tmpPlayer) {
				tmpPlayer->setIsKicked(netLeft.gameplayerleftreason() == GamePlayerLeftMessage::leftKicked);
			}
		}
		// Signal to GUI and remove from data list.
		int removeReason;
		switch (netLeft.gameplayerleftreason()) {
		case GamePlayerLeftMessage::leftKicked :
			removeReason = NTF_NET_REMOVED_KICKED;
			break;
		default :
			removeReason = NTF_NET_REMOVED_ON_REQUEST;
			break;
		}
		client->RemovePlayerData(netLeft.playerid(), removeReason);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameAdminChangedMessage) {
		// New admin for the game.
		const GameAdminChangedMessage &netChanged = tmpPacket->GetMsg()->gameadminchangedmessage();

		// Set new game admin and signal to GUI.
		client->SetNewGameAdmin(netChanged.newadminplayerid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GamePlayerJoinedMessage) {
		// Another player joined the network game.
		const GamePlayerJoinedMessage &netPlayerJoined = tmpPacket->GetMsg()->gameplayerjoinedmessage();

		boost::shared_ptr<PlayerData> playerData = client->CreatePlayerData(netPlayerJoined.playerid(), netPlayerJoined.isgameadmin());
		client->AddPlayerData(playerData);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameSpectatorJoinedMessage) {
		// Another spectator joined the network game.
		const GameSpectatorJoinedMessage &netSpectatorJoined = tmpPacket->GetMsg()->gamespectatorjoinedmessage();
		// Request player info if needed.
		PlayerInfo info;
		if (!client->GetCachedPlayerInfo(netSpectatorJoined.playerid(), info)) {
			client->RequestPlayerInfo(netSpectatorJoined.playerid());
		}
		client->ModifyGameInfoAddSpectatorDuringGame(netSpectatorJoined.playerid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameSpectatorLeftMessage) {
		// A spectator left the network game.
		const GameSpectatorLeftMessage &netSpectatorLeft = tmpPacket->GetMsg()->gamespectatorleftmessage();
		// Signal to GUI and remove from data list.
		int removeReason;
		switch (netSpectatorLeft.gamespectatorleftreason()) {
		case GamePlayerLeftMessage::leftKicked :
			removeReason = NTF_NET_REMOVED_KICKED;
			break;
		default :
			removeReason = NTF_NET_REMOVED_ON_REQUEST;
			break;
		}
		client->ModifyGameInfoRemoveSpectatorDuringGame(netSpectatorLeft.playerid(), removeReason);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_TimeoutWarningMessage) {
		const TimeoutWarningMessage &tmpTimeout = tmpPacket->GetMsg()->timeoutwarningmessage();
		client->GetCallback().SignalNetClientShowTimeoutDialog((NetTimeoutReason)tmpTimeout.timeoutreason(), tmpTimeout.remainingseconds());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_ChatMessage) {
		// Chat message - display it in the GUI.
		const ChatMessage &netMessage = tmpPacket->GetMsg()->chatmessage();

		string playerName;
		if (netMessage.chattype() == ChatMessage::chatTypeBroadcast) {
			client->GetCallback().SignalNetClientGameChatMsg("(global notice)", netMessage.chattext());
			client->GetCallback().SignalNetClientLobbyChatMsg("(global notice)", netMessage.chattext());
		} else if (netMessage.chattype() == ChatMessage::chatTypeBot) {
			client->GetCallback().SignalNetClientGameChatMsg("(chat bot)", netMessage.chattext());
			client->GetCallback().SignalNetClientLobbyChatMsg("(chat bot)", netMessage.chattext());
		} else if (netMessage.chattype() == ChatMessage::chatTypeGame) {
			unsigned playerId = netMessage.playerid();
			boost::shared_ptr<PlayerData> tmpPlayer = client->GetPlayerDataByUniqueId(playerId);
			if (tmpPlayer.get())
				playerName = tmpPlayer->GetName();
			if (!playerName.empty())
				client->GetCallback().SignalNetClientGameChatMsg(playerName, netMessage.chattext());
		} else if (netMessage.chattype() == ChatMessage::chatTypeLobby) {
			unsigned playerId = netMessage.playerid();
			PlayerInfo info;
			if (client->GetCachedPlayerInfo(playerId, info))
				client->GetCallback().SignalNetClientLobbyChatMsg(info.playerName, netMessage.chattext());
		} else if (netMessage.chattype() == ChatMessage::chatTypePrivate) {
			unsigned playerId = netMessage.playerid();
			PlayerInfo info;
			if (client->GetCachedPlayerInfo(playerId, info))
				client->GetCallback().SignalNetClientPrivateChatMsg(info.playerName, netMessage.chattext());
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_ChatRejectMessage) {
		const ChatRejectMessage &netMessage = tmpPacket->GetMsg()->chatrejectmessage();
		client->GetCallback().SignalNetClientGameChatMsg("(notice)", "Chat rejected: " + netMessage.chattext());
		client->GetCallback().SignalNetClientLobbyChatMsg("(notice)", "Chat rejected: " + netMessage.chattext());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_DialogMessage) {
		// Message box - display it in the GUI.
		const DialogMessage &netDialog = tmpPacket->GetMsg()->dialogmessage();
		client->GetCallback().SignalNetClientMsgBox(netDialog.notificationtext());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_PlayerListMessage) {
		const PlayerListMessage &netPlayerList = tmpPacket->GetMsg()->playerlistmessage();

		if (netPlayerList.playerlistnotification() == PlayerListMessage::playerListNew) {
			client->GetCallback().SignalLobbyPlayerJoined(netPlayerList.playerid(), client->GetPlayerName(netPlayerList.playerid()));
		} else if (netPlayerList.playerlistnotification() == PlayerListMessage::playerListLeft) {
			client->GetCallback().SignalLobbyPlayerLeft(netPlayerList.playerid());
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListNewMessage) {
		// A new game was created on the server.
		const GameListNewMessage &netListNew = tmpPacket->GetMsg()->gamelistnewmessage();

		// Request player info for players if needed.
		GameInfo tmpInfo;
		list<unsigned> requestList;
		// All players.
		for (int i = 0; i < netListNew.playerids_size(); i++) {
			PlayerInfo info;
			unsigned playerId = netListNew.playerids(i);
			if (!client->GetCachedPlayerInfo(playerId, info)) {
				requestList.push_back(playerId);
			}
			tmpInfo.players.push_back(playerId);
		}
		// All spectators.
		for (int i = 0; i < netListNew.spectatorids_size(); i++) {
			PlayerInfo info;
			unsigned playerId = netListNew.spectatorids(i);
			if (!client->GetCachedPlayerInfo(playerId, info)) {
				requestList.push_back(playerId);
			}
			tmpInfo.spectators.push_back(playerId);
		}
		// Send request for multiple players (will only act if list is non-empty).
		client->RequestPlayerInfo(requestList);

		tmpInfo.adminPlayerId = netListNew.adminplayerid();
		tmpInfo.isPasswordProtected = netListNew.isprivate();
		tmpInfo.mode = static_cast<GameMode>(netListNew.gamemode());
		tmpInfo.name = netListNew.gameinfo().gamename();
		NetPacket::GetGameData(netListNew.gameinfo(), tmpInfo.data);

		client->AddGameInfo(netListNew.gameid(), tmpInfo);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListUpdateMessage) {
		// An existing game was updated on the server.
		const GameListUpdateMessage &netListUpdate = tmpPacket->GetMsg()->gamelistupdatemessage();
		if (netListUpdate.gamemode() == netGameClosed)
			client->RemoveGameInfo(netListUpdate.gameid());
		else
			client->UpdateGameInfoMode(netListUpdate.gameid(), static_cast<GameMode>(netListUpdate.gamemode()));
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListPlayerJoinedMessage) {
		const GameListPlayerJoinedMessage &netListJoined = tmpPacket->GetMsg()->gamelistplayerjoinedmessage();

		client->ModifyGameInfoAddPlayer(netListJoined.gameid(), netListJoined.playerid());
		// Request player info if needed.
		PlayerInfo info;
		if (!client->GetCachedPlayerInfo(netListJoined.playerid(), info)) {
			client->RequestPlayerInfo(netListJoined.playerid());
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListPlayerLeftMessage) {
		const GameListPlayerLeftMessage &netListLeft = tmpPacket->GetMsg()->gamelistplayerleftmessage();

		client->ModifyGameInfoRemovePlayer(netListLeft.gameid(), netListLeft.playerid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListSpectatorJoinedMessage) {
		const GameListSpectatorJoinedMessage &netListJoined = tmpPacket->GetMsg()->gamelistspectatorjoinedmessage();

		client->ModifyGameInfoAddSpectator(netListJoined.gameid(), netListJoined.playerid());
		// Request player info if needed.
		PlayerInfo info;
		if (!client->GetCachedPlayerInfo(netListJoined.playerid(), info)) {
			client->RequestPlayerInfo(netListJoined.playerid());
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListSpectatorLeftMessage) {
		const GameListSpectatorLeftMessage &netListLeft = tmpPacket->GetMsg()->gamelistspectatorleftmessage();

		client->ModifyGameInfoRemoveSpectator(netListLeft.gameid(), netListLeft.playerid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListAdminChangedMessage) {
		const GameListAdminChangedMessage &netListAdmin = tmpPacket->GetMsg()->gamelistadminchangedmessage();

		client->UpdateGameInfoAdmin(netListAdmin.gameid(), netListAdmin.newadminplayerid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_StartKickPetitionMessage) {
		const StartKickPetitionMessage &netStartPetition = tmpPacket->GetMsg()->startkickpetitionmessage();
		client->StartPetition(netStartPetition.petitionid(), netStartPetition.proposingplayerid(),
							  netStartPetition.kickplayerid(), netStartPetition.kicktimeoutsec(), netStartPetition.numvotesneededtokick());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_KickPetitionUpdateMessage) {
		const KickPetitionUpdateMessage &netPetitionUpdate = tmpPacket->GetMsg()->kickpetitionupdatemessage();
		client->UpdatePetition(netPetitionUpdate.petitionid(), netPetitionUpdate.numvotesagainstkicking(),
							   netPetitionUpdate.numvotesinfavourofkicking(), netPetitionUpdate.numvotesneededtokick());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_EndKickPetitionMessage) {
		const EndKickPetitionMessage &netEndPetition = tmpPacket->GetMsg()->endkickpetitionmessage();
		client->EndPetition(netEndPetition.petitionid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarHeaderMessage) {
		const AvatarHeaderMessage &netAvatarHeader = tmpPacket->GetMsg()->avatarheadermessage();
		client->AddTempAvatarFile(netAvatarHeader.requestid(), netAvatarHeader.avatarsize(), static_cast<AvatarFileType>(netAvatarHeader.avatartype()));
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarDataMessage) {
		const AvatarDataMessage &netAvatarData = tmpPacket->GetMsg()->avatardatamessage();
		vector<unsigned char> fileData(netAvatarData.avatarblock().size());
		memcpy(&fileData[0], netAvatarData.avatarblock().data(), netAvatarData.avatarblock().size());
		client->StoreInTempAvatarFile(netAvatarData.requestid(), fileData);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarEndMessage) {
		const AvatarEndMessage &netAvatarEnd = tmpPacket->GetMsg()->avatarendmessage();
		client->CompleteTempAvatarFile(netAvatarEnd.requestid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_UnknownAvatarMessage) {
		const UnknownAvatarMessage &netUnknownAvatar = tmpPacket->GetMsg()->unknownavatarmessage();
		client->SetUnknownAvatar(netUnknownAvatar.requestid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_ReportAvatarAckMessage) {
		const ReportAvatarAckMessage &netReportAck = tmpPacket->GetMsg()->reportavatarackmessage();
		unsigned msgCode;
		switch (netReportAck.reportavatarresult()) {
		case ReportAvatarAckMessage::avatarReportAccepted:
			msgCode = MSG_NET_AVATAR_REPORT_ACCEPTED;
			break;
		case ReportAvatarAckMessage::avatarReportDuplicate:
			msgCode = MSG_NET_AVATAR_REPORT_DUP;
			break;
		default:
			msgCode = MSG_NET_AVATAR_REPORT_REJECTED;
			break;
		}
		client->GetCallback().SignalNetClientMsgBox(msgCode);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_ReportGameAckMessage) {
		const ReportGameAckMessage &netReportAck = tmpPacket->GetMsg()->reportgameackmessage();
		unsigned msgCode;
		switch (netReportAck.reportgameresult()) {
		case ReportGameAckMessage::gameReportAccepted:
			msgCode = MSG_NET_GAMENAME_REPORT_ACCEPTED;
			break;
		case ReportGameAckMessage::gameReportDuplicate:
			msgCode = MSG_NET_GAMENAME_REPORT_DUP;
			break;
		default:
			msgCode = MSG_NET_GAMENAME_REPORT_REJECTED;
			break;
		}
		client->GetCallback().SignalNetClientMsgBox(msgCode);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AdminRemoveGameAckMessage) {
		const AdminRemoveGameAckMessage &netRemoveAck = tmpPacket->GetMsg()->adminremovegameackmessage();
		unsigned msgCode;
		switch (netRemoveAck.removegameresult()) {
		case AdminRemoveGameAckMessage::gameRemoveAccepted:
			msgCode = MSG_NET_ADMIN_REMOVE_GAME_ACCEPTED;
			break;
		default:
			msgCode = MSG_NET_ADMIN_REMOVE_GAME_REJECTED;
			break;
		}
		client->GetCallback().SignalNetClientMsgBox(msgCode);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AdminBanPlayerAckMessage) {
		const AdminBanPlayerAckMessage &netBanAck = tmpPacket->GetMsg()->adminbanplayerackmessage();
		unsigned msgCode;
		switch (netBanAck.banplayerresult()) {
		case AdminBanPlayerAckMessage::banPlayerAccepted:
			msgCode = MSG_NET_ADMIN_BAN_PLAYER_ACCEPTED;
			break;
		case AdminBanPlayerAckMessage::banPlayerPending:
			msgCode = MSG_NET_ADMIN_BAN_PLAYER_PENDING;
			break;
		case AdminBanPlayerAckMessage::banPlayerNoDB:
			msgCode = MSG_NET_ADMIN_BAN_PLAYER_NODB;
			break;
		case AdminBanPlayerAckMessage::banPlayerDBError:
			msgCode = MSG_NET_ADMIN_BAN_PLAYER_DBERROR;
			break;
		default:
			msgCode = MSG_NET_ADMIN_BAN_PLAYER_REJECTED;
			break;
		}
		client->GetCallback().SignalNetClientMsgBox(msgCode);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_StatisticsMessage) {
		const StatisticsMessage &netStatistics = tmpPacket->GetMsg()->statisticsmessage();

		unsigned numStats = netStatistics.statisticsdata_size();
		// Request player info for players if needed.
		if (numStats) {
			ServerStats tmpStats;
			for (unsigned i = 0; i < numStats; i++) {
				if (netStatistics.statisticsdata(i).statisticstype() == StatisticsMessage::StatisticsData::statNumberOfPlayers)
					tmpStats.numberOfPlayersOnServer = netStatistics.statisticsdata(i).statisticsvalue();
			}
			client->UpdateStatData(tmpStats);
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_ErrorMessage) {
		// Server reported an error.
		const ErrorMessage &netError = tmpPacket->GetMsg()->errormessage();
		// Show the error.
		throw ClientException(__FILE__, __LINE__, NetPacket::NetErrorToGameError(netError.errorreason()), 0);
	}

	InternalHandlePacket(client, tmpPacket);
}

//-----------------------------------------------------------------------------

ClientStateStartSession &
ClientStateStartSession::Instance()
{
	static ClientStateStartSession state;
	return state;
}

ClientStateStartSession::ClientStateStartSession()
{
}

ClientStateStartSession::~ClientStateStartSession()
{
}

void
ClientStateStartSession::Enter(boost::shared_ptr<ClientThread> client)
{
	// Now we finally start receiving data.
	client->StartAsyncRead();
}

void
ClientStateStartSession::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateStartSession::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AnnounceMessage) {
		// Server has send announcement - check data.
		const AnnounceMessage &netAnnounce = tmpPacket->GetMsg()->announcemessage();
		// Check current game version.
		if (netAnnounce.latestgameversion().majorversion() != POKERTH_VERSION_MAJOR
				|| netAnnounce.latestgameversion().minorversion() != POKERTH_VERSION_MINOR) {
			client->GetCallback().SignalNetClientNotification(NTF_NET_NEW_RELEASE_AVAILABLE);
		} else if (POKERTH_BETA_REVISION && netAnnounce.latestbetarevision() != POKERTH_BETA_REVISION) {
			client->GetCallback().SignalNetClientNotification(NTF_NET_OUTDATED_BETA);
		}
		ClientContext &context = client->GetContext();

		// CASE 1: Authenticated login (username, challenge/response for password).
		if (netAnnounce.servertype() == AnnounceMessage::serverTypeInternetAuth) {
			client->GetCallback().SignalNetClientLoginShow();
			client->SetState(ClientStateWaitEnterLogin::Instance());
		}
		// CASE 2: Unauthenticated login (network game or dedicated server without auth backend).
		else if (netAnnounce.servertype() == AnnounceMessage::serverTypeInternetNoAuth
				 || netAnnounce.servertype() == AnnounceMessage::serverTypeLAN) {
			boost::shared_ptr<NetPacket> init(new NetPacket);
			init->GetMsg()->set_messagetype(PokerTHMessage::Type_InitMessage);
			InitMessage *netInit = init->GetMsg()->mutable_initmessage();
			netInit->mutable_requestedversion()->set_majorversion(NET_VERSION_MAJOR);
			netInit->mutable_requestedversion()->set_minorversion(NET_VERSION_MINOR);
			netInit->set_buildid(0);
			if (!context.GetSessionGuid().empty()) {
				netInit->set_mylastsessionid(context.GetSessionGuid());
			}
			if (!context.GetServerPassword().empty()) {
				netInit->set_authserverpassword(context.GetServerPassword());
			}
			netInit->set_login(InitMessage::unauthenticatedLogin);
			netInit->set_nickname(context.GetPlayerName());
			string avatarFile = client->GetQtToolsInterface().stringFromUtf8(context.GetAvatarFile());
			if (!avatarFile.empty()) {
				MD5Buf tmpMD5;
				if (client->GetAvatarManager().GetHashForAvatar(avatarFile, tmpMD5)) {
					// Send MD5 hash of avatar.
					netInit->set_avatarhash(tmpMD5.GetData(), MD5_DATA_SIZE);
				}
			}
			client->GetSender().Send(context.GetSessionData(), init);
			client->SetState(ClientStateWaitSession::Instance());
		}
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitEnterLogin &
ClientStateWaitEnterLogin::Instance()
{
	static ClientStateWaitEnterLogin state;
	return state;
}

ClientStateWaitEnterLogin::ClientStateWaitEnterLogin()
{
}

ClientStateWaitEnterLogin::~ClientStateWaitEnterLogin()
{
}

void
ClientStateWaitEnterLogin::Enter(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().expires_from_now(
		milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
	client->GetStateTimer().async_wait(
		boost::bind(
			&ClientStateWaitEnterLogin::TimerLoop, this, boost::asio::placeholders::error, client));
}

void
ClientStateWaitEnterLogin::Exit(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().cancel();
}

void
ClientStateWaitEnterLogin::HandlePacket(boost::shared_ptr<ClientThread> /*client*/, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_ErrorMessage) {
		// Server reported an error.
		const ErrorMessage &netError = tmpPacket->GetMsg()->errormessage();
		// Show the error.
		throw ClientException(__FILE__, __LINE__, NetPacket::NetErrorToGameError(netError.errorreason()), 0);
	}
}

void
ClientStateWaitEnterLogin::TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this) {
		ClientThread::LoginData loginData;
		if (client->GetLoginData(loginData)) {
			ClientContext &context = client->GetContext();
			boost::shared_ptr<NetPacket> init(new NetPacket);
			init->GetMsg()->set_messagetype(PokerTHMessage::Type_InitMessage);
			InitMessage *netInit = init->GetMsg()->mutable_initmessage();
			netInit->mutable_requestedversion()->set_majorversion(NET_VERSION_MAJOR);
			netInit->mutable_requestedversion()->set_minorversion(NET_VERSION_MINOR);
			netInit->set_buildid(0);
			if (!context.GetSessionGuid().empty()) {
				netInit->set_mylastsessionid(context.GetSessionGuid());
			}
			if (!context.GetServerPassword().empty()) {
				netInit->set_authserverpassword(context.GetServerPassword());
			}

			context.SetPlayerName(loginData.userName);

			// Handle guest login first.
			if (loginData.isGuest) {
				context.SetPassword("");
				context.SetPlayerRights(PLAYER_RIGHTS_GUEST);
				netInit->set_login(InitMessage::guestLogin);
				netInit->set_nickname(context.GetPlayerName());

				client->GetSender().Send(context.GetSessionData(), init);
				client->SetState(ClientStateWaitSession::Instance());
			}
			// If the player is not a guest, authenticate.
			else {
				context.SetPassword(loginData.password);
				netInit->set_login(InitMessage::authenticatedLogin);
				// Send authentication user data for challenge/response in init.
				boost::shared_ptr<SessionData> tmpSession = context.GetSessionData();
				tmpSession->CreateClientAuthSession(client->GetAuthContext(), context.GetPlayerName(), context.GetPassword());
				if (!tmpSession->AuthStep(1, ""))
					throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD, 0);
				string outUserData(tmpSession->AuthGetNextOutMsg());
				netInit->set_clientuserdata(outUserData);
				string avatarFile = client->GetQtToolsInterface().stringFromUtf8(context.GetAvatarFile());
				if (!avatarFile.empty()) {
					MD5Buf tmpMD5;
					if (client->GetAvatarManager().GetHashForAvatar(avatarFile, tmpMD5)) {
						// TODO: use sha1.
						netInit->set_avatarhash(tmpMD5.GetData(), MD5_DATA_SIZE);
					}
				}
				client->GetSender().Send(context.GetSessionData(), init);
				client->SetState(ClientStateWaitAuthChallenge::Instance());
			}
		} else {
			client->GetStateTimer().expires_from_now(
				milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
			client->GetStateTimer().async_wait(
				boost::bind(
					&ClientStateWaitEnterLogin::TimerLoop, this, boost::asio::placeholders::error, client));
		}
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitAuthChallenge &
ClientStateWaitAuthChallenge::Instance()
{
	static ClientStateWaitAuthChallenge state;
	return state;
}

ClientStateWaitAuthChallenge::ClientStateWaitAuthChallenge()
{
}

ClientStateWaitAuthChallenge::~ClientStateWaitAuthChallenge()
{
}

void
ClientStateWaitAuthChallenge::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitAuthChallenge::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitAuthChallenge::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AuthServerChallengeMessage) {
		const AuthServerChallengeMessage &netAuth = tmpPacket->GetMsg()->authserverchallengemessage();
		string challengeStr(netAuth.serverchallenge());
		boost::shared_ptr<SessionData> tmpSession = client->GetContext().GetSessionData();
		if (!tmpSession->AuthStep(2, challengeStr.c_str()))
			throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD, 0);
		string outUserData(tmpSession->AuthGetNextOutMsg());

		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AuthClientResponseMessage);
		AuthClientResponseMessage *outAuth = packet->GetMsg()->mutable_authclientresponsemessage();
		outAuth->set_clientresponse(outUserData);
		client->GetSender().Send(tmpSession, packet);
		client->SetState(ClientStateWaitAuthVerify::Instance());
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitAuthVerify &
ClientStateWaitAuthVerify::Instance()
{
	static ClientStateWaitAuthVerify state;
	return state;
}

ClientStateWaitAuthVerify::ClientStateWaitAuthVerify()
{
}

ClientStateWaitAuthVerify::~ClientStateWaitAuthVerify()
{
}

void
ClientStateWaitAuthVerify::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitAuthVerify::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitAuthVerify::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AuthServerVerificationMessage) {
		// Check subtype.
		const AuthServerVerificationMessage &netAuth = tmpPacket->GetMsg()->authserververificationmessage();
		string verificationStr(netAuth.serververification());
		boost::shared_ptr<SessionData> tmpSession = client->GetContext().GetSessionData();
		if (!tmpSession->AuthStep(3, verificationStr.c_str()))
			throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD, 0);

		client->SetState(ClientStateWaitSession::Instance());
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitSession &
ClientStateWaitSession::Instance()
{
	static ClientStateWaitSession state;
	return state;
}

ClientStateWaitSession::ClientStateWaitSession()
{
}

ClientStateWaitSession::~ClientStateWaitSession()
{
}

void
ClientStateWaitSession::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitSession::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitSession::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_InitAckMessage) {
		// Everything is fine - we are in the lobby.
		const InitAckMessage &netInitAck = tmpPacket->GetMsg()->initackmessage();
		client->SetGuiPlayerId(netInitAck.yourplayerid());

		client->GetContext().SetSessionGuid(netInitAck.yoursessionid());
		client->SetSessionEstablished(true);
		client->GetCallback().SignalNetClientConnect(MSG_SOCK_SESSION_DONE);
		if (netInitAck.has_rejoingameid())
			client->GetCallback().SignalNetClientRejoinPossible(netInitAck.rejoingameid());
		client->SetState(ClientStateWaitJoin::Instance());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarRequestMessage) {
		// Before letting us join the lobby, the server requests our avatar.
		const AvatarRequestMessage &netAvatarRequest = tmpPacket->GetMsg()->avatarrequestmessage();

		// TODO compare SHA1.
		NetPacketList tmpList;
		int avatarError = client->GetAvatarManager().AvatarFileToNetPackets(
							  client->GetQtToolsInterface().stringFromUtf8(client->GetContext().GetAvatarFile()),
							  netAvatarRequest.requestid(),
							  tmpList);

		if (!avatarError)
			client->GetSender().Send(client->GetContext().GetSessionData(), tmpList);
		else
			throw ClientException(__FILE__, __LINE__, avatarError, 0);
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitJoin &
ClientStateWaitJoin::Instance()
{
	static ClientStateWaitJoin state;
	return state;
}

ClientStateWaitJoin::ClientStateWaitJoin()
{
}

ClientStateWaitJoin::~ClientStateWaitJoin()
{
}

void
ClientStateWaitJoin::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitJoin::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitJoin::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	ClientContext &context = client->GetContext();

	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_JoinGameAckMessage) {
		const JoinGameAckMessage &netJoinAck = tmpPacket->GetMsg()->joingameackmessage();
		// Successfully joined a game.
		client->SetGameId(netJoinAck.gameid());
		GameData tmpData;
		NetPacket::GetGameData(netJoinAck.gameinfo(), tmpData);
		client->SetGameData(tmpData);
		client->ModifyGameInfoClearSpectatorsDuringGame();

		// Player number is 0 on init. Will be set when the game starts.
		boost::shared_ptr<PlayerData> playerData(
			new PlayerData(client->GetGuiPlayerId(), 0, PLAYER_TYPE_HUMAN,
						   context.GetPlayerRights(), netJoinAck.areyougameadmin()));
		playerData->SetName(context.GetPlayerName());
		playerData->SetAvatarFile(context.GetAvatarFile());
		client->AddPlayerData(playerData);

		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_JOIN);
		client->SetState(ClientStateWaitGame::Instance());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_JoinGameFailedMessage) {
		// Failed to join a game.
		const JoinGameFailedMessage &netJoinFailed = tmpPacket->GetMsg()->joingamefailedmessage();

		int failureCode;
		switch (netJoinFailed.joingamefailurereason()) {
		case JoinGameFailedMessage::invalidGame :
			failureCode = NTF_NET_JOIN_GAME_INVALID;
			break;
		case JoinGameFailedMessage::gameIsFull :
			failureCode = NTF_NET_JOIN_GAME_FULL;
			break;
		case JoinGameFailedMessage::gameIsRunning :
			failureCode = NTF_NET_JOIN_ALREADY_RUNNING;
			break;
		case JoinGameFailedMessage::invalidPassword :
			failureCode = NTF_NET_JOIN_INVALID_PASSWORD;
			break;
		case JoinGameFailedMessage::notAllowedAsGuest :
			failureCode = NTF_NET_JOIN_GUEST_FORBIDDEN;
			break;
		case JoinGameFailedMessage::notInvited :
			failureCode = NTF_NET_JOIN_NOT_INVITED;
			break;
		case JoinGameFailedMessage::gameNameInUse :
			failureCode = NTF_NET_JOIN_GAME_NAME_IN_USE;
			break;
		case JoinGameFailedMessage::badGameName :
			failureCode = NTF_NET_JOIN_GAME_BAD_NAME;
			break;
		case JoinGameFailedMessage::invalidSettings :
			failureCode = NTF_NET_JOIN_INVALID_SETTINGS;
			break;
		case JoinGameFailedMessage::ipAddressBlocked :
			failureCode = NTF_NET_JOIN_IP_BLOCKED;
			break;
		case JoinGameFailedMessage::rejoinFailed :
			failureCode = NTF_NET_JOIN_REJOIN_FAILED;
			break;
		default :
			failureCode = NTF_NET_INTERNAL;
			break;
		}

		client->GetCallback().SignalNetClientNotification(failureCode);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_InviteNotifyMessage) {
		const InviteNotifyMessage &netInvNotify = tmpPacket->GetMsg()->invitenotifymessage();
		if (netInvNotify.playeridwho() == client->GetGuiPlayerId()) {
			client->GetCallback().SignalSelfGameInvitation(netInvNotify.gameid(), netInvNotify.playeridbywhom());
		}
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitGame &
ClientStateWaitGame::Instance()
{
	static ClientStateWaitGame state;
	return state;
}

ClientStateWaitGame::ClientStateWaitGame()
{
}

ClientStateWaitGame::~ClientStateWaitGame()
{
}

void
ClientStateWaitGame::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitGame::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitGame::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_StartEventMessage) {
		const StartEventMessage &netStartEvent = tmpPacket->GetMsg()->starteventmessage();
		if (netStartEvent.starteventtype() == StartEventMessage::rejoinEvent) {
			client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_SYNCREJOIN);
		} else {
			client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_SYNCSTART);
		}
		client->SetState(ClientStateSynchronizeStart::Instance());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_InviteNotifyMessage) {
		const InviteNotifyMessage &netInvNotify = tmpPacket->GetMsg()->invitenotifymessage();
		client->GetCallback().SignalPlayerGameInvitation(
			netInvNotify.gameid(),
			netInvNotify.playeridwho(),
			netInvNotify.playeridbywhom());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_RejectInvNotifyMessage) {
		const RejectInvNotifyMessage &netRejNotify = tmpPacket->GetMsg()->rejectinvnotifymessage();
		client->GetCallback().SignalRejectedGameInvitation(
			netRejNotify.gameid(),
			netRejNotify.playerid(),
			static_cast<DenyGameInvitationReason>(netRejNotify.playerrejectreason()));
	}
}

//-----------------------------------------------------------------------------

ClientStateSynchronizeStart &
ClientStateSynchronizeStart::Instance()
{
	static ClientStateSynchronizeStart state;
	return state;
}

ClientStateSynchronizeStart::ClientStateSynchronizeStart()
{
}

ClientStateSynchronizeStart::~ClientStateSynchronizeStart()
{
}

void
ClientStateSynchronizeStart::Enter(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().expires_from_now(
		milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
	client->GetStateTimer().async_wait(
		boost::bind(
			&ClientStateSynchronizeStart::TimerLoop, this, boost::asio::placeholders::error, client));
}

void
ClientStateSynchronizeStart::Exit(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().cancel();
}

void
ClientStateSynchronizeStart::TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this) {
		if (client->IsSynchronized()) {
			// Acknowledge start.
			boost::shared_ptr<NetPacket> startAck(new NetPacket);
			startAck->GetMsg()->set_messagetype(PokerTHMessage::Type_StartEventAckMessage);
			StartEventAckMessage *netStartAck = startAck->GetMsg()->mutable_starteventackmessage();
			netStartAck->set_gameid(client->GetGameId());

			client->GetSender().Send(client->GetContext().GetSessionData(), startAck);
			// Unsubscribe lobby messages.
			client->UnsubscribeLobbyMsg();

			client->SetState(ClientStateWaitStart::Instance());
		} else {
			client->GetStateTimer().expires_from_now(
				milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
			client->GetStateTimer().async_wait(
				boost::bind(
					&ClientStateSynchronizeStart::TimerLoop, this, boost::asio::placeholders::error, client));
		}
	}
}

void
ClientStateSynchronizeStart::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameStartInitialMessage) {
		// Try to start anyway. Terminating here is very bad because rejoin is not possible then.
		// Unsubscribe lobby messages.
		client->UnsubscribeLobbyMsg();
		client->SetState(ClientStateWaitStart::Instance());
		// Forward the game start message to the next state.
		client->GetState().HandlePacket(client, tmpPacket);
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitStart &
ClientStateWaitStart::Instance()
{
	static ClientStateWaitStart state;
	return state;
}

ClientStateWaitStart::ClientStateWaitStart()
{
}

ClientStateWaitStart::~ClientStateWaitStart()
{
}

void
ClientStateWaitStart::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitStart::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitStart::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameStartInitialMessage
			|| tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameStartRejoinMessage) {
		PlayerIdList tmpPlayerList;
		unsigned tmpHandId = 0;

		if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameStartInitialMessage) {
			// Start the network game as client.
			const GameStartInitialMessage &netStartModeInitial = tmpPacket->GetMsg()->gamestartinitialmessage();

			StartData startData;
			startData.startDealerPlayerId = netStartModeInitial.startdealerplayerid();
			startData.numberOfPlayers = netStartModeInitial.playerseats_size();
			client->SetStartData(startData);

			// Set player numbers using the game start data slots.
			unsigned numPlayers = netStartModeInitial.playerseats_size();
			// Request player info for players if needed.
			if (numPlayers) {
				for (unsigned i = 0; i < numPlayers; i++) {
					unsigned playerId = netStartModeInitial.playerseats(i);
					boost::shared_ptr<PlayerData> tmpPlayer = client->GetPlayerDataByUniqueId(playerId);
					if (!tmpPlayer)
						throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
					tmpPlayer->SetNumber(i);
				}
			} else {
				throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PLAYER_COUNT, 0);
			}
		} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameStartRejoinMessage) {
			const GameStartRejoinMessage &netStartModeRejoin = tmpPacket->GetMsg()->gamestartrejoinmessage();

			StartData startData;
			startData.startDealerPlayerId = netStartModeRejoin.startdealerplayerid();
			startData.numberOfPlayers = netStartModeRejoin.rejoinplayerdata_size();
			client->SetStartData(startData);
			tmpHandId = netStartModeRejoin.handnum();

			// Set player numbers using the game start data slots.
			unsigned numPlayers = netStartModeRejoin.rejoinplayerdata_size();
			// Request player info for players if needed.
			if (numPlayers) {
				for (unsigned i = 0; i < numPlayers; i++) {
					const GameStartRejoinMessage::RejoinPlayerData &playerData = netStartModeRejoin.rejoinplayerdata(i);
					boost::shared_ptr<PlayerData> tmpPlayer = client->GetPlayerDataByUniqueId(playerData.playerid());
					if (!tmpPlayer) {
						// If the player is not found: The corresponding session left. We need to create a generic player object.
						// In order to have a complete seat list, we need all players, even those who left.
						tmpPlayer = client->CreatePlayerData(playerData.playerid(), false);
						client->AddPlayerData(tmpPlayer);
						tmpPlayerList.push_back(playerData.playerid());
					}
					tmpPlayer->SetNumber(i);
					tmpPlayer->SetStartCash(playerData.playermoney());
				}
			} else
				throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PLAYER_COUNT, 0);
		}
		client->InitGame();
		client->GetGame()->setCurrentHandID(tmpHandId);
		// We need to remove the temporary player data objects after creating the game.
		BOOST_FOREACH(unsigned tmpPlayerId, tmpPlayerList) {
			client->RemovePlayerData(tmpPlayerId, NTF_NET_REMOVED_ON_REQUEST);
		}
		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_START);
		client->SetState(ClientStateWaitHand::Instance());
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitHand &
ClientStateWaitHand::Instance()
{
	static ClientStateWaitHand state;
	return state;
}

ClientStateWaitHand::ClientStateWaitHand()
{
}

ClientStateWaitHand::~ClientStateWaitHand()
{
}

void
ClientStateWaitHand::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitHand::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitHand::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_HandStartMessage) {
		// Hand was started.
		// These are the cards. Good luck.
		const HandStartMessage &netHandStart = tmpPacket->GetMsg()->handstartmessage();
		int myCards[2];
		string userPassword(client->GetContext().GetPassword());
		if (netHandStart.has_plaincards() && userPassword.empty()) {
			const HandStartMessage::PlainCards &plainCards = netHandStart.plaincards();
			myCards[0] = (int)plainCards.plaincard1();
			myCards[1] = (int)plainCards.plaincard2();
		} else if (netHandStart.has_encryptedcards() && !userPassword.empty()) {
			const string &encryptedCards = netHandStart.encryptedcards();
			string plainCards;
			if (!CryptHelper::AES128Decrypt((const unsigned char *)userPassword.c_str(),
											(unsigned)userPassword.size(),
											(const unsigned char *)encryptedCards.data(),
											(unsigned)encryptedCards.size(),
											plainCards)) {
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			}
			istringstream cardDataStream(plainCards);
			unsigned tmpPlayerId, tmpGameId;
			int tmpHandNum;
			cardDataStream >> tmpPlayerId;
			cardDataStream >> tmpGameId;
			cardDataStream >> tmpHandNum;
			if (tmpPlayerId != client->GetGuiPlayerId()
					|| tmpGameId != client->GetGameId()
					|| tmpHandNum != client->GetGame()->getCurrentHandID() + 1) {
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			}
			cardDataStream >> myCards[0];
			cardDataStream >> myCards[1];
		}
		// Retrieve state for each seat (not based on player id).
		unsigned numPlayers = netHandStart.seatstates_size();
		// Request player info for players if needed.
		for (int i = 0; i < (int)numPlayers; i++) {
			NetPlayerState seatState = netHandStart.seatstates(i);
			int numberDiff = client->GetStartData().numberOfPlayers - client->GetOrigGuiPlayerNum();
			boost::shared_ptr<PlayerInterface> tmpPlayer = client->GetGame()->getPlayerByNumber((i + numberDiff) % client->GetStartData().numberOfPlayers);
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			switch (seatState) {
			case netPlayerStateNormal :
				tmpPlayer->setIsSessionActive(true);
				break;
			case netPlayerStateSessionInactive :
				tmpPlayer->setIsSessionActive(false);
				break;
			case netPlayerStateNoMoney :
				tmpPlayer->setMyCash(0);
				break;
			}
		}

		// Basic synchronisation before a new hand is started.
		client->GetGui().waitForGuiUpdateDone();
		// Start new hand.
		client->GetGame()->getSeatsList()->front()->setMyCards(myCards);
		client->GetGame()->initHand();
		client->GetGame()->getCurrentHand()->setSmallBlind(netHandStart.smallblind());
		client->GetGame()->getCurrentHand()->getCurrentBeRo()->setMinimumRaise(2 * netHandStart.smallblind());
		client->GetGame()->startHand();
		client->GetGui().dealHoleCards();
		client->GetGui().refreshGameLabels(GAME_STATE_PREFLOP);
		client->GetGui().refreshPot();
		client->GetGui().waitForGuiUpdateDone();

		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_HAND_START);
		client->SetState(ClientStateRunHand::Instance());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_EndOfGameMessage) {
		boost::shared_ptr<Game> curGame = client->GetGame();
		if (curGame) {
			const EndOfGameMessage &netEndOfGame = tmpPacket->GetMsg()->endofgamemessage();

			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(netEndOfGame.winnerplayerid());
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			client->GetGui().logPlayerWinGame(tmpPlayer->getMyName(), curGame->getMyGameID());
			// Resubscribe Lobby messages.
			client->ResubscribeLobbyMsg();
			// Show Lobby dialog.
			client->GetCallback().SignalNetClientWaitDialog();
			client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_END);
			client->SetState(ClientStateWaitGame::Instance());
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AfterHandShowCardsMessage) {
		const AfterHandShowCardsMessage &showCards = tmpPacket->GetMsg()->afterhandshowcardsmessage();
		const PlayerResult &r = showCards.playerresult();

		boost::shared_ptr<PlayerInterface> tmpPlayer = client->GetGame()->getPlayerByUniqueId(r.playerid());
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		int tmpCards[2];
		int bestHandPos[5];
		tmpCards[0] = static_cast<int>(r.resultcard1());
		tmpCards[1] = static_cast<int>(r.resultcard2());
		tmpPlayer->setMyCards(tmpCards);
		for (int num = 0; num < 5; num++) {
			bestHandPos[num] = r.besthandposition(num);
		}
		if (r.cardsvalue()) {
			tmpPlayer->setMyCardsValueInt(r.cardsvalue());
		}
		tmpPlayer->setMyBestHandPosition(bestHandPos);
		tmpPlayer->setMyCash(r.playermoney());
		tmpPlayer->setLastMoneyWon(r.moneywon());

		client->GetCallback().SignalNetClientPostRiverShowCards(r.playerid());
		client->GetClientLog()->logHoleCardsHandName(client->GetGame()->getActivePlayerList(), tmpPlayer, true);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_PlayerIdChangedMessage) {
		boost::shared_ptr<Game> curGame = client->GetGame();
		if (curGame) {
			// Perform Id change.
			const PlayerIdChangedMessage &idChanged = tmpPacket->GetMsg()->playeridchangedmessage();
			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(idChanged.oldplayerid());
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			tmpPlayer->setMyUniqueID(idChanged.newplayerid());
			// This player is now active again.
			tmpPlayer->setMyStayOnTableStatus(true);
			// Also update the dealer, if necessary.
			curGame->replaceDealer(idChanged.oldplayerid(), idChanged.newplayerid());
			// Update the player name, if necessary.
			PlayerInfo info;
			if (client->GetCachedPlayerInfo(idChanged.newplayerid(), info)) {
				tmpPlayer->setMyName(info.playerName);
			}
		}
	}
}

//-----------------------------------------------------------------------------

ClientStateRunHand &
ClientStateRunHand::Instance()
{
	static ClientStateRunHand state;
	return state;
}

ClientStateRunHand::ClientStateRunHand()
{
}

ClientStateRunHand::~ClientStateRunHand()
{
}

void
ClientStateRunHand::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateRunHand::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateRunHand::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	boost::shared_ptr<Game> curGame = client->GetGame();
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_PlayersActionDoneMessage) {
		const PlayersActionDoneMessage &netActionDone = tmpPacket->GetMsg()->playersactiondonemessage();

		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(netActionDone.playerid());
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		bool isBigBlind = false;

		if (netActionDone.gamestate() == netStatePreflopSmallBlind) {
			curGame->getCurrentHand()->getCurrentBeRo()->setSmallBlindPositionId(tmpPlayer->getMyUniqueID());
			tmpPlayer->setMyButton(BUTTON_SMALL_BLIND);
		} else if (netActionDone.gamestate() == netStatePreflopBigBlind) {
			curGame->getCurrentHand()->getCurrentBeRo()->setBigBlindPositionId(tmpPlayer->getMyUniqueID());
			tmpPlayer->setMyButton(BUTTON_BIG_BLIND);
			isBigBlind = true;
		} else { // no blind -> log
			if (netActionDone.playeraction()) {
				assert((int)netActionDone.totalplayerbet() >= tmpPlayer->getMySet());
				client->GetGui().logPlayerActionMsg(
					tmpPlayer->getMyName(),
					netActionDone.playeraction(),
					netActionDone.totalplayerbet() - tmpPlayer->getMySet());
				client->GetClientLog()->logPlayerAction(
					tmpPlayer->getMyName(),
					client->GetClientLog()->transformPlayerActionLog(PlayerAction(netActionDone.playeraction())),
					netActionDone.totalplayerbet() - tmpPlayer->getMySet()
				);
				if (tmpPlayer->getMyID() == 0) {
					client->EndPing();
				}
			}
			// Update last players turn only after the blinds.
			curGame->getCurrentHand()->setPreviousPlayerID(tmpPlayer->getMyID());
		}

		tmpPlayer->setMyAction(PlayerAction(netActionDone.playeraction()));
		tmpPlayer->setMySetAbsolute(netActionDone.totalplayerbet());
		tmpPlayer->setMyCash(netActionDone.playermoney());
		curGame->getCurrentHand()->getCurrentBeRo()->setHighestSet(netActionDone.highestset());
		curGame->getCurrentHand()->getCurrentBeRo()->setMinimumRaise(netActionDone.minimumraise());
		curGame->getCurrentHand()->getBoard()->collectSets();
		curGame->getCurrentHand()->switchRounds();

		//log blinds sets after setting bigblind-button
		if (isBigBlind) {
			client->GetGui().logNewBlindsSetsMsg(
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getSmallBlindPositionId())->getMySet(),
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getBigBlindPositionId())->getMySet(),
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getSmallBlindPositionId())->getMyName(),
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getBigBlindPositionId())->getMyName());
			client->GetGui().flushLogAtHand();
			client->GetClientLog()->logNewHandMsg(
				curGame->getCurrentHandID(),
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getDealerPosition())->getMyID()+1,
				curGame->getCurrentHand()->getSmallBlind(),
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getSmallBlindPositionId())->getMyID()+1,
				curGame->getCurrentHand()->getSmallBlind()*2,
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getBigBlindPositionId())->getMyID()+1,
				curGame->getSeatsList()
			);
		}

		// Stop the timeout for the player.
		client->GetGui().stopTimeoutAnimation(tmpPlayer->getMyID());

		// Unmark last player in GUI.
		client->GetGui().refreshGroupbox(tmpPlayer->getMyID(), 3);

		// Refresh GUI
		if (tmpPlayer->getMyID() == 0)
			client->GetGui().disableMyButtons();
		client->GetGui().refreshAction(tmpPlayer->getMyID(), tmpPlayer->getMyAction());
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		client->GetGui().refreshCash();
		client->GetGui().refreshButton();
		client->GetGui().updateMyButtonsState();
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_PlayersTurnMessage) {
		const PlayersTurnMessage &netPlayersTurn = tmpPacket->GetMsg()->playersturnmessage();

		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(netPlayersTurn.playerid());
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		// Set round.
		if (curGame->getCurrentHand()->getCurrentRound() != static_cast<GameState>(netPlayersTurn.gamestate())) {
			ResetPlayerActions(*curGame);
			curGame->getCurrentHand()->setCurrentRound(static_cast<GameState>(netPlayersTurn.gamestate()));
			client->GetClientLog()->setCurrentRound(static_cast<GameState>(netPlayersTurn.gamestate()));
			// Refresh actions.
			client->GetGui().refreshSet();
			client->GetGui().refreshAction();
		}

		// Next player's turn.
		curGame->getCurrentHand()->getCurrentBeRo()->setCurrentPlayersTurnId(tmpPlayer->getMyID());

		// Mark current player in GUI.
		int guiStatus = 2;
		if (!tmpPlayer->getMyActiveStatus())
			guiStatus = 0;
		else if (tmpPlayer->getMyAction() == PLAYER_ACTION_FOLD)
			guiStatus = 1;
		client->GetGui().refreshGroupbox(tmpPlayer->getMyID(), guiStatus);
		client->GetGui().refreshAction(tmpPlayer->getMyID(), PLAYER_ACTION_NONE);

		// Start displaying the timeout for the player.
		client->GetGui().startTimeoutAnimation(tmpPlayer->getMyID(), client->GetGameData().playerActionTimeoutSec);

		if (tmpPlayer->getMyID() == 0) // Is this the GUI player?
			client->GetGui().meInAction();
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_DealFlopCardsMessage) {
		const DealFlopCardsMessage &netDealFlop = tmpPacket->GetMsg()->dealflopcardsmessage();

		int tmpCards[5];
		tmpCards[0] = static_cast<int>(netDealFlop.flopcard1());
		tmpCards[1] = static_cast<int>(netDealFlop.flopcard2());
		tmpCards[2] = static_cast<int>(netDealFlop.flopcard3());
		tmpCards[3] = tmpCards[4] = 0;
		curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
		curGame->getCurrentHand()->getBoard()->collectPot();
		curGame->getCurrentHand()->setPreviousPlayerID(-1);

		client->GetGui().logDealBoardCardsMsg(GAME_STATE_FLOP, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
		client->GetClientLog()->setCurrentRound(GAME_STATE_FLOP);
		client->GetClientLog()->logBoardCards(tmpCards);
		client->GetGui().refreshGameLabels(GAME_STATE_FLOP);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		client->GetGui().dealBeRoCards(1);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_DealTurnCardMessage) {
		const DealTurnCardMessage &netDealTurn = tmpPacket->GetMsg()->dealturncardmessage();

		int tmpCards[5];
		curGame->getCurrentHand()->getBoard()->getMyCards(tmpCards);
		tmpCards[3] = static_cast<int>(netDealTurn.turncard());
		curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
		curGame->getCurrentHand()->getBoard()->collectPot();
		curGame->getCurrentHand()->setPreviousPlayerID(-1);

		client->GetGui().logDealBoardCardsMsg(GAME_STATE_TURN, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
		client->GetClientLog()->setCurrentRound(GAME_STATE_TURN);
		client->GetClientLog()->logBoardCards(tmpCards);
		client->GetGui().refreshGameLabels(GAME_STATE_TURN);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		client->GetGui().dealBeRoCards(2);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_DealRiverCardMessage) {
		const DealRiverCardMessage &netDealRiver = tmpPacket->GetMsg()->dealrivercardmessage();

		int tmpCards[5];
		curGame->getCurrentHand()->getBoard()->getMyCards(tmpCards);
		tmpCards[4] = static_cast<int>(netDealRiver.rivercard());
		curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
		curGame->getCurrentHand()->getBoard()->collectPot();
		curGame->getCurrentHand()->setPreviousPlayerID(-1);

		client->GetGui().logDealBoardCardsMsg(GAME_STATE_RIVER, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
		client->GetClientLog()->setCurrentRound(GAME_STATE_RIVER);
		client->GetClientLog()->logBoardCards(tmpCards);
		client->GetGui().refreshGameLabels(GAME_STATE_RIVER);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		client->GetGui().dealBeRoCards(3);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AllInShowCardsMessage) {
		const AllInShowCardsMessage &netAllInShow = tmpPacket->GetMsg()->allinshowcardsmessage();

		curGame->getCurrentHand()->setAllInCondition(true);

		// Set player numbers using the game start data slots.
		unsigned numPlayers = netAllInShow.playersallin_size();
		// Request player info for players if needed.
		for (unsigned i = 0; i < numPlayers; i++) {
			const AllInShowCardsMessage::PlayerAllIn &p = netAllInShow.playersallin(i);

			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(p.playerid());
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

			int tmpCards[2];
			tmpCards[0] = static_cast<int>(p.allincard1());
			tmpCards[1] = static_cast<int>(p.allincard2());
			tmpPlayer->setMyCards(tmpCards);
		}
		client->GetGui().flipHolecardsAllIn();
		if(curGame->getCurrentHand()->getCurrentRound()<GAME_STATE_RIVER) {
			client->GetClientLog()->logHoleCardsHandName(
				curGame->getActivePlayerList()
			);
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_EndOfHandHideCardsMessage) {
		const EndOfHandHideCardsMessage &hideCards = tmpPacket->GetMsg()->endofhandhidecardsmessage();
		curGame->getCurrentHand()->getBoard()->collectPot();
		// Reset player sets
		ResetPlayerSets(*curGame);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		// Synchronize with GUI.
		client->GetGui().waitForGuiUpdateDone();

		// End of Hand, but keep cards hidden.
		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(hideCards.playerid());
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		tmpPlayer->setMyCash(hideCards.playermoney());
		tmpPlayer->setLastMoneyWon(hideCards.moneywon());
		list<unsigned> winnerList;
		winnerList.push_back(tmpPlayer->getMyUniqueID());

		curGame->getCurrentHand()->getBoard()->setPot(0);
		curGame->getCurrentHand()->getBoard()->setWinners(winnerList);

		// logging
		client->GetClientLog()->logHandWinner(curGame->getActivePlayerList(), tmpPlayer->getMyCardsValueInt(), winnerList);

		client->GetGui().postRiverRunAnimation1();

		// Wait for next Hand.
		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_SERVER_HAND_END);
		client->SetState(ClientStateWaitHand::Instance());

		// logging
		client->GetClientLog()->logPlayerSitsOut(curGame->getActivePlayerList());
		client->GetClientLog()->logGameWinner(curGame->getActivePlayerList());
		client->GetClientLog()->logAfterHand();
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_EndOfHandShowCardsMessage) {
		const EndOfHandShowCardsMessage &showCards = tmpPacket->GetMsg()->endofhandshowcardsmessage();

		curGame->getCurrentHand()->getBoard()->collectPot();
		// Reset player sets
		ResetPlayerSets(*curGame);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		// Synchronize with GUI.
		client->GetGui().waitForGuiUpdateDone();

		// End of Hand, show cards.
		list<unsigned> winnerList;
		list<unsigned> showList;
		int highestValueOfCards = 0;
		unsigned numResults = showCards.playerresults_size();
		// Request player info for players if needed.
		for (unsigned i = 0; i < numResults; i++) {
			const PlayerResult &r = showCards.playerresults(i);

			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(r.playerid());
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

			int tmpCards[2];
			int bestHandPos[5];
			tmpCards[0] = static_cast<int>(r.resultcard1());
			tmpCards[1] = static_cast<int>(r.resultcard2());
			tmpPlayer->setMyCards(tmpCards);
			for (int num = 0; num < 5; num++) {
				bestHandPos[num] = r.besthandposition(num);
			}
			if (r.has_cardsvalue()) {
				tmpPlayer->setMyCardsValueInt(r.cardsvalue());
			}
			tmpPlayer->setMyBestHandPosition(bestHandPos);
			if (tmpPlayer->getMyCardsValueInt() > highestValueOfCards)
				highestValueOfCards = tmpPlayer->getMyCardsValueInt();
			tmpPlayer->setMyCash(r.playermoney());
			tmpPlayer->setLastMoneyWon(r.moneywon());
			if (r.moneywon())
				winnerList.push_back(r.playerid());
			showList.push_back(r.playerid());
		}

		curGame->getCurrentHand()->setCurrentRound(GAME_STATE_POST_RIVER);
		client->GetClientLog()->setCurrentRound(GAME_STATE_POST_RIVER);
		curGame->getCurrentHand()->getCurrentBeRo()->setHighestCardsValue(highestValueOfCards);
		curGame->getCurrentHand()->getBoard()->setPot(0);
		curGame->getCurrentHand()->getBoard()->setWinners(winnerList);
		curGame->getCurrentHand()->getBoard()->setPlayerNeedToShowCards(showList);

		// logging
		client->GetClientLog()->logHoleCardsHandName(curGame->getActivePlayerList());
		client->GetClientLog()->logHandWinner(curGame->getActivePlayerList(), highestValueOfCards, winnerList);

		client->GetGui().postRiverRunAnimation1();

		// Wait for next Hand.
		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_HAND_END);
		client->SetState(ClientStateWaitHand::Instance());

		// logging
		client->GetClientLog()->logPlayerSitsOut(curGame->getActivePlayerList());
		client->GetClientLog()->logGameWinner(curGame->getActivePlayerList());
		client->GetClientLog()->logAfterHand();
	}

	// Synchronize with GUI.
	client->GetGui().waitForGuiUpdateDone();
}

void
ClientStateRunHand::ResetPlayerActions(Game &curGame)
{
	// Reset player actions
	PlayerListIterator i = curGame.getSeatsList()->begin();
	PlayerListIterator end = curGame.getSeatsList()->end();

	while (i != end) {
		int action = (*i)->getMyAction();
		if (action != 1 && action != 6)
			(*i)->setMyAction(PLAYER_ACTION_NONE);
		(*i)->setMySetNull();
		++i;
	}
}

void
ClientStateRunHand::ResetPlayerSets(Game &curGame)
{
	PlayerListIterator i = curGame.getSeatsList()->begin();
	PlayerListIterator end = curGame.getSeatsList()->end();
	while (i != end) {
		(*i)->setMySetNull();
		++i;
	}
}

//-----------------------------------------------------------------------------

ClientStateFinal &
ClientStateFinal::Instance()
{
	static ClientStateFinal state;
	return state;
}
