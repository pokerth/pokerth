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
#include <net/senderthread.h>
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

int
ClientStateInit::Process(ClientThread &client)
{
	ClientContext &context = client.GetContext();

	if (context.GetServerAddr().empty())
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_SERVERADDR_NOT_SET, 0);

	if (context.GetServerPort() < 1024)
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_PORT, 0);

	client.SetContextSocket(socket(context.GetAddrFamily(), SOCK_STREAM, context.GetProtocol()));
	if (!IS_VALID_SOCKET(context.GetSocket()))
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());

	unsigned long mode = 1;
	if (IOCTLSOCKET(context.GetSocket(), FIONBIO, &mode) == SOCKET_ERROR)
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());

	// The following calls are optional - the return value is not checked.
	int nodelay = 1;
	setsockopt(context.GetSocket(), SOL_SOCKET, TCP_NODELAY, (char *)&nodelay, sizeof(nodelay));

#ifdef SO_NOSIGPIPE
	int nosigpipe = 1;
	setsockopt(context.GetSocket(), SOL_SOCKET, SO_NOSIGPIPE, (char *)&nosigpipe, sizeof(nosigpipe));
#endif

	if (context.GetUseServerList())
		client.SetState(ClientStateStartServerListDownload::Instance());
	else
		client.SetState(ClientStateStartResolve::Instance());

	return MSG_SOCK_INIT_DONE;
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

int
ClientStateStartResolve::Process(ClientThread &client)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	ClientContext &context = client.GetContext();

	context.GetClientSockaddr()->ss_family = context.GetAddrFamily();

	// Treat the server address as numbers first.
	if (socket_string_to_addr(
		context.GetServerAddr().c_str(),
		context.GetAddrFamily(),
		(struct sockaddr *)context.GetClientSockaddr(),
		context.GetClientSockaddrSize()))
	{
		// Success - but we still need to set the port.
		if (!socket_set_port(context.GetServerPort(), context.GetAddrFamily(), (struct sockaddr *)context.GetClientSockaddr(), context.GetClientSockaddrSize()))
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_SET_PORT_FAILED, 0);

		// No need to resolve - start connecting.
		client.SetState(ClientStateStartConnect::Instance());
		retVal = MSG_SOCK_RESOLVE_DONE;
	}
	else
	{
		// Start name resolution in a separate thread, since it is blocking
		// for up to about 30 seconds.
		std::auto_ptr<ResolverThread> resolver(new ResolverThread);
		resolver->Init(context);
		resolver->Run();

		ClientStateResolving::Instance().SetResolver(resolver.release());
		client.SetState(ClientStateResolving::Instance());
	}

	return retVal;
}

//-----------------------------------------------------------------------------

ClientStateResolving &
ClientStateResolving::Instance()
{
	static ClientStateResolving state;
	return state;
}

ClientStateResolving::ClientStateResolving()
: m_resolver(NULL)
{
}

ClientStateResolving::~ClientStateResolving()
{
	Cleanup();
}

void
ClientStateResolving::SetResolver(ResolverThread *resolver)
{
	Cleanup();
	m_resolver = resolver;
}

int
ClientStateResolving::Process(ClientThread &client)
{
	int retVal;

	if (!m_resolver)
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_RESOLVE_FAILED, 0);

	if (m_resolver->Join(CLIENT_WAIT_TIMEOUT_MSEC))
	{
		ClientContext &context = client.GetContext();
		bool success = m_resolver->GetResult(context);
		Cleanup(); // Not required, but better keep things clean.

		if (!success)
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_RESOLVE_FAILED, 0);

		client.SetState(ClientStateStartConnect::Instance());
		retVal = MSG_SOCK_RESOLVE_DONE;
	}
	else
		retVal = MSG_SOCK_INTERNAL_PENDING;

	return retVal;
}


