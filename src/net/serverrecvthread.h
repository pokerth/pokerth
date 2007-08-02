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
/* Network server receive thread. */

#ifndef _SERVERRECVTHREAD_H_
#define _SERVERRECVTHREAD_H_

#include <core/thread.h>
#include <net/connectdata.h>
#include <net/sessiondata.h>
#include <gui/guiinterface.h>
#include <gamedata.h>

#include <deque>
#include <map>
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>
#include <core/boost/timer.hpp>

#define RECEIVER_THREAD_TERMINATE_TIMEOUT	200


class ServerRecvState;
class SenderThread;
class ReceiverHelper;
class ServerSenderCallback;
class NetPacket;
class ConfigFile;
struct GameData;
class Game;

struct SessionWrapper
{
	SessionWrapper() {}
	SessionWrapper(boost::shared_ptr<SessionData> s, boost::shared_ptr<PlayerData> p)
		: sessionData(s), playerData(p) {}
	boost::shared_ptr<SessionData>	sessionData;
	boost::shared_ptr<PlayerData>	playerData;
};

class ServerRecvThread : public Thread
{
public:
	ServerRecvThread(GuiInterface &gui, ConfigFile *playerConfig);
	virtual ~ServerRecvThread();

	void Init(const std::string &pwd, const GameData &gameData);

	void AddConnection(boost::shared_ptr<ConnectData> data);
	void AddNotification(unsigned message, const std::string &param);

	ServerCallback &GetCallback();

	void SendError(SOCKET s, int errorCode);
	void SendToAllPlayers(boost::shared_ptr<NetPacket> packet);
	void SendToAllButOnePlayers(boost::shared_ptr<NetPacket> packet, SOCKET except);

protected:

	struct Notification
	{
		Notification(unsigned m, std::string p)
			: message(m), param(p) {}
		unsigned message;
		std::string param;
	};

	typedef std::deque<boost::shared_ptr<ConnectData> > ConnectQueue;
	typedef std::map<SOCKET, SessionWrapper> SocketSessionMap;
	typedef std::list<SessionWrapper> SessionList;
	typedef std::deque<Notification> NotificationQueue;
	typedef std::list<std::pair<boost::microsec_timer, boost::shared_ptr<SessionData> > > CloseSessionList;

	// Main function of the thread.
	virtual void Main();

	void NotificationLoop();
	void CloseSessionLoop();

	SOCKET Select();

	void CleanupConnectQueue();
	void CleanupSessionMap();

	void InternalStartGame();
	void InternalKickPlayer(unsigned uniqueId);

	SessionWrapper GetSession(SOCKET sock) const;
	SessionWrapper GetSessionByPlayerName(const std::string playerName) const;
	SessionWrapper GetSessionByUniquePlayerId(unsigned uniqueId) const;
	void AddSession(boost::shared_ptr<SessionData> sessionData); // new Sessions have no player data
	void SessionError(SessionWrapper session, int errorCode);
	void RejectNewConnection(boost::shared_ptr<ConnectData> connData);
	void CloseSessionDelayed(SessionWrapper session);
	void RemoveNotEstablishedSessions();
	void RemoveDisconnectedPlayers();

	size_t GetCurNumberOfPlayers() const;
	bool IsPlayerConnected(const std::string &playerName) const;
	bool IsPlayerConnected(unsigned uniquePlayerId) const;
	void SetSessionPlayerData(boost::shared_ptr<SessionData> sessionData, boost::shared_ptr<PlayerData> playerData);
	PlayerDataList GetPlayerDataList() const;

	void AssignPlayerNumbers();

	ServerRecvState &GetState();
	void SetState(ServerRecvState &newState);

	SenderThread &GetSender();
	ReceiverHelper &GetReceiver();

	Game &GetGame();
	const GameData &GetGameData() const;
	const StartData &GetStartData() const;
	void SetStartData(const StartData &startData);
	bool CheckPassword(const std::string &password) const;

	ServerSenderCallback &GetSenderCallback();
	GuiInterface &GetGui();

private:

	ConnectQueue m_connectQueue;
	mutable boost::mutex m_connectQueueMutex;
	ServerRecvState *m_curState;

	NotificationQueue m_notificationQueue;
	mutable boost::mutex m_notificationQueueMutex;

	SocketSessionMap m_sessionMap;
	mutable boost::mutex m_sessionMapMutex;

	CloseSessionList m_closeSessionList;

	std::auto_ptr<ReceiverHelper> m_receiver;
	std::auto_ptr<SenderThread> m_sender;

	std::auto_ptr<Game> m_game;
	GameData m_gameData;
	StartData m_startData;
	unsigned m_curGameId;
	std::auto_ptr<ServerSenderCallback> m_senderCallback;

	std::string m_password;

	GuiInterface &m_gui;

	ConfigFile *m_playerConfig;

friend class ServerRecvStateInit;
friend class ServerRecvStateReceiving;
friend class ServerRecvStateRunning;
friend class ServerRecvStateStartGame;
friend class ServerRecvStateStartHand;
friend class ServerRecvStateStartRound;
friend class ServerRecvStateWaitPlayerAction;
friend class ServerRecvStateShowCardsDelay;
friend class ServerRecvStateDealCardsDelay;
friend class ServerRecvStateNextHandDelay;
friend class ServerRecvStateNextGameDelay;
};

#endif
