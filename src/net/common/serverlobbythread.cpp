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

#include <net/serverlobbythread.h>
#include <net/serverexception.h>
#include <net/senderthread.h>
#include <net/sendercallback.h>
#include <net/receiverhelper.h>
#include <net/socket_msg.h>
#include <core/rand.h>

#include <boost/lambda/lambda.hpp>

#define SERVER_CLOSE_SESSION_DELAY_SEC	10
#define SERVER_MAX_NUM_SESSIONS			64 // Maximum number of idle users in lobby.

#define SERVER_COMPUTER_PLAYER_NAME				"Computer"

using namespace std;


class ServerSenderCallback : public SenderCallback
{
public:
	ServerSenderCallback(ServerLobbyThread &server) : m_server(server) {}
	virtual ~ServerSenderCallback() {}

	virtual void SignalNetError(SOCKET sock, int errorID, int osErrorID)
	{
		// We just ignore send errors for now, on server side.
		// A serious send error should trigger a read error or a read
		// returning 0 afterwards, and we will handle this error.
	}

private:
	ServerLobbyThread &m_server;
};


ServerLobbyThread::ServerLobbyThread(GuiInterface &gui, ConfigFile *playerConfig)
: m_gui(gui), m_playerConfig(playerConfig)
{
	m_senderCallback.reset(new ServerSenderCallback(*this));
	m_sender.reset(new SenderThread(GetSenderCallback()));
	m_receiver.reset(new ReceiverHelper);
}

ServerLobbyThread::~ServerLobbyThread()
{
	CleanupConnectQueue();
}

void
ServerLobbyThread::Init(const string &pwd)
{
	m_password = pwd;
}

void
ServerLobbyThread::AddConnection(boost::shared_ptr<ConnectData> data)
{
	boost::mutex::scoped_lock lock(m_connectQueueMutex);
	m_connectQueue.push_back(data);
}

u_int32_t
ServerLobbyThread::GetNextUniquePlayerId()
{
	return m_curUniquePlayerId++;
}

void
ServerLobbyThread::Main()
{
	GetSender().Run();

	try
	{
		while (!ShouldTerminate())
		{
			{
				// Handle one incoming connection at a time.
				boost::shared_ptr<ConnectData> tmpData;
				{
					boost::mutex::scoped_lock lock(m_connectQueueMutex);
					if (!m_connectQueue.empty())
					{
						tmpData = m_connectQueue.front();
						m_connectQueue.pop_front();
					}
				}
				if (tmpData.get())
					HandleNewConnection(tmpData);
			}
			// Process loop.
			ProcessLoop();
			// Close sessions.
			CloseSessionLoop();
		}
	} catch (const NetException &e)
	{
		GetCallback().SignalNetServerError(e.GetErrorId(), e.GetOsErrorCode());
	}
	GetSender().SignalTermination();
	GetSender().Join(SENDER_THREAD_TERMINATE_TIMEOUT);

	CleanupConnectQueue();
	m_sessionManager.Clear();
}

void
ServerLobbyThread::ProcessLoop()
{
	// Wait for data.
	SessionWrapper session = m_sessionManager.Select(RECV_TIMEOUT_MSEC);

	if (session.sessionData.get())
	{
		boost::shared_ptr<NetPacket> packet;
		try
		{
			// Receive the next packet.
			packet = GetReceiver().Recv(session.sessionData->GetSocket());
		} catch (const NetException &)
		{
			// On error: Close this session.
			CloseSessionDelayed(session);
			return;
		}
		if (packet.get())
		{
			if (packet->ToNetPacketInit())
			{
				// Session should be in initial state.
				if (session.sessionData->GetState() != SessionData::Init)
					SessionError(session, ERR_SOCK_INVALID_STATE);
				else
					HandleNetPacketInit(session, *packet->ToNetPacketInit());
			}
			// Session should be established.
			else if (session.sessionData->GetState() != SessionData::Established)
				SessionError(session, ERR_SOCK_INVALID_STATE);
			else
			{
				if (packet->ToNetPacketCreateGame())
					HandleNetPacketCreateGame(session, *packet->ToNetPacketCreateGame());
				else if (packet->ToNetPacketJoinGame())
					HandleNetPacketJoinGame(session, *packet->ToNetPacketJoinGame());
			}
		}
	}
}