void
ClientStateResolving::Cleanup()
{
	if (m_resolver)
	{
		if (m_resolver->Join(500))
			delete m_resolver;
		// If the resolver does not terminate fast enough, leave it
		// as memory leak.
		m_resolver = NULL;
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

int
ClientStateStartServerListDownload::Process(ClientThread &client)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	const ClientContext &context = client.GetContext();
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
		std::auto_ptr<DownloadHelper> downloader(new DownloadHelper);
		downloader->Init(serverListUrl + ".md5", tmpServerListPath.directory_string());
		ClientStateSynchronizingServerList::Instance().SetDownloadHelper(downloader.release());
		client.SetState(ClientStateSynchronizingServerList::Instance());
	}
	else
	{
		// Download server list.
		std::auto_ptr<DownloadHelper> downloader(new DownloadHelper);
		downloader->Init(serverListUrl, tmpServerListPath.directory_string());
		ClientStateDownloadingServerList::Instance().SetDownloadHelper(downloader.release());
		client.SetState(ClientStateDownloadingServerList::Instance());
	}

	return retVal;
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
ClientStateSynchronizingServerList::SetDownloadHelper(DownloadHelper *helper)
{
	m_downloadHelper.reset(helper);
}

int
ClientStateSynchronizingServerList::Process(ClientThread &client)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (m_downloadHelper->Process())
	{
		m_downloadHelper.reset(NULL);
		ClientContext &context = client.GetContext();
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
			client.SetState(ClientStateReadingServerList::Instance());
		}
		else
		{
			// Download new server list.
			// Paranoia check before removing the file, we do not want to delete wrong files.
			path tmpPath(serverListPath);
			if (path(context.GetCacheDir()) == tmpPath.remove_leaf())
			{
				remove(serverListPath);
				client.SetState(ClientStateStartServerListDownload::Instance());
			}
			else
				throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_URL, 0);
		}
	}

	return retVal;
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
ClientStateDownloadingServerList::SetDownloadHelper(DownloadHelper *helper)
{
	m_downloadHelper.reset(helper);
}

int
ClientStateDownloadingServerList::Process(ClientThread &client)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (m_downloadHelper->Process())
	{
		m_downloadHelper.reset(NULL);
		client.SetState(ClientStateReadingServerList::Instance());
	}

	return retVal;
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

int
ClientStateReadingServerList::Process(ClientThread &client)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	ClientContext &context = client.GetContext();
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
		client.ClearServerInfoMap();
		int serverCount = 0;
		unsigned lastServerInfoId = 0;
		TiXmlHandle docHandle(&doc);
		const TiXmlElement *nextServer = docHandle.FirstChild("ServerList" ).FirstChild("Server").ToElement();
		while (nextServer)
		{
			ServerInfo serverInfo;
			{
				int tmpId;
				nextServer->QueryIntAttribute("value", &tmpId);
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

			client.AddServerInfo(serverInfo.id, serverInfo);
			nextServer = nextServer->NextSiblingElement();
			lastServerInfoId = serverInfo.id;
			serverCount++;
		}

		if (serverCount == 1)
		{
			client.UseServer(lastServerInfoId);
			client.SetState(ClientStateStartResolve::Instance());
			retVal = MSG_SOCK_SERVER_LIST_DONE;
		}
		else if (serverCount > 1)
		{
			client.GetCallback().SignalNetClientServerListShow();
			client.SetState(ClientStateWaitChooseServer::Instance());
		}
		else
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_XML, 0);
	}
	else
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_XML, 0);


	return retVal;
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

int
ClientStateWaitChooseServer::Process(ClientThread &client)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	unsigned serverId;
	if (client.GetSelectedServer(serverId))
	{
		client.UseServer(serverId);
		client.SetState(ClientStateStartResolve::Instance());
		retVal = MSG_SOCK_SERVER_LIST_DONE;
	}
	else
		Thread::Msleep(20);

	return retVal;
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

int
ClientStateStartConnect::Process(ClientThread &client)
{
	int retVal;
	ClientContext &context = client.GetContext();

	int connectResult = connect(context.GetSocket(), (struct sockaddr *)context.GetClientSockaddr(), context.GetClientSockaddrSize());

	if (IS_VALID_CONNECT(connectResult))
	{
		client.SetState(ClientStateStartSession::Instance());
		retVal = MSG_SOCK_CONNECT_DONE;
	}
	else
	{
		int errCode = SOCKET_ERRNO();
		if (IS_SOCKET_ERR_WOULDBLOCK(errCode))
		{
			boost::timers::portable::microsec_timer connectTimer;
			ClientStateConnecting::Instance().SetTimer(connectTimer);
			client.SetState(ClientStateConnecting::Instance());
			retVal = MSG_SOCK_INTERNAL_PENDING;
		}
		else
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_FAILED, SOCKET_ERRNO());
	}

	return retVal;
}

//-----------------------------------------------------------------------------

ClientStateConnecting &
ClientStateConnecting::Instance()
{
	static ClientStateConnecting state;
	return state;
}

ClientStateConnecting::ClientStateConnecting()
{
}

ClientStateConnecting::~ClientStateConnecting()
{
}

void
ClientStateConnecting::SetTimer(const boost::timers::portable::microsec_timer &timer)
{
	m_connectTimer = timer;
}

