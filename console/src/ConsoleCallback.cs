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
using pokerth_lib;

namespace pokerth_console
{
	class ConsoleCallback : pokerth_lib.ICallback
	{
		public void InitDone()
		{
			Console.WriteLine("Init successful.");
		}

		public void JoinedGame(string name)
		{
			Console.WriteLine("Successfully joined game \"{0}\".", name);
		}

		public void GameStarted(List<string> players)
		{
			string outPlayers = "";
			foreach (string s in players)
			{
				if (outPlayers.Length != 0)
					outPlayers += ", ";
				outPlayers += s;
			}
			Console.WriteLine("Game was started. Players: {0}", outPlayers);
		}

		public void HandStarted(pokerth_lib.Hand h)
		{
			Console.WriteLine("New hand. Your cards: {0} {1}. Your money: {2}.",
				Log.CardToString(h.Players[h.MyPlayerId].Cards[0]),
				Log.CardToString(h.Players[h.MyPlayerId].Cards[1]),
				h.Players[h.MyPlayerId].Money);
		}

		public void MyTurn(Hand.State state)
		{
			Console.WriteLine("--> {0}: Your turn.",
				Log.StateToString(state));
		}

		public void PlayersTurn(Hand.State state, string player)
		{
			Console.WriteLine("{0}: {1}'s turn.",
				Log.StateToString(state),
				player);
		}

		public void ActionDone(string name, Hand.Action action, uint totalBet, uint money, uint highestSet, uint minimumRaise)
		{
			Console.WriteLine("{0} acts: {1}. Total bet: {2}. Money left: {3}",
				name, Log.ActionToString(action), totalBet, money);
			Console.WriteLine("Highest set: {0}. Minimum raise: {1}",
				highestSet, minimumRaise);
		}

		public void Error(string message)
		{
			Console.WriteLine("Error: " + message);
		}
	}
}
