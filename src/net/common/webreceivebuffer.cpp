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

#include <net/sessiondata.h>
#include <net/webreceivebuffer.h>
#include <core/loghelper.h>

using namespace std;


WebReceiveBuffer::WebReceiveBuffer()
{
}

void
WebReceiveBuffer::StartAsyncRead(boost::shared_ptr<SessionData> /*session*/)
{
	// Nothing to do. This is handled internally by websocketpp.
}

void
WebReceiveBuffer::HandleRead(boost::shared_ptr<SessionData> /*session*/, const boost::system::error_code &/*error*/, size_t /*bytesRead*/)
{
	LOG_ERROR("WebReceiveBuffer::HandleRead should never be called because Websocket I/O is message based.");
}

void
WebReceiveBuffer::HandleMessage(boost::shared_ptr<SessionData> session, const string &msg)
{
	boost::shared_ptr<NetPacket> tmpPacket;
	try {
		tmpPacket = NetPacket::Create(msg.c_str(), msg.size());
		if (!validator.IsValidPacket(*tmpPacket)) {
			LOG_ERROR("Session " << session->GetId() << " - Invalid packet: " << tmpPacket->GetMsg()->messagetype());
			tmpPacket.reset();
		}
	} catch (const exception &e) {
		LOG_ERROR("Session " << session->GetId() << " - " << e.what());
	}
	if (tmpPacket) {
		session->HandlePacket(tmpPacket);
	}
}

