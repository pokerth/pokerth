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
struct GCC_PACKED NetPacketPlayerInfoData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
	u_int16_t			playerFlags;
	u_int16_t			playerNameLength;
	u_int32_t			reserved;
};
*/

namespace pokerth_console
{
	class NetPacketPlayerInfo : NetPacket
	{
		public const int PlayerFlagHuman = 0x01;
		public const int PlayerFlagAvatar = 0x02;

		public NetPacketPlayerInfo()
			: base(NetPacket.NetTypePlayerInfo)
		{
		}

		public NetPacketPlayerInfo(int size, BinaryReader r)
			: base(NetPacket.NetTypePlayerInfo)
		{
			if (size < 16)
				throw new NetPacketException("NetPacketPlayerInfo invalid size.");
			Properties.Add(PropertyType.PropPlayerId,
				Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
			int playerFlags = IPAddress.NetworkToHostOrder((short)r.ReadUInt16());
			Properties.Add(PropertyType.PropPlayerFlags, Convert.ToString(playerFlags));
			int playerNameLen = IPAddress.NetworkToHostOrder((short)r.ReadUInt16());
			r.ReadUInt32(); // reserved
			if ((playerFlags & PlayerFlagAvatar) == PlayerFlagAvatar)
				r.ReadBytes(16); // Skip avatar md5.

			byte[] tmpName = r.ReadBytes(playerNameLen);
			Properties.Add(PropertyType.PropPlayerName,
				Encoding.UTF8.GetString(tmpName));
		}

		public override byte[] ToByteArray()
		{
			throw new NotImplementedException();
		}
	}
}