int
ClientStateConnecting::Process(ClientThread &client)
{
	int retVal;
	ClientContext &context = client.GetContext();

	fd_set writeSet;
	struct timeval timeout;

	FD_ZERO(&writeSet);
	FD_SET(context.GetSocket(), &writeSet);

	timeout.tv_sec  = 0;
	timeout.tv_usec = CLIENT_WAIT_TIMEOUT_MSEC * 1000;
	int selectResult = select(context.GetSocket() + 1, NULL, &writeSet, NULL, &timeout);

	if (selectResult > 0) // success
	{
		// Check whether the connect call succeeded.
		int connectResult = 0;
		socklen_t tmpSize = sizeof(connectResult);
		getsockopt(context.GetSocket(), SOL_SOCKET, SO_ERROR, (char *)&connectResult, &tmpSize);
		if (connectResult != 0)
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_FAILED, connectResult);
		client.SetState(ClientStateStartSession::Instance());
		retVal = MSG_SOCK_CONNECT_DONE;
	}
	else if (selectResult == 0) // timeout
	{
		if (m_connectTimer.elapsed().total_seconds() >= CLIENT_CONNECT_TIMEOUT_SEC)
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_TIMEOUT, 0);
		else
			retVal = MSG_SOCK_INTERNAL_PENDING;
	}
	else
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_SELECT_FAILED, SOCKET_ERRNO());


	return retVal;
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

