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
#include <third_party/boost/timers.hpp>
#include <string>

#include <net/socket_helper.h>
#include <net/receivebuffer.h>
#include <net/sessiondatacallback.h>

#define INVALID_SESSION			0
#define SESSION_ID_INIT			INVALID_SESSION
#define SESSION_ID_GENERIC		0xFFFFFFFF

struct Gsasl;
struct Gsasl_session;
class SendDataManager;

class SessionData
{
public:
	enum State { Init, ReceivingAvatar, Established, Game, Closed };

	SessionData(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, SessionId id, SessionDataCallback &cb);
	~SessionData();

	SessionId GetId() const;

	unsigned GetGameId() const;
	void SetGameId(unsigned gameId);

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
		return m_receiveBuffer;
	}
	SendDataManager &GetSendDataManager() {
		return *m_sendDataManager;
	}

	void ResetActivityTimer();
	unsigned GetActivityTimerElapsedSec() const;
	bool HasActivityNoticeBeenSent() const;
	void MarkActivityNotice();
	unsigned GetAutoDisconnectTimerElapsedSec() const;

protected:
	SessionData(const SessionData &other);
	SessionData &operator=(const SessionData &other);
	void InternalClearAuthSession();

private:
	boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
	const SessionId					m_id;
	unsigned						m_gameId;
	State							m_state;
	std::string						m_clientAddr;
	ReceiveBuffer					m_receiveBuffer;
	boost::shared_ptr<SendDataManager> m_sendDataManager;
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

	mutable boost::mutex			m_dataMutex;
};

#endif
