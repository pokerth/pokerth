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
			context.GetSessionData()->GetAsioSocket()->close();
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
		client->GetContext().GetSessionData()->GetAsioSocket()->close();
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_TIMEOUT, 0);
	}
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
	ClientContext &context = client->GetContext();

	NetPacketInit::Data initData;
	initData.password = context.GetPassword();
	initData.playerName = context.GetPlayerName();
	string avatarFile = client->GetQtToolsInterface().stringFromUtf8(context.GetAvatarFile());
	initData.showAvatar = false;
	if (!avatarFile.empty())
	{
		if (client->GetAvatarManager().GetHashForAvatar(avatarFile, initData.avatar))
			initData.showAvatar = true;
	}

	boost::shared_ptr<NetPacket> packet(new NetPacketInit);
	((NetPacketInit *)packet.get())->SetData(initData);

	client->GetSender().Send(context.GetSessionData(), packet);

	client->SetState(ClientStateWaitSession::Instance());
}

void
ClientStateStartSession::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
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
	if (tmpPacket->ToNetPacketPlayerInfo())
	{
		NetPacketPlayerInfo::Data infoData;
		tmpPacket->ToNetPacketPlayerInfo()->GetData(infoData);
		client->SetPlayerInfo(infoData.playerId, infoData.playerInfo);
	}
	else if (tmpPacket->ToNetPacketUnknownPlayerId())
	{
		NetPacketUnknownPlayerId::Data unknownIdData;
		tmpPacket->ToNetPacketUnknownPlayerId()->GetData(unknownIdData);
		client->SetUnknownPlayer(unknownIdData.playerId);
	}
	else if (tmpPacket->ToNetPacketRemovedFromGame())
	{
		NetPacketRemovedFromGame::Data removedData;
		tmpPacket->ToNetPacketRemovedFromGame()->GetData(removedData);
		client->ClearPlayerDataList();
		// Resubscribe Lobby messages.
		client->ResubscribeLobbyMsg();
		// Show Lobby.
		client->GetCallback().SignalNetClientWaitDialog();
		client->GetCallback().SignalNetClientRemovedFromGame(removedData.removeReason);
		client->SetState(ClientStateWaitJoin::Instance());
	}
	else if (tmpPacket->ToNetPacketTimeoutWarning())
	{
		NetPacketTimeoutWarning::Data warningData;
		tmpPacket->ToNetPacketTimeoutWarning()->GetData(warningData);
		client->GetCallback().SignalNetClientShowTimeoutDialog(warningData.timeoutReason, warningData.remainingSeconds);
	}
	else if (tmpPacket->ToNetPacketChatText())
	{
		// Chat message - display it in the GUI.
		NetPacketChatText::Data chatData;
		tmpPacket->ToNetPacketChatText()->GetData(chatData);

		string playerName;
		if (chatData.playerId == 0)
		{
			playerName = "(global notice)";
		}
		else
		{
			boost::shared_ptr<PlayerData> tmpPlayer = client->GetPlayerDataByUniqueId(chatData.playerId);
			if (tmpPlayer.get())
				playerName = tmpPlayer->GetName();
		}
		if (!playerName.empty())
			client->GetCallback().SignalNetClientChatMsg(playerName, chatData.text);
	}
	else if (tmpPacket->ToNetPacketMsgBoxText())
	{
		// Message box - display it in the GUI.
		NetPacketMsgBoxText::Data msgData;
		tmpPacket->ToNetPacketMsgBoxText()->GetData(msgData);
		client->GetCallback().SignalNetClientMsgBox(msgData.text);
	}
	else if (tmpPacket->ToNetPacketPlayerLeft())
	{
		// A player left the game.
		NetPacketPlayerLeft::Data playerLeftData;
		tmpPacket->ToNetPacketPlayerLeft()->GetData(playerLeftData);

		// Signal to GUI and remove from data list.
		client->RemovePlayerData(playerLeftData.playerId, playerLeftData.removeReason);
	}
	else if (tmpPacket->ToNetPacketGameAdminChanged())
	{
		// New admin for the game.
		NetPacketGameAdminChanged::Data adminChangedData;
		tmpPacket->ToNetPacketGameAdminChanged()->GetData(adminChangedData);

		// Set new game admin and signal to GUI.
		client->SetNewGameAdmin(adminChangedData.playerId);
	}
	else if (tmpPacket->ToNetPacketGameListNew())
	{
		// A new game was created on the server.
		NetPacketGameListNew::Data gameListNewData;
		tmpPacket->ToNetPacketGameListNew()->GetData(gameListNewData);

		// Request player info for players if needed.
		PlayerIdList::const_iterator i = gameListNewData.gameInfo.players.begin();
		PlayerIdList::const_iterator end = gameListNewData.gameInfo.players.end();
		while (i != end)
		{
			PlayerInfo info;
			if (!client->GetCachedPlayerInfo(*i, info))
			{
				// Request player info.
				client->RequestPlayerInfo(*i);
			}
			++i;
		}

		client->AddGameInfo(gameListNewData.gameId, gameListNewData.gameInfo);
	}
	else if (tmpPacket->ToNetPacketGameListUpdate())
	{
		// An existing game was updated on the server.
		NetPacketGameListUpdate::Data gameListUpdateData;
		tmpPacket->ToNetPacketGameListUpdate()->GetData(gameListUpdateData);
		if (gameListUpdateData.gameMode == GAME_MODE_CLOSED)
			client->RemoveGameInfo(gameListUpdateData.gameId);
		else
			client->UpdateGameInfoMode(gameListUpdateData.gameId, gameListUpdateData.gameMode);
	}
	else if (tmpPacket->ToNetPacketGameListPlayerJoined())
	{
		NetPacketGameListPlayerJoined::Data playerJoinedData;
		tmpPacket->ToNetPacketGameListPlayerJoined()->GetData(playerJoinedData);
		client->ModifyGameInfoAddPlayer(playerJoinedData.gameId, playerJoinedData.playerId);
		// Request player info if needed.
		PlayerInfo info;
		if (!client->GetCachedPlayerInfo(playerJoinedData.playerId, info))
		{
			client->RequestPlayerInfo(playerJoinedData.playerId);
		}
	}
	else if (tmpPacket->ToNetPacketGameListPlayerLeft())
	{
		NetPacketGameListPlayerLeft::Data playerLeftData;
		tmpPacket->ToNetPacketGameListPlayerLeft()->GetData(playerLeftData);
		client->ModifyGameInfoRemovePlayer(playerLeftData.gameId, playerLeftData.playerId);
	}
	else if (tmpPacket->ToNetPacketGameListAdminChanged())
	{
		NetPacketGameListAdminChanged::Data adminChangedData;
		tmpPacket->ToNetPacketGameListAdminChanged()->GetData(adminChangedData);
		client->UpdateGameInfoAdmin(adminChangedData.gameId, adminChangedData.newAdminplayerId);
	}
	else if (tmpPacket->ToNetPacketStartKickPlayerPetition())
	{
		NetPacketStartKickPlayerPetition::Data startPetitionData;
		tmpPacket->ToNetPacketStartKickPlayerPetition()->GetData(startPetitionData);
		client->StartPetition(startPetitionData.petitionId, startPetitionData.proposingPlayerId,
			startPetitionData.kickPlayerId, startPetitionData.kickTimeoutSec, startPetitionData.numVotesNeededToKick);
	}
	else if (tmpPacket->ToNetPacketKickPlayerPetitionUpdate())
	{
		NetPacketKickPlayerPetitionUpdate::Data updatePetitionData;
		tmpPacket->ToNetPacketKickPlayerPetitionUpdate()->GetData(updatePetitionData);
		client->UpdatePetition(updatePetitionData.petitionId, updatePetitionData.numVotesAgainstKicking,
			updatePetitionData.numVotesInFavourOfKicking, updatePetitionData.numVotesNeededToKick);
	}
	else if (tmpPacket->ToNetPacketEndKickPlayerPetition())
	{
		NetPacketEndKickPlayerPetition::Data endPetitionData;
		tmpPacket->ToNetPacketEndKickPlayerPetition()->GetData(endPetitionData);
		client->EndPetition(endPetitionData.petitionId);
	}
	else if (tmpPacket->ToNetPacketAvatarHeader())
	{
		NetPacketAvatarHeader::Data headerData;
		tmpPacket->ToNetPacketAvatarHeader()->GetData(headerData);
		client->AddTempAvatarData(headerData.requestId, headerData.avatarFileSize, headerData.avatarFileType);
	}
	else if (tmpPacket->ToNetPacketAvatarFile())
	{
		NetPacketAvatarFile::Data fileData;
		tmpPacket->ToNetPacketAvatarFile()->GetData(fileData);
		client->StoreInTempAvatarData(fileData.requestId, fileData.fileData);
	}
	else if (tmpPacket->ToNetPacketAvatarEnd())
	{
		NetPacketAvatarEnd::Data endData;
		tmpPacket->ToNetPacketAvatarEnd()->GetData(endData);
		client->CompleteTempAvatarData(endData.requestId);
	}
	else if (tmpPacket->ToNetPacketUnknownAvatar())
	{
		NetPacketUnknownAvatar::Data unknownAvatarData;
		tmpPacket->ToNetPacketUnknownAvatar()->GetData(unknownAvatarData);
		client->SetUnknownAvatar(unknownAvatarData.requestId);
	}
	else if (tmpPacket->ToNetPacketStatisticsChanged())
	{
		NetPacketStatisticsChanged::Data statData;
		tmpPacket->ToNetPacketStatisticsChanged()->GetData(statData);
		client->UpdateStatData(statData.stats);
	}
	else if (tmpPacket->ToNetPacketError())
	{
		// Server reported an error.
		NetPacketError::Data errorData;
		tmpPacket->ToNetPacketError()->GetData(errorData);
		// Show the error.
		throw ClientException(__FILE__, __LINE__, errorData.errorCode, 0);
	}
	else
		InternalHandlePacket(client, tmpPacket);
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
ClientStateWaitSession::Enter(boost::shared_ptr<ClientThread> client)
{
	// Now we finally start receiving data.
	client->StartAsyncRead();
}

