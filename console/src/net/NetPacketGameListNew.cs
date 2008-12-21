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
struct GCC_PACKED GameInfoData
{
	u_int16_t			maxNumberOfPlayers;
	u_int16_t			raiseIntervalMode;
	u_int16_t			raiseSmallBlindInterval;
	u_int16_t			raiseMode;
	u_int16_t			endRaiseMode;
	u_int16_t			numberOfManualBlinds;
	u_int16_t			proposedGuiSpeed;
	u_int16_t			playerActionTimeout;
	u_int32_t			firstSmallBlind;
	u_int32_t			endRaiseSmallBlindValue;
	u_int32_t			startMoney;
};
*/

/*
struct GCC_PACKED NetPacketGameListNewData
{
	NetPacketHeader		head;
	u_int32_t			gameId;
	u_int32_t			adminPlayerId;
	u_int16_t			gameMode;
	u_int16_t			gameNameLength;
	u_int16_t			curNumberOfPlayers;
	u_int16_t			gameFlags;
	GameInfoData		gameData;
};
*/

namespace pokerth_console
{
	class NetPacketGameListNew : NetPacket
	{
		public NetPacketGameListNew()
			: base(NetPacket.NetTypeGameListNew)
		{
		}

		public NetPacketGameListNew(int size, BinaryReader r)
			: base(NetPacket.NetTypeGameListNew)
		{
			if (size < 20)
				throw new NetPacketException("NetPacketGameListNew invalid size.");
			Properties.Add(PropertyType.PropGameId,
				Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
			Properties.Add(PropertyType.PropAdminPlayerId,
				Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
			Properties.Add(PropertyType.PropGameMode,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));

			int gameNameLen = IPAddress.NetworkToHostOrder((short)r.ReadUInt16());

			int curNumPlayers = IPAddress.NetworkToHostOrder((short)r.ReadUInt16());
			Properties.Add(PropertyType.PropCurNumPlayers, Convert.ToString(curNumPlayers));
			Properties.Add(PropertyType.PropGamePrivacyFlags,
				Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));

			r.ReadBytes(28); // Skip game info block for now.

			// Read name of the game.
			byte[] tmpName = r.ReadBytes(gameNameLen);
			Properties.Add(PropertyType.PropGameName,
				Encoding.UTF8.GetString(tmpName));
			// Skip the padding.
			int namePadding = AddPadding(tmpName.Length) - tmpName.Length;
			if (namePadding > 0)
				r.ReadBytes(namePadding);

			// Read player ids.
			List<string> playerSlots = new List<string>();
			for (int i = 0; i < curNumPlayers; i++)
				playerSlots.Add(Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
			ListProperties.Add(ListPropertyType.PropPlayerSlots, playerSlots);
		}

		public override void Accept(INetPacketVisitor visitor)
		{
			visitor.VisitGameListNew(this);
		}

		public override byte[] ToByteArray()
		{
			throw new NotImplementedException();
		}
	}
}