int
ClientStateStartSession::Process(ClientThread &client)
{
	ClientContext &context = client.GetContext();

	NetPacketInit::Data initData;
	initData.password = context.GetPassword();
	initData.playerName = context.GetPlayerName();
	string avatarFile = client.GetQtToolsInterface().stringFromUtf8(context.GetAvatarFile());
	initData.showAvatar = false;
	if (!avatarFile.empty())
	{
		if (client.GetAvatarManager().GetHashForAvatar(avatarFile, initData.avatar))
			initData.showAvatar = true;
	}

	boost::shared_ptr<NetPacket> packet(new NetPacketInit);
	((NetPacketInit *)packet.get())->SetData(initData);
	
	context.GetSessionData()->GetSender().Send(context.GetSessionData(), packet);

	client.SetState(ClientStateWaitSession::Instance());

	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

AbstractClientStateReceiving::AbstractClientStateReceiving()
{
}

AbstractClientStateReceiving::~AbstractClientStateReceiving()
{
}

int
AbstractClientStateReceiving::Process(ClientThread &client)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	// Check for avatar downloads.
	client.CheckAvatarDownloads();

	// Delegate to receiver helper class.
	boost::shared_ptr<NetPacket> tmpPacket =
		client.GetReceiver().Recv(client.GetContext().GetSocket(), client.GetContext().GetReceiveBuffer());

	if (tmpPacket.get())
	{
		if (tmpPacket->ToNetPacketPlayerInfo())
		{
			NetPacketPlayerInfo::Data infoData;
			tmpPacket->ToNetPacketPlayerInfo()->GetData(infoData);
			client.SetPlayerInfo(infoData.playerId, infoData.playerInfo);
		}
		else if (tmpPacket->ToNetPacketUnknownPlayerId())
		{
			NetPacketUnknownPlayerId::Data unknownIdData;
			tmpPacket->ToNetPacketUnknownPlayerId()->GetData(unknownIdData);
			client.SetUnknownPlayer(unknownIdData.playerId);
		}
		else if (tmpPacket->ToNetPacketRemovedFromGame())
		{
			NetPacketRemovedFromGame::Data removedData;
			tmpPacket->ToNetPacketRemovedFromGame()->GetData(removedData);
			client.ClearPlayerDataList();
			// Resubscribe Lobby messages.
			client.ResubscribeLobbyMsg();
			// Show Lobby.
			client.GetCallback().SignalNetClientWaitDialog();
			client.GetCallback().SignalNetClientRemovedFromGame(removedData.removeReason);
			client.SetState(ClientStateWaitJoin::Instance());
		}
		else if (tmpPacket->ToNetPacketTimeoutWarning())
		{
			NetPacketTimeoutWarning::Data warningData;
			tmpPacket->ToNetPacketTimeoutWarning()->GetData(warningData);
			client.GetCallback().SignalNetClientShowTimeoutDialog(warningData.timeoutReason, warningData.remainingSeconds);
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
				boost::shared_ptr<PlayerData> tmpPlayer = client.GetPlayerDataByUniqueId(chatData.playerId);
				if (tmpPlayer.get())
					playerName = tmpPlayer->GetName();
			}
			if (!playerName.empty())
				client.GetCallback().SignalNetClientChatMsg(playerName, chatData.text);
		}
		else if (tmpPacket->ToNetPacketMsgBoxText())
		{
			// Message box - display it in the GUI.
			NetPacketMsgBoxText::Data msgData;
			tmpPacket->ToNetPacketMsgBoxText()->GetData(msgData);
			client.GetCallback().SignalNetClientMsgBox(msgData.text);
		}
		else if (tmpPacket->ToNetPacketPlayerLeft())
		{
			// A player left the game.
			NetPacketPlayerLeft::Data playerLeftData;
			tmpPacket->ToNetPacketPlayerLeft()->GetData(playerLeftData);

			// Signal to GUI and remove from data list.
			client.RemovePlayerData(playerLeftData.playerId, playerLeftData.removeReason);
		}
		else if (tmpPacket->ToNetPacketGameAdminChanged())
		{
			// New admin for the game.
			NetPacketGameAdminChanged::Data adminChangedData;
			tmpPacket->ToNetPacketGameAdminChanged()->GetData(adminChangedData);

			// Set new game admin and signal to GUI.
			client.SetNewGameAdmin(adminChangedData.playerId);
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
				if (!client.GetCachedPlayerInfo(*i, info))
				{
					// Request player info.
					client.RequestPlayerInfo(*i);
				}
				++i;
			}

			client.AddGameInfo(gameListNewData.gameId, gameListNewData.gameInfo);
		}
		else if (tmpPacket->ToNetPacketGameListUpdate())
		{
			// An existing game was updated on the server.
			NetPacketGameListUpdate::Data gameListUpdateData;
			tmpPacket->ToNetPacketGameListUpdate()->GetData(gameListUpdateData);
			if (gameListUpdateData.gameMode == GAME_MODE_CLOSED)
				client.RemoveGameInfo(gameListUpdateData.gameId);
			else
				client.UpdateGameInfoMode(gameListUpdateData.gameId, gameListUpdateData.gameMode);
		}
		else if (tmpPacket->ToNetPacketGameListPlayerJoined())
		{
			NetPacketGameListPlayerJoined::Data playerJoinedData;
			tmpPacket->ToNetPacketGameListPlayerJoined()->GetData(playerJoinedData);
			client.ModifyGameInfoAddPlayer(playerJoinedData.gameId, playerJoinedData.playerId);
			// Request player info if needed.
			PlayerInfo info;
			if (!client.GetCachedPlayerInfo(playerJoinedData.playerId, info))
			{
				client.RequestPlayerInfo(playerJoinedData.playerId);
			}
		}
		else if (tmpPacket->ToNetPacketGameListPlayerLeft())
		{
			NetPacketGameListPlayerLeft::Data playerLeftData;
			tmpPacket->ToNetPacketGameListPlayerLeft()->GetData(playerLeftData);
			client.ModifyGameInfoRemovePlayer(playerLeftData.gameId, playerLeftData.playerId);
		}
		else if (tmpPacket->ToNetPacketGameListAdminChanged())
		{
			NetPacketGameListAdminChanged::Data adminChangedData;
			tmpPacket->ToNetPacketGameListAdminChanged()->GetData(adminChangedData);
			client.UpdateGameInfoAdmin(adminChangedData.gameId, adminChangedData.newAdminplayerId);
		}
		else if (tmpPacket->ToNetPacketStartKickPlayerPetition())
		{
			NetPacketStartKickPlayerPetition::Data startPetitionData;
			tmpPacket->ToNetPacketStartKickPlayerPetition()->GetData(startPetitionData);
			client.StartPetition(startPetitionData.petitionId, startPetitionData.proposingPlayerId,
				startPetitionData.kickPlayerId, startPetitionData.kickTimeoutSec, startPetitionData.numVotesNeededToKick);
		}
		else if (tmpPacket->ToNetPacketKickPlayerPetitionUpdate())
		{
			NetPacketKickPlayerPetitionUpdate::Data updatePetitionData;
			tmpPacket->ToNetPacketKickPlayerPetitionUpdate()->GetData(updatePetitionData);
			client.UpdatePetition(updatePetitionData.petitionId, updatePetitionData.numVotesAgainstKicking,
				updatePetitionData.numVotesInFavourOfKicking, updatePetitionData.numVotesNeededToKick);
		}
		else if (tmpPacket->ToNetPacketEndKickPlayerPetition())
		{
			NetPacketEndKickPlayerPetition::Data endPetitionData;
			tmpPacket->ToNetPacketEndKickPlayerPetition()->GetData(endPetitionData);
			client.EndPetition(endPetitionData.petitionId);
		}
		else if (tmpPacket->ToNetPacketAvatarHeader())
		{
			NetPacketAvatarHeader::Data headerData;
			tmpPacket->ToNetPacketAvatarHeader()->GetData(headerData);
			client.AddTempAvatarData(headerData.requestId, headerData.avatarFileSize, headerData.avatarFileType);
		}
		else if (tmpPacket->ToNetPacketAvatarFile())
		{
			NetPacketAvatarFile::Data fileData;
			tmpPacket->ToNetPacketAvatarFile()->GetData(fileData);
			client.StoreInTempAvatarData(fileData.requestId, fileData.fileData);
		}
		else if (tmpPacket->ToNetPacketAvatarEnd())
		{
			NetPacketAvatarEnd::Data endData;
			tmpPacket->ToNetPacketAvatarEnd()->GetData(endData);
			client.CompleteTempAvatarData(endData.requestId);
		}
		else if (tmpPacket->ToNetPacketUnknownAvatar())
		{
			NetPacketUnknownAvatar::Data unknownAvatarData;
			tmpPacket->ToNetPacketUnknownAvatar()->GetData(unknownAvatarData);
			client.SetUnknownAvatar(unknownAvatarData.requestId);
		}
		else if (tmpPacket->ToNetPacketStatisticsChanged())
		{
			NetPacketStatisticsChanged::Data statData;
			tmpPacket->ToNetPacketStatisticsChanged()->GetData(statData);
			client.UpdateStatData(statData.stats);
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
			retVal = InternalProcess(client, tmpPacket);
	}

	return retVal;
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

