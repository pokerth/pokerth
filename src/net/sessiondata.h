/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
/* Session data (a session is a valid client connection). */

#ifndef _SESSIONDATA_H_
#define _SESSIONDATA_H_

typedef unsigned SessionId;

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>

#include <net/socket_helper.h>
#include <net/sessiondatacallback.h>

#define INVALID_SESSION			0
#define SESSION_ID_INIT			INVALID_SESSION
#define SESSION_ID_GENERIC		0xFFFFFFFF

struct Gsasl;
struct Gsasl_session;
struct WebSocketData;
class ReceiveBuffer;
class SendBuffer;
class NetPacket;
class PlayerData;
class ServerGame;

class SessionData : public boost::enable_shared_from_this<SessionData>
{
public:
	enum State { Init = 1, ReceivingAvatar = 2, Established = 4, Game = 8, Spectating = 16, SpectatorWaiting = 32, Closed = 128 };

	SessionData(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, SessionId id, SessionDataCallback &cb, boost::asio::io_service &ioService);
	SessionData(boost::shared_ptr<WebSocketData> webData, SessionId id, SessionDataCallback &cb, boost::asio::io_service &ioService, int filler);
	~SessionData();

	SessionId GetId() const;

	boost::shared_ptr<ServerGame> GetGame() const;
	void SetGame(boost::shared_ptr<ServerGame> game);

	State GetState() const;
	void SetState(State state);

	boost::shared_ptr<boost::asio::ip::tcp::socket> GetAsioSocket();
	boost::shared_ptr<WebSocketData> GetWebData();

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

	ReceiveBuffer &GetReceiveBuffer()
	{
		return *m_receiveBuffer;
	}
	SendBuffer &GetSendBuffer()
	{
		return *m_sendBuffer;
	}

	void Close()
	{
		m_callback.CloseSession(shared_from_this());
	}
	void CloseSocketHandle();
	void CloseWebSocketHandle();
	void HandlePacket(boost::shared_ptr<NetPacket> packet)
	{
		m_callback.HandlePacket(shared_from_this(), packet);
	}

	void ResetActivityTimer();

	void StartTimerInitTimeout(unsigned timeoutSec);
	void StartTimerGlobalTimeout(unsigned timeoutSec);
	void StartTimerActivityTimeout(unsigned timeoutSec, unsigned warningRemainingSec);
	void CancelTimers();

	void SetPlayerData(boost::shared_ptr<PlayerData> player);
	boost::shared_ptr<PlayerData> GetPlayerData();

	std::string GetRemoteIPAddressFromSocket() const;

protected:
	SessionData(const SessionData &other);
	SessionData &operator=(const SessionData &other);
	void InternalClearAuthSession();
	void TimerInitTimeout(const boost::system::error_code &ec);
	void TimerSessionTimeout(const boost::system::error_code &ec);
	void TimerActivityWarning(const boost::system::error_code &ec);

private:
	boost::shared_ptr<boost::asio::ip::tcp::socket>	m_socket;
	boost::shared_ptr<WebSocketData>	m_webData;
	const SessionId					m_id;
	boost::weak_ptr<ServerGame>		m_game;
	State							m_state;
	std::string						m_clientAddr;
	boost::shared_ptr<ReceiveBuffer>	m_receiveBuffer;
	boost::shared_ptr<SendBuffer>	m_sendBuffer;
	bool							m_readyFlag;
	bool							m_wantsLobbyMsg;
	unsigned						m_activityTimeoutSec;
	unsigned						m_activityWarningRemainingSec;
	boost::asio::steady_timer		m_initTimeoutTimer;
	boost::asio::steady_timer		m_globalTimeoutTimer;
	boost::asio::steady_timer		m_activityTimeoutTimer;
	SessionDataCallback				&m_callback;
	Gsasl_session					*m_authSession;
	int								m_curAuthStep;
	std::string						m_nextGsaslMsg;
	std::string						m_password;
	boost::shared_ptr<PlayerData>	m_playerData;

	mutable boost::mutex			m_dataMutex;
};

#endif
