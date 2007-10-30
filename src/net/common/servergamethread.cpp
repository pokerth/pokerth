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
#include <core/loghelper.h>
#include <game.h>
#include <localenginefactory.h>
#include <tools.h>

#include <boost/bind.hpp>

using namespace std;


class GameSenderCallback : public SenderCallback
{
public:
	GameSenderCallback(ServerGameThread &server) : m_server(server) {}
	virtual ~GameSenderCallback() {}

	virtual void SignalNetError(SessionId /*session*/, int /*errorID*/, int /*osErrorID*/)
	{
		// We just ignore send errors for now, on server side.
		// A serious send error should trigger a read error or a read
		// returning 0 afterwards, and we will handle this error.
	}

private:
	ServerGameThread &m_server;
};


ServerGameThread::ServerGameThread(ServerLobbyThread &lobbyThread, u_int32_t id, const string &name, const string &pwd, const GameData &gameData, unsigned adminPlayerId, GuiInterface &gui, ConfigFile *playerConfig)
: m_adminPlayerId(adminPlayerId), m_lobbyThread(lobbyThread), m_gui(gui),
  m_gameData(gameData), m_id(id), m_name(name), m_password(pwd), m_playerConfig(playerConfig),
  m_curState(NULL), m_gameNum(1)
{
	m_senderCallback.reset(new GameSenderCallback(*this));
	m_sender.reset(new SenderThread(GetSenderCallback()));
	m_receiver.reset(new ReceiverHelper);
}

ServerGameThread::~ServerGameThread()
{
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
	return static_cast<GameState>(GetGame().getCurrentHand()->getCurrentRound());
}

void
ServerGameThread::SendToAllPlayers(boost::shared_ptr<NetPacket> packet, SessionData::State state)
{
	GetSessionManager().SendToAllSessions(GetSender(), packet, state);
}

void
ServerGameThread::RemoveAllSessions()
{
	// Called from lobby thread.
	// Clean up ALL sessions which are left.
	ServerLobbyThread &lobbyThread = GetLobbyThread();
	boost::mutex::scoped_lock lock(m_sessionQueueMutex);
	while (!m_sessionQueue.empty())
	{
		SessionWrapper tmpSession = m_sessionQueue.front();
		m_sessionQueue.pop_front();
		lobbyThread.RemoveSessionFromGame(tmpSession);
	}
	GetSessionManager().ForEach(boost::bind(&ServerLobbyThread::RemoveSessionFromGame, boost::ref(lobbyThread), _1));
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
	} catch (const PokerTHException &e)
	{
		GetCallback().SignalNetServerError(e.GetErrorId(), e.GetOsErrorCode());
		LOG_ERROR(e.what());
	}
	GetSender().SignalTermination();
	if (!GetSender().Join(SENDER_THREAD_TERMINATE_TIMEOUT))
		LOG_ERROR("Fatal error: Unable to terminated Sender Thread in Game.");

	ResetComputerPlayerList();
	GetLobbyThread().RemoveGame(GetId());
}

void
ServerGameThread::InternalStartGame()
{
	// Set order of players.
	AssignPlayerNumbers();

	// Initialize the game.
	GuiInterface &gui = GetGui();
	PlayerDataList playerData = GetFullPlayerDataList();

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

	int tmpPos = 0;
	while (player_i != player_end)
	{
		startData.startDealerPlayerId = static_cast<unsigned>((*player_i)->GetUniqueId());
		if (tmpPos == tmpDealerPos)
			break;
		++tmpPos;
		++player_i;
	}
	assert(player_i != player_end);

	SetStartData(startData);

	m_game.reset(new Game(&gui, factory, playerData, GetGameData(), GetStartData(), GetNextGameNum()));

	GetLobbyThread().NotifyStartingGame(GetId());
}

void
ServerGameThread::ResetGame()
{
	m_game.reset();
}

void
ServerGameThread::InternalKickPlayer(unsigned playerId)
{
	SessionWrapper tmpSession = GetSessionManager().GetSessionByUniquePlayerId(playerId);
	MoveSessionToLobby(tmpSession, NTF_NET_REMOVED_KICKED);
}

PlayerDataList
ServerGameThread::GetFullPlayerDataList() const
{
	PlayerDataList playerList(GetSessionManager().GetPlayerDataList());
	boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
	copy(m_computerPlayerList.begin(), m_computerPlayerList.end(), back_inserter(playerList));

	return playerList;
}

boost::shared_ptr<PlayerData>
ServerGameThread::GetPlayerDataByUniqueId(unsigned playerId) const
{
	boost::shared_ptr<PlayerData> tmpPlayer;
	SessionWrapper session = GetSessionManager().GetSessionByUniquePlayerId(playerId);
	if (session.playerData.get())
	{
		tmpPlayer = session.playerData;
	}
	else
	{
		boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
		PlayerDataList::const_iterator i = m_computerPlayerList.begin();
		PlayerDataList::const_iterator end = m_computerPlayerList.end();
		while (i != end)
		{
			if ((*i)->GetUniqueId() == playerId)
			{
				tmpPlayer = *i;
				break;
			}
			++i;
		}
	}
	return tmpPlayer;
}

PlayerIdList
ServerGameThread::GetPlayerIdList() const
{
	PlayerIdList idList(GetSessionManager().GetPlayerIdList());
	boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
	PlayerDataList::const_iterator i = m_computerPlayerList.begin();
	PlayerDataList::const_iterator end = m_computerPlayerList.end();
	while (i != end)
	{
		idList.push_back((*i)->GetUniqueId());
		++i;
	}

	return idList;
}

