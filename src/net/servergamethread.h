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
/* Network server game thread. */

#ifndef _SERVERGAMETHREAD_H_
#define _SERVERGAMETHREAD_H_

#include <net/sessionmanager.h>
#include <gui/guiinterface.h>
#include <gamedata.h>

#include <deque>

#define GAME_THREAD_TERMINATE_TIMEOUT	200


class SenderThread;
class ReceiverHelper;
class ServerLobbyThread;
class ServerSenderCallback;
class ServerGameState;
class ConfigFile;
struct GameData;
class Game;

class ServerGameThread : public Thread
{
public:
	ServerGameThread(ServerLobbyThread &lobbyThread, u_int32_t id, const std::string &name, const std::string &pwd, GuiInterface &gui, ConfigFile *playerConfig);
	virtual ~ServerGameThread();

	void Init(const GameData &gameData);

	u_int32_t GetId() const;
	const std::string &GetName() const;

	void AddSession(SessionWrapper session);

	ServerCallback &GetCallback();
	GameState GetCurRound() const;

	void SendToAllPlayers(boost::shared_ptr<NetPacket> packet, SessionData::State state);

	bool IsPasswordProtected() const;
	bool CheckPassword(const std::string &password) const;
	const GameData &GetGameData() const;

	boost::shared_ptr<PlayerData> GetPlayerDataByUniqueId(unsigned playerId) const;
	PlayerIdList GetPlayerIdList() const;
	bool IsPlayerConnected(const std::string &name) const;

protected:

	typedef std::deque<SessionWrapper> SessionQueue;

	// Main function of the thread.
	virtual void Main();

	void InternalStartGame();
	void InternalKickPlayer(unsigned playerId);

	PlayerDataList GetFullPlayerDataList() const;

	void AddComputerPlayer(boost::shared_ptr<PlayerData> player);
	void ResetComputerPlayerList();

	void RemoveSession(SessionWrapper session);
	void SessionError(SessionWrapper session, int errorCode);
	void MoveSessionToLobby(SessionWrapper session, int reason);

	void RemoveDisconnectedPlayers();
	size_t GetCurNumberOfPlayers() const;
	void AssignPlayerNumbers();

	ServerLobbyThread &GetLobbyThread();

	ServerGameState &GetState();
	void SetState(ServerGameState &newState);

	SenderThread &GetSender();
	ReceiverHelper &GetReceiver();

	Game &GetGame();
	const Game &GetGame() const;

	const StartData &GetStartData() const;
	void SetStartData(const StartData &startData);

	ServerSenderCallback &GetSenderCallback();
	GuiInterface &GetGui();

	unsigned GetNextGameNum();

	const SessionManager &GetSessionManager() const;
	SessionManager &GetSessionManager();

private:

	SessionQueue m_sessionQueue;
	mutable boost::mutex m_sessionQueueMutex;

	SessionManager m_sessionManager;
	PlayerDataList m_computerPlayerList;
	mutable boost::mutex m_computerPlayerListMutex;

	ServerLobbyThread &m_lobbyThread;
	std::auto_ptr<ReceiverHelper> m_receiver;
	std::auto_ptr<SenderThread> m_sender;
	std::auto_ptr<ServerSenderCallback> m_senderCallback;
	GuiInterface &m_gui;

	GameData			m_gameData;
	StartData			m_startData;
	std::auto_ptr<Game>	m_game;
	const u_int32_t		m_id;
	const std::string	m_name;
	const std::string	m_password;
	ConfigFile		   *m_playerConfig;
	ServerGameState	   *m_curState;
	unsigned			m_gameNum;

friend class AbstractServerGameStateReceiving;
friend class AbstractServerGameStateRunning;
friend class ServerGameStateInit;
friend class ServerGameStateStartGame;
friend class ServerGameStateStartHand;
friend class ServerGameStateStartRound;
friend class ServerGameStateWaitPlayerAction;
friend class ServerGameStateComputerAction;
friend class ServerGameStateDealCardsDelay;
friend class ServerGameStateShowCardsDelay;
friend class ServerGameStateNextHandDelay;
friend class ServerGameStateNextGameDelay;
};

#endif
