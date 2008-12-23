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

		public static string ActionToString(Hand.Action action)
		{
			string actionString;

			switch (action)
			{
				case Hand.Action.None:
					actionString = "None";
					break;
				case Hand.Action.Fold:
					actionString = "Fold";
					break;
				case Hand.Action.Check:
					actionString = "Check";
					break;
				case Hand.Action.Call:
					actionString = "Call";
					break;
				case Hand.Action.Bet:
					actionString = "Bet";
					break;
				case Hand.Action.Raise:
					actionString = "Raise";
					break;
				case Hand.Action.AllIn:
					actionString = "All in";
					break;
				default :
					actionString = "Unknown action";
					break;
			}
			return actionString;
		}
	}
}
