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

package pokerth_test;

import java.net.InetAddress;
import java.net.Socket;
import java.sql.Connection;
import java.sql.DriverManager;
import static org.junit.Assert.fail;

import java.util.ArrayList;
import java.util.Collection;

import org.junit.Before;
import org.junit.Test;

import pokerth_protocol.*;
import pokerth_protocol.MyActionRequestMessage.MyActionRequestMessageSequenceType;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;
import pokerth_protocol.StartEventAckMessage.StartEventAckMessageSequenceType;
import pokerth_protocol.StartEventMessage.StartEventMessageSequenceType;


public class RunRankingGameTest extends TestBase {

	Connection dbConn;

	@Before
	public void dbInit() throws Exception {
		final String userName = "username";
		final String password = "password";
		final String url = "jdbc:mysql://localhost:3306/database";
		Class.forName("com.mysql.jdbc.Driver").newInstance ();
		dbConn = DriverManager.getConnection(url, userName, password);
	}

	@Test
	public void testRunRankingGame() throws Exception {
		long firstPlayerId = userInit();

		Collection<Integer> l = new ArrayList<Integer>();
		NetGameInfo gameInfo = createGameInfo(5, EndRaiseModeEnumType.EnumType.doubleBlinds, 0, 50, GuestUser + " run ranking game", l, 10, 0, 11, 10000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				NetGameTypeEnumType.EnumType.rankingGame,
				5,
				7,
				""));

		PokerTHMessage msg;

		// Game list update (new game)
		msg = receiveMessage();
		if (!msg.isGameListMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		// Join game ack.
		msg = receiveMessage();
		if (msg.isJoinGameReplyMessageSelected()) {
			if (!msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected()) {
				fail("Could not create game!");
			}
		}
		else {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
		long gameId = msg.getJoinGameReplyMessage().getValue().getGameId().getValue();

		// Game list update (player joined).
		msg = receiveMessage();
		if (!msg.isGameListMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		// Let 9 additional clients join.
		Socket s[] = new Socket[9];
		long playerId[] = new long[9];
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("::1", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password);
			sendMessage(joinGameRequestMsg(gameId, ""), s[i]);
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.isJoinGameReplyMessageSelected());
			if (!msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected()) {
				fail("User " + username + " could not join ranking game.");
			}

			// The player should have joined the game.
			msg = receiveMessage();
			if (!msg.isPlayerListMessageSelected()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			msg = receiveMessage();
			if (!msg.isGamePlayerMessageSelected()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			msg = receiveMessage();
			if (!msg.isGameListMessageSelected()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
		}

		// Server should automatically send start event.
		msg = receiveMessage();
		if (!msg.isStartEventMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.isStartEventMessageSelected());
		}
		// Acknowledge start event.
		StartEventAckMessageSequenceType startType = new StartEventAckMessageSequenceType();
		startType.setGameId(new NonZeroId(gameId));
		StartEventAckMessage startAck = new StartEventAckMessage();
		startAck.setValue(startType);
		msg = new PokerTHMessage();
		msg.selectStartEventAckMessage(startAck);
		sendMessage(msg);
		for (int i = 0; i < 9; i++) {
			sendMessage(msg, s[i]);
		}

		// Game list update (game now running).
		msg = receiveMessage();
		if (!msg.isGameListMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		msg = receiveMessage();
		if (!msg.isGameStartMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		long handNum = 0;
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
			if (msg.isHandStartMessageSelected()) {
				handNum++;
			}
			else if (msg.isPlayersTurnMessageSelected()) {
				if (msg.getPlayersTurnMessage().getValue().getPlayerId().getValue() == firstPlayerId) {
					NetPlayerAction action = new NetPlayerAction();
					action.setValue(NetPlayerAction.EnumType.actionAllIn);
					MyActionRequestMessageSequenceType myRequest = new MyActionRequestMessageSequenceType();
					myRequest.setGameId(new NonZeroId(gameId));
					myRequest.setGameState(msg.getPlayersTurnMessage().getValue().getGameState());
					myRequest.setHandNum(new NonZeroId(handNum));
					myRequest.setMyAction(action);
					myRequest.setMyRelativeBet(0);
					MyActionRequestMessage myAction = new MyActionRequestMessage();
					myAction.setValue(myRequest);
					PokerTHMessage outMsg = new PokerTHMessage();
					outMsg.selectMyActionRequestMessage(myAction);
					sendMessage(outMsg);
				}
			}
			for (int i = 0; i < 9; i++) {
				while (s[i].getInputStream().available() > 0) {
					PokerTHMessage inMsg = receiveMessage(s[i]);
					failOnErrorMessage(inMsg);
					if (inMsg.isPlayersTurnMessageSelected()) {
						if (inMsg.getPlayersTurnMessage().getValue().getPlayerId().getValue() == playerId[i]) {
							NetPlayerAction action = new NetPlayerAction();
							action.setValue(NetPlayerAction.EnumType.actionFold);
							MyActionRequestMessageSequenceType myRequest = new MyActionRequestMessageSequenceType();
							myRequest.setGameId(new NonZeroId(gameId));
							myRequest.setGameState(inMsg.getPlayersTurnMessage().getValue().getGameState());
							myRequest.setHandNum(new NonZeroId(handNum));
							myRequest.setMyAction(action);
							myRequest.setMyRelativeBet(0);
							MyActionRequestMessage myAction = new MyActionRequestMessage();
							myAction.setValue(myRequest);
							PokerTHMessage outMsg = new PokerTHMessage();
							outMsg.selectMyActionRequestMessage(myAction);
							sendMessage(outMsg, s[i]);
						}
					}
				}
			}
		} while (!msg.isEndOfGameMessageSelected());
	}
}
