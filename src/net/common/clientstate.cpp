/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <net/clientstate.h>
#include <net/clientthread.h>
#include <net/clientcontext.h>
#include <net/senderhelper.h>
#include <net/receiverhelper.h>
#include <net/netpacket.h>
#include <net/resolverthread.h>
#include <net/clientexception.h>
#include <net/socket_helper.h>
#include <net/socket_msg.h>
#include <net/downloadhelper.h>
#include <core/avatarmanager.h>
#include <core/crypthelper.h>
#include <qttoolsinterface.h>

#include <game.h>
#include <playerinterface.h>

#include "tinyxml.h"
#include "zlib.h"
#include <boost/bind.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
using namespace boost::filesystem;

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
	if (!ec && &client->GetState() == this)
	{
		client->GetCallback().SignalNetClientConnect(MSG_SOCK_RESOLVE_DONE);
		// Use the first resolver result.
		ClientStateStartConnect::Instance().SetRemoteEndpoint(endpoint_iterator);
		client->SetState(ClientStateStartConnect::Instance());
	}
	else
	{
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
	const ClientContext &context = client->GetContext();
	path tmpServerListPath(context.GetCacheDir());
	string serverListUrl(context.GetServerListUrl());
	// Retrieve the file name from the URL.
	size_t pos = serverListUrl.find_last_of('/');
	if (serverListUrl.empty() || pos == string::npos || ++pos >= serverListUrl.length())
	{
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_URL, 0);
	}
	tmpServerListPath /= serverListUrl.substr(pos);
	if (exists(tmpServerListPath))
	{
		// Download and compare md5.
		tmpServerListPath = change_extension(tmpServerListPath, extension(tmpServerListPath) + ".md5");
		boost::shared_ptr<DownloadHelper> downloader(new DownloadHelper);
		downloader->Init(serverListUrl + ".md5", tmpServerListPath.directory_string());
		ClientStateSynchronizingServerList::Instance().SetDownloadHelper(downloader);
		client->SetState(ClientStateSynchronizingServerList::Instance());
	}
	else
	{
		// Download server list.
		boost::shared_ptr<DownloadHelper> downloader(new DownloadHelper);
		downloader->Init(serverListUrl, tmpServerListPath.directory_string());
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

ClientStateSynchronizingServerList &
ClientStateSynchronizingServerList::Instance()
{
	static ClientStateSynchronizingServerList state;
	return state;
}

ClientStateSynchronizingServerList::ClientStateSynchronizingServerList()
{
}

ClientStateSynchronizingServerList::~ClientStateSynchronizingServerList()
{
}

void
ClientStateSynchronizingServerList::Enter(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().expires_from_now(
		boost::posix_time::milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
	client->GetStateTimer().async_wait(
		boost::bind(
			&ClientStateSynchronizingServerList::TimerLoop, this, boost::asio::placeholders::error, client));
}

void
ClientStateSynchronizingServerList::Exit(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().cancel();
}

void
ClientStateSynchronizingServerList::SetDownloadHelper(boost::shared_ptr<DownloadHelper> helper)
{
	m_downloadHelper = helper;
}

void
ClientStateSynchronizingServerList::TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this)
	{
		if (m_downloadHelper->Process())
		{
			m_downloadHelper.reset();
			ClientContext &context = client->GetContext();
			path md5ServerListPath(context.GetCacheDir());

			// No more checking needed as this was done before.
			md5ServerListPath /= context.GetServerListUrl().substr(context.GetServerListUrl().find_last_of('/') + 1) + ".md5";
			path serverListPath = change_extension(md5ServerListPath, "");
			// Compare the md5 sums.
			string tmpMd5;
			{
				ifstream inFile(md5ServerListPath.directory_string().c_str(), ios_base::in);
				if (inFile.fail())
					throw ClientException(__FILE__, __LINE__, ERR_SOCK_OPEN_MD5_FAILED, 0);
				inFile >> tmpMd5;
			}
			MD5Buf downloadedMd5;
			if (!downloadedMd5.FromString(tmpMd5))
				throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_MD5, 0);
			MD5Buf currentMd5;
			if (!CryptHelper::MD5Sum(serverListPath.directory_string(), currentMd5))
				throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_MD5, 0);
			if (downloadedMd5 == currentMd5)
			{
				// Server list is still current.
				client->SetState(ClientStateReadingServerList::Instance());
			}
			else
			{
				// Download new server list.
				// Paranoia check before removing the file, we do not want to delete wrong files.
				path tmpPath(serverListPath);
				if (path(context.GetCacheDir()) == tmpPath.remove_leaf())
				{
					remove(serverListPath);
					client->SetState(ClientStateStartServerListDownload::Instance());
				}
				else
					throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_URL, 0);
			}
		}
		else
		{
			// Download still in process. Delay.
			client->GetStateTimer().expires_from_now(
				boost::posix_time::milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
			client->GetStateTimer().async_wait(
				boost::bind(
					&ClientStateSynchronizingServerList::TimerLoop, this, boost::asio::placeholders::error, client));
		}
	}
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
		boost::posix_time::milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
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
	if (!ec && &client->GetState() == this)
	{
		if (m_downloadHelper->Process())
		{
			m_downloadHelper.reset();
			client->SetState(ClientStateReadingServerList::Instance());
		}
		else
		{
			client->GetStateTimer().expires_from_now(
				boost::posix_time::milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
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
	if (extension(zippedServerListPath) == ".z")
	{
		xmlServerListPath = change_extension(zippedServerListPath, "");

		// Unzip the file using zlib.
		try {
			ifstream inFile(zippedServerListPath.directory_string().c_str(), ios_base::in | ios_base::binary);
			ofstream outFile(xmlServerListPath.directory_string().c_str(), ios_base::out);
			boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
			in.push(boost::iostreams::zlib_decompressor());
			in.push(inFile);
			boost::iostreams::copy(in, outFile);
		} catch (...)
		{
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_UNZIP_FAILED, 0);
		}
	}
	else
		xmlServerListPath = zippedServerListPath;

	// Parse the server address.
	TiXmlDocument doc(xmlServerListPath.directory_string());

	if (doc.LoadFile())
	{
		client->ClearServerInfoMap();
		int serverCount = 0;
		unsigned lastServerInfoId = 0;
		TiXmlHandle docHandle(&doc);
		const TiXmlElement *nextServer = docHandle.FirstChild("ServerList" ).FirstChild("Server").ToElement();
		while (nextServer)
		{
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
			const TiXmlNode *portNode = nextServer->FirstChild("Port");

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
			if (sctpNode && sctpNode->ToElement())
			{
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

		if (serverCount == 1)
		{
			client->UseServer(lastServerInfoId);
			client->GetCallback().SignalNetClientConnect(MSG_SOCK_SERVER_LIST_DONE);
			client->SetState(ClientStateStartResolve::Instance());
		}
		else if (serverCount > 1)
		{
			client->GetCallback().SignalNetClientServerListShow();
			client->SetState(ClientStateWaitChooseServer::Instance());
		}
		else
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_XML, 0);
	}
	else
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
		boost::posix_time::milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
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
	if (!ec && &client->GetState() == this)
	{
		unsigned serverId;
		if (client->GetSelectedServer(serverId))
		{
			client->UseServer(serverId);
			client->GetCallback().SignalNetClientConnect(MSG_SOCK_SERVER_LIST_DONE);
			client->SetState(ClientStateStartResolve::Instance());
		}
		else
		{
			client->GetStateTimer().expires_from_now(
				boost::posix_time::milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
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
		boost::posix_time::seconds(CLIENT_CONNECT_TIMEOUT_SEC));
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
	if (&client->GetState() == this)
	{
		if (!ec)
		{
			client->GetCallback().SignalNetClientConnect(MSG_SOCK_CONNECT_DONE);
			client->SetState(ClientStateStartSession::Instance());
		}
		else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
		{
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
		}
		else
		{
			if (ec != boost::asio::error::operation_aborted)
				throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_FAILED, ec.value());
		}
	}
}

void
ClientStateStartConnect::TimerTimeout(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this)
	{
		boost::system::error_code ec;
		client->GetContext().GetSessionData()->GetAsioSocket()->close(ec);
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_TIMEOUT, 0);
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
	if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_playerInfoReplyMessage)
	{
		unsigned playerId = tmpPacket->GetMsg()->choice.playerInfoReplyMessage.playerId;
		if (tmpPacket->GetMsg()->choice.playerInfoReplyMessage.playerInfoResult.present == playerInfoResult_PR_playerInfoData)
		{
			PlayerInfo tmpInfo;
			PlayerInfoData_t *netInfo = &tmpPacket->GetMsg()->choice.playerInfoReplyMessage.playerInfoResult.choice.playerInfoData;
			tmpInfo.playerName = STL_STRING_FROM_OCTET_STRING(netInfo->playerName);
			tmpInfo.ptype = netInfo->isHuman ? PLAYER_TYPE_HUMAN : PLAYER_TYPE_COMPUTER;
			tmpInfo.isGuest = netInfo->playerRights == PlayerInfoRights_playerRightsGuest;
			if (netInfo->avatarData != NULL)
			{
				tmpInfo.hasAvatar = true;
				memcpy(tmpInfo.avatar.GetData(), netInfo->avatarData->avatar.buf, MD5_DATA_SIZE);
				tmpInfo.avatarType = (AvatarFileType)netInfo->avatarData->avatarType;
			}
			client->SetPlayerInfo(
				playerId,
				tmpInfo);
		}
		else
		{
			client->SetUnknownPlayer(playerId);
		}
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_gamePlayerMessage)
	{
		GamePlayerMessage_t *netGamePlayer = &tmpPacket->GetMsg()->choice.gamePlayerMessage;
//		unsigned gameId = netGamePlayer->gameId;
		if (netGamePlayer->gamePlayerNotification.present == gamePlayerNotification_PR_removedFromGame)
		{
			RemovedFromGame_t *netRemoved = &netGamePlayer->gamePlayerNotification.choice.removedFromGame;

			client->ClearPlayerDataList();
			// Resubscribe Lobby messages.
			client->ResubscribeLobbyMsg();
			// Show Lobby.
			client->GetCallback().SignalNetClientWaitDialog();
			int removeReason;
			switch (netRemoved->removedFromGameReason)
			{
				case removedFromGameReason_kickedFromGame :
					removeReason = NTF_NET_REMOVED_KICKED;
					break;
				case removedFromGameReason_gameIsFull :
					removeReason = NTF_NET_REMOVED_GAME_FULL;
					break;
				case removedFromGameReason_gameIsRunning :
					removeReason = NTF_NET_REMOVED_ALREADY_RUNNING;
					break;
				case removedFromGameReason_gameTimeout :
					removeReason = NTF_NET_REMOVED_TIMEOUT;
					break;
				case removedFromGameReason_removedStartFailed :
					removeReason = NTF_NET_REMOVED_START_FAILED;
					break;
				default :
					removeReason = NTF_NET_REMOVED_ON_REQUEST;
					break;
			}
			client->GetCallback().SignalNetClientRemovedFromGame(removeReason);
			client->SetState(ClientStateWaitJoin::Instance());
		}
		else if (netGamePlayer->gamePlayerNotification.present == gamePlayerNotification_PR_gamePlayerLeft)
		{
			// A player left the game.
			GamePlayerLeft_t *netLeft = &netGamePlayer->gamePlayerNotification.choice.gamePlayerLeft;

			// Signal to GUI and remove from data list.
			client->RemovePlayerData(netLeft->playerId, netLeft->gamePlayerLeftReason);
		}
		else if (netGamePlayer->gamePlayerNotification.present == gamePlayerNotification_PR_gameAdminChanged)
		{
			// New admin for the game.
			GameAdminChanged_t *netChanged = &netGamePlayer->gamePlayerNotification.choice.gameAdminChanged;

			// Set new game admin and signal to GUI.
			client->SetNewGameAdmin(netChanged->newAdminPlayerId);
		}
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_timeoutWarningMessage)
	{
		TimeoutWarningMessage_t *tmpTimeout = &tmpPacket->GetMsg()->choice.timeoutWarningMessage;
		client->GetCallback().SignalNetClientShowTimeoutDialog((NetTimeoutReason)tmpTimeout->timeoutReason, tmpTimeout->remainingSeconds);
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_chatMessage)
	{
		// Chat message - display it in the GUI.
		ChatMessage_t *netMessage = &tmpPacket->GetMsg()->choice.chatMessage;

		string playerName;
		if (netMessage->chatType.present == chatType_PR_chatTypeBroadcast)
		{
			client->GetCallback().SignalNetClientGameChatMsg("(global notice)", STL_STRING_FROM_OCTET_STRING(netMessage->chatText));
			client->GetCallback().SignalNetClientLobbyChatMsg("(global notice)", STL_STRING_FROM_OCTET_STRING(netMessage->chatText));
		}
		else if (netMessage->chatType.present == chatType_PR_chatTypeBot)
		{
			client->GetCallback().SignalNetClientLobbyChatMsg("(chat bot)", STL_STRING_FROM_OCTET_STRING(netMessage->chatText));
		}
		else if (netMessage->chatType.present == chatType_PR_chatTypeGame)
		{
			unsigned playerId = netMessage->chatType.choice.chatTypeGame.playerId;
			boost::shared_ptr<PlayerData> tmpPlayer = client->GetPlayerDataByUniqueId(playerId);
			if (tmpPlayer.get())
				playerName = tmpPlayer->GetName();
			if (!playerName.empty())
				client->GetCallback().SignalNetClientGameChatMsg(playerName, STL_STRING_FROM_OCTET_STRING(netMessage->chatText));
		}
		else if (netMessage->chatType.present == chatType_PR_chatTypeLobby)
		{
			unsigned playerId = netMessage->chatType.choice.chatTypeLobby.playerId;
			PlayerInfo info;
			if (client->GetCachedPlayerInfo(playerId, info))
				client->GetCallback().SignalNetClientLobbyChatMsg(info.playerName, STL_STRING_FROM_OCTET_STRING(netMessage->chatText));
		}
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_dialogMessage)
	{
		// Message box - display it in the GUI.
		DialogMessage_t *netDialog = &tmpPacket->GetMsg()->choice.dialogMessage;
		client->GetCallback().SignalNetClientMsgBox(STL_STRING_FROM_OCTET_STRING(netDialog->notificationText));
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_playerListMessage)
	{
		PlayerListMessage_t *netPlayerList = &tmpPacket->GetMsg()->choice.playerListMessage;

		if (netPlayerList->playerListNotification == playerListNotification_playerListNew)
		{
			PlayerInfo info;
			if (!client->GetCachedPlayerInfo(netPlayerList->playerId, info))
			{
				// Request player info.
				ostringstream name;
				name << "#" << netPlayerList->playerId;
				info.playerName = name.str();
				client->RequestPlayerInfo(netPlayerList->playerId);
			}
			client->GetCallback().SignalLobbyPlayerJoined(netPlayerList->playerId, info.playerName);
		}
		else if (netPlayerList->playerListNotification == playerListNotification_playerListLeft)
		{
			client->GetCallback().SignalLobbyPlayerLeft(netPlayerList->playerId);
		}
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_gameListMessage)
	{
		GameListMessage_t *netGameList = &tmpPacket->GetMsg()->choice.gameListMessage;
		unsigned gameId = netGameList->gameId;

		if (netGameList->gameListNotification.present == gameListNotification_PR_gameListNew)
		{
			// A new game was created on the server.
			GameListNew_t *netListNew = &netGameList->gameListNotification.choice.gameListNew;

			NonZeroId_t **playerIds = netListNew->playerIds.list.array;
			unsigned numPlayers = netListNew->playerIds.list.count;
			// Request player info for players if needed.
			GameInfo tmpInfo;
			for (unsigned i = 0; i < numPlayers; i++)
			{
				PlayerInfo info;
				unsigned playerId = *playerIds[i];
				if (!client->GetCachedPlayerInfo(playerId, info))
				{
					// Request player info.
					client->RequestPlayerInfo(playerId);
				}
				tmpInfo.players.push_back(playerId);
			}
			tmpInfo.adminPlayerId = netListNew->adminPlayerId;
			tmpInfo.isPasswordProtected = netListNew->isPrivate ? true : false;
			tmpInfo.mode = static_cast<GameMode>(netListNew->gameMode);
			tmpInfo.name = STL_STRING_FROM_OCTET_STRING(netListNew->gameInfo.gameName);
			NetPacket::GetGameData(&netListNew->gameInfo, tmpInfo.data);

			client->AddGameInfo(gameId, tmpInfo);
		}
		else if (netGameList->gameListNotification.present == gameListNotification_PR_gameListUpdate)
		{
			// An existing game was updated on the server.
			GameListUpdate_t *netListUpdate = &netGameList->gameListNotification.choice.gameListUpdate;
			if (netListUpdate->gameMode == NetGameMode_gameClosed)
				client->RemoveGameInfo(gameId);
			else
				client->UpdateGameInfoMode(gameId, static_cast<GameMode>(netListUpdate->gameMode));
		}
		else if (netGameList->gameListNotification.present == gameListNotification_PR_gameListPlayerJoined)
		{
			GameListPlayerJoined_t *netListJoined = &netGameList->gameListNotification.choice.gameListPlayerJoined;

			client->ModifyGameInfoAddPlayer(gameId, netListJoined->playerId);
			// Request player info if needed.
			PlayerInfo info;
			if (!client->GetCachedPlayerInfo(netListJoined->playerId, info))
			{
				client->RequestPlayerInfo(netListJoined->playerId);
			}
		}
		else if (netGameList->gameListNotification.present == gameListNotification_PR_gameListPlayerLeft)
		{
			GameListPlayerLeft_t *netListLeft = &netGameList->gameListNotification.choice.gameListPlayerLeft;

			client->ModifyGameInfoRemovePlayer(gameId, netListLeft->playerId);
		}
		else if (netGameList->gameListNotification.present == gameListNotification_PR_gameListAdminChanged)
		{
			GameListAdminChanged_t *netListAdmin = &netGameList->gameListNotification.choice.gameListAdminChanged;

			client->UpdateGameInfoAdmin(gameId, netListAdmin->newAdminPlayerId);
		}
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_startKickPetitionMessage)
	{
		StartKickPetitionMessage_t *netStartPetition = &tmpPacket->GetMsg()->choice.startKickPetitionMessage;
		client->StartPetition(netStartPetition->petitionId, netStartPetition->proposingPlayerId,
			netStartPetition->kickPlayerId, netStartPetition->kickTimeoutSec, netStartPetition->numVotesNeededToKick);
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_kickPetitionUpdateMessage)
	{
		KickPetitionUpdateMessage_t *netPetitionUpdate = &tmpPacket->GetMsg()->choice.kickPetitionUpdateMessage;
		client->UpdatePetition(netPetitionUpdate->petitionId, netPetitionUpdate->numVotesAgainstKicking,
			netPetitionUpdate->numVotesInFavourOfKicking, netPetitionUpdate->numVotesNeededToKick);
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_endKickPetitionMessage)
	{
		EndKickPetitionMessage_t *netEndPetition = &tmpPacket->GetMsg()->choice.endKickPetitionMessage;
		client->EndPetition(netEndPetition->petitionId);
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_avatarReplyMessage)
	{
		AvatarReplyMessage_t *netAvatarReply = &tmpPacket->GetMsg()->choice.avatarReplyMessage;
		unsigned requestId = netAvatarReply->requestId;
		if (netAvatarReply->avatarResult.present == avatarResult_PR_avatarHeader)
		{
			AvatarHeader_t *netAvatarHeader = &netAvatarReply->avatarResult.choice.avatarHeader;
			client->AddTempAvatarFile(requestId, netAvatarHeader->avatarSize, static_cast<AvatarFileType>(netAvatarHeader->avatarType));
		}
		else if (netAvatarReply->avatarResult.present == avatarResult_PR_avatarData)
		{
			AvatarData_t *netAvatarData = &netAvatarReply->avatarResult.choice.avatarData;
			vector<unsigned char> fileData(netAvatarData->avatarBlock.size);
			memcpy(&fileData[0], netAvatarData->avatarBlock.buf, netAvatarData->avatarBlock.size);
			client->StoreInTempAvatarFile(requestId, fileData);
		}
		else if (netAvatarReply->avatarResult.present == avatarResult_PR_avatarEnd)
		{
			client->CompleteTempAvatarFile(requestId);
		}
		else if (netAvatarReply->avatarResult.present == avatarResult_PR_unknownAvatar)
		{
			client->SetUnknownAvatar(requestId);
		}
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_statisticsMessage)
	{
		StatisticsMessage_t *netStatistics = &tmpPacket->GetMsg()->choice.statisticsMessage;

		StatisticsData **statData = netStatistics->statisticsData.list.array;
		unsigned numStats = netStatistics->statisticsData.list.count;
		// Request player info for players if needed.
		if (numStats && statData && *statData)
		{
			ServerStats tmpStats;
			for (unsigned i = 0; i < numStats; i++)
			{
				if (statData[i]->statisticsType == statisticsType_statNumberOfPlayers)
					tmpStats.numberOfPlayersOnServer = statData[i]->statisticsValue;
			}
			client->UpdateStatData(tmpStats);
		}
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_errorMessage)
	{
		// Server reported an error.
		ErrorMessage_t *netError = &tmpPacket->GetMsg()->choice.errorMessage;
		// Show the error.
		throw ClientException(__FILE__, __LINE__, NetPacket::NetErrorToGameError(netError->errorReason), 0);
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
	if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_announceMessage)
	{
		// Server has send announcement - check data.
		AnnounceMessage_t *netAnnounce = &tmpPacket->GetMsg()->choice.announceMessage;
		// Check current game version.
		if (netAnnounce->latestGameVersion.major != POKERTH_VERSION_MAJOR
			|| netAnnounce->latestGameVersion.minor != POKERTH_VERSION_MINOR)
		{
			client->GetCallback().SignalNetClientNotification(NTF_NET_NEW_RELEASE_AVAILABLE);
		}
		else if (POKERTH_BETA_REVISION && netAnnounce->latestBetaRevision != POKERTH_BETA_REVISION)
		{
			client->GetCallback().SignalNetClientNotification(NTF_NET_OUTDATED_BETA);
		}
		else
		{
			ClientContext &context = client->GetContext();

			// CASE 1: Authenticated login (username, challenge/response for password).
			if (netAnnounce->serverType == serverType_serverTypeInternetAuth)
			{
				client->GetCallback().SignalNetClientLoginShow();
				client->SetState(ClientStateWaitEnterLogin::Instance());
			}
			// CASE 2: Unauthenticated login (network game or dedicated server without auth backend).
			else if (netAnnounce->serverType == serverType_serverTypeInternetNoAuth
				|| netAnnounce->serverType == serverType_serverTypeLAN)
			{
				boost::shared_ptr<NetPacket> init(new NetPacket(NetPacket::Alloc));
				init->GetMsg()->present = PokerTHMessage_PR_initMessage;
				InitMessage_t *netInit = &init->GetMsg()->choice.initMessage;
				netInit->requestedVersion.major = NET_VERSION_MAJOR;
				netInit->requestedVersion.minor = NET_VERSION_MINOR;
				netInit->login.present = login_PR_unauthenticatedLogin;
				UnauthenticatedLogin_t *noauthLogin = &netInit->login.choice.unauthenticatedLogin;
				OCTET_STRING_fromBuf(&noauthLogin->nickName,
									 context.GetPlayerName().c_str(),
									 context.GetPlayerName().length());
				string avatarFile = client->GetQtToolsInterface().stringFromUtf8(context.GetAvatarFile());
				if (!avatarFile.empty())
				{
					MD5Buf tmpMD5;
					if (client->GetAvatarManager().GetHashForAvatar(avatarFile, tmpMD5))
					{
						// TODO: use sha1.
						noauthLogin->avatar =
								OCTET_STRING_new_fromBuf(
									&asn_DEF_OCTET_STRING,
									(const char *)tmpMD5.GetData(),
									MD5_DATA_SIZE);
					}
				}
				client->GetSender().Send(context.GetSessionData(), init);
				client->SetState(ClientStateWaitSession::Instance());
			}
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
		boost::posix_time::milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
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
ClientStateWaitEnterLogin::TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this)
	{
		ClientThread::LoginData loginData;
		if (client->GetLoginData(loginData))
		{
			ClientContext &context = client->GetContext();
			boost::shared_ptr<NetPacket> init(new NetPacket(NetPacket::Alloc));
			init->GetMsg()->present = PokerTHMessage_PR_initMessage;
			InitMessage_t *netInit = &init->GetMsg()->choice.initMessage;
			netInit->requestedVersion.major = NET_VERSION_MAJOR;
			netInit->requestedVersion.minor = NET_VERSION_MINOR;

			context.SetPlayerName(loginData.userName);

			// Handle guest login first.
			if (loginData.isGuest)
			{
				context.SetPassword("");
				context.SetPlayerRights(PLAYER_RIGHTS_GUEST);
				netInit->login.present = login_PR_guestLogin;
				GuestLogin_t *guestLogin = &netInit->login.choice.guestLogin;
				OCTET_STRING_fromBuf(&guestLogin->nickName,
									 context.GetPlayerName().c_str(),
									 context.GetPlayerName().length());

				client->GetSender().Send(context.GetSessionData(), init);
				client->SetState(ClientStateWaitSession::Instance());
			}
			// If the player is not a guest, authenticate.
			else
			{
				context.SetPassword(loginData.password);
				netInit->login.present = login_PR_authenticatedLogin;
				AuthenticatedLogin_t *authLogin = &netInit->login.choice.authenticatedLogin;
				// Send authentication user data for challenge/response in init.
				boost::shared_ptr<SessionData> tmpSession = context.GetSessionData();
				tmpSession->CreateClientAuthSession(client->GetAuthContext(), context.GetPlayerName(), context.GetPassword());
				if (!tmpSession->AuthStep(1, ""))
					throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD, 0);
				string outUserData(tmpSession->AuthGetNextOutMsg());
				OCTET_STRING_fromBuf(&authLogin->clientUserData,
									 outUserData.c_str(),
									 outUserData.length());
				string avatarFile = client->GetQtToolsInterface().stringFromUtf8(context.GetAvatarFile());
				if (!avatarFile.empty())
				{
					MD5Buf tmpMD5;
					if (client->GetAvatarManager().GetHashForAvatar(avatarFile, tmpMD5))
					{
						// TODO: use sha1.
						authLogin->avatar =
								OCTET_STRING_new_fromBuf(
									&asn_DEF_OCTET_STRING,
									(const char *)tmpMD5.GetData(),
									MD5_DATA_SIZE);
					}
				}
				client->GetSender().Send(context.GetSessionData(), init);
				client->SetState(ClientStateWaitAuthChallenge::Instance());
			}
		}
		else
		{
			client->GetStateTimer().expires_from_now(
				boost::posix_time::milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
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
ClientStateWaitAuthChallenge::Enter(boost::shared_ptr<ClientThread> client)
{
}

void
ClientStateWaitAuthChallenge::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitAuthChallenge::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_authMessage)
	{
		// Check subtype.
		AuthMessage_t *netAuth = &tmpPacket->GetMsg()->choice.authMessage;
		if (netAuth->present == AuthMessage_PR_authServerChallenge)
		{
			AuthServerChallenge_t *netChallenge = &netAuth->choice.authServerChallenge;
			string challengeStr = STL_STRING_FROM_OCTET_STRING(netChallenge->serverChallenge);
			boost::shared_ptr<SessionData> tmpSession = client->GetContext().GetSessionData();
			if (!tmpSession->AuthStep(2, challengeStr.c_str()))
				throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD, 0);
			string outUserData(tmpSession->AuthGetNextOutMsg());


			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_authMessage;
			AuthMessage_t *outAuth = &packet->GetMsg()->choice.authMessage;
			outAuth->present = AuthMessage_PR_authClientResponse;
			AuthClientResponse_t *outResponse = &outAuth->choice.authClientResponse;

			OCTET_STRING_fromBuf(&outResponse->clientResponse,
								 outUserData.c_str(),
								 outUserData.length());
			client->GetSender().Send(tmpSession, packet);
			client->SetState(ClientStateWaitAuthVerify::Instance());
		}
		else
			throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD, 0);
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
	if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_authMessage)
	{
		// Check subtype.
		AuthMessage_t *netAuth = &tmpPacket->GetMsg()->choice.authMessage;
		if (netAuth->present == AuthMessage_PR_authServerVerification)
		{
			AuthServerVerification_t *netVerification = &netAuth->choice.authServerVerification;
			string verificationStr = STL_STRING_FROM_OCTET_STRING(netVerification->serverVerification);
			boost::shared_ptr<SessionData> tmpSession = client->GetContext().GetSessionData();
			if (!tmpSession->AuthStep(3, verificationStr.c_str()))
				throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD, 0);

			client->SetState(ClientStateWaitSession::Instance());
		}
		else
			throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD, 0);
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
	if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_initAckMessage)
	{
		// Everything is fine - we are in the lobby.
		InitAckMessage_t *netInitAck = &tmpPacket->GetMsg()->choice.initAckMessage;
		client->SetGuiPlayerId(netInitAck->yourPlayerId);

		client->SetSessionEstablished(true);
		client->GetCallback().SignalNetClientConnect(MSG_SOCK_SESSION_DONE);
		client->SetState(ClientStateWaitJoin::Instance());
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_avatarRequestMessage)
	{
		// Before letting us join the lobby, the server requests our avatar.
		AvatarRequestMessage_t *netAvatarRequest = &tmpPacket->GetMsg()->choice.avatarRequestMessage;

		// TODO compare SHA1.
		NetPacketList tmpList;
		int avatarError = client->GetAvatarManager().AvatarFileToNetPackets(
			client->GetQtToolsInterface().stringFromUtf8(client->GetContext().GetAvatarFile()),
			netAvatarRequest->requestId,
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

	if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_joinGameReplyMessage)
	{
		JoinGameReplyMessage_t *netJoinReply = &tmpPacket->GetMsg()->choice.joinGameReplyMessage;
		unsigned gameId = netJoinReply->gameId;
		if (netJoinReply->joinGameResult.present == joinGameResult_PR_joinGameAck)
		{
			// Successfully joined a game.
			JoinGameAck_t *netJoinAck = &netJoinReply->joinGameResult.choice.joinGameAck;
			client->SetGameId(gameId);
			GameData tmpData;
			NetPacket::GetGameData(&netJoinAck->gameInfo, tmpData);
			client->SetGameData(tmpData);

			// Player number is 0 on init. Will be set when the game starts.
			boost::shared_ptr<PlayerData> playerData(
				new PlayerData(client->GetGuiPlayerId(), 0, PLAYER_TYPE_HUMAN,
					context.GetPlayerRights(), netJoinAck->areYouGameAdmin));
			playerData->SetName(context.GetPlayerName());
			playerData->SetAvatarFile(context.GetAvatarFile());
			client->AddPlayerData(playerData);

			client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_JOIN);
			client->SetState(ClientStateWaitGame::Instance());
		}
		else if (netJoinReply->joinGameResult.present == joinGameResult_PR_joinGameFailed)
		{
			// Failed to join a game.
			JoinGameFailed_t *netJoinFailed = &netJoinReply->joinGameResult.choice.joinGameFailed;

			int failureCode;
			switch (netJoinFailed->joinGameFailureReason)
			{
				case joinGameFailureReason_invalidGame :
					failureCode = NTF_NET_JOIN_GAME_INVALID;
					break;
				case joinGameFailureReason_gameIsFull :
					failureCode = NTF_NET_JOIN_GAME_FULL;
					break;
				case joinGameFailureReason_gameIsRunning :
					failureCode = NTF_NET_JOIN_ALREADY_RUNNING;
					break;
				case joinGameFailureReason_invalidPassword :
					failureCode = NTF_NET_JOIN_INVALID_PASSWORD;
					break;
				case joinGameFailureReason_notAllowedAsGuest :
					failureCode = NTF_NET_JOIN_GUEST_FORBIDDEN;
					break;
				case joinGameFailureReason_notInvited :
					failureCode = NTF_NET_JOIN_NOT_INVITED;
					break;
				default :
					failureCode = NTF_NET_INTERNAL;
					break;
			}

			client->GetCallback().SignalNetClientNotification(failureCode);
		}
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_inviteNotifyMessage)
	{
		InviteNotifyMessage_t *netInvNotify = &tmpPacket->GetMsg()->choice.inviteNotifyMessage;
		if (netInvNotify->playerIdWho == client->GetGuiPlayerId())
		{
			client->GetCallback().SignalSelfGameInvitation(netInvNotify->gameId, netInvNotify->playerIdByWhom);
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
	if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_startEventMessage)
	{
		client->SetState(ClientStateSynchronizeStart::Instance());
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_gamePlayerMessage)
	{
		GamePlayerMessage_t *netGamePlayer = &tmpPacket->GetMsg()->choice.gamePlayerMessage;
//		unsigned gameId = netGamePlayer->gameId;
		if (netGamePlayer->gamePlayerNotification.present == gamePlayerNotification_PR_gamePlayerJoined)
		{
			// Another player joined the network game.
			GamePlayerJoined_t *netPlayerJoined = &netGamePlayer->gamePlayerNotification.choice.gamePlayerJoined;

			boost::shared_ptr<PlayerData> playerData;
			PlayerInfo info;
			if (client->GetCachedPlayerInfo(netPlayerJoined->playerId, info))
			{
				playerData.reset(
					new PlayerData(netPlayerJoined->playerId, 0, info.ptype,
					info.isGuest ? PLAYER_RIGHTS_GUEST : PLAYER_RIGHTS_NORMAL, netPlayerJoined->isGameAdmin));
				playerData->SetName(info.playerName);
				if (info.hasAvatar)
				{
					string avatarFile;
					if (client->GetAvatarManager().GetAvatarFileName(info.avatar, avatarFile))
						playerData->SetAvatarFile(client->GetQtToolsInterface().stringToUtf8(avatarFile));
					else
						client->RetrieveAvatarIfNeeded(netPlayerJoined->playerId, info);
				}
			}
			else
			{
				ostringstream name;
				name << "#" << netPlayerJoined->playerId;

				// Request player info.
				client->RequestPlayerInfo(netPlayerJoined->playerId, true);
				// Use temporary data until the PlayerInfo request is completed.
				playerData.reset(
					new PlayerData(netPlayerJoined->playerId, 0, PLAYER_TYPE_HUMAN, PLAYER_RIGHTS_NORMAL, netPlayerJoined->isGameAdmin));
				playerData->SetName(name.str());
			}
			client->AddPlayerData(playerData);
		}
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_inviteNotifyMessage)
	{
		InviteNotifyMessage_t *netInvNotify = &tmpPacket->GetMsg()->choice.inviteNotifyMessage;
		client->GetCallback().SignalPlayerGameInvitation(
			netInvNotify->gameId,
			netInvNotify->playerIdWho,
			netInvNotify->playerIdByWhom);
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_rejectInvNotifyMessage)
	{
		RejectInvNotifyMessage_t *netRejNotify = &tmpPacket->GetMsg()->choice.rejectInvNotifyMessage;
		client->GetCallback().SignalPlayerGameInvitation(
			netRejNotify->gameId,
			netRejNotify->playerId,
			static_cast<DenyGameInvitationReason>(netRejNotify->playerRejectReason));
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
		boost::posix_time::milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
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
	if (!ec && &client->GetState() == this)
	{
		if (client->IsSynchronized())
		{
			// Acknowledge start.
			boost::shared_ptr<NetPacket> startAck(new NetPacket(NetPacket::Alloc));
			startAck->GetMsg()->present = PokerTHMessage_PR_startEventAckMessage;
			StartEventAckMessage_t *netStartAck = &startAck->GetMsg()->choice.startEventAckMessage;
			netStartAck->gameId = client->GetGameId();

			client->GetSender().Send(client->GetContext().GetSessionData(), startAck);
			// Unsubscribe lobby messages.
			client->UnsubscribeLobbyMsg();

			client->SetState(ClientStateWaitStart::Instance());
		}
		else
		{
			client->GetStateTimer().expires_from_now(
				boost::posix_time::milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
			client->GetStateTimer().async_wait(
				boost::bind(
					&ClientStateSynchronizeStart::TimerLoop, this, boost::asio::placeholders::error, client));
		}
	}
}

void
ClientStateSynchronizeStart::InternalHandlePacket(boost::shared_ptr<ClientThread> /*client*/, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_gameStartMessage)
		throw ClientException(__FILE__, __LINE__, ERR_NET_START_TIMEOUT, 0);
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
	if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_gameStartMessage)
	{
		// Start the network game as client.
		GameStartMessage_t *netGameStart = &tmpPacket->GetMsg()->choice.gameStartMessage;

		StartData startData;
		startData.numberOfPlayers = netGameStart->playerSeats.list.count;
		startData.startDealerPlayerId = netGameStart->startDealerPlayerId;
		client->SetStartData(startData);

		// Set player numbers using the game start data slots.
		NonZeroId_t **playerIds = netGameStart->playerSeats.list.array;
		unsigned numPlayers = netGameStart->playerSeats.list.count;
		// Request player info for players if needed.
		if (numPlayers && playerIds && *playerIds)
		{
			for (unsigned i = 0; i < numPlayers; i++)
			{
				unsigned playerId = *playerIds[i];
				boost::shared_ptr<PlayerData> tmpPlayer = client->GetPlayerDataByUniqueId(playerId);
				if (!tmpPlayer.get())
					throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
				tmpPlayer->SetNumber(i);
			}
		}
		else
			throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PLAYER_COUNT, 0);

		client->InitGame();
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
	if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_handStartMessage)
	{
		// Remove all players which left the server.
		client->RemoveDisconnectedPlayers();

		// Hand was started.
		// These are the cards. Good luck.
		HandStartMessage_t *netHandStart = &tmpPacket->GetMsg()->choice.handStartMessage;
		int myCards[2];
		myCards[0] = (int)netHandStart->yourCard1;
		myCards[1] = (int)netHandStart->yourCard2;
		client->GetGame()->getSeatsList()->front()->setMyCards(myCards);
		client->GetGame()->initHand();
		client->GetGame()->getCurrentHand()->setSmallBlind(netHandStart->smallBlind);
		client->GetGame()->getCurrentHand()->getCurrentBeRo()->setMinimumRaise(2 * netHandStart->smallBlind);
		client->GetGame()->startHand();
		client->GetGui().dealHoleCards();
		client->GetGui().refreshGameLabels(GAME_STATE_PREFLOP);
		client->GetGui().refreshPot();
		client->GetGui().waitForGuiUpdateDone();

		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_HAND_START);
		client->SetState(ClientStateRunHand::Instance());
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_endOfGameMessage)
	{
		boost::shared_ptr<Game> curGame = client->GetGame();
		if (curGame.get())
		{
			EndOfGameMessage_t *netEndOfGame = &tmpPacket->GetMsg()->choice.endOfGameMessage;

			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(netEndOfGame->winnerPlayerId);
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
	if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_playersActionDoneMessage)
	{
		PlayersActionDoneMessage_t *netActionDone = &tmpPacket->GetMsg()->choice.playersActionDoneMessage;

		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(netActionDone->playerId);
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		bool isBigBlind = false;

		if (netActionDone->gameState == NetGameState_statePreflopSmallBlind)
		{
			curGame->getCurrentHand()->getCurrentBeRo()->setSmallBlindPositionId(tmpPlayer->getMyUniqueID());
			tmpPlayer->setMyButton(BUTTON_SMALL_BLIND);
		}
		else if (netActionDone->gameState == NetGameState_statePreflopBigBlind)
		{
			curGame->getCurrentHand()->getCurrentBeRo()->setBigBlindPositionId(tmpPlayer->getMyUniqueID());
			tmpPlayer->setMyButton(BUTTON_BIG_BLIND);
			isBigBlind = true;
		}
		else // no blind -> log
		{
			if (netActionDone->playerAction)
			{
				assert(netActionDone->totalPlayerBet >= (long)tmpPlayer->getMySet());
				client->GetGui().logPlayerActionMsg(
					tmpPlayer->getMyName(),
					netActionDone->playerAction,
					netActionDone->totalPlayerBet - tmpPlayer->getMySet());
			}
			// Update last players turn only after the blinds.
			curGame->getCurrentHand()->setLastPlayersTurn(tmpPlayer->getMyID());
		}

		tmpPlayer->setMyAction(netActionDone->playerAction);
		tmpPlayer->setMySetAbsolute(netActionDone->totalPlayerBet);
		tmpPlayer->setMyCash(netActionDone->playerMoney);
		curGame->getCurrentHand()->getCurrentBeRo()->setHighestSet(netActionDone->highestSet);
		curGame->getCurrentHand()->getCurrentBeRo()->setMinimumRaise(netActionDone->minimumRaise);
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
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_playersTurnMessage)
	{
		PlayersTurnMessage_t *netPlayersTurn = &tmpPacket->GetMsg()->choice.playersTurnMessage;

		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(netPlayersTurn->playerId);
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		// Set round.
		if (curGame->getCurrentHand()->getCurrentRound() != netPlayersTurn->gameState)
		{
			ResetPlayerActions(*curGame);
			curGame->getCurrentHand()->setCurrentRound(netPlayersTurn->gameState);
			// Refresh actions.
			client->GetGui().refreshSet();
			client->GetGui().refreshAction();
		}

		// Next player's turn.
		curGame->getCurrentHand()->getCurrentBeRo()->setCurrentPlayersTurnId(tmpPlayer->getMyID());
		curGame->getCurrentHand()->getCurrentBeRo()->setPlayersTurn(tmpPlayer->getMyID());

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
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_dealFlopCardsMessage)
	{
		DealFlopCardsMessage_t *netDealFlop = &tmpPacket->GetMsg()->choice.dealFlopCardsMessage;

		int tmpCards[5];
		tmpCards[0] = static_cast<int>(netDealFlop->flopCard1);
		tmpCards[1] = static_cast<int>(netDealFlop->flopCard2);
		tmpCards[2] = static_cast<int>(netDealFlop->flopCard3);
		tmpCards[3] = tmpCards[4] = 0;
		curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
		curGame->getCurrentHand()->getBoard()->collectPot();
		curGame->getCurrentHand()->setLastPlayersTurn(-1);

		client->GetGui().logDealBoardCardsMsg(GAME_STATE_FLOP, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
		client->GetGui().refreshGameLabels(GAME_STATE_FLOP);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		client->GetGui().dealBeRoCards(1);
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_dealTurnCardMessage)
	{
		DealTurnCardMessage_t *netDealTurn = &tmpPacket->GetMsg()->choice.dealTurnCardMessage;

		int tmpCards[5];
		curGame->getCurrentHand()->getBoard()->getMyCards(tmpCards);
		tmpCards[3] = static_cast<int>(netDealTurn->turnCard);
		curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
		curGame->getCurrentHand()->getBoard()->collectPot();
		curGame->getCurrentHand()->setLastPlayersTurn(-1);

		client->GetGui().logDealBoardCardsMsg(GAME_STATE_TURN, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
		client->GetGui().refreshGameLabels(GAME_STATE_TURN);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		client->GetGui().dealBeRoCards(2);
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_dealRiverCardMessage)
	{
		DealRiverCardMessage_t *netDealRiver = &tmpPacket->GetMsg()->choice.dealRiverCardMessage;

		int tmpCards[5];
		curGame->getCurrentHand()->getBoard()->getMyCards(tmpCards);
		tmpCards[4] = static_cast<int>(netDealRiver->riverCard);
		curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
		curGame->getCurrentHand()->getBoard()->collectPot();
		curGame->getCurrentHand()->setLastPlayersTurn(-1);

		client->GetGui().logDealBoardCardsMsg(GAME_STATE_RIVER, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
		client->GetGui().refreshGameLabels(GAME_STATE_RIVER);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		client->GetGui().dealBeRoCards(3);
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_allInShowCardsMessage)
	{
		AllInShowCardsMessage_t *netAllInShow = &tmpPacket->GetMsg()->choice.allInShowCardsMessage;

		curGame->getCurrentHand()->setAllInCondition(true);

		// Set player numbers using the game start data slots.
		PlayerAllIn_t **allInPlayers = netAllInShow->playersAllIn.list.array;
		unsigned numPlayers = netAllInShow->playersAllIn.list.count;
		// Request player info for players if needed.
		for (unsigned i = 0; i < numPlayers; i++)
		{
			PlayerAllIn_t *p = allInPlayers[i];

			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(p->playerId);
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

			int tmpCards[2];
			tmpCards[0] = static_cast<int>(p->allInCard1);
			tmpCards[1] = static_cast<int>(p->allInCard2);
			tmpPlayer->setMyCards(tmpCards);
		}
		client->GetGui().flipHolecardsAllIn();
	}
	else if (tmpPacket->GetMsg()->present == PokerTHMessage_PR_endOfHandMessage)
	{
		EndOfHandMessage_t *netEndHand = &tmpPacket->GetMsg()->choice.endOfHandMessage;

		if (netEndHand->endOfHandType.present == endOfHandType_PR_endOfHandHideCards)
		{
			EndOfHandHideCards_t *hideCards = &netEndHand->endOfHandType.choice.endOfHandHideCards;
			curGame->getCurrentHand()->getBoard()->collectPot();
			// Reset player sets
			ResetPlayerSets(*curGame);
			client->GetGui().refreshPot();
			client->GetGui().refreshSet();
			// Synchronize with GUI.
			client->GetGui().waitForGuiUpdateDone();

			// End of Hand, but keep cards hidden.
			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(hideCards->playerId);
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

			tmpPlayer->setMyCash(hideCards->playerMoney);
			tmpPlayer->setLastMoneyWon(hideCards->moneyWon);
			list<unsigned> winnerList;
			winnerList.push_back(tmpPlayer->getMyUniqueID());

			curGame->getCurrentHand()->getBoard()->setPot(0);
			curGame->getCurrentHand()->getBoard()->setWinners(winnerList);

			client->GetGui().postRiverRunAnimation1();

			// Wait for next Hand.
			client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_SERVER_HAND_END);
			client->SetState(ClientStateWaitHand::Instance());
		}
		else if (netEndHand->endOfHandType.present == endOfHandType_PR_endOfHandShowCards)
		{
			EndOfHandShowCards_t *showCards = &netEndHand->endOfHandType.choice.endOfHandShowCards;

			curGame->getCurrentHand()->getBoard()->collectPot();
			// Reset player sets
			ResetPlayerSets(*curGame);
			client->GetGui().refreshPot();
			client->GetGui().refreshSet();
			// Synchronize with GUI.
			client->GetGui().waitForGuiUpdateDone();

			// End of Hand, show cards.
			list<unsigned> winnerList;
			int highestValueOfCards = 0;
			PlayerResult_t **playerResults = showCards->playerResults.list.array;
			unsigned numResults = showCards->playerResults.list.count;
			// Request player info for players if needed.
			for (unsigned i = 0; i < numResults; i++)
			{
				PlayerResult_t *r = playerResults[i];

				boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(r->playerId);
				if (!tmpPlayer)
					throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

				int tmpCards[2];
				int bestHandPos[5];
				tmpCards[0] = static_cast<int>(r->resultCard1);
				tmpCards[1] = static_cast<int>(r->resultCard2);
				tmpPlayer->setMyCards(tmpCards);
				for (int num = 0; num < 5; num++)
					bestHandPos[num] = *r->bestHandPosition.list.array[num];
				tmpPlayer->setMyCardsValueInt(r->cardsValue);
				tmpPlayer->setMyBestHandPosition(bestHandPos);
				if (tmpPlayer->getMyCardsValueInt() > highestValueOfCards)
					highestValueOfCards = tmpPlayer->getMyCardsValueInt();
				tmpPlayer->setMyCash(r->playerMoney);
				tmpPlayer->setLastMoneyWon(r->moneyWon);
				if (r->moneyWon)
					winnerList.push_back(r->playerId);
			}
	
			curGame->getCurrentHand()->getCurrentBeRo()->setHighestCardsValue(highestValueOfCards);
			curGame->getCurrentHand()->getBoard()->setPot(0);
			curGame->getCurrentHand()->getBoard()->setWinners(winnerList);

			client->GetGui().postRiverRunAnimation1();

			// Wait for next Hand.
			client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_HAND_END);
			client->SetState(ClientStateWaitHand::Instance());
		}
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

	while (i != end)
	{
		int action = (*i)->getMyAction();
		if (action != 1 && action != 6)
			(*i)->setMyAction(0);
		(*i)->setMySetNull();
		++i;
	}
}

void
ClientStateRunHand::ResetPlayerSets(Game &curGame)
{
	PlayerListIterator i = curGame.getSeatsList()->begin();
	PlayerListIterator end = curGame.getSeatsList()->end();
	while (i != end)
	{
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
