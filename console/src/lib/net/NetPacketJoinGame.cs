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


namespace pokerth_lib
{
	class NetPacketJoinGame : NetPacket
	{
		public NetPacketJoinGame()
			: base(NetPacket.NetTypeJoinGame)
		{
		}

		public override void Accept(INetPacketVisitor visitor)
		{
			visitor.VisitJoinGame(this);
		}

		public override byte[] ToByteArray()
		{
			MemoryStream memStream = new MemoryStream();
			BinaryWriter w = new BinaryWriter(memStream);

			string gamePassword = Properties[PropType.GamePassword];
			byte[] tmpPassword = Encoding.UTF8.GetBytes(gamePassword);
			int passwordWithPadding = AddPadding(tmpPassword.Length);
			int size = 12 + passwordWithPadding;

			w.Write(IPAddress.HostToNetworkOrder((short)Type));
			w.Write(IPAddress.HostToNetworkOrder((short)size));
			w.Write(IPAddress.HostToNetworkOrder((int)
				Convert.ToUInt32(Properties[PropType.GameId])));
			w.Write(IPAddress.HostToNetworkOrder((short)gamePassword.Length));
			w.Write(IPAddress.HostToNetworkOrder((short)0)); // Reserved.

			w.Write(tmpPassword);
			// Add padding.
			int passwordPadding = passwordWithPadding - tmpPassword.Length;
			if (passwordPadding > 0)
				w.Write(new byte[passwordPadding]);

			return memStream.ToArray();
		}
	}
}
