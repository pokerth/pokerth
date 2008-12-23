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
		void Error(string message);
	}
}
