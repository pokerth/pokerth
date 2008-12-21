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

namespace pokerth_console
{
	class NetParser : INetPacketVisitor
	{
		public NetParser(PokerTHData data, SenderThread sender)
		{
			m_data = data;
			m_sender = sender;
		}

		public void VisitInit(NetPacketInit p)
		{
			throw new NotImplementedException();
		}

		public void VisitInitAck(NetPacketInitAck p)
		{
			// TODO
		}

		public void VisitGameListNew(NetPacketGameListNew p)
		{
			// Add game to list.
			m_data.GameList.AddGameInfo(new GameInfo(
				Convert.ToUInt32(p.Properties[NetPacket.PropertyType.PropGameId]),
				p.Properties[NetPacket.PropertyType.PropGameName]));
		}

		public void VisitRetrievePlayerInfo(NetPacketRetrievePlayerInfo p)
		{
			throw new NotImplementedException();
		}

		public void VisitPlayerInfo(NetPacketPlayerInfo p)
		{
			// Add player to list.
			m_data.PlayerList.AddPlayerInfo(new PlayerInfo(
				Convert.ToUInt32(p.Properties[NetPacket.PropertyType.PropPlayerId]),
				p.Properties[NetPacket.PropertyType.PropPlayerName]));
		}

		public void VisitJoinGame(NetPacketJoinGame p)
		{
			throw new NotImplementedException();
		}

		public void VisitStartEvent(NetPacketStartEvent p)
		{
			// Acknowledge start event.
			NetPacketStartEventAck ack = new NetPacketStartEventAck();
			m_sender.Send(ack);
		}

		public void VisitStartEventAck(NetPacketStartEventAck p)
		{
			throw new NotImplementedException();
		}

		public void VisitGameStart(NetPacketGameStart p)
		{
			// Request player names for player ids.
			List<string> playerList = p.ListProperties[NetPacket.ListPropertyType.PropPlayerSlots];
			foreach (string player in playerList)
			{
				if (!m_data.PlayerList.HasPlayer(Convert.ToUInt32(player)))
				{
					NetPacketRetrievePlayerInfo request = new NetPacketRetrievePlayerInfo();
					request.Properties.Add(NetPacket.PropertyType.PropPlayerId, player);
					m_sender.Send(request);
				}
			}
		}

		private PokerTHData m_data;
		private SenderThread m_sender;
	}
}
