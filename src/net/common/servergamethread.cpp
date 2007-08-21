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

#include <net/servergamethread.h>
#include <net/servergamestate.h>
#include <net/serverlobbythread.h>
#include <net/serverexception.h>
#include <net/senderthread.h>
#include <net/sendercallback.h>
#include <net/receiverhelper.h>
#include <net/socket_msg.h>
#include <game.h>
#include <localenginefactory.h>
#include <tools.h>

using namespace std;


class ServerSenderCallback : public SenderCallback
{
public:
	ServerSenderCallback(ServerGameThread &server) : m_server(server) {}
	virtual ~ServerSenderCallback() {}

	virtual void SignalNetError(SOCKET sock, int errorID, int osErrorID)
	{
		// We just ignore send errors for now, on server side.
		// A serious send error should trigger a read error or a read
		// returning 0 afterwards, and we will handle this error.
	}

private:
	ServerGameThread &m_server;
};


ServerGameThread::ServerGameThread(ServerLobbyThread &lobbyThread, u_int32_t id, const string &name, GuiInterface &gui, ConfigFile *playerConfig)
: m_lobbyThread(lobbyThread), m_gui(gui), m_id(id), m_name(name), m_playerConfig(playerConfig), m_curState(NULL), m_gameNum(1)
{
	m_senderCallback.reset(new ServerSenderCallback(*this));
	m_sender.reset(new SenderThread(GetSenderCallback()));
	m_receiver.reset(new ReceiverHelper);
}

ServerGameThread::~ServerGameThread()
{
}

void
ServerGameThread::Init(const string &pwd, const GameData &gameData)
{
	m_password = pwd;
	m_gameData = gameData;
}

u_int32_t
ServerGameThread::GetId() const
{
	return m_id;
}

const std::string &
ServerGameThread::GetName() const
{
	return m_name;
}

void
ServerGameThread::AddSession(SessionWrapper session)
{
	// Must be thread safe.
	boost::mutex::scoped_lock lock(m_sessionQueueMutex);
	m_sessionQueue.push_back(session);
}

GameState
ServerGameThread::GetCurRound() const
{
	return static_cast<GameState>(GetGame().getCurrentHand()->getActualRound());
}

void
ServerGameThread::SendToAllPlayers(boost::shared_ptr<NetPacket> packet, SessionData::State state)
{
	GetSessionManager().SendToAllSessions(GetSender(), packet, state);
}

void
ServerGameThread::Main()
{
	SetState(SERVER_INITIAL_STATE::Instance());
	GetSender().Run();

	try
	{
		do
		{
			{
				// Handle one new session at a time.
				SessionWrapper tmpSession;
				{
					boost::mutex::scoped_lock lock(m_sessionQueueMutex);
					if (!m_sessionQueue.empty())
					{
						tmpSession = m_sessionQueue.front();
						m_sessionQueue.pop_front();
					}
				}
				if (tmpSession.sessionData.get())
					GetState().HandleNewSession(*this, tmpSession);
			}
			// Process current state.
			GetState().Process(*this);
		} while (!ShouldTerminate() && GetSessionManager().HasSessions());
	} catch (const NetException &e)
	{
		GetCallback().SignalNetServerError(e.GetErrorId(), e.GetOsErrorCode());
	}
	GetSender().SignalTermination();
	GetSender().Join(SENDER_THREAD_TERMINATE_TIMEOUT);

	GetLobbyThread().RemoveGame(GetId());
}

void
ServerGameThread::InternalStartGame()
{
	// Set order of players.
	AssignPlayerNumbers();

	// Initialize the game.
	GuiInterface &gui = GetGui();
	PlayerDataList playerData = GetSessionManager().GetPlayerDataList();

	// Create EngineFactory
	boost::shared_ptr<EngineFactory> factory(new LocalEngineFactory(m_playerConfig)); // LocalEngine erstellen

	// Set start data.
	StartData startData;
	startData.numberOfPlayers = playerData.size();

	int tmpDealerPos = 0;
	Tools::getRandNumber(0, startData.numberOfPlayers-1, 1, &tmpDealerPos, 0);
	// The Player Id is not continuous. Therefore, the start dealer position
	// needs to be converted to a player Id, and cannot be directly generated
	// as player Id.
	PlayerDataList::const_iterator player_i = playerData.begin();
	PlayerDataList::const_iterator player_end = playerData.end();

	bool randDealerFound = false;
	while (player_i != player_end)
	{
		if ((*player_i)->GetNumber() == tmpDealerPos)
		{
			// Get ID of the dealer.
			startData.startDealerPlayerId = static_cast<unsigned>((*player_i)->GetUniqueId());
			randDealerFound = true;
			break;
		}
		++player_i;
	}
	assert(randDealerFound); // TODO: Throw exception.

	SetStartData(startData);

	m_game.reset(new Game(&gui, factory, playerData, GetGameData(), GetStartData(), GetNextGameNum()));
}

void
ServerGameThread::InternalKickPlayer(unsigned playerId)
{
// TODO
//	SessionWrapper tmpSession = GetSessionByUniquePlayerId(uniqueId);
//	SessionError(tmpSession, ERR_NET_PLAYER_KICKED);
}

