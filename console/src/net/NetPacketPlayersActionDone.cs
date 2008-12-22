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
struct GCC_PACKED NetPacketPlayersActionDoneData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
	u_int16_t			gameState;
	u_int16_t			playerAction;
	u_int32_t			totalPlayerBet;
	u_int32_t			playerMoney;
	u_int32_t			highestSet;
	u_int32_t			minimumRaise;
};
*/

namespace pokerth_console
{
	class NetPacketPlayersActionDone : NetPacket
	{
		public NetPacketPlayersActionDone()
			: base(NetPacket.NetTypePlayersActionDone)
		{
		}

		public NetPacketPlayersActionDone(int size, BinaryReader r)
			: base(NetPacket.NetTypePlayersActionDone)
		{
			if (size != 28)
				throw new NetPacketException("NetPacketPlayersActionDone invalid size.");
			Properties.Add(PropertyType.PlayerId,
				Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
			Properties.Add(PropertyType.GameState,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
			Properties.Add(PropertyType.PlayerAction,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
			Properties.Add(PropertyType.PlayerBetTotal,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt32())));
			Properties.Add(PropertyType.PlayerMoney,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt32())));
			Properties.Add(PropertyType.HighestSet,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt32())));
			Properties.Add(PropertyType.MinimumRaise,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt32())));
		}

		public override void Accept(INetPacketVisitor visitor)
		{
			visitor.VisitPlayersActionDone(this);
		}

		public override byte[] ToByteArray()
		{
			throw new NotImplementedException();
		}
	}
}
