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

#include <net/sessiondata.h>
#include <net/asioreceivebuffer.h>
#include <net/webreceivebuffer.h>
#include <net/asiosendbuffer.h>
#include <net/websendbuffer.h>
#include <net/socket_msg.h>
#include <net/websocketdata.h>
#include <core/loghelper.h>
#include <boost/asio/ssl.hpp>

using namespace std;
using boost::asio::ip::tcp;

#ifdef BOOST_ASIO_HAS_STD_CHRONO
using namespace std::chrono;
#else
using namespace boost::chrono;
#endif

SessionData::SessionData(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, SessionId id, SessionDataCallback &cb, boost::asio::io_context &ioService)
	: m_socket(sock), m_id(id), m_state(SessionData::Init), m_readyFlag(false), m_wantsLobbyMsg(true),
	  m_activityTimeoutSec(0), m_activityWarningRemainingSec(0), m_globalTimeoutSec(0), m_initTimeoutTimer(ioService), m_globalTimeoutTimer(ioService),
	  m_activityTimeoutTimer(ioService), m_callback(cb), m_authSession(NULL), m_curAuthStep(0)
{
	m_receiveBuffer.reset(new AsioReceiveBuffer);
	m_sendBuffer.reset(new AsioSendBuffer);
}

SessionData::SessionData(boost::shared_ptr<WebSocketData> webData, SessionId id, SessionDataCallback &cb, boost::asio::io_context &ioService, int /*filler*/)
	: m_webData(webData), m_id(id), m_state(SessionData::Init), m_readyFlag(false), m_wantsLobbyMsg(true),
	  m_activityTimeoutSec(0), m_activityWarningRemainingSec(0), m_globalTimeoutSec(0), m_initTimeoutTimer(ioService), m_globalTimeoutTimer(ioService),
	  m_activityTimeoutTimer(ioService), m_callback(cb), m_authSession(NULL), m_curAuthStep(0)
{
	m_receiveBuffer.reset(new WebReceiveBuffer);
	m_sendBuffer.reset(new WebSendBuffer);
}

SessionData::SessionData(boost::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> sslStream, SessionId id, SessionDataCallback &cb, boost::asio::io_context &ioService, int /*filler*/)
    : m_socket(), m_webData(), m_id(id), m_game(), m_state(SessionData::Init), m_clientAddr(),
      m_receiveBuffer(), m_sendBuffer(), m_readyFlag(false), m_wantsLobbyMsg(true),
      m_activityTimeoutSec(0), m_activityWarningRemainingSec(0), m_globalTimeoutSec(0),
      m_initTimeoutTimer(ioService), m_globalTimeoutTimer(ioService), m_activityTimeoutTimer(ioService),
      m_callback(cb), m_authSession(NULL), m_curAuthStep(0)
{
    m_sslStream = sslStream;
    m_receiveBuffer.reset(new AsioReceiveBuffer);
    m_sendBuffer.reset(new AsioSendBuffer);
}

SessionData::~SessionData()
{
	InternalClearAuthSession();
	// Web Socket handle needs to be manually closed, asio socket is closed automatically.
	CloseWebSocketHandle();
}

SessionId
SessionData::GetId() const
{
	// const value - no mutex needed.
	return m_id;
}

boost::shared_ptr<ServerGame>
SessionData::GetGame() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_game.lock();
}

void
SessionData::SetGame(boost::shared_ptr<ServerGame> game)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_game = game;
}

SessionData::State
SessionData::GetState() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_state;
}

void
SessionData::SetState(SessionData::State state)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_state = state;
}

boost::shared_ptr<boost::asio::ip::tcp::socket>
SessionData::GetAsioSocket()
{
	return m_socket;
}

boost::shared_ptr<WebSocketData>
SessionData::GetWebData()
{
	return m_webData;
}

boost::shared_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> SessionData::GetSslStream()
{
    boost::mutex::scoped_lock lock(m_dataMutex);
    return m_sslStream;
}

