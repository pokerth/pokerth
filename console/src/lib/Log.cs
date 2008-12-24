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
	}
}