bool
ServerGameThread::IsPlayerConnected(const std::string &name) const
{
	return GetSessionManager().IsPlayerConnected(name);
}

bool
ServerGameThread::IsRunning() const
{
	return m_game.get() != NULL;
}

unsigned
ServerGameThread::GetAdminPlayerId() const
{
	boost::mutex::scoped_lock lock(m_adminPlayerIdMutex);
	return m_adminPlayerId;
}

void
ServerGameThread::SetAdminPlayerId(unsigned playerId)
{
	boost::mutex::scoped_lock lock(m_adminPlayerIdMutex);
	m_adminPlayerId = playerId;
}

void
ServerGameThread::AddComputerPlayer(boost::shared_ptr<PlayerData> player)
{
	{
		boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
		m_computerPlayerList.push_back(player);
	}
	GetLobbyThread().AddComputerPlayer(player);
}

void
ServerGameThread::ResetComputerPlayerList()
{
	boost::mutex::scoped_lock lock(m_computerPlayerListMutex);

	PlayerDataList::iterator i = m_computerPlayerList.begin();
	PlayerDataList::iterator end = m_computerPlayerList.end();

	while (i != end)
	{
		GetLobbyThread().RemoveComputerPlayer(*i);
		RemovePlayerData(*i);
		++i;
	}

	m_computerPlayerList.clear();
}

void
ServerGameThread::GracefulRemoveSession(SessionWrapper session)
{
	assert(session.sessionData.get());
	GetSessionManager().RemoveSession(session.sessionData->GetId());

	boost::shared_ptr<PlayerData> tmpPlayerData = session.playerData;
	if (tmpPlayerData.get() && !tmpPlayerData->GetName().empty())
	{
		RemovePlayerData(tmpPlayerData);
	}
}

void
ServerGameThread::RemovePlayerData(boost::shared_ptr<PlayerData> player)
{
	if (player->GetRights() == PLAYER_RIGHTS_ADMIN)
	{
		// Find new admin for the game
		PlayerDataList playerList(GetSessionManager().GetPlayerDataList());
		if (!playerList.empty())
		{
			boost::shared_ptr<PlayerData> newAdmin = playerList.front();
			SetAdminPlayerId(newAdmin->GetUniqueId());
			newAdmin->SetRights(PLAYER_RIGHTS_ADMIN);
			// Send "Game Admin Changed" to clients.
			boost::shared_ptr<NetPacket> adminChanged(new NetPacketGameAdminChanged);
			NetPacketGameAdminChanged::Data adminChangedData;
			adminChangedData.playerId = newAdmin->GetUniqueId(); // Choose next player as admin.
			static_cast<NetPacketGameAdminChanged *>(adminChanged.get())->SetData(adminChangedData);
			GetSessionManager().SendToAllSessions(GetSender(), adminChanged, SessionData::Game);

			GetLobbyThread().NotifyGameAdminChanged(GetId(), newAdmin->GetUniqueId());
		}
	}
	// Reset player rights.
	player->SetRights(PLAYER_RIGHTS_NORMAL);

	// Send "Player Left" to clients.
	boost::shared_ptr<NetPacket> thisPlayerLeft(new NetPacketPlayerLeft);
	NetPacketPlayerLeft::Data thisPlayerLeftData;
	thisPlayerLeftData.playerId = player->GetUniqueId();
	static_cast<NetPacketPlayerLeft *>(thisPlayerLeft.get())->SetData(thisPlayerLeftData);
	GetSessionManager().SendToAllSessions(GetSender(), thisPlayerLeft, SessionData::Game);

	GetLobbyThread().NotifyPlayerLeftGame(GetId(), player->GetUniqueId());
}

void
ServerGameThread::ErrorRemoveSession(SessionWrapper session)
{
	GetLobbyThread().RemoveSessionFromGame(session);
	GracefulRemoveSession(session);
}

void
ServerGameThread::SessionError(SessionWrapper session, int errorCode)
{
	assert(session.sessionData.get());
	ErrorRemoveSession(session);
	GetLobbyThread().SessionError(session, errorCode);
}

void
ServerGameThread::MoveSessionToLobby(SessionWrapper session, int reason)
{
	GracefulRemoveSession(session);
	GetLobbyThread().ReAddSession(session, reason);
}

void
ServerGameThread::RemoveDisconnectedPlayers()
{
	// This should only be called between hands.
	if (m_game.get())
	{
		PlayerListIterator i = m_game->getSeatsList()->begin();
		PlayerListIterator end = m_game->getSeatsList()->end();
		while (i != end)
		{
			boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
			if (!GetSessionManager().IsPlayerConnected(tmpPlayer->getMyUniqueID()) && tmpPlayer->getMyType() == PLAYER_TYPE_HUMAN)
			{
				// Setting player cash to 0 will deactivate the player.
				tmpPlayer->setMyCash(0);
				tmpPlayer->setNetSessionData(boost::shared_ptr<SessionData>());
			}
			++i;
		}
	}
}

size_t
ServerGameThread::GetCurNumberOfPlayers() const
{
	return GetFullPlayerDataList().size();
}

void
ServerGameThread::AssignPlayerNumbers()
{
	int playerNumber = 0;

	PlayerDataList playerList = GetFullPlayerDataList();
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
ServerGameThread::IsPasswordProtected() const
{
	return !m_password.empty();
}

bool
ServerGameThread::CheckPassword(const string &password) const
{
	return (password == m_password);
}

GameSenderCallback &
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

