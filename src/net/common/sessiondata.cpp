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

#include <net/sessiondata.h>

SessionData::SessionData(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, SessionId id, SessionDataCallback &cb)
: m_socket(sock), m_id(id), m_gameId(0), m_state(SessionData::Init), m_readyFlag(false),
  m_wantsLobbyMsg(true), m_activityTimeoutNoticeSent(false), m_callback(cb),
  m_maxNumPlayers(0)
{
}

SessionData::~SessionData()
{
	m_callback.SignalSessionTerminated(m_id);
}

SessionId
SessionData::GetId() const
{
	// const value - no mutex needed.
	return m_id;
}

unsigned
SessionData::GetGameId() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_gameId;
}

void
SessionData::SetGameId(unsigned gameId)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_gameId = gameId;
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

ReceiveBuffer &
SessionData::GetReceiveBuffer()
{
	// mutex protection, if needed, within buffer.
	return m_receiveBuffer;
}

void
SessionData::ResetActivityTimer()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_activityTimeoutNoticeSent = false;
	m_activityTimer.reset();
	m_activityTimer.start();
}

unsigned
SessionData::GetActivityTimerElapsedSec() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_activityTimer.elapsed().total_seconds();
}

bool
SessionData::HasActivityNoticeBeenSent() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_activityTimeoutNoticeSent;
}

void
SessionData::MarkActivityNotice()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_activityTimeoutNoticeSent = true;
}

unsigned
SessionData::GetAutoDisconnectTimerElapsedSec() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_autoDisconnectTimer.elapsed().total_seconds();
}

unsigned
SessionData::GetMaxNumPlayers() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_maxNumPlayers;
}

void
SessionData::SetMaxNumPlayers(unsigned numPlayers)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_maxNumPlayers = numPlayers;
}
