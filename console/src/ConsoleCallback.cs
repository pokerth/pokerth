using System;
using System.Collections.Generic;
using System.Text;

namespace pokerth_console
{
	class ConsoleCallback : ICallback
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

		public void HandStarted(Hand h)
		{
			Console.WriteLine("New hand. Your cards: {0} {1}. Your money: {2}.",
				Log.CardToString(h.Players[h.MyPlayerId].Cards[0]),
				Log.CardToString(h.Players[h.MyPlayerId].Cards[1]),
				h.Players[h.MyPlayerId].Money);
		}

		public void Error(string message)
		{
			Console.WriteLine("Error: " + message);
		}
	}
}