int
ClientStateWaitSession::InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet->ToNetPacketInitAck())
	{
		// Everything is fine - we are in the lobby.
		NetPacketInitAck::Data initAckData;
		packet->ToNetPacketInitAck()->GetData(initAckData);
		// Check current game version.
		if (initAckData.latestGameVersion != POKERTH_VERSION)
			client.GetCallback().SignalNetClientNotification(NTF_NET_NEW_RELEASE_AVAILABLE);
		else if (POKERTH_BETA_REVISION && initAckData.latestBetaRevision != POKERTH_BETA_REVISION)
			client.GetCallback().SignalNetClientNotification(NTF_NET_OUTDATED_BETA);

		client.SetGuiPlayerId(initAckData.playerId);

		client.SetState(ClientStateWaitJoin::Instance());
		client.SetSessionEstablished(true);
		retVal = MSG_SOCK_SESSION_DONE;
	}
	else if (packet->ToNetPacketRetrieveAvatar())
	{
		// Before letting us join the lobby, the server requests our avatar.
		NetPacketRetrieveAvatar::Data retrieveAvatarData;
		packet->ToNetPacketRetrieveAvatar()->GetData(retrieveAvatarData);

		NetPacketList tmpList;
		int avatarError = client.GetAvatarManager().AvatarFileToNetPackets(
			client.GetQtToolsInterface().stringFromUtf8(client.GetContext().GetAvatarFile()),
			retrieveAvatarData.requestId,
			tmpList);

		if (!avatarError)
			client.GetContext().GetSessionData()->GetSender().Send(client.GetContext().GetSessionData(), tmpList);
		else
			throw ClientException(__FILE__, __LINE__, avatarError, 0);
	}

	return retVal;
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

int
ClientStateWaitJoin::InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;
	ClientContext &context = client.GetContext();

	if (packet->ToNetPacketJoinGameAck())
	{
		// Successfully joined a game.
		NetPacketJoinGameAck::Data joinGameAckData;
		packet->ToNetPacketJoinGameAck()->GetData(joinGameAckData);
		client.SetGameId(joinGameAckData.gameId);
		client.SetGameData(joinGameAckData.gameData);

		// Player number is 0 on init. Will be set when the game starts.
		boost::shared_ptr<PlayerData> playerData(
			new PlayerData(client.GetGuiPlayerId(), 0, PLAYER_TYPE_HUMAN, joinGameAckData.prights));
		playerData->SetName(context.GetPlayerName());
		playerData->SetAvatarFile(context.GetAvatarFile());
		client.AddPlayerData(playerData);

		client.SetState(ClientStateWaitGame::Instance());
		retVal = MSG_NET_GAME_CLIENT_JOIN;
	}
	else if (packet->ToNetPacketJoinGameFailed())
	{
		// Failed to join a game.
		NetPacketJoinGameFailed::Data joinGameFailedData;
		packet->ToNetPacketJoinGameFailed()->GetData(joinGameFailedData);
		client.GetCallback().SignalNetClientNotification(joinGameFailedData.failureCode);
	}

	return retVal;
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

