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

namespace pokerth_lib
{
	public interface INetPacketVisitor
	{
		void VisitInit(NetPacket p);
		void VisitInitAck(NetPacket p);
		void VisitGameListNew(NetPacket p);
		void VisitGameListUpdate(NetPacket p);
		void VisitRetrievePlayerInfo(NetPacket p);
		void VisitPlayerInfo(NetPacket p);
		void VisitCreateGame(NetPacket p);
		void VisitJoinGame(NetPacket p);
		void VisitJoinGameAck(NetPacket p);
		void VisitStartEvent(NetPacket p);
		void VisitStartEventAck(NetPacket p);
		void VisitGameStart(NetPacket p);
		void VisitHandStart(NetPacket p);
		void VisitPlayersTurn(NetPacket p);
		void VisitPlayersAction(NetPacket p);
		void VisitPlayersActionDone(NetPacket p);
		void VisitPlayersActionRejected(NetPacket p);
		void VisitDealFlopCards(NetPacket p);
		void VisitDealTurnCard(NetPacket p);
		void VisitDealRiverCard(NetPacket p);
		void VisitEndOfHandShowCards(NetPacket p);
		void VisitEndOfHandHideCards(NetPacket p);
	}
}
