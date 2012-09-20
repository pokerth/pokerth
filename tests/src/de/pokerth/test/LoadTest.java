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


public class LoadTest extends TestBase {

	static final int NumGames = 4; // Number of games which are run, 10 players each.
	
	@Test
	public void testRunMultipleGames() throws Exception {

		// We don't need the default socket.
		sock.close();

		// We need a lot of sockets and player ids.
		Socket s[] = new Socket[NumGames * 10];
		long playerId[] = new long[NumGames * 10];
		long gameId[] = new long[NumGames];

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
			} while (msg.isGameListMessageSelected() || msg.isGamePlayerMessageSelected());
			if (!msg.isPlayerListMessageSelected()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}

			Collection<InitialNonZeroAmountOfMoney> l = new ArrayList<InitialNonZeroAmountOfMoney>();
			String gameName = AuthUser + " load game " + i;
			NetGameInfo gameInfo = createGameInfo(5, EndRaiseModeEnumType.EnumType.doubleBlinds, 0, 200, gameName, l, 10, 0, 1, 10000);
			sendMessage(createGameRequestMsg(
					gameInfo,
					NetGameTypeEnumType.EnumType.normalGame,
					5,
					7,
					"",
					false),
					s[i * 10]);

			// Game list update (new game)
			do {
				msg = receiveMessage(s[i * 10]);
				failOnErrorMessage(msg);
			} while (msg.isGameListMessageSelected() || msg.isPlayerListMessageSelected());

			// Join game ack.
			if (msg.isJoinGameReplyMessageSelected()) {
				if (!msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected()) {
					fail("Could not create game!");
				}
			}
			else {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			gameId[i] = msg.getJoinGameReplyMessage().getValue().getGameId().getValue();
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
			} while (msg.isGameListMessageSelected() || msg.isGamePlayerMessageSelected());
			if (!msg.isPlayerListMessageSelected()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			sendMessage(joinGameRequestMsg(gameId[i/10], "", false), s[i]);
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.isJoinGameReplyMessageSelected());
			if (!msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected()) {
				fail("User " + username + " could not join ranking game.");
			}
		}

		boolean abort = false;
		long handNum = 0;
		do {
			for (int i = 0; i < NumGames * 10; i++) {
				while (s[i].getInputStream().available() > 0) {
					msg = receiveMessage(s[i]);
					failOnErrorMessage(msg);
					if (msg.isHandStartMessageSelected()) {
						handNum++;
					}
					else if (msg.isPlayersTurnMessageSelected()) {
						if (msg.getPlayersTurnMessage().getValue().getPlayerId().getValue() == playerId[i / 10]) {
							NetPlayerAction action = new NetPlayerAction();
							action.setValue(NetPlayerAction.EnumType.actionAllIn);
							MyActionRequestMessageSequenceType myRequest = new MyActionRequestMessageSequenceType();
							myRequest.setGameId(new NonZeroId(gameId[i / 10]));
							myRequest.setGameState(msg.getPlayersTurnMessage().getValue().getGameState());
							myRequest.setHandNum(new NonZeroId(handNum));
							myRequest.setMyAction(action);
							myRequest.setMyRelativeBet(new AmountOfMoney(0));
							MyActionRequestMessage myAction = new MyActionRequestMessage();
							myAction.setValue(myRequest);
							PokerTHMessage outMsg = new PokerTHMessage();
							outMsg.selectMyActionRequestMessage(myAction);
							sendMessage(outMsg, s[i]);
						}
					}
					else if (msg.isEndOfGameMessageSelected()) {
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
