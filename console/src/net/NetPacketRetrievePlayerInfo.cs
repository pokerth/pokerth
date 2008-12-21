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
struct GCC_PACKED NetPacketRetrievePlayerInfoData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
};
*/

namespace pokerth_console
{
	class NetPacketRetrievePlayerInfo : NetPacket
	{
		public NetPacketRetrievePlayerInfo()
			: base(NetPacket.NetTypeRetrievePlayerInfo)
		{
		}

		public override void Accept(INetPacketVisitor visitor)
		{
			visitor.VisitRetrievePlayerInfo(this);
		}

		public override byte[] ToByteArray()
		{
			MemoryStream memStream = new MemoryStream();
			BinaryWriter w = new BinaryWriter(memStream);

			w.Write(IPAddress.HostToNetworkOrder((short)Type));
			w.Write(IPAddress.HostToNetworkOrder((short)8));
			w.Write(IPAddress.HostToNetworkOrder((int)
				Convert.ToUInt32(Properties[PropertyType.PropPlayerId])));

			return memStream.ToArray();
		}
	}
}