int
ClientStateWaitGame::InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet->ToNetPacketStartEvent())
	{
		client.SetState(ClientStateSynchronizeStart::Instance());
	}
	else if (packet->ToNetPacketPlayerJoined())
	{
		// Another player joined the network game.
		NetPacketPlayerJoined::Data netPlayerData;
		packet->ToNetPacketPlayerJoined()->GetData(netPlayerData);

		boost::shared_ptr<PlayerData> playerData;
		PlayerInfo info;
		if (client.GetCachedPlayerInfo(netPlayerData.playerId, info))
		{
			playerData.reset(
				new PlayerData(netPlayerData.playerId, 0, info.ptype, netPlayerData.prights));
			playerData->SetName(info.playerName);
			if (info.hasAvatar)
			{
				string avatarFile;
				if (client.GetAvatarManager().GetAvatarFileName(info.avatar, avatarFile))
					playerData->SetAvatarFile(client.GetQtToolsInterface().stringToUtf8(avatarFile));
				else
					client.RetrieveAvatarIfNeeded(netPlayerData.playerId, info);
			}
		}
		else
		{
			ostringstream name;
			name << "#" << netPlayerData.playerId;

			// Request player info.
			client.RequestPlayerInfo(netPlayerData.playerId, true);
			// Use temporary data until the PlayerInfo request is completed.
			playerData.reset(
				new PlayerData(netPlayerData.playerId, 0, PLAYER_TYPE_HUMAN, netPlayerData.prights));
			playerData->SetName(name.str());
		}
		client.AddPlayerData(playerData);
	}

	return retVal;
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

int
ClientStateSynchronizeStart::Process(ClientThread &client)
{
	int retVal = AbstractClientStateReceiving::Process(client);

	if (client.IsSynchronized())
	{
		// Acknowledge start.
		boost::shared_ptr<NetPacket> startAck(new NetPacketStartEventAck);
		client.GetContext().GetSessionData()->GetSender().Send(client.GetContext().GetSessionData(), startAck);
		// Unsubscribe lobby messages.
		client.UnsubscribeLobbyMsg();

		client.SetState(ClientStateWaitStart::Instance());
	}

	return retVal;
}

int
ClientStateSynchronizeStart::InternalProcess(ClientThread &/*client*/, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet->ToNetPacketGameStart())
		throw ClientException(__FILE__, __LINE__, ERR_NET_START_TIMEOUT, 0);

	return retVal;
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

int
ClientStateWaitStart::InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet->ToNetPacketGameStart())
	{
		// Start the network game as client.
		NetPacketGameStart::Data gameStartData;
		packet->ToNetPacketGameStart()->GetData(gameStartData);

		client.SetStartData(gameStartData.startData);

		// Set player numbers using the game start data slots.
		NetPacketGameStart::PlayerSlotList::const_iterator slot_i = gameStartData.playerSlots.begin();
		NetPacketGameStart::PlayerSlotList::const_iterator slot_end = gameStartData.playerSlots.end();
		int num = 0;

		while (slot_i != slot_end)
		{
			unsigned playerId = (*slot_i).playerId;
			boost::shared_ptr<PlayerData> tmpPlayer = client.GetPlayerDataByUniqueId(playerId);
			if (!tmpPlayer.get())
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			tmpPlayer->SetNumber(num);

			++num;
			++slot_i;
		}

		client.SetState(ClientStateWaitHand::Instance());
		retVal = MSG_NET_GAME_CLIENT_START;
	}

	return retVal;
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

