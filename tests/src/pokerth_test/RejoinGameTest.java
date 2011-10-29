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

import java.net.Socket;

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import pokerth_protocol.*;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;
import pokerth_protocol.PlayerListMessage.PlayerListMessageSequenceType.PlayerListNotificationEnumType;

import pokerth_protocol.StartEventAckMessage.StartEventAckMessageSequenceType;


public class RejoinGameTest extends TestBase {

	@Test
	public void testRejoinGame() throws Exception {

		Guid firstPlayerSession = new Guid();
		long firstPlayerId = userInit(sock, AuthUser, AuthPassword, null, firstPlayerSession);

		// Waiting for player list update.
		PokerTHMessage msg;
		msg = receiveMessage();
		if (!msg.isPlayerListMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		Collection<InitialNonZeroAmountOfMoney> l = new ArrayList<InitialNonZeroAmountOfMoney>();
		String gameName = AuthUser + " rejoin game";
		NetGameInfo gameInfo = createGameInfo(5, EndRaiseModeEnumType.EnumType.doubleBlinds, 0, 50, gameName, l, 10, 0, 11, 10000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				NetGameTypeEnumType.EnumType.rankingGame,
				5,
				7,
				"",
				false));

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
			sendMessage(joinGameRequestMsg(gameId, "", false), s[i]);
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

		// Wait for start of hand.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
			for (int i = 0; i < 9; i++) {
				while (s[i].getInputStream().available() > 0) {
					PokerTHMessage inMsg = receiveMessage(s[i]);
					failOnErrorMessage(inMsg);
				}
			}
		} while (!msg.isHandStartMessageSelected());

		// Leave the game by closing the socket.
		sock.close();
		// No rejoin game id set yet.
		assertEquals(0, lastRejoinGameId);

		// All other players should have received "player left".
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.isPlayerListMessageSelected());
			assertEquals(firstPlayerId, msg.getPlayerListMessage().getValue().getPlayerId().getValue().longValue());
			assertEquals(PlayerListNotificationEnumType.EnumType.playerListLeft, msg.getPlayerListMessage().getValue().getPlayerListNotification().getValue());
		}

		sock = new Socket("localhost", 7234);
		// Reconnect to the server.
		long firstPlayerIdAfterRejoin = userInit(sock, AuthUser, AuthPassword, null, firstPlayerSession);
		assertEquals(gameId, lastRejoinGameId);
		// Waiting for player list update.
		do {
			msg = receiveMessage();
		} while (msg.isGameListMessageSelected() || msg.isGamePlayerMessageSelected());
		if (!msg.isPlayerListMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
		// Rejoin the game.
		sendMessage(rejoinGameRequestMsg(gameId, false));
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.isJoinGameReplyMessageSelected());
		if (!msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected()) {
			fail("User " + AuthUser + " could not rejoin ranking game.");
		}

		// All other players should have received "player joined".
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.isPlayerListMessageSelected());
			assertEquals(firstPlayerIdAfterRejoin, msg.getPlayerListMessage().getValue().getPlayerId().getValue().longValue());
			assertEquals(PlayerListNotificationEnumType.EnumType.playerListNew, msg.getPlayerListMessage().getValue().getPlayerListNotification().getValue());
		}

		// Wait for start event.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.isStartEventMessageSelected());

		assertEquals(gameId, msg.getStartEventMessage().getValue().getGameId().getValue().longValue());
		assertTrue(msg.getStartEventMessage().getValue().getStartEventType().isRejoinEventSelected());

		// Acknowledge start event.
		startType = new StartEventAckMessageSequenceType();
		startType.setGameId(new NonZeroId(gameId));
		startAck = new StartEventAckMessage();
		startAck.setValue(startType);
		msg = new PokerTHMessage();
		msg.selectStartEventAckMessage(startAck);
		sendMessage(msg);

		// Wait for game start. This may take a while, because rejoin is performed at the beginning of the next hand.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.isGameStartMessageSelected());

		// Check whether we got all necessary data to rejoin.
		assertEquals(gameId, msg.getGameStartMessage().getValue().getGameId().getValue().longValue());
		assertTrue(msg.getGameStartMessage().getValue().getGameStartMode().isGameStartModeRejoinSelected());
		GameStartModeRejoin rejoinData = msg.getGameStartMessage().getValue().getGameStartMode().getGameStartModeRejoin();
		// We left at the first hand.
		assertEquals(1, rejoinData.getHandNum().getValue().longValue());
		// 10 Players should now be active again.
		assertEquals(10, rejoinData.getRejoinPlayerData().size());

		// All other players should have received "player id changed".
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.isPlayerIdChangedMessageSelected());
			assertEquals(firstPlayerId, msg.getPlayerIdChangedMessage().getValue().getOldPlayerId().getValue().longValue());
			assertEquals(firstPlayerIdAfterRejoin, msg.getPlayerIdChangedMessage().getValue().getNewPlayerId().getValue().longValue());
		}

		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
	}
}
