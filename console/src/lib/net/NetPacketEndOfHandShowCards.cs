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
	class NetPacketEndOfHandShowCards : NetPacket
	{
		public NetPacketEndOfHandShowCards()
			: base(NetPacket.NetTypeEndOfHandShowCards)
		{
		}

		public NetPacketEndOfHandShowCards(int size, BinaryReader r)
			: base(NetPacket.NetTypeEndOfHandShowCards)
		{
			if (size < 40)
				throw new NetPacketException("NetPacketEndOfHandShowCards invalid size.");
			int numPlayerResults = IPAddress.NetworkToHostOrder((short)r.ReadUInt16());
			r.ReadUInt16(); //reserved

			List<Dictionary<PropType, string>> tmpRecordList = new List<Dictionary<PropType, string>>();
			for (int i = 0; i < numPlayerResults; i++)
			{
				Dictionary<PropType, string> tmpList = new Dictionary<PropType, string>();
				tmpList.Add(PropType.PlayerId,
					Convert.ToString(IPAddress.NetworkToHostOrder((int)r.ReadUInt32())));
				tmpList.Add(PropType.FirstCard,
					Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
				tmpList.Add(PropType.SecondCard,
					Convert.ToString(IPAddress.NetworkToHostOrder((short)r.ReadUInt16())));
			}
			RecordProperties.Add(RecordPropType.PlayerResult, tmpRecordList);
		}

		public override void Accept(INetPacketVisitor visitor)
		{
			visitor.VisitEndOfHandHideCards(this);
		}

		public override byte[] ToByteArray()
		{
			throw new NotImplementedException();
		}
	}
}
