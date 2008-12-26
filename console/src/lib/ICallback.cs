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
		void HandStarted(int[] cards);
		void SmallBlind(string name, uint blind);
		void BigBlind(string name, uint blind);
		void MyTurn(Hand.State state, uint highestSet, uint minimumRaise, uint money);
		void PlayersTurn(Hand.State state, string player);
		void ActionDone(string name, Hand.Action action, uint curBet);
		void ShowFlopCards(int[] cards);
		void ShowTurnCards(int[] cards);
		void ShowRiverCards(int[] cards);
		void AllInState();
		void ShowCards(string name, int[] cards);
		void EndOfHandShowCards(string name, int[] cards, int cardsValue, bool allIn);
		void HandResult();
		void PlayerWinsHand(string name, uint moneyWon);
		void PlayerWinsGame(string name);
		void Error(string message);
	}
}
