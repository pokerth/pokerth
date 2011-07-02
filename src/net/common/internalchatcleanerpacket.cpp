/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/

#include <net/internalchatcleanerpacket.h>
#include <third_party/asn1/ChatCleanerMessage.h>
#include <cstring>

using namespace std;

InternalChatCleanerPacket::InternalChatCleanerPacket()
{
	m_msg = (ChatCleanerMessage_t *)calloc(1, sizeof(ChatCleanerMessage_t));
}

InternalChatCleanerPacket::~InternalChatCleanerPacket()
{
	ASN_STRUCT_FREE(asn_DEF_ChatCleanerMessage, m_msg);
}