int
ClientStateWaitHand::InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet->ToNetPacketHandStart())
	{
		// Remove all players which left the server.
		client.RemoveDisconnectedPlayers();
		// Hand was started.
		// These are the cards. Good luck.
		NetPacketHandStart::Data tmpData;
		packet->ToNetPacketHandStart()->GetData(tmpData);
		int myCards[2];
		myCards[0] = (int)tmpData.yourCards[0];
		myCards[1] = (int)tmpData.yourCards[1];
		client.GetGame()->getSeatsList()->front()->setMyCards(myCards);
		client.GetGame()->initHand();
		client.GetGame()->getCurrentHand()->setSmallBlind(tmpData.smallBlind);
		client.GetGame()->getCurrentHand()->getCurrentBeRo()->setMinimumRaise(2 * tmpData.smallBlind);
		client.GetGame()->startHand();
		client.GetGui().dealHoleCards();
		client.GetGui().refreshGameLabels(GAME_STATE_PREFLOP);
		client.GetGui().refreshPot();
		client.GetGui().waitForGuiUpdateDone();

		client.SetState(ClientStateRunHand::Instance());

		retVal = MSG_NET_GAME_CLIENT_HAND_START;
	}
	else if (packet->ToNetPacketEndOfGame())
	{
		boost::shared_ptr<Game> curGame = client.GetGame();
		if (curGame.get())
		{
			NetPacketEndOfGame::Data endData;
			packet->ToNetPacketEndOfGame()->GetData(endData);
			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(endData.winnerPlayerId);
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			client.GetGui().logPlayerWinGame(tmpPlayer->getMyName(), curGame->getMyGameID());
			// Resubscribe Lobby messages.
			client.ResubscribeLobbyMsg();
			// Show Lobby dialog.
			client.GetCallback().SignalNetClientWaitDialog();
			client.SetState(ClientStateWaitGame::Instance());
			retVal = MSG_NET_GAME_CLIENT_END;
		}
	}

	return MSG_SOCK_INTERNAL_PENDING;
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