void
ServerLobbyThread::HandleNetPacketInit(SessionWrapper session, const NetPacketInit &tmpPacket)
{
	NetPacketInit::Data initData;
	tmpPacket.GetData(initData);

	// Check the protocol version.
	if (initData.versionMajor != NET_VERSION_MAJOR)
	{
		SessionError(session, ERR_NET_VERSION_NOT_SUPPORTED);
		return;
	}

	// Check the server password.
	if (!CheckPassword(initData.password))
	{
		SessionError(session, ERR_NET_INVALID_PASSWORD);
		return;
	}

	// Check whether the player name is correct.
	// Partly, this is also done in netpacket.
	// However, some disallowed names are checked only here.
	if (initData.playerName.empty() || initData.playerName.size() > MAX_NAME_SIZE
		|| initData.playerName.substr(0, sizeof(SERVER_COMPUTER_PLAYER_NAME) - 1) == SERVER_COMPUTER_PLAYER_NAME)
	{
		SessionError(session, ERR_NET_INVALID_PLAYER_NAME);
		return;
	}

	// Check whether this player is already connected.
	if (IsPlayerConnected(initData.playerName))
	{
		SessionError(session, ERR_NET_PLAYER_NAME_IN_USE);
		return;
	}

	// Create player data object.
	boost::shared_ptr<PlayerData> tmpPlayerData(
		new PlayerData(GetNextUniquePlayerId(), 0, PLAYER_TYPE_HUMAN, PLAYER_RIGHTS_NORMAL));
	tmpPlayerData->SetName(initData.playerName);
	tmpPlayerData->SetNetSessionData(session.sessionData);

	// Send ACK to client.
	boost::shared_ptr<NetPacket> initAck(new NetPacketInitAck);
	NetPacketInitAck::Data initAckData;
	initAckData.sessionId = session.sessionData->GetId(); // TODO: currently unused.
	initAckData.playerId = tmpPlayerData->GetUniqueId();
	static_cast<NetPacketInitAck *>(initAck.get())->SetData(initAckData);
	GetSender().Send(session.sessionData->GetSocket(), initAck);

	// Send the game list to the client.
	/*GameThreadList::iterator game_i = m_gameList.begin();
	GameThreadList::iterator game_end = m_gameList.end();
	while (game_i != game_end)
	{
		GetSender().Send(session.sessionData->GetSocket(), CreateNetPacketGameListUpdate(*(*game_i)));
		++game_i;
	}*/

	// Set player data for session.
	m_sessionManager.SetSessionPlayerData(session.sessionData->GetSocket(), tmpPlayerData);

	// Session is now established.
	session.sessionData->SetState(SessionData::Established);
}

void
ServerLobbyThread::HandleNetPacketCreateGame(SessionWrapper session, const NetPacketCreateGame &tmpPacket)
{
}

void
ServerLobbyThread::HandleNetPacketJoinGame(SessionWrapper session, const NetPacketJoinGame &tmpPacket)
{
}

void
ServerLobbyThread::CloseSessionLoop()
{
	CloseSessionList::iterator i = m_closeSessionList.begin();
	CloseSessionList::iterator end = m_closeSessionList.end();

	while (i != end)
	{
		CloseSessionList::iterator cur = i++;

		if (cur->first.elapsed().total_seconds() >= SERVER_CLOSE_SESSION_DELAY_SEC)
			m_closeSessionList.erase(cur);
	}
}

