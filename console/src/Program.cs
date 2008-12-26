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
using System.Threading;
using System.Net.Sockets;
using pokerth_lib;

namespace pokerth_console
{
	class Program
	{
		static void ChooseOpenGame(Client client, PokerTHData data)
		{
			string input;
			bool joined = false;
			do
			{
				do
				{
					Console.WriteLine("Open Games:");
					Console.Write(data.GameList.GetOpenGamesString());
					Console.WriteLine();
					Console.WriteLine("Enter game id (or press enter to refresh)");
					input = Console.ReadLine();
				} while (input == "" || !Char.IsDigit(input[0]));
				Console.WriteLine("Joining game...");
				uint gameId = Convert.ToUInt32(input);
				client.JoinGame(gameId);
				// Hack this, because Mono does not support WaitOne(...).
				DateTime cur = DateTime.Now;
				while (!client.HasJoinedGame())
				{
					Thread.Sleep(15);
					if (DateTime.Now.Subtract(cur).Seconds > 5)
						break;
				}
				if (client.HasJoinedGame())
					joined = true;
				else
					Console.WriteLine("Could not join game.");
			} while (!joined);
			Console.WriteLine("Waiting for admin to start the game...");
		}

		static void GameLoop(Client client, PokerTHData data)
		{
			string input;
			do
			{
				while (Console.KeyAvailable)
					Console.ReadKey();
				input = Console.ReadLine();
				if (input.Length > 0)
				{
					Hand.Action action = Hand.Action.None;
					uint bet = 0;
					try
					{
						switch (input[0])
						{
							case 'f':
								action = Hand.Action.Fold;
								break;
							case 'c':
								action = Hand.Action.Check;
								break;
							case 'l':
								action = Hand.Action.Call;
								break;
							case 'b':
								action = Hand.Action.Bet;
								bet = Convert.ToUInt32(input.Substring(1));
								break;
							case 'r':
								action = Hand.Action.Raise;
								bet = Convert.ToUInt32(input.Substring(1));
								break;
							case 'a':
								action = Hand.Action.AllIn;
								break;
						}
						if (action != Hand.Action.None)
							client.MyAction(action, bet);
					}
					catch (FormatException)
					{
					}
				}
			} while (data.JoinedGame);
		}

		static int Main(string[] args)
		{
			Console.WriteLine("pokerth_console V0.1 - Copyright (C) 2008 by Lothar May");
			Console.WriteLine("See license.txt for license terms.");
			Console.WriteLine();
			Console.WriteLine("Enter your nickname:");
			string name = Console.ReadLine();
			Console.WriteLine("Connecting to server...");
			Settings settings = new Settings();
			PokerTHData data = new PokerTHData(name);
			ConsoleCallback callback = new ConsoleCallback();
			Client client = new Client(settings, data, callback);
			try
			{
				client.Connect();
			}
			catch (SocketException)
			{
				Console.WriteLine("Unable to connect to server.");
				return 1;
			}
			client.Start();
			Thread.Sleep(2000);
			ChooseOpenGame(client, data);
			GameLoop(client, data);
			client.SetTerminateFlag();
			client.WaitTermination();
			return 0;
		}
	}
}