int
ClientStateRunHand::InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet.get())
	{
		boost::shared_ptr<Game> curGame = client.GetGame();
		if (packet->ToNetPacketPlayersActionDone())
		{
			NetPacketPlayersActionDone::Data actionDoneData;
			packet->ToNetPacketPlayersActionDone()->GetData(actionDoneData);
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
					client.GetGui().logPlayerActionMsg(
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
				client.GetGui().logNewBlindsSetsMsg(curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getSmallBlindPositionId())->getMySet(), curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getBigBlindPositionId())->getMySet(), curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getSmallBlindPositionId())->getMyName(),  curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getBigBlindPositionId())->getMyName());
				client.GetGui().flushLogAtHand();		
			}

			// Stop the timeout for the player.
			client.GetGui().stopTimeoutAnimation(tmpPlayer->getMyID());

			// Unmark last player in GUI.
			client.GetGui().refreshGroupbox(tmpPlayer->getMyID(), 3);

			// Refresh GUI
			if (tmpPlayer->getMyID() == 0)
				client.GetGui().disableMyButtons();
			client.GetGui().refreshAction(tmpPlayer->getMyID(), tmpPlayer->getMyAction());
			client.GetGui().refreshPot();
			client.GetGui().refreshSet();
			client.GetGui().refreshCash();
			client.GetGui().refreshButton();
			client.GetGui().updateMyButtonsState();
		}
		else if (packet->ToNetPacketPlayersTurn())
		{
			NetPacketPlayersTurn::Data turnData;
			packet->ToNetPacketPlayersTurn()->GetData(turnData);
			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(turnData.playerId);
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

			// Set round.
			if (curGame->getCurrentHand()->getCurrentRound() != turnData.gameState)
			{
				ResetPlayerActions(*curGame);
				curGame->getCurrentHand()->setCurrentRound(turnData.gameState);
				// Refresh actions.
				client.GetGui().refreshSet();
				client.GetGui().refreshAction();
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
			client.GetGui().refreshGroupbox(tmpPlayer->getMyID(), guiStatus);
			client.GetGui().refreshAction(tmpPlayer->getMyID(), PLAYER_ACTION_NONE);

			// Start displaying the timeout for the player.
			client.GetGui().startTimeoutAnimation(tmpPlayer->getMyID(), client.GetGameData().playerActionTimeoutSec);

			if (tmpPlayer->getMyID() == 0) // Is this the GUI player?
				client.GetGui().meInAction();
		}
		else if (packet->ToNetPacketDealFlopCards())
		{
			NetPacketDealFlopCards::Data cardsData;
			packet->ToNetPacketDealFlopCards()->GetData(cardsData);
			int tmpCards[5];
			for (int num = 0; num < 3; num++)
				tmpCards[num] = static_cast<int>(cardsData.flopCards[num]);
			tmpCards[3] = tmpCards[4] = 0;
			curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
			curGame->getCurrentHand()->getBoard()->collectPot();
			curGame->getCurrentHand()->setLastPlayersTurn(-1);

			client.GetGui().logDealBoardCardsMsg(GAME_STATE_FLOP, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
			client.GetGui().refreshGameLabels(GAME_STATE_FLOP);
			client.GetGui().refreshPot();
			client.GetGui().refreshSet();
			client.GetGui().dealBeRoCards(1);
		}
		else if (packet->ToNetPacketDealTurnCard())
		{
			NetPacketDealTurnCard::Data cardsData;
			packet->ToNetPacketDealTurnCard()->GetData(cardsData);
			int tmpCards[5];
			curGame->getCurrentHand()->getBoard()->getMyCards(tmpCards);
			tmpCards[3] = static_cast<int>(cardsData.turnCard);
			curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
			curGame->getCurrentHand()->getBoard()->collectPot();
			curGame->getCurrentHand()->setLastPlayersTurn(-1);

			client.GetGui().logDealBoardCardsMsg(GAME_STATE_TURN, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
			client.GetGui().refreshGameLabels(GAME_STATE_TURN);
			client.GetGui().refreshPot();
			client.GetGui().refreshSet();
			client.GetGui().dealBeRoCards(2);
		}
		else if (packet->ToNetPacketDealRiverCard())
		{
			NetPacketDealRiverCard::Data cardsData;
			packet->ToNetPacketDealRiverCard()->GetData(cardsData);
			int tmpCards[5];
			curGame->getCurrentHand()->getBoard()->getMyCards(tmpCards);
			tmpCards[4] = static_cast<int>(cardsData.riverCard);
			curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
			curGame->getCurrentHand()->getBoard()->collectPot();
			curGame->getCurrentHand()->setLastPlayersTurn(-1);

			client.GetGui().logDealBoardCardsMsg(GAME_STATE_RIVER, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
			client.GetGui().refreshGameLabels(GAME_STATE_RIVER);
			client.GetGui().refreshPot();
			client.GetGui().refreshSet();
			client.GetGui().dealBeRoCards(3);
		}
		else if (packet->ToNetPacketAllInShowCards())
		{
			curGame->getCurrentHand()->setAllInCondition(true);

			NetPacketAllInShowCards::Data allInData;
			packet->ToNetPacketAllInShowCards()->GetData(allInData);

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
			client.GetGui().flipHolecardsAllIn();
		}
		else if (packet->ToNetPacketEndOfHandHideCards())
		{
			curGame->getCurrentHand()->getBoard()->collectPot();
			// Reset player sets
			ResetPlayerSets(*curGame);
			client.GetGui().refreshPot();
			client.GetGui().refreshSet();
			// Synchronize with GUI.
			client.GetGui().waitForGuiUpdateDone();

			// End of Hand, but keep cards hidden.
			NetPacketEndOfHandHideCards::Data endHandData;
			packet->ToNetPacketEndOfHandHideCards()->GetData(endHandData);

			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(endHandData.playerId);
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

			tmpPlayer->setMyCash(endHandData.playerMoney);
			tmpPlayer->setLastMoneyWon(endHandData.moneyWon);
			list<unsigned> winnerList;
			winnerList.push_back(tmpPlayer->getMyUniqueID());

			curGame->getCurrentHand()->getBoard()->setPot(0);
			curGame->getCurrentHand()->getBoard()->setWinners(winnerList);

			client.GetGui().postRiverRunAnimation1();

			// Wait for next Hand.
			client.SetState(ClientStateWaitHand::Instance());
			retVal = MSG_NET_GAME_SERVER_HAND_END;
		}
		else if (packet->ToNetPacketEndOfHandShowCards())
		{
			curGame->getCurrentHand()->getBoard()->collectPot();
			// Reset player sets
			ResetPlayerSets(*curGame);
			client.GetGui().refreshPot();
			client.GetGui().refreshSet();
			// Synchronize with GUI.
			client.GetGui().waitForGuiUpdateDone();

			// End of Hand, show cards.
			NetPacketEndOfHandShowCards::Data endHandData;
			packet->ToNetPacketEndOfHandShowCards()->GetData(endHandData);

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

			client.GetGui().postRiverRunAnimation1();

			// Wait for next Hand.
			client.SetState(ClientStateWaitHand::Instance());
			retVal = MSG_NET_GAME_CLIENT_HAND_END;
		}
	}

	// Synchronize with GUI.
	client.GetGui().waitForGuiUpdateDone();

	return retVal;
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

ClientStateFinal::ClientStateFinal()
{
}

ClientStateFinal::~ClientStateFinal()
{
}

int
ClientStateFinal::Process(ClientThread &/*client*/)
{
	Thread::Msleep(20);

	return MSG_SOCK_INTERNAL_PENDING;
}
