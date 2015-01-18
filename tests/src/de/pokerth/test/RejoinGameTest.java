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

import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.GameManagementMessage;
import de.pokerth.protocol.ProtoBuf.GameManagementMessage.GameManagementMessageType;
import de.pokerth.protocol.ProtoBuf.GameMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.PlayerListMessage.PlayerListNotification;
import de.pokerth.protocol.ProtoBuf.StartEventAckMessage;
import de.pokerth.protocol.ProtoBuf.GameMessage.GameMessageType;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.NetGameType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;
import de.pokerth.protocol.ProtoBuf.StartEventMessage.StartEventType;


public class RejoinGameTest extends TestBase {

	@Test
	public void testRejoinGame() throws Exception {

		Guid firstPlayerSession = new Guid();
		int firstPlayerId = userInit(sock, AuthUser, AuthPassword, null, firstPlayerSession);

		// Waiting for player list update.
		PokerTHMessage msg;
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		Collection<Integer> l = new ArrayList<Integer>();
		String gameName = AuthUser + " rejoin game";
		NetGameInfo gameInfo = createGameInfo(NetGameType.normalGame, 5, 7, 5, EndRaiseMode.doubleBlinds, 0, 50, gameName, l, 10, 0, 11, 10000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				"",
				false));

		// Game list update (new game)
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListNewMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		// Join game ack.
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || msg.getLobbyMessage().hasJoinGameAckMessage()) {
			failOnErrorMessage(msg);
			fail("Could not create game!");
		}
		int gameId = msg.getLobbyMessage().getJoinGameAckMessage().getGameId();

		// Game list update (player joined).
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || msg.getLobbyMessage().hasGameListPlayerJoinedMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		// Let 9 additional clients join.
		Socket s[] = new Socket[9];
		int playerId[] = new int[9];
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password);
			// Waiting for player list update.
			do {
				msg = receiveMessage(s[i]);
			} while ((msg.hasLobbyMessage() && (msg.getLobbyMessage().hasGameListNewMessage() || msg.getLobbyMessage().hasGameListPlayerJoinedMessage()))
					|| (msg.hasGameMessage() && msg.getGameMessage().hasGameManagementMessage() && msg.getGameMessage().getGameManagementMessage().hasGamePlayerJoinedMessage()));
			if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			sendMessage(joinGameRequestMsg(gameId, "", false), s[i]);
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!(msg.hasLobbyMessage() && (msg.getLobbyMessage().hasJoinGameAckMessage() || msg.getLobbyMessage().hasJoinGameFailedMessage())));
			if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasJoinGameAckMessage()) {
				fail("User " + username + " could not join ranking game.");
			}

			// The player should have joined the game.
			msg = receiveMessage();
			if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			msg = receiveMessage();
			if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasGamePlayerJoinedMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			msg = receiveMessage();
			if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListPlayerJoinedMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
		}

		// Server should automatically send start event.
		msg = receiveMessage();
		if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasStartEventMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasStartEventMessage());
		}
		// Acknowledge start event.
		StartEventAckMessage startAck = StartEventAckMessage.newBuilder()
			.build();

		GameManagementMessage gameManagement = GameManagementMessage.newBuilder()
				.setMessageType(GameManagementMessageType.Type_StartEventAckMessage)
				.setStartEventAckMessage(startAck)
				.build();

		GameMessage game = GameMessage.newBuilder()
				.setGameId(gameId)
				.setMessageType(GameMessageType.Type_GameManagementMessage)
				.setGameManagementMessage(gameManagement)
				.build();
			
		msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_GameMessage)
				.setGameMessage(game)
				.build();

		sendMessage(msg);
		for (int i = 0; i < 9; i++) {
			sendMessage(msg, s[i]);
		}

		// Game list update (game now running).
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListUpdateMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		msg = receiveMessage();
		if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasGameStartInitialMessage()) {
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
		} while (!msg.hasGameMessage() || !msg.getGameMessage().hasGameEngineMessage() || !msg.getGameMessage().getGameEngineMessage().hasHandStartMessage());

		// Leave the game by closing the socket.
		sock.close();
		// No rejoin game id set yet.
		assertEquals(0, lastRejoinGameId);

		// All other players should have received "player left".
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage());
			assertEquals(firstPlayerId, msg.getLobbyMessage().getPlayerListMessage().getPlayerId());
			assertEquals(PlayerListNotification.playerListLeft, msg.getLobbyMessage().getPlayerListMessage().getPlayerListNotification());
		}

		sock = new Socket("localhost", 7234);
		// Reconnect to the server.
		int firstPlayerIdAfterRejoin = userInit(sock, AuthUser, AuthPassword, null, firstPlayerSession);
		assertEquals(gameId, lastRejoinGameId);
		// Waiting for player list update.
		do {
			msg = receiveMessage();
		} while ((msg.hasLobbyMessage() && (msg.getLobbyMessage().hasGameListNewMessage() || msg.getLobbyMessage().hasGameListPlayerJoinedMessage()))
				|| (msg.hasGameMessage() && msg.getGameMessage().hasGameManagementMessage() && msg.getGameMessage().getGameManagementMessage().hasGamePlayerJoinedMessage()));
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
		// Rejoin the game.
		sendMessage(rejoinGameRequestMsg(gameId, false));
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!(msg.hasLobbyMessage() && (msg.getLobbyMessage().hasJoinGameAckMessage() || msg.getLobbyMessage().hasJoinGameFailedMessage())));
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasJoinGameAckMessage()) {
			fail("User " + AuthUser + " could not rejoin normal game.");
		}

		// All other players should have received "player joined".
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage());
			assertEquals(firstPlayerIdAfterRejoin, msg.getLobbyMessage().getPlayerListMessage().getPlayerId());
			assertEquals(PlayerListNotification.playerListNew, msg.getLobbyMessage().getPlayerListMessage().getPlayerListNotification());
		}

		// Wait for start event.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasStartEventMessage());

		assertEquals(gameId, msg.getGameMessage().getGameId());
		assertEquals(StartEventType.rejoinEvent, msg.getGameMessage().getGameManagementMessage().getStartEventMessage().getStartEventType());

		// Acknowledge start event.
		startAck = StartEventAckMessage.newBuilder()
				.build();

		gameManagement = GameManagementMessage.newBuilder()
				.setMessageType(GameManagementMessageType.Type_StartEventAckMessage)
				.setStartEventAckMessage(startAck)
				.build();

		game = GameMessage.newBuilder()
				.setGameId(gameId)
				.setMessageType(GameMessageType.Type_GameManagementMessage)
				.setGameManagementMessage(gameManagement)
				.build();
			
		msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_GameMessage)
				.setGameMessage(game)
				.build();

		sendMessage(msg);

		// Wait for game start. This may take a while, because rejoin is performed at the beginning of the next hand.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasGameStartRejoinMessage());

		// Check whether we got all necessary data to rejoin.
		assertEquals(gameId, msg.getGameMessage().getGameId());
		// We left at the first hand.
		assertEquals(1, msg.getGameMessage().getGameManagementMessage().getGameStartRejoinMessage().getHandNum());
		// 10 Players should now be active again.
		assertEquals(10, msg.getGameMessage().getGameManagementMessage().getGameStartRejoinMessage().getRejoinPlayerDataCount());

		// All other players should have received "player id changed".
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasPlayerIdChangedMessage());
			assertEquals(firstPlayerId, msg.getGameMessage().getGameManagementMessage().getPlayerIdChangedMessage().getOldPlayerId());
			assertEquals(firstPlayerIdAfterRejoin, msg.getGameMessage().getGameManagementMessage().getPlayerIdChangedMessage().getNewPlayerId());
		}

		// Everyone should receive a "hand start message" now.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasGameMessage() || !msg.getGameMessage().hasGameEngineMessage() || !msg.getGameMessage().getGameEngineMessage().hasHandStartMessage());
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasGameMessage() || !msg.getGameMessage().hasGameEngineMessage() || !msg.getGameMessage().getGameEngineMessage().hasHandStartMessage());
		}

		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
	}
}