void
ClientStateWaitSession::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitSession::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->ToNetPacketInitAck())
	{
		// Everything is fine - we are in the lobby.
		NetPacketInitAck::Data initAckData;
		tmpPacket->ToNetPacketInitAck()->GetData(initAckData);
		// Check current game version.
		if (initAckData.latestGameVersion != POKERTH_VERSION)
			client->GetCallback().SignalNetClientNotification(NTF_NET_NEW_RELEASE_AVAILABLE);
		else if (POKERTH_BETA_REVISION && initAckData.latestBetaRevision != POKERTH_BETA_REVISION)
			client->GetCallback().SignalNetClientNotification(NTF_NET_OUTDATED_BETA);

		client->SetGuiPlayerId(initAckData.playerId);

		client->SetSessionEstablished(true);
		client->GetCallback().SignalNetClientConnect(MSG_SOCK_SESSION_DONE);
		client->SetState(ClientStateWaitJoin::Instance());
	}
	else if (tmpPacket->ToNetPacketRetrieveAvatar())
	{
		// Before letting us join the lobby, the server requests our avatar.
		NetPacketRetrieveAvatar::Data retrieveAvatarData;
		tmpPacket->ToNetPacketRetrieveAvatar()->GetData(retrieveAvatarData);

		NetPacketList tmpList;
		int avatarError = client->GetAvatarManager().AvatarFileToNetPackets(
			client->GetQtToolsInterface().stringFromUtf8(client->GetContext().GetAvatarFile()),
			retrieveAvatarData.requestId,
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

	if (tmpPacket->ToNetPacketJoinGameAck())
	{
		// Successfully joined a game.
		NetPacketJoinGameAck::Data joinGameAckData;
		tmpPacket->ToNetPacketJoinGameAck()->GetData(joinGameAckData);
		client->SetGameId(joinGameAckData.gameId);
		client->SetGameData(joinGameAckData.gameData);

		// Player number is 0 on init. Will be set when the game starts.
		boost::shared_ptr<PlayerData> playerData(
			new PlayerData(client->GetGuiPlayerId(), 0, PLAYER_TYPE_HUMAN, joinGameAckData.prights));
		playerData->SetName(context.GetPlayerName());
		playerData->SetAvatarFile(context.GetAvatarFile());
		client->AddPlayerData(playerData);

		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_JOIN);
		client->SetState(ClientStateWaitGame::Instance());
	}
	else if (tmpPacket->ToNetPacketJoinGameFailed())
	{
		// Failed to join a game.
		NetPacketJoinGameFailed::Data joinGameFailedData;
		tmpPacket->ToNetPacketJoinGameFailed()->GetData(joinGameFailedData);
		client->GetCallback().SignalNetClientNotification(joinGameFailedData.failureCode);
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
	if (tmpPacket->ToNetPacketStartEvent())
	{
		client->SetState(ClientStateSynchronizeStart::Instance());
	}
	else if (tmpPacket->ToNetPacketPlayerJoined())
	{
		// Another player joined the network game.
		NetPacketPlayerJoined::Data netPlayerData;
		tmpPacket->ToNetPacketPlayerJoined()->GetData(netPlayerData);

		boost::shared_ptr<PlayerData> playerData;
		PlayerInfo info;
		if (client->GetCachedPlayerInfo(netPlayerData.playerId, info))
		{
			playerData.reset(
				new PlayerData(netPlayerData.playerId, 0, info.ptype, netPlayerData.prights));
			playerData->SetName(info.playerName);
			if (info.hasAvatar)
			{
				string avatarFile;
				if (client->GetAvatarManager().GetAvatarFileName(info.avatar, avatarFile))
					playerData->SetAvatarFile(client->GetQtToolsInterface().stringToUtf8(avatarFile));
				else
					client->RetrieveAvatarIfNeeded(netPlayerData.playerId, info);
			}
		}
		else
		{
			ostringstream name;
			name << "#" << netPlayerData.playerId;

			// Request player info.
			client->RequestPlayerInfo(netPlayerData.playerId, true);
			// Use temporary data until the PlayerInfo request is completed.
			playerData.reset(
				new PlayerData(netPlayerData.playerId, 0, PLAYER_TYPE_HUMAN, netPlayerData.prights));
			playerData->SetName(name.str());
		}
		client->AddPlayerData(playerData);
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
			boost::shared_ptr<NetPacket> startAck(new NetPacketStartEventAck);
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
ClientStateSynchronizeStart::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->ToNetPacketGameStart())
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
	if (tmpPacket->ToNetPacketGameStart())
	{
		// Start the network game as client.
		NetPacketGameStart::Data gameStartData;
		tmpPacket->ToNetPacketGameStart()->GetData(gameStartData);

		client->SetStartData(gameStartData.startData);

		// Set player numbers using the game start data slots.
		NetPacketGameStart::PlayerSlotList::const_iterator slot_i = gameStartData.playerSlots.begin();
		NetPacketGameStart::PlayerSlotList::const_iterator slot_end = gameStartData.playerSlots.end();
		int num = 0;

		while (slot_i != slot_end)
		{
			unsigned playerId = (*slot_i).playerId;
			boost::shared_ptr<PlayerData> tmpPlayer = client->GetPlayerDataByUniqueId(playerId);
			if (!tmpPlayer.get())
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			tmpPlayer->SetNumber(num);

			++num;
			++slot_i;
		}

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
	if (tmpPacket->ToNetPacketHandStart())
	{
		// Remove all players which left the server.
		client->RemoveDisconnectedPlayers();
		// Hand was started.
		// These are the cards. Good luck.
		NetPacketHandStart::Data tmpData;
		tmpPacket->ToNetPacketHandStart()->GetData(tmpData);
		int myCards[2];
		myCards[0] = (int)tmpData.yourCards[0];
		myCards[1] = (int)tmpData.yourCards[1];
		client->GetGame()->getSeatsList()->front()->setMyCards(myCards);
		client->GetGame()->initHand();
		client->GetGame()->getCurrentHand()->setSmallBlind(tmpData.smallBlind);
		client->GetGame()->getCurrentHand()->getCurrentBeRo()->setMinimumRaise(2 * tmpData.smallBlind);
		client->GetGame()->startHand();
		client->GetGui().dealHoleCards();
		client->GetGui().refreshGameLabels(GAME_STATE_PREFLOP);
		client->GetGui().refreshPot();
		client->GetGui().waitForGuiUpdateDone();

		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_HAND_START);
		client->SetState(ClientStateRunHand::Instance());
	}
	else if (tmpPacket->ToNetPacketEndOfGame())
	{
		boost::shared_ptr<Game> curGame = client->GetGame();
		if (curGame.get())
		{
			NetPacketEndOfGame::Data endData;
			tmpPacket->ToNetPacketEndOfGame()->GetData(endData);
			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(endData.winnerPlayerId);
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
	if (tmpPacket->ToNetPacketPlayersActionDone())
	{
		NetPacketPlayersActionDone::Data actionDoneData;
		tmpPacket->ToNetPacketPlayersActionDone()->GetData(actionDoneData);
		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(actionDoneData.playerId);
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		bool isBigBlind = false;

		if (actionDoneData.gameState == GAME_STATE_PREFLOP_SMALL_BLIND)
		{
			curGame->getCurrentHand()->getCurrentBeRo()->setSmallBlindPositionId(tmpPlayer->getMyUniqueID());
			tmpPlayer->setMyButton(BUTTON_SMALL_BLIND);
		}
		else if (actionDoneData.gameState == GAME_STATE_PREFLOP_BIG_BLIND)
		{
			curGame->getCurrentHand()->getCurrentBeRo()->setBigBlindPositionId(tmpPlayer->getMyUniqueID());
			tmpPlayer->setMyButton(BUTTON_BIG_BLIND);
			isBigBlind = true;
		}
		else // no blind -> log
		{
			if (actionDoneData.playerAction)
			{
				assert(actionDoneData.totalPlayerBet >= (unsigned)tmpPlayer->getMySet());
				client->GetGui().logPlayerActionMsg(
					tmpPlayer->getMyName(),
					actionDoneData.playerAction,
					actionDoneData.totalPlayerBet - tmpPlayer->getMySet());
			}
			// Update last players turn only after the blinds.
			curGame->getCurrentHand()->setLastPlayersTurn(tmpPlayer->getMyID());
		}

		tmpPlayer->setMyAction(actionDoneData.playerAction);
		tmpPlayer->setMySetAbsolute(actionDoneData.totalPlayerBet);
		tmpPlayer->setMyCash(actionDoneData.playerMoney);
		curGame->getCurrentHand()->getCurrentBeRo()->setHighestSet(actionDoneData.highestSet);
		curGame->getCurrentHand()->getCurrentBeRo()->setMinimumRaise(actionDoneData.minimumRaise);
		curGame->getCurrentHand()->getBoard()->collectSets();
		curGame->getCurrentHand()->switchRounds();

		//log blinds sets after setting bigblind-button
		if (isBigBlind) {
			client->GetGui().logNewBlindsSetsMsg(curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getSmallBlindPositionId())->getMySet(), curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getBigBlindPositionId())->getMySet(), curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getSmallBlindPositionId())->getMyName(),  curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getBigBlindPositionId())->getMyName());
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
	else if (tmpPacket->ToNetPacketPlayersTurn())
	{
		NetPacketPlayersTurn::Data turnData;
		tmpPacket->ToNetPacketPlayersTurn()->GetData(turnData);
		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(turnData.playerId);
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		// Set round.
		if (curGame->getCurrentHand()->getCurrentRound() != turnData.gameState)
		{
			ResetPlayerActions(*curGame);
			curGame->getCurrentHand()->setCurrentRound(turnData.gameState);
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
	else if (tmpPacket->ToNetPacketDealFlopCards())
	{
		NetPacketDealFlopCards::Data cardsData;
		tmpPacket->ToNetPacketDealFlopCards()->GetData(cardsData);
		int tmpCards[5];
		for (int num = 0; num < 3; num++)
			tmpCards[num] = static_cast<int>(cardsData.flopCards[num]);
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
	else if (tmpPacket->ToNetPacketDealTurnCard())
	{
		NetPacketDealTurnCard::Data cardsData;
		tmpPacket->ToNetPacketDealTurnCard()->GetData(cardsData);
		int tmpCards[5];
		curGame->getCurrentHand()->getBoard()->getMyCards(tmpCards);
		tmpCards[3] = static_cast<int>(cardsData.turnCard);
		curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
		curGame->getCurrentHand()->getBoard()->collectPot();
		curGame->getCurrentHand()->setLastPlayersTurn(-1);

		client->GetGui().logDealBoardCardsMsg(GAME_STATE_TURN, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
		client->GetGui().refreshGameLabels(GAME_STATE_TURN);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		client->GetGui().dealBeRoCards(2);
	}
	else if (tmpPacket->ToNetPacketDealRiverCard())
	{
		NetPacketDealRiverCard::Data cardsData;
		tmpPacket->ToNetPacketDealRiverCard()->GetData(cardsData);
		int tmpCards[5];
		curGame->getCurrentHand()->getBoard()->getMyCards(tmpCards);
		tmpCards[4] = static_cast<int>(cardsData.riverCard);
		curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
		curGame->getCurrentHand()->getBoard()->collectPot();
		curGame->getCurrentHand()->setLastPlayersTurn(-1);

		client->GetGui().logDealBoardCardsMsg(GAME_STATE_RIVER, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
		client->GetGui().refreshGameLabels(GAME_STATE_RIVER);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		client->GetGui().dealBeRoCards(3);
	}
	else if (tmpPacket->ToNetPacketAllInShowCards())
	{
		curGame->getCurrentHand()->setAllInCondition(true);

		NetPacketAllInShowCards::Data allInData;
		tmpPacket->ToNetPacketAllInShowCards()->GetData(allInData);

		NetPacketAllInShowCards::PlayerCardsList::const_iterator i
			= allInData.playerCards.begin();
		NetPacketAllInShowCards::PlayerCardsList::const_iterator end
			= allInData.playerCards.end();

		while (i != end)
		{
			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId((*i).playerId);
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

			int tmpCards[2];
			tmpCards[0] = static_cast<int>((*i).cards[0]);
			tmpCards[1] = static_cast<int>((*i).cards[1]);
			tmpPlayer->setMyCards(tmpCards);
			++i;
		}
		client->GetGui().flipHolecardsAllIn();
	}
	else if (tmpPacket->ToNetPacketEndOfHandHideCards())
	{
		curGame->getCurrentHand()->getBoard()->collectPot();
		// Reset player sets
		ResetPlayerSets(*curGame);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		// Synchronize with GUI.
		client->GetGui().waitForGuiUpdateDone();

		// End of Hand, but keep cards hidden.
		NetPacketEndOfHandHideCards::Data endHandData;
		tmpPacket->ToNetPacketEndOfHandHideCards()->GetData(endHandData);

		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(endHandData.playerId);
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		tmpPlayer->setMyCash(endHandData.playerMoney);
		tmpPlayer->setLastMoneyWon(endHandData.moneyWon);
		list<unsigned> winnerList;
		winnerList.push_back(tmpPlayer->getMyUniqueID());

		curGame->getCurrentHand()->getBoard()->setPot(0);
		curGame->getCurrentHand()->getBoard()->setWinners(winnerList);

		client->GetGui().postRiverRunAnimation1();

		// Wait for next Hand.
		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_SERVER_HAND_END);
		client->SetState(ClientStateWaitHand::Instance());
	}
	else if (tmpPacket->ToNetPacketEndOfHandShowCards())
	{
		curGame->getCurrentHand()->getBoard()->collectPot();
		// Reset player sets
		ResetPlayerSets(*curGame);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		// Synchronize with GUI.
		client->GetGui().waitForGuiUpdateDone();

		// End of Hand, show cards.
		NetPacketEndOfHandShowCards::Data endHandData;
		tmpPacket->ToNetPacketEndOfHandShowCards()->GetData(endHandData);

		NetPacketEndOfHandShowCards::PlayerResultList::const_iterator i
			= endHandData.playerResults.begin();
		NetPacketEndOfHandShowCards::PlayerResultList::const_iterator end
			= endHandData.playerResults.end();

		list<unsigned> winnerList;
		int highestValueOfCards = 0;
		while (i != end)
		{
			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId((*i).playerId);
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

			int tmpCards[2];
			int bestHandPos[5];
			tmpCards[0] = static_cast<int>((*i).cards[0]);
			tmpCards[1] = static_cast<int>((*i).cards[1]);
			tmpPlayer->setMyCards(tmpCards);
			for (int num = 0; num < 5; num++)
				bestHandPos[num] = (*i).bestHandPos[num];
			tmpPlayer->setMyCardsValueInt((*i).valueOfCards);
			tmpPlayer->setMyBestHandPosition(bestHandPos);
			if (tmpPlayer->getMyCardsValueInt() > highestValueOfCards)
				highestValueOfCards = tmpPlayer->getMyCardsValueInt();
			tmpPlayer->setMyCash((*i).playerMoney);
			tmpPlayer->setLastMoneyWon((*i).moneyWon);
			if ((*i).moneyWon)
				winnerList.push_back((*i).playerId);

			++i;
		}
		curGame->getCurrentHand()->getCurrentBeRo()->setHighestCardsValue(highestValueOfCards);
		curGame->getCurrentHand()->getBoard()->setPot(0);
		curGame->getCurrentHand()->getBoard()->setWinners(winnerList);

		client->GetGui().postRiverRunAnimation1();

		// Wait for next Hand.
		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_HAND_END);
		client->SetState(ClientStateWaitHand::Instance());
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