void
ServerGameThread::AddComputerPlayer(boost::shared_ptr<PlayerData> player)
{
	// TODO
	m_computerPlayers.push_back(player);
}

void
ServerGameThread::ResetComputerPlayerList()
{
	m_computerPlayers.clear();
}

void
ServerGameThread::SessionError(SessionWrapper session, int errorCode)
{
	if (session.sessionData.get())
	{
		SendError(session.sessionData->GetSocket(), errorCode);
		CloseSessionDelayed(session);
	}
}

void
ServerGameThread::CloseSessionDelayed(SessionWrapper session)
{
	GetSessionManager().RemoveSession(session.sessionData->GetSocket());

	boost::shared_ptr<PlayerData> tmpPlayerData = session.playerData;
	if (tmpPlayerData.get() && !tmpPlayerData->GetName().empty())
	{
		// Send "Player Left" to clients.
		boost::shared_ptr<NetPacket> thisPlayerLeft(new NetPacketPlayerLeft);
		NetPacketPlayerLeft::Data thisPlayerLeftData;
		thisPlayerLeftData.playerId = tmpPlayerData->GetUniqueId();
		static_cast<NetPacketPlayerLeft *>(thisPlayerLeft.get())->SetData(thisPlayerLeftData);
		GetSessionManager().SendToAllSessions(GetSender(), thisPlayerLeft, SessionData::Game);

		GetCallback().SignalNetServerPlayerLeft(tmpPlayerData->GetName());
	}

	GetLobbyThread().CloseSessionDelayed(session);
}

void
ServerGameThread::SendError(SOCKET s, int errorCode)
{
	boost::shared_ptr<NetPacket> packet(new NetPacketError);
	NetPacketError::Data errorData;
	errorData.errorCode = errorCode;
	static_cast<NetPacketError *>(packet.get())->SetData(errorData);
	GetSender().Send(s, packet);
}

void
ServerGameThread::RejectSession(SessionWrapper session)
{
	// TODO
}

void
ServerGameThread::RemoveDisconnectedPlayers()
{
	// This should only be called between hands.
	if (m_game.get())
	{
		for (int i = 0; i < m_game->getStartQuantityPlayers(); i++)
		{
			boost::shared_ptr<PlayerInterface> tmpPlayer = m_game->getPlayerArray()[i];
			if (!GetSessionManager().IsPlayerConnected(tmpPlayer->getMyUniqueID()) && tmpPlayer->getMyType() == PLAYER_TYPE_HUMAN)
			{
				tmpPlayer->setMyCash(0);
				tmpPlayer->setMyActiveStatus(false);
				tmpPlayer->setNetSessionData(boost::shared_ptr<SessionData>());
			}
		}
	}
}

size_t
ServerGameThread::GetCurNumberOfPlayers() const
{
	return GetSessionManager().GetPlayerDataList().size();
}

void
ServerGameThread::AssignPlayerNumbers()
{
	int playerNumber = 0;

	PlayerDataList playerList = GetSessionManager().GetPlayerDataList();
	PlayerDataList::iterator player_i = playerList.begin();
	PlayerDataList::iterator player_end = playerList.end();

	while (player_i != player_end)
	{
		(*player_i)->SetNumber(playerNumber);
		++playerNumber;
		++player_i;
	}
}

SessionManager &
ServerGameThread::GetSessionManager()
{
	return m_sessionManager;
}

const SessionManager &
ServerGameThread::GetSessionManager() const
{
	return m_sessionManager;
}

ServerLobbyThread &
ServerGameThread::GetLobbyThread()
{
	return m_lobbyThread;
}

ServerCallback &
ServerGameThread::GetCallback()
{
	return m_gui;
}

ServerGameState &
ServerGameThread::GetState()
{
	assert(m_curState);
	return *m_curState;
}

void
ServerGameThread::SetState(ServerGameState &newState)
{
	newState.Init();
	m_curState = &newState;
}

SenderThread &
ServerGameThread::GetSender()
{
	assert(m_sender.get());
	return *m_sender;
}

ReceiverHelper &
ServerGameThread::GetReceiver()
{
	assert(m_receiver.get());
	return *m_receiver;
}

Game &
ServerGameThread::GetGame()
{
	assert(m_game.get());
	return *m_game;
}

const Game &
ServerGameThread::GetGame() const
{
	assert(m_game.get());
	return *m_game;
}

const GameData &
ServerGameThread::GetGameData() const
{
	return m_gameData;
}

const StartData &
ServerGameThread::GetStartData() const
{
	return m_startData;
}

void
ServerGameThread::SetStartData(const StartData &startData)
{
	m_startData = startData;
}

bool
ServerGameThread::CheckPassword(const string &password) const
{
	return (password == m_password);
}

ServerSenderCallback &
ServerGameThread::GetSenderCallback()
{
	assert(m_senderCallback.get());
	return *m_senderCallback;
}

GuiInterface &
ServerGameThread::GetGui()
{
	return m_gui;
}

unsigned
ServerGameThread::GetNextGameNum()
{
	return m_gameNum++;
}

