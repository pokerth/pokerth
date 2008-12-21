/***************************************************************************
 *   Copyright (C) 2008 by Lothar May                                      *
 *                                                                         *
 *   This file is part of pokerth_console.                                 *
 *   pokerth_console is free software: you can redistribute it and/or      *
 *   modify it under the terms of the GNU Affero General Public License    *
 *   as published by the Free Software Foundation, either version 3 of     *
 *   the License, or (at your option) any later version.                   *
 *                                                                         *
 *   pokerth_console is distributed in the hope that it will be useful,    *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the                                *
 *   GNU Affero General Public License along with pokerth_console.         *
 *   If not, see <http://www.gnu.org/licenses/>.                           *
 ***************************************************************************/

using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System.IO;

/*
struct GCC_PACKED NetPacketGameStartData
{
	NetPacketHeader		head;
	u_int32_t			startDealerPlayerId;
	u_int16_t			numberOfPlayers;
	u_int16_t			reserved;
};
*/

namespace pokerth_console
{
	class NetPacketGameStart : NetPacket
	{
		public NetPacketGameStart()
			: base(NetPacket.NetTypeGameStart)
		{
		}

		public NetPacketGameStart(int size, BinaryReader r)
			: base(NetPacket.NetTypeGameListNew)
		{
			if (size < 20)
				throw new NetPacketException("NetPacketGameStart invalid size.");
			Properties.Add(PropertyType.PropStartDealerPlayerId,
				Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
			int curNumPlayers = IPAddress.NetworkToHostOrder((short)r.ReadUInt16());
			Properties.Add(PropertyType.PropCurNumPlayers, Convert.ToString(curNumPlayers));
			r.ReadBytes(2); // reserved

			// Read player ids.
			List<string> playerSlots = new List<string>();
			for (int i = 0; i < curNumPlayers; i++)
				playerSlots.Add(Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
			ListProperties.Add(ListPropertyType.PropPlayerSlots, playerSlots);
		}

		public override void Accept(INetPacketVisitor visitor)
		{
			visitor.VisitGameStart(this);
		}

		public override byte[] ToByteArray()
		{
			throw new NotImplementedException();
		}
	}
}
