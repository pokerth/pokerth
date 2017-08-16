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
#include <gsasl.h>

using namespace std;
using boost::asio::ip::tcp;

#ifdef BOOST_ASIO_HAS_STD_CHRONO
using namespace std::chrono;
#else
using namespace boost::chrono;
#endif

SessionData::SessionData(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, SessionId id, SessionDataCallback &cb, boost::asio::io_service &ioService)
	: m_socket(sock), m_id(id), m_state(SessionData::Init), m_readyFlag(false), m_wantsLobbyMsg(true),
	  m_activityTimeoutSec(0), m_activityWarningRemainingSec(0), m_initTimeoutTimer(ioService), m_globalTimeoutTimer(ioService),
	  m_activityTimeoutTimer(ioService), m_callback(cb), m_authSession(NULL), m_curAuthStep(0)
{
	m_receiveBuffer.reset(new AsioReceiveBuffer);
	m_sendBuffer.reset(new AsioSendBuffer);
}

SessionData::SessionData(boost::shared_ptr<WebSocketData> webData, SessionId id, SessionDataCallback &cb, boost::asio::io_service &ioService, int /*filler*/)
	: m_webData(webData), m_id(id), m_state(SessionData::Init), m_readyFlag(false), m_wantsLobbyMsg(true),
	  m_activityTimeoutSec(0), m_activityWarningRemainingSec(0), m_initTimeoutTimer(ioService), m_globalTimeoutTimer(ioService),
	  m_activityTimeoutTimer(ioService), m_callback(cb), m_authSession(NULL), m_curAuthStep(0)
{
	m_receiveBuffer.reset(new WebReceiveBuffer);
	m_sendBuffer.reset(new WebSendBuffer);
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

bool
SessionData::CreateServerAuthSession(Gsasl *context)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	InternalClearAuthSession();
	int errorCode;
	errorCode = gsasl_server_start(context, "SCRAM-SHA-1", &m_authSession);
	return errorCode == GSASL_OK;
}

bool
SessionData::CreateClientAuthSession(Gsasl *context, const string &userName, const string &password)
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_dataMutex);
	InternalClearAuthSession();
	int errorCode;
	errorCode = gsasl_client_start(context, "SCRAM-SHA-1", &m_authSession);
	if (errorCode == GSASL_OK) {
		gsasl_property_set(m_authSession, GSASL_AUTHID, userName.c_str());
		gsasl_property_set(m_authSession, GSASL_PASSWORD, password.c_str());

		retVal = true;
	}
	return retVal;
}

bool
SessionData::AuthStep(int stepNum, const std::string &inData)
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_dataMutex);
	if (m_authSession && stepNum == m_curAuthStep + 1) {
		m_curAuthStep = stepNum;
		char *tmpOut;
		size_t tmpOutSize;
		int errorCode = gsasl_step(m_authSession, inData.c_str(), inData.length(), &tmpOut, &tmpOutSize);
		if (errorCode == GSASL_NEEDS_MORE) {
			m_nextGsaslMsg = string(tmpOut, tmpOutSize);
			retVal = true;
		} else if (errorCode == GSASL_OK && stepNum != 1) {
			m_nextGsaslMsg = string(tmpOut, tmpOutSize);
			retVal = true;
			InternalClearAuthSession();
		}
		gsasl_free(tmpOut);
	}
	return retVal;
}

string
SessionData::AuthGetUser() const
{
	string retStr;
	if (m_authSession) {
		const char *tmpUser = gsasl_property_fast(m_authSession, GSASL_AUTHID);
		if (tmpUser)
			retStr = tmpUser;
	}
	return retStr;
}

void
SessionData::AuthSetPassword(const std::string &password)
{
	if (m_authSession)
		gsasl_property_set(m_authSession, GSASL_PASSWORD, password.c_str());
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
	return m_nextGsaslMsg;
}

int
SessionData::AuthGetCurStepNum() const
{
	return m_curAuthStep;
}

void
SessionData::InternalClearAuthSession()
{
	if (m_authSession) {
		gsasl_finish(m_authSession);
		m_authSession = NULL;
		m_curAuthStep = 0;
	}
}

void
SessionData::TimerInitTimeout(const boost::system::error_code &ec)
{
	if (!ec) {
		if (GetState() == SessionData::Init) {
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

		m_activityTimeoutTimer.expires_from_now(
			seconds(m_activityWarningRemainingSec));
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
		m_socket->close(ec);
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
	m_activityTimeoutTimer.expires_from_now(
		seconds(m_activityTimeoutSec - m_activityWarningRemainingSec));
	m_activityTimeoutTimer.async_wait(
		boost::bind(
			&SessionData::TimerActivityWarning, shared_from_this(), boost::asio::placeholders::error));
}

void
SessionData::StartTimerInitTimeout(unsigned timeoutSec)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_initTimeoutTimer.expires_from_now(
		seconds(timeoutSec));
	m_initTimeoutTimer.async_wait(
		boost::bind(
			&SessionData::TimerInitTimeout, shared_from_this(), boost::asio::placeholders::error));
}

void
SessionData::StartTimerGlobalTimeout(unsigned timeoutSec)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_globalTimeoutTimer.expires_from_now(
		seconds(timeoutSec));
	m_globalTimeoutTimer.async_wait(
		boost::bind(
			&SessionData::TimerSessionTimeout, shared_from_this(), boost::asio::placeholders::error));
}

void
SessionData::StartTimerActivityTimeout(unsigned timeoutSec, unsigned warningRemainingSec)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_activityTimeoutSec = timeoutSec;
	m_activityWarningRemainingSec = warningRemainingSec;

	m_activityTimeoutTimer.expires_from_now(
		seconds(timeoutSec - warningRemainingSec));
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

string
SessionData::GetRemoteIPAddressFromSocket() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	string ipAddress;
	if (m_socket) {
		boost::system::error_code errCode;
		tcp::endpoint clientEndpoint = m_socket->remote_endpoint(errCode);
		if (!errCode) {
			ipAddress = clientEndpoint.address().to_string(errCode);
		}
	} else {
		boost::system::error_code errCode;
		server::connection_ptr con = m_webData->webSocketServer->get_con_from_hdl(m_webData->webHandle);
		tcp::endpoint webClientEndpoint = con->get_raw_socket().remote_endpoint(errCode);
		if (!errCode) {
			ipAddress = webClientEndpoint.address().to_string(errCode);
		}
	}
	return ipAddress;
}

