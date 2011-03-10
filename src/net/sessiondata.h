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
/* Session data (a session is a valid client connection). */

#ifndef _SESSIONDATA_H_
#define _SESSIONDATA_H_

typedef unsigned SessionId;

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <third_party/boost/timers.hpp>
#include <string>

#include <net/socket_helper.h>
#include <net/sessiondatacallback.h>

#define INVALID_SESSION			0
#define SESSION_ID_INIT			INVALID_SESSION
#define SESSION_ID_GENERIC		0xFFFFFFFF

struct Gsasl;
struct Gsasl_session;
class ReceiveBuffer;
class SendBuffer;
class NetPacket;
class PlayerData;
class ServerGame;

class SessionData : public boost::enable_shared_from_this<SessionData>
{
public:
	enum State { Init, ReceivingAvatar, Established, Game, Closed };

	SessionData(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, SessionId id, SessionDataCallback &cb);
	~SessionData();

	SessionId GetId() const;

	boost::shared_ptr<ServerGame> GetGame() const;
	void SetGame(boost::shared_ptr<ServerGame> game);

	State GetState() const;
	void SetState(State state);

	boost::shared_ptr<boost::asio::ip::tcp::socket> GetAsioSocket();

	bool CreateServerAuthSession(Gsasl *context);
	bool CreateClientAuthSession(Gsasl *context, const std::string &userName, const std::string &password);
	bool AuthStep(int stepNum, const std::string &inData);
	std::string AuthGetUser() const;
	void AuthSetPassword(const std::string &password);
	std::string AuthGetPassword() const;
	std::string AuthGetNextOutMsg() const;
	int AuthGetCurStepNum() const;

	void SetReadyFlag();
	void ResetReadyFlag();
	bool IsReady() const;
	void SetWantsLobbyMsg();
	void ResetWantsLobbyMsg();
	bool WantsLobbyMsg() const;

	const std::string &GetClientAddr() const;
	void SetClientAddr(const std::string &addr);

	ReceiveBuffer &GetReceiveBuffer() {
		return *m_receiveBuffer;
	}
	SendBuffer &GetSendBuffer() {
		return *m_sendBuffer;
	}

	void Close() {
		m_callback.CloseSession(shared_from_this());
	}
	void HandlePacket(boost::shared_ptr<NetPacket> packet) {
		m_callback.HandlePacket(shared_from_this(), packet);
	}

	void ResetActivityTimer();
	unsigned GetActivityTimerElapsedSec() const;
	bool HasActivityNoticeBeenSent() const;
	void MarkActivityNotice();
	unsigned GetAutoDisconnectTimerElapsedSec() const;

	void SetPlayerData(boost::shared_ptr<PlayerData> player);
	boost::shared_ptr<PlayerData> GetPlayerData();

protected:
	SessionData(const SessionData &other);
	SessionData &operator=(const SessionData &other);
	void InternalClearAuthSession();

private:
	boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
	const SessionId					m_id;
	boost::weak_ptr<ServerGame>		m_game;
	State							m_state;
	std::string						m_clientAddr;
	boost::shared_ptr<ReceiveBuffer>	m_receiveBuffer;
	boost::shared_ptr<SendBuffer>	m_sendBuffer;
	bool							m_readyFlag;
	bool							m_wantsLobbyMsg;
	boost::timers::portable::microsec_timer m_activityTimer;
	bool							m_activityTimeoutNoticeSent;
	boost::timers::portable::microsec_timer m_autoDisconnectTimer;
	SessionDataCallback				&m_callback;
	Gsasl_session					*m_authSession;
	int								m_curAuthStep;
	std::string						m_nextGsaslMsg;
	std::string						m_password;
	boost::shared_ptr<PlayerData>	m_playerData;

	mutable boost::mutex			m_dataMutex;
};

#endif
