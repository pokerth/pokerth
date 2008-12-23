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
		void Error(string message);
	}
}
