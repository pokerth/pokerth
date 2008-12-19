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
struct GCC_PACKED NetPacketInitAckData
{
	NetPacketHeader		head;
	u_int16_t			latestGameVersion;
	u_int16_t			latestBetaRevision;
	u_int32_t			sessionId;
	u_int32_t			playerId;
};
*/

namespace pokerth_console
{
	class NetPacketInitAck : NetPacket
	{
		public NetPacketInitAck()
			: base(NetPacket.NetTypeInitAck)
		{
		}

		public NetPacketInitAck(int size, BinaryReader r)
			: base(NetPacket.NetTypeInitAck)
		{
			if (size != 16)
				throw new NetPacketException("NetPacketInitAck invalid size.");
			Properties.Add(PropertyType.PropLatestGameVersion,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
			Properties.Add(PropertyType.PropLatestBetaRevision,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
			Properties.Add(PropertyType.PropSessionId,
				Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
			Properties.Add(PropertyType.PropPlayerId,
				Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
		}

		public override byte[] ToByteArray()
		{
			throw new NotImplementedException();
		}
	}
}