void
ServerLobbyThread::HandleNewConnection(boost::shared_ptr<ConnectData> connData)
{
	if (m_sessionManager.GetRawSessionCount() <= SERVER_MAX_NUM_SESSIONS)
	{
		// Create a random session id.
		// This id can be used to reconnect to the server if the connection was lost.
		unsigned sessionId;
		RandomBytes((unsigned char *)&sessionId, sizeof(sessionId)); // TODO: check for collisions.

		// Create a new session.
		boost::shared_ptr<SessionData> sessionData(new SessionData(connData->ReleaseSocket(), sessionId));
		m_sessionManager.AddSession(sessionData);
	}
	else
	{
		// Server is full.
		// Create a generic session with Id 0.
		boost::shared_ptr<SessionData> sessionData(new SessionData(connData->ReleaseSocket(), 0));
		// Gracefully close this session.
		SessionError(SessionWrapper(sessionData, boost::shared_ptr<PlayerData>()), ERR_NET_SERVER_FULL);
	}
}

void
ServerLobbyThread::CleanupConnectQueue()
{
	boost::mutex::scoped_lock lock(m_connectQueueMutex);

	// Sockets will be closed automatically.
	m_connectQueue.clear();
}

void
ServerLobbyThread::SessionError(SessionWrapper session, int errorCode)
{
	if (session.sessionData.get())
	{
		SendError(session.sessionData->GetSocket(), errorCode);
		CloseSessionDelayed(session);
	}
}

void
ServerLobbyThread::CloseSessionDelayed(SessionWrapper session)
{
	m_sessionManager.RemoveSession(session.sessionData->GetSocket());

	boost::shared_ptr<PlayerData> tmpPlayerData = session.playerData;
	if (tmpPlayerData.get() && !tmpPlayerData->GetName().empty())
	{
		// Send "Player Left" to clients.
		boost::shared_ptr<NetPacket> thisPlayerLeft(new NetPacketPlayerLeft);
		NetPacketPlayerLeft::Data thisPlayerLeftData;
		thisPlayerLeftData.playerId = tmpPlayerData->GetUniqueId();
		static_cast<NetPacketPlayerLeft *>(thisPlayerLeft.get())->SetData(thisPlayerLeftData);
		m_sessionManager.SendToAllSessions(GetSender(), thisPlayerLeft);

		GetCallback().SignalNetServerPlayerLeft(tmpPlayerData->GetName());
	}

	boost::microsec_timer closeTimer;
	closeTimer.start();
	CloseSessionList::value_type closeSessionData(closeTimer, session.sessionData);
	m_closeSessionList.push_back(closeSessionData);
}

void
ServerLobbyThread::SendError(SOCKET s, int errorCode)
{
	boost::shared_ptr<NetPacket> packet(new NetPacketError);
	NetPacketError::Data errorData;
	errorData.errorCode = errorCode;
	static_cast<NetPacketError *>(packet.get())->SetData(errorData);
	GetSender().Send(s, packet);
}

bool
ServerLobbyThread::IsPlayerConnected(const string &playerName) const
{
	bool retVal = false;

	SessionWrapper tmpSession = m_sessionManager.GetSessionByPlayerName(playerName);

	if (tmpSession.sessionData.get() && tmpSession.playerData.get())
		retVal = true;

	return retVal;
}

ServerCallback &
ServerLobbyThread::GetCallback()
{
	return m_gui;
}

SenderThread &
ServerLobbyThread::GetSender()
{
	assert(m_sender.get());
	return *m_sender;
}

ReceiverHelper &
ServerLobbyThread::GetReceiver()
{
	assert(m_receiver.get());
	return *m_receiver;
}

bool
ServerLobbyThread::CheckPassword(const string &password) const
{
	return (password == m_password);
}

ServerSenderCallback &
ServerLobbyThread::GetSenderCallback()
{
	assert(m_senderCallback.get());
	return *m_senderCallback;
}

GuiInterface &
ServerLobbyThread::GetGui()
{
	return m_gui;
}

