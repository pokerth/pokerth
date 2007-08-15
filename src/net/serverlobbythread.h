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

#include <net/connectdata.h>
#include <net/sessionmanager.h>
#include <net/netpacket.h>
#include <gui/guiinterface.h>
#include <gamedata.h>

#include <deque>
#include <list>
#include <core/boost/timer.hpp>

#define LOBBY_THREAD_TERMINATE_TIMEOUT	200


class SenderThread;
class ReceiverHelper;
class ServerSenderCallback;
class ConfigFile;
struct GameData;
class Game;

class ServerLobbyThread : public Thread
{
public:
	ServerLobbyThread(GuiInterface &gui, ConfigFile *playerConfig);
	virtual ~ServerLobbyThread();

	void Init(const std::string &pwd);

	void AddConnection(boost::shared_ptr<ConnectData> data);

	u_int32_t GetNextUniquePlayerId();
	ServerCallback &GetCallback();

protected:

	typedef std::deque<boost::shared_ptr<ConnectData> > ConnectQueue;
	typedef std::list<SessionWrapper> SessionList;
	typedef std::list<std::pair<boost::microsec_timer, boost::shared_ptr<SessionData> > > CloseSessionList;

	// Main function of the thread.
	virtual void Main();

	void ProcessLoop();
	void HandleNetPacketInit(SessionWrapper session, const NetPacketInit &tmpPacket);
	void HandleNetPacketCreateGame(SessionWrapper session, const NetPacketCreateGame &tmpPacket);
	void HandleNetPacketJoinGame(SessionWrapper session, const NetPacketJoinGame &tmpPacket);
	void CloseSessionLoop();

	void HandleNewConnection(boost::shared_ptr<ConnectData> connData);

	SOCKET Select();

	void CleanupConnectQueue();
	void CleanupSessionMap();

	void SessionError(SessionWrapper session, int errorCode);
	void CloseSessionDelayed(SessionWrapper session);
	void SendError(SOCKET s, int errorCode);

	bool IsPlayerConnected(const std::string &playerName) const;

	SenderThread &GetSender();
	ReceiverHelper &GetReceiver();

	bool CheckPassword(const std::string &password) const;

	ServerSenderCallback &GetSenderCallback();
	GuiInterface &GetGui();

private:

	ConnectQueue m_connectQueue;
	mutable boost::mutex m_connectQueueMutex;

	SessionManager m_sessionManager;

	CloseSessionList m_closeSessionList;

	std::auto_ptr<ReceiverHelper> m_receiver;
	std::auto_ptr<SenderThread> m_sender;
	std::auto_ptr<ServerSenderCallback> m_senderCallback;
	GuiInterface &m_gui;

	std::string m_password;
	ConfigFile *m_playerConfig;
	u_int32_t m_curUniquePlayerId;
};

#endif
