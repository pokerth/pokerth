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
struct GCC_PACKED NetPacketInitData
{
	NetPacketHeader		head;
	u_int16_t			requestedVersionMajor;
	u_int16_t			requestedVersionMinor;
	u_int16_t			passwordLength;
	u_int16_t			playerNameLength;
	u_int16_t			privacyFlags;
	u_int16_t			reserved;
};
*/

namespace pokerth_lib
{
	class NetPacketInit : NetPacket
	{
		public NetPacketInit()
			: base(NetPacket.NetTypeInit)
		{
		}

		public override void Accept(INetPacketVisitor visitor)
		{
			visitor.VisitInit(this);
		}

		public override byte[] ToByteArray()
		{
			MemoryStream memStream = new MemoryStream();
			BinaryWriter w = new BinaryWriter(memStream);

			string playerPassword = Properties[PropType.PlayerPassword];
			byte[] tmpPassword = Encoding.UTF8.GetBytes(playerPassword);
			int passwordWithPadding = AddPadding(tmpPassword.Length);
			string playerName = Properties[PropType.PlayerName];
			byte[] tmpName = Encoding.UTF8.GetBytes(playerName);
			int nameWithPadding = AddPadding(tmpName.Length);
			int size = 16 + passwordWithPadding + nameWithPadding;

			w.Write(IPAddress.HostToNetworkOrder((short)Type));
			w.Write(IPAddress.HostToNetworkOrder((short)size));
			w.Write(IPAddress.HostToNetworkOrder((short)
				Convert.ToUInt16(Properties[PropType.RequestedVersionMajor])));
			w.Write(IPAddress.HostToNetworkOrder((short)
				Convert.ToUInt16(Properties[PropType.RequestedVersionMinor])));
			w.Write(IPAddress.HostToNetworkOrder((short)playerPassword.Length));
			w.Write(IPAddress.HostToNetworkOrder((short)playerName.Length));
			w.Write(IPAddress.HostToNetworkOrder((short)0)); // Privacy flags.
			w.Write(IPAddress.HostToNetworkOrder((short)0)); // Reserved.

			w.Write(tmpPassword);
			// Add padding.
			int passwordPadding = passwordWithPadding - tmpPassword.Length;
			if (passwordPadding > 0)
				w.Write(new byte[passwordPadding]);

			w.Write(tmpName);
			// Add padding.
			int namePadding = nameWithPadding - tmpName.Length;
			if (namePadding > 0)
				w.Write(new byte[namePadding]);

			return memStream.ToArray();
		}
	}
}
