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
struct GCC_PACKED NetPacketStartEventData
{
	NetPacketHeader		head;
	u_int16_t			startFlags;
	u_int16_t			reserved;
};
*/

namespace pokerth_console
{
	class NetPacketStartEvent : NetPacket
	{
		public NetPacketStartEvent()
			: base(NetPacket.NetTypeStartEvent)
		{
		}

		public NetPacketStartEvent(int size, BinaryReader r)
			: base(NetPacket.NetTypeStartEvent)
		{
			if (size != 8)
				throw new NetPacketException("NetTypeStartEvent invalid size.");
			Properties.Add(PropertyType.StartFlags,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
		}

		public override void Accept(INetPacketVisitor visitor)
		{
			visitor.VisitStartEvent(this);
		}

		public override byte[] ToByteArray()
		{
			MemoryStream memStream = new MemoryStream();
			BinaryWriter w = new BinaryWriter(memStream);

			w.Write(IPAddress.HostToNetworkOrder((short)Type));
			w.Write(IPAddress.HostToNetworkOrder((short)8));
			w.Write(IPAddress.HostToNetworkOrder((short)
				Convert.ToUInt16(Properties[PropertyType.StartFlags])));
			w.Write(IPAddress.HostToNetworkOrder((short)0)); // reserved

			return memStream.ToArray();
		}
	}
}
