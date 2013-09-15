/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2013 Felix Hammer, Florian Thauer, Lothar May          *
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
/* Buffer interface for sending network data. */

#ifndef _SENDBUFFER_H_
#define _SENDBUFFER_H_

#include <net/websocket_defs.h>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>

class SessionData;
class NetPacket;

class SendBuffer : public boost::enable_shared_from_this<SendBuffer>
{
public:
	virtual ~SendBuffer();

	virtual void SetCloseAfterSend() = 0;

	virtual void AsyncSendNextPacket(boost::shared_ptr<SessionData> session) = 0;
	virtual void InternalStorePacket(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet) = 0;

	virtual void HandleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const boost::system::error_code &error) = 0;

	mutable boost::mutex dataMutex;
};

#endif

