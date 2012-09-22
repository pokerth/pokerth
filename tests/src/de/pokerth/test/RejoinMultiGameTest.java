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

package de.pokerth.test;

import java.net.Socket;

import static org.junit.Assert.*;

import java.sql.ResultSet;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.StartEventAckMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.NetGameType;
import de.pokerth.protocol.ProtoBuf.PlayerListMessage.PlayerListNotification;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;
import de.pokerth.protocol.ProtoBuf.StartEventMessage.StartEventType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;


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
		if (!msg.hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		Collection<Integer> l = new ArrayList<Integer>();
		String gameName = AuthUser + " rejoin game";
		NetGameInfo gameInfo = createGameInfo(NetGameType.rankingGame, 5, 7, 5, EndRaiseMode.doubleBlinds, 0, 50, gameName, l, 10, 0, 11, 10000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				"",
				false));

		// Game list update (new game)
		msg = receiveMessage();
		if (!msg.hasGameListNewMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		// Join game ack.
		msg = receiveMessage();
		if (!msg.hasJoinGameAckMessage()) {
			failOnErrorMessage(msg);
			fail("Could not create game!");
		}
		int gameId = msg.getJoinGameAckMessage().getGameId();

		// Game list update (player joined).
		msg = receiveMessage();
		if (!msg.hasGameListPlayerJoinedMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		// Let 9 additional clients join.
		Socket s[] = new Socket[9];
		int playerId[] = new int[9];
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
			} while (msg.hasGameListNewMessage() || msg.hasGameListPlayerJoinedMessage() || msg.hasGamePlayerJoinedMessage());
			if (!msg.hasPlayerListMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			sendMessage(joinGameRequestMsg(gameId, "", false), s[i]);
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasJoinGameAckMessage() && !msg.hasJoinGameFailedMessage());
			if (!msg.hasJoinGameAckMessage()) {
				fail("User " + username + " could not join ranking game.");
			}

			// The player should have joined the game.
			msg = receiveMessage();
			if (!msg.hasPlayerListMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			msg = receiveMessage();
			if (!msg.hasGamePlayerJoinedMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			msg = receiveMessage();
			if (!msg.hasGameListPlayerJoinedMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
		}

		// Server should automatically send start event.
		msg = receiveMessage();
		if (!msg.hasStartEventMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasStartEventMessage());
		}
		// Acknowledge start event.
		StartEventAckMessage startAck = StartEventAckMessage.newBuilder()
			.setGameId(gameId)
			.build();
		msg = PokerTHMessage.newBuilder()
			.setMessageType(PokerTHMessageType.Type_StartEventAckMessage)
			.setStartEventAckMessage(startAck)
			.build();
		sendMessage(msg);
		for (int i = 0; i < 9; i++) {
			sendMessage(msg, s[i]);
		}

		// Game list update (game now running).
		msg = receiveMessage();
		if (!msg.hasGameListUpdateMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		msg = receiveMessage();
		if (!msg.hasGameStartInitialMessage()) {
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
		} while (!msg.hasHandStartMessage());

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
			} while (!msg.hasPlayerListMessage());
			assertEquals(playerId[i], msg.getPlayerListMessage().getPlayerId());
			assertEquals(PlayerListNotification.playerListLeft, msg.getPlayerListMessage().getPlayerListNotification());
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
			} while (msg.hasGameListNewMessage() || msg.hasGameListPlayerJoinedMessage() || msg.hasGamePlayerJoinedMessage());
			if (!msg.hasPlayerListMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			sendMessage(rejoinGameRequestMsg(gameId, false), s[i]);
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasJoinGameAckMessage() && !msg.hasJoinGameFailedMessage());
			if (!msg.hasJoinGameAckMessage()) {
				fail("User " + username + " could not rejoin ranking game.");
			}
		}
		// The remaining player should have received "player joined" 9 times.
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage();
				failOnErrorMessage(msg);
			} while (!msg.hasPlayerListMessage());
			assertEquals(playerIdAfterRejoin[i], msg.getPlayerListMessage().getPlayerId());
			assertEquals(PlayerListNotification.playerListNew, msg.getPlayerListMessage().getPlayerListNotification());
		}

		for (int i = 0; i < 9; i++) {
			// Wait for start event.
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasStartEventMessage());
	
			assertEquals(gameId, msg.getStartEventMessage().getGameId());
			assertEquals(StartEventType.rejoinEvent, msg.getStartEventMessage().getStartEventType());
	
			// Acknowledge start event.
			startAck = StartEventAckMessage.newBuilder()
				.setGameId(gameId)
				.build();
			msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_StartEventAckMessage)
				.setStartEventAckMessage(startAck)
				.build();
			sendMessage(msg, s[i]);
		}
	
		for (int i = 0; i < 9; i++) {
			// Wait for game start. This may take a while, because rejoin is performed at the beginning of the next hand.
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasGameStartRejoinMessage());
	
			// Check whether we got all necessary data to rejoin.
			assertEquals(gameId, msg.getGameStartRejoinMessage().getGameId());
			// We left at the first hand.
			assertTrue(msg.getGameStartRejoinMessage().getHandNum() >= 1);
			// 10 Players should now be active again.
			assertEquals(10, msg.getGameStartRejoinMessage().getRejoinPlayerDataCount());
		}

		// The remaining player should have received 9 "player id changed".
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage();
				failOnErrorMessage(msg);
			} while (!msg.hasPlayerIdChangedMessage());
			assertEquals(playerId[i], msg.getPlayerIdChangedMessage().getOldPlayerId());
			assertEquals(playerIdAfterRejoin[i], msg.getPlayerIdChangedMessage().getNewPlayerId());
		}

		// Everyone should receive a "hand start message" now.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasHandStartMessage());
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasHandStartMessage());
		}

		// The game should continue to the end.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasEndOfGameMessage());

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
