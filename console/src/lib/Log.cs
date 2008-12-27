/***************************************************************************
 *   Copyright (C) 2008 by Lothar May                                      *
*    Inspired by log.cpp of the PokerTH client which is                    *
 *   Copyright (C) 2006 by FThauer FHammer                                 *
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
	public class Log
	{
		public static string CardToString(int card)
		{
			int cardValue = card % 13;
			int cardColor = card / 13;
			string cardString;

			switch (cardValue)
			{
				case 0:
					cardString = "2";
					break;
				case 1:
					cardString = "3";
					break;
				case 2:
					cardString = "4";
					break;
				case 3:
					cardString = "5";
					break;
				case 4:
					cardString = "6";
					break;
				case 5:
					cardString = "7";
					break;
				case 6:
					cardString = "8";
					break;
				case 7:
					cardString = "9";
					break;
				case 8:
					cardString = "T";
					break;
				case 9:
					cardString = "J";
					break;
				case 10:
					cardString = "Q";
					break;
				case 11:
					cardString = "K";
					break;
				case 12:
					cardString = "A";
					break;
				default:
					cardString = "Invalid Card ";
					break;
			}
			switch (cardColor)
			{
				case 0 :
					cardString += "d";
					break;
				case 1 :
					cardString += "h";
					break;
				case 2 :
					cardString += "s";
					break;
				case 3:
					cardString += "c";
					break;
				default:
					cardString += " Invalid Color";
					break;
			}
			return cardString;
		}

		public static string StateToString(Hand.State state)
		{
			string stateString;

			switch (state)
			{
				case Hand.State.Preflop :
					stateString = "Preflop";
					break;
				case Hand.State.Flop :
					stateString = "Flop";
					break;
				case Hand.State.Turn :
					stateString = "Turn";
					break;
				case Hand.State.River :
					stateString = "River";
					break;
				default :
					stateString = "Invalid state";
					break;
			}

			return stateString;
		}

		public static string ActionToString(Hand.Action action, uint curSet)
		{
			string actionString;

			switch (action)
			{
				case Hand.Action.Fold:
					actionString = "folds";
					break;
				case Hand.Action.Check:
					actionString = "checks";
					break;
				case Hand.Action.Call:
					actionString = "calls";
					break;
				case Hand.Action.Bet:
					actionString = "bets";
					break;
				case Hand.Action.Raise:
					actionString = "bets";
					break;
				case Hand.Action.AllIn:
					actionString = "is all in with";
					break;
				default :
					actionString = "(invalid action)";
					break;
			}

			if ((int)action >= 3)
				actionString += " $" + Convert.ToString(curSet);

			return actionString;
		}

		protected static string CardCodeToString(int cardCode, bool plural)
		{
			string cardString;

			switch (cardCode)
			{
				case 12 :
					cardString = "Ace";
					break;
				case 11 :
					cardString = "King";
					break;
				case 10 :
					cardString = "Queen";
					break;
				case 9 :
					cardString = "Jack";
					break;
				case 8 :
					cardString = "Ten";
					break;
				case 7 :
					cardString = "Nine";
					break;
				case 6 :
					cardString = "Eight";
					break;
				case 5 :
					cardString = "Seven";
					break;
				case 4 :
					cardString = "Six";
					break;
				case 3 :
					cardString = "Five";
					break;
				case 2 :
					cardString = "Four";
					break;
				case 1 :
					cardString = "Three";
					break;
				case 0 :
					cardString = "Deuce";
				break;
				default :
					cardString = "Invalid Card";
					break;
			}
			if (plural)
			{
				if (cardCode == 4) // sixes
					cardString += "e";
				cardString += "s";
			}
			return cardString;
		}

		public static string CardsValueToString(int cardsValue)
		{
			string cardString;

			int firstPart = cardsValue / 100000000;
			int secondPart = cardsValue / 1000000 - firstPart * 100;
			int thirdPart = cardsValue / 10000 - firstPart * 10000
				- secondPart * 100;
			int fourthPart = cardsValue / 100 - firstPart * 1000000
				- secondPart * 10000 - thirdPart * 100;
			int fifthPart = cardsValue - firstPart * 100000000
				- secondPart * 1000000 - thirdPart * 10000 - fourthPart * 100;
			int fifthPartA = cardsValue / 10 - firstPart * 10000000
				- secondPart * 100000 - thirdPart * 1000 - fourthPart * 10;
			int fifthPartB = cardsValue - firstPart * 100000000
				- secondPart * 1000000 - thirdPart * 10000 - fourthPart*100
				- fifthPartA*10;

			switch (firstPart)
			{
				// Royal Flush
				case 9 :
					cardString = "Royal Flush";
					break;
				// Straight Flush
				case 8 :
					cardString = "Straight Flush, ";
					if (secondPart >= 3)
						cardString += CardCodeToString(secondPart, false) + " high";
					else
						cardString += "Invalid high";
					break;
				// Four of a Kind
				case 7 :
					cardString = "Four of a Kind, " + CardCodeToString(secondPart, true);
					break;
				// Full House
				case 6 :
					cardString = "Full House, "
						+ CardCodeToString(secondPart, true)
						+ " full of "
						+ CardCodeToString(thirdPart, true);
					break;
				// Flush
				case 5 :
					cardString = "Flush, ";
					if (secondPart >= 4)
						cardString += CardCodeToString(secondPart, false) + " high";
					else
						cardString += "Invalid high";
					break;
				// Straight
				case 4 :
					cardString = "Straight, ";
					if (secondPart >= 3)
						cardString += CardCodeToString(secondPart, false) + " high";
					else
						cardString += "Invalid high";
					break;
				// Three of a Kind
				case 3 :
					cardString = "Three of a Kind, " + CardCodeToString(secondPart, true);
					break;
				// Two Pairs
				case 2 :
					cardString = "Two Pairs, "
						+ CardCodeToString(secondPart, true)
						+ " and "
						+ CardCodeToString(thirdPart, true);
					break;
				// One Pair
				case 1 :
					cardString = "One Pair, "
						+ CardCodeToString(secondPart, true);
				break;
				// Highest Card
				case 0 :
					cardString = "High Card, "
						+ CardCodeToString(secondPart, false);
					break;
				default:
					cardString = "Invalid Card Value";
					break;
			}
			return cardString;
		}
	}
}
