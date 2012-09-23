/*  PokerTH automated tests.
	Copyright (C) 2010 Lothar May

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

package de.pokerth.test;

import java.net.Socket;

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.NetGameType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;
import de.pokerth.protocol.ProtoBuf.MyActionRequestMessage;
import de.pokerth.protocol.ProtoBuf.NetPlayerAction;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;


public class LoadTest extends TestBase {

	static final int NumGames = 4; // Number of games which are run, 10 players each.
	
	@Test
	public void testRunMultipleGames() throws Exception {

		// We don't need the default socket.
		sock.close();

		// We need a lot of sockets and player ids.
		Socket s[] = new Socket[NumGames * 10];
		int playerId[] = new int[NumGames * 10];
		int gameId[] = new int[NumGames];

		PokerTHMessage msg;
		// First players are game admins.
		// Create several games.
		for (int i = 0; i < NumGames; i++) {
			s[i * 10] = new Socket("localhost", 7234);
			String username = "test" + (i*10+1);
			String password = username;
			playerId[i * 10] = userInit(s[i * 10], username, password);

			do {
				msg = receiveMessage(s[i * 10]);
			} while (msg.hasGameListNewMessage() || msg.hasGameListPlayerJoinedMessage() || msg.hasGamePlayerJoinedMessage());
			if (!msg.hasPlayerListMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}

			Collection<Integer> l = new ArrayList<Integer>();
			String gameName = AuthUser + " load game " + i;
			NetGameInfo gameInfo = createGameInfo(NetGameType.normalGame, 5, 7, 5, EndRaiseMode.doubleBlinds, 0, 200, gameName, l, 10, 0, 1, 10000);
			sendMessage(createGameRequestMsg(
					gameInfo,
					"",
					false),
					s[i * 10]);

			// Game list update (new game)
			do {
				msg = receiveMessage(s[i * 10]);
				failOnErrorMessage(msg);
			} while (msg.hasGameListNewMessage() || msg.hasGameListPlayerJoinedMessage() || msg.hasPlayerListMessage());

			// Join game ack.
			if (!msg.hasJoinGameAckMessage()) {
				failOnErrorMessage(msg);
				fail("Could not create game!");
			}
			gameId[i] = msg.getJoinGameAckMessage().getGameId();
		}


		// Let additional clients join.
		for (int i = 0; i < NumGames * 10; i++) {
			if (i % 10 == 0) {
				continue;
			}
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password);
			// Waiting for player list update.
			do {
				msg = receiveMessage(s[i]);
			} while (msg.hasGameListPlayerJoinedMessage() || msg.hasGamePlayerJoinedMessage());
			if (!msg.hasPlayerListMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			sendMessage(joinGameRequestMsg(gameId[i/10], "", false), s[i]);
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasJoinGameAckMessage() && !msg.hasJoinGameFailedMessage());
			if (!msg.hasJoinGameAckMessage()) {
				fail("User " + username + " could not join normal game.");
			}
		}

		boolean abort = false;
		int handNum[] = new int[NumGames];
		do {
			for (int i = 0; i < NumGames * 10; i++) {
				while (s[i].getInputStream().available() > 0) {
					msg = receiveMessage(s[i]);
					failOnErrorMessage(msg);
					if (msg.hasHandStartMessage()) {
						handNum[i / 10]++;
					}
					else if (msg.hasPlayersTurnMessage()) {
						if (msg.getPlayersTurnMessage().getPlayerId() == playerId[i / 10]) {
							MyActionRequestMessage myRequest = MyActionRequestMessage.newBuilder()
								.setGameId(gameId[i / 10])
								.setGameState(msg.getPlayersTurnMessage().getGameState())
								.setHandNum(handNum[i / 10])
								.setMyAction(NetPlayerAction.netActionAllIn)
								.setMyRelativeBet(0)
								.build();
							PokerTHMessage outMsg = PokerTHMessage.newBuilder()
									.setMessageType(PokerTHMessageType.Type_MyActionRequestMessage)
									.setMyActionRequestMessage(myRequest)
									.build();
							sendMessage(outMsg, s[i]);
						}
					}
					else if (msg.hasEndOfGameMessage()) {
						abort = true;
					}
				}
			}
		} while (!abort);

		for (int i = 0; i < NumGames; i++) {
			s[i].close();
		}
	}
}
