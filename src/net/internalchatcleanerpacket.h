/***************************************************************************
 *   Copyright (C) 2009 by Lothar May                                      *
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
/* PokerTH chat cleaner packet. */

#ifndef _INTERNALCHATCLEANERPACKET_H_
#define _INTERNALCHATCLEANERPACKET_H_

#define CLEANER_PROTOCOL_VERSION		1
#define MAX_CLEANER_PACKET_SIZE			384

typedef struct ChatCleanerMessage ChatCleanerMessage_t;

class InternalChatCleanerPacket
{
public:
	InternalChatCleanerPacket();

	InternalChatCleanerPacket(ChatCleanerMessage_t *msg)
	: m_msg(msg)
	{
	}
	~InternalChatCleanerPacket();
	ChatCleanerMessage_t *GetMsg()
	{
		return m_msg;
	}
	ChatCleanerMessage_t **GetMsgPtr()
	{
		return &m_msg;
	}

private:
	ChatCleanerMessage_t *m_msg;
};

#endif

