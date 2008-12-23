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
	public interface ICallback
	{
		void InitDone();
		void JoinedGame(string name);
		void GameStarted(List<string> players);
		void HandStarted(Hand h);
		void MyTurn(Hand.State state);
		void PlayersTurn(Hand.State state, string player);
		void ActionDone(string name, Hand.Action action, uint totalBet, uint money, uint highestSet, uint minimumRaise);
		void ShowFlopCards(int firstFlopCard, int secondFlopCard, int thirdFlopCard);
		void ShowTurnCard(int turnCard);
		void ShowRiverCard(int riverCard);
		void Error(string message);
	}
}
