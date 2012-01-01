/*  PokerTH automated tests.
	Copyright (C) 2011 Lothar May

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

import java.sql.ResultSet;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import pokerth_protocol.*;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;
import pokerth_protocol.PlayerListMessage.PlayerListMessageSequenceType.PlayerListNotificationEnumType;

import pokerth_protocol.StartEventAckMessage.StartEventAckMessageSequenceType;


public class RejoinMultiGameTest extends TestBase {

	@Test
	public void testRejoinMultiGame() throws Exception {

		Statement dbStatement = dbConn.createStatement();
		ResultSet countBeforeResult = dbStatement.executeQuery("SELECT COUNT(idgame) FROM game");
		countBeforeResult.first();
		long countBefore = countBeforeResult.getLong(1);

		userInit();

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
		Guid playerSession[] = new Guid[9];
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			playerSession[i] = new Guid();
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password, null, playerSession[i]);
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

		// 9 players leave the game by closing the socket.
		for (int i = 0; i < 9; i++) {
			s[i].close();
			Thread.sleep(500);
		}
		// No rejoin game id set yet.
		assertEquals(0, lastRejoinGameId);

		// The remaining player should have received "player left" 9 times.
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage();
				failOnErrorMessage(msg);
			} while (!msg.isPlayerListMessageSelected());
			assertEquals(playerId[i], msg.getPlayerListMessage().getValue().getPlayerId().getValue().longValue());
			assertEquals(PlayerListNotificationEnumType.EnumType.playerListLeft, msg.getPlayerListMessage().getValue().getPlayerListNotification().getValue());
		}

		// Let all players reconnect.
		long playerIdAfterRejoin[] = new long[9];
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerIdAfterRejoin[i] = userInit(s[i], username, password, null, playerSession[i]);
			assertEquals(gameId, lastRejoinGameId);
			// Waiting for player list update.
			do {
				msg = receiveMessage(s[i]);
			} while (msg.isGameListMessageSelected() || msg.isGamePlayerMessageSelected());
			if (!msg.isPlayerListMessageSelected()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			sendMessage(rejoinGameRequestMsg(gameId, false), s[i]);
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.isJoinGameReplyMessageSelected());
			if (!msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected()) {
				fail("User " + username + " could not rejoin ranking game.");
			}
		}
		// The remaining player should have received "player joined" 9 times.
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage();
				failOnErrorMessage(msg);
			} while (!msg.isPlayerListMessageSelected());
			assertEquals(playerIdAfterRejoin[i], msg.getPlayerListMessage().getValue().getPlayerId().getValue().longValue());
			assertEquals(PlayerListNotificationEnumType.EnumType.playerListNew, msg.getPlayerListMessage().getValue().getPlayerListNotification().getValue());
		}

		for (int i = 0; i < 9; i++) {
			// Wait for start event.
			do {
				msg = receiveMessage(s[i]);
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
			sendMessage(msg, s[i]);
		}
	
		for (int i = 0; i < 9; i++) {
			// Wait for game start. This may take a while, because rejoin is performed at the beginning of the next hand.
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.isGameStartMessageSelected());
	
			// Check whether we got all necessary data to rejoin.
			assertEquals(gameId, msg.getGameStartMessage().getValue().getGameId().getValue().longValue());
			assertTrue(msg.getGameStartMessage().getValue().getGameStartMode().isGameStartModeRejoinSelected());
			GameStartModeRejoin rejoinData = msg.getGameStartMessage().getValue().getGameStartMode().getGameStartModeRejoin();
			// We left at the first hand.
			assertTrue(rejoinData.getHandNum().getValue().longValue() >= 1);
			// 10 Players should now be active again.
			assertEquals(10, rejoinData.getRejoinPlayerData().size());
		}

		// The remaining player should have received 9 "player id changed".
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage();
				failOnErrorMessage(msg);
			} while (!msg.isPlayerIdChangedMessageSelected());
			assertEquals(playerId[i], msg.getPlayerIdChangedMessage().getValue().getOldPlayerId().getValue().longValue());
			assertEquals(playerIdAfterRejoin[i], msg.getPlayerIdChangedMessage().getValue().getNewPlayerId().getValue().longValue());
		}

		// Everyone should receive a "hand start message" now.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.isHandStartMessageSelected());
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.isHandStartMessageSelected());
		}

		// The game should continue to the end.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.isEndOfGameMessageSelected());

		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
		Thread.sleep(2000);

		// Check database entry for the game.
		ResultSet countAfterResult = dbStatement.executeQuery("SELECT COUNT(idgame) FROM game");
		countAfterResult.first();
		long countAfter = countAfterResult.getLong(1);
		assertEquals(countBefore + 1, countAfter);

		// Select the latest game.
		ResultSet gameResult = dbStatement.executeQuery("SELECT idgame, name, start_time, end_time FROM game WHERE start_time = (SELECT MAX(start_time) from game)");
		gameResult.first();
		long idgame = gameResult.getLong(1);

		// Check database entries for the players in the game.
		// There should be exactly 10 entries, just as usual.
		ResultSet gamePlayerResult = dbStatement.executeQuery("SELECT COUNT(*) FROM game_has_player WHERE game_idgame = " + idgame);
		gamePlayerResult.first();
		assertEquals(10, gamePlayerResult.getLong(1));
		// Each player should have a place in the range 1..10
		ResultSet winnerResult = dbStatement.executeQuery(
				"SELECT place FROM game_has_player LEFT JOIN player_login on (game_has_player.player_idplayer = player_login.id) WHERE game_idgame = " + idgame);
		winnerResult.first();
		for (int i = 0; i < 9; i++) {
			assertTrue(winnerResult.getLong(1) >= 1);
			assertTrue(winnerResult.getLong(1) <= 10);
			winnerResult.next();
		}
	}
}
