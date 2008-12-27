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
			Console.WriteLine("Connection established.");
			Console.WriteLine("Retrieving list of games...");
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
			Console.WriteLine("Game was started. Players: {0}.", outPlayers);
		}

		public void HandStarted(int[] cards)
		{
			Console.WriteLine("\nNew hand. Your cards: {0}, {1}.",
				Log.CardToString(cards[0]),
				Log.CardToString(cards[1]));
		}

		public void SmallBlind(string name, uint blind)
		{
			Console.WriteLine("{0} posts small blind (${1}).", name, blind);
		}

		public void BigBlind(string name, uint blind)
		{
			Console.WriteLine("{0} posts big blind (${1}).", name, blind);
		}

		public void MyTurn(Hand.State state, uint highestSet, uint minimumRaise, uint money)
		{
			Console.WriteLine("\n--> {0}: Your turn. Highest set: ${1}. Minimum raise: ${2}. Cash: ${3}.",
				Log.StateToString(state), highestSet, minimumRaise, money);
			Console.WriteLine("Enter: (f)old, (c)heck, ca(l)l, (b)et<num>, (r)aise<num>, (a)ll in");
		}

		public void PlayersTurn(Hand.State state, string player)
		{
			Console.WriteLine("\n{0}: {1}'s turn.",
				Log.StateToString(state),
				player);
		}

		public void ActionDone(string name, Hand.Action action, uint curBet, uint money, List<string> nonFoldPlayers)
		{
			Console.WriteLine("{0} {1}. Cash: {2}.", name, Log.ActionToString(action, curBet), money);
			if (action == Hand.Action.Fold)
			{
				string outPlayers = "";
				foreach (string s in nonFoldPlayers)
				{
					if (outPlayers.Length != 0)
						outPlayers += ", ";
					outPlayers += s;
				}
				Console.WriteLine("Remaining players: {0}", outPlayers);
			}
		}

		public void ActionRejected()
		{
			Console.WriteLine("Your action was rejected by the server.");
		}

		public void ShowPot(uint pot)
		{
			Console.WriteLine("\nPot is now {0}.", pot);
		}

		public void ShowFlopCards(int[] cards)
		{
			Console.WriteLine("--- Flop --- [{0}, {1}, {2}]",
				Log.CardToString(cards[0]),
				Log.CardToString(cards[1]),
				Log.CardToString(cards[2]));
		}

		public void ShowTurnCards(int[] cards)
		{
			Console.WriteLine("--- Turn --- [{0}, {1}, {2}, {3}]",
				Log.CardToString(cards[0]),
				Log.CardToString(cards[1]),
				Log.CardToString(cards[2]),
				Log.CardToString(cards[3]));
		}

		public void ShowRiverCards(int[] cards)
		{
			Console.WriteLine("--- River --- [{0}, {1}, {2}, {3}, {4}]",
				Log.CardToString(cards[0]),
				Log.CardToString(cards[1]),
				Log.CardToString(cards[2]),
				Log.CardToString(cards[3]),
				Log.CardToString(cards[4]));
		}

		public void AllInState()
		{
			Console.WriteLine(); // simply print a newline
		}

		public void ShowCards(string name, int[] cards)
		{
			Console.WriteLine("{0} shows [{1}, {2}].",
				name,
				Log.CardToString(cards[0]),
				Log.CardToString(cards[1]));
		}

		public void EndOfHandShowCards(string name, int[] cards, int cardsValue, bool allIn)
		{
			Console.WriteLine("{0} {1} [{2}, {3}] - {4}.",
				name,
				allIn ? "has" : "shows",
				Log.CardToString(cards[0]),
				Log.CardToString(cards[1]),
				Log.CardsValueToString(cardsValue));
		}

		public void HandResult()
		{
			Console.WriteLine();
		}

		public void PlayerWinsHand(string name, uint moneyWon)
		{
			Console.WriteLine("{0} wins ${1}.", name, moneyWon);
		}

		public void PlayerWinsGame(string name)
		{
			Console.WriteLine("\n{0} wins the game.", name);
		}

		public void RemovedFromGame()
		{
			Console.WriteLine("You have left the game.");
			System.Environment.Exit(1);
		}

		public void ChatText(string name, string message)
		{
			Console.WriteLine("{0}: {1}",
				name,
				message);
		}

		public void Error(string message)
		{
			Console.WriteLine("Error: " + message);
			Console.WriteLine("Hint: Try using a different nickname.");
			System.Environment.Exit(1);
		}
	}
}