bool SessionData::IsSsl() const
{
    boost::mutex::scoped_lock lock(m_dataMutex);
    return (m_sslStream != nullptr);
}

bool
SessionData::CreateServerAuthSession(Gsasl *context)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	InternalClearAuthSession();
	m_authSession = NULL;
	m_curAuthStep = 0;
	return true;
}

bool
SessionData::CreateClientAuthSession(Gsasl *context, const string &userName, const string &password)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	InternalClearAuthSession();
	m_password = password;
	m_authSession = NULL;
	m_curAuthStep = 0;
	(void)context; (void)userName;
	return true;
}

bool
SessionData::AuthStep(int stepNum, const std::string &inData)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	if (stepNum == m_curAuthStep + 1) {
		m_curAuthStep = stepNum;
		m_nextGsaslMsg.clear();
		return true;
	}
	return false;
}

string
SessionData::AuthGetUser() const
{
	return std::string();
}

void
SessionData::AuthSetPassword(const std::string &password)
{
	m_password = password;
}

string
SessionData::AuthGetPassword() const
{
    return m_password;
}

string
SessionData::AuthGetNextOutMsg() const
{
    boost::mutex::scoped_lock lock(m_dataMutex);
    return m_nextGsaslMsg;
}

int
SessionData::AuthGetCurStepNum() const
{
    boost::mutex::scoped_lock lock(m_dataMutex);
    return m_curAuthStep;
}

void
SessionData::InternalClearAuthSession()
{
    m_authSession = NULL;
    m_curAuthStep = 0;
    m_nextGsaslMsg.clear();
}

void
SessionData::TimerInitTimeout(const boost::system::error_code &ec)
{
	if (!ec) {
		// Close session ONLY if still in Init state (hanging TLS handshake or stuck login)
		// Do NOT close sessions that are Established, Game, Spectating, etc.
		if (GetState() == SessionData::Init) {
			// Force-close the socket to abort any pending async operations (e.g., hanging TLS handshake)
			CloseSocketHandle();
			m_callback.SessionError(shared_from_this(), ERR_NET_SESSION_TIMED_OUT);
		}
	}
}

void
SessionData::TimerSessionTimeout(const boost::system::error_code &ec)
{
	if (!ec) {
		m_callback.SessionError(shared_from_this(), ERR_NET_SESSION_TIMED_OUT);
	}
}

void
SessionData::TimerActivityWarning(const boost::system::error_code &ec)
{
	if (!ec) {
		m_callback.SessionTimeoutWarning(shared_from_this(), m_activityWarningRemainingSec);

		m_activityTimeoutTimer.expires_after(seconds(m_activityWarningRemainingSec));
		m_activityTimeoutTimer.async_wait(
			boost::bind(
				&SessionData::TimerSessionTimeout, shared_from_this(), boost::asio::placeholders::error));
	}
}

void
SessionData::SetReadyFlag()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_readyFlag = true;
}

void
SessionData::ResetReadyFlag()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_readyFlag = false;
}

bool
SessionData::IsReady() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_readyFlag;
}

void
SessionData::SetWantsLobbyMsg()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_wantsLobbyMsg = true;
}

void
SessionData::ResetWantsLobbyMsg()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_wantsLobbyMsg = false;
}

bool
SessionData::WantsLobbyMsg() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_wantsLobbyMsg;
}

const std::string &
SessionData::GetClientAddr() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_clientAddr;
}

void
SessionData::SetClientAddr(const std::string &addr)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_clientAddr = addr;
}

void
SessionData::CloseSocketHandle()
{
    if (m_socket) {
        boost::system::error_code ec;
        // Cancel all pending async operations first
        m_socket->cancel(ec);
        // Then close the socket
        m_socket->close(ec);
    } else if (m_sslStream) {
        boost::system::error_code ec;
        // Cancel all pending async operations first
        m_sslStream->lowest_layer().cancel(ec);
        // Then close underlying socket
        m_sslStream->lowest_layer().close(ec);
    }
}

void
SessionData::CloseWebSocketHandle()
{
	if (m_webData) {
#if defined(__GXX_EXPERIMENTAL_CXX0X__) || (__cplusplus >= 201103L) // c++11 
		std::error_code std_ec;
		m_webData->webSocketServer->close(m_webData->webHandle, websocketpp::close::status::normal, "PokerTH server closed the connection.", std_ec);
#else
		boost::system::error_code ec;
		m_webData->webSocketServer->close(m_webData->webHandle, websocketpp::close::status::normal, "PokerTH server closed the connection.", ec);
#endif
	}
}

void
SessionData::ResetActivityTimer()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	unsigned delaySec = m_activityTimeoutSec - m_activityWarningRemainingSec;
	m_activityTimeoutTimer.expires_after(seconds(delaySec));
	m_activityTimeoutTimer.async_wait(
		boost::bind(
			&SessionData::TimerActivityWarning, shared_from_this(), boost::asio::placeholders::error));
}

void
SessionData::StartTimerInitTimeout(unsigned timeoutSec)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_initTimeoutTimer.expires_after(seconds(timeoutSec)); 
	m_initTimeoutTimer.async_wait(
		boost::bind(
			&SessionData::TimerInitTimeout, shared_from_this(), boost::asio::placeholders::error));
}

void
SessionData::StartTimerGlobalTimeout(unsigned timeoutSec)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_globalTimeoutSec = timeoutSec;
	m_globalTimeoutTimer.expires_after(seconds(timeoutSec));
	m_globalTimeoutTimer.async_wait(
		boost::bind(
			&SessionData::TimerSessionTimeout, shared_from_this(), boost::asio::placeholders::error));
}

void
SessionData::ResetGlobalTimeout()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	if (m_globalTimeoutSec > 0) {
		m_globalTimeoutTimer.expires_after(seconds(m_globalTimeoutSec));
		m_globalTimeoutTimer.async_wait(
			boost::bind(
				&SessionData::TimerSessionTimeout, shared_from_this(), boost::asio::placeholders::error));
	}
}

void
SessionData::StartTimerActivityTimeout(unsigned timeoutSec, unsigned warningRemainingSec)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_activityTimeoutSec = timeoutSec;
	m_activityWarningRemainingSec = warningRemainingSec;

	unsigned delaySec = timeoutSec - warningRemainingSec;
	m_activityTimeoutTimer.expires_after(seconds(delaySec));
	m_activityTimeoutTimer.async_wait(
		boost::bind(
			&SessionData::TimerActivityWarning, shared_from_this(), boost::asio::placeholders::error));
}

void
SessionData::CancelTimers()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_initTimeoutTimer.cancel();
	m_globalTimeoutTimer.cancel();
	m_activityTimeoutTimer.cancel();
}

void
SessionData::SetPlayerData(boost::shared_ptr<PlayerData> player)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_playerData = player;
}

boost::shared_ptr<PlayerData>
SessionData::GetPlayerData()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_playerData;
}

std::string
SessionData::GetRemoteIPAddressFromSocket() const
{
    boost::mutex::scoped_lock lock(m_dataMutex);
    boost::system::error_code ec;

    if (m_sslStream) {
        try {
            auto &lowest = m_sslStream->lowest_layer();
            auto ep = lowest.remote_endpoint(ec);
            if (!ec) return ep.address().to_string();
        } catch (...) {
        }
    }

    if (m_socket) {
        try {
            auto sock = m_socket;
            auto ep = sock->remote_endpoint(ec);
            if (!ec) return ep.address().to_string();
        } catch (...) {
        }
    }

    if (m_webData && m_webData->webSocketServer) {
        try {
            auto con = m_webData->webSocketServer->get_con_from_hdl(m_webData->webHandle);
            auto ep = con->get_raw_socket().remote_endpoint(ec);
            if (!ec) return ep.address().to_string();
        } catch (...) {
        }
    }

    return std::string();
}

