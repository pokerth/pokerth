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

import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.PlayerListMessage.PlayerListNotification;
import de.pokerth.protocol.ProtoBuf.StartEventAckMessage;
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
		if (!msg.hasPlayerListMessage()) {
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
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password);
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

		// Leave the game by closing the socket.
		sock.close();
		// No rejoin game id set yet.
		assertEquals(0, lastRejoinGameId);

		// All other players should have received "player left".
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasPlayerListMessage());
			assertEquals(firstPlayerId, msg.getPlayerListMessage().getPlayerId());
			assertEquals(PlayerListNotification.playerListLeft, msg.getPlayerListMessage().getPlayerListNotification());
		}

		sock = new Socket("localhost", 7234);
		// Reconnect to the server.
		int firstPlayerIdAfterRejoin = userInit(sock, AuthUser, AuthPassword, null, firstPlayerSession);
		assertEquals(gameId, lastRejoinGameId);
		// Waiting for player list update.
		do {
			msg = receiveMessage();
		} while (msg.hasGameListNewMessage() || msg.hasGameListPlayerJoinedMessage() || msg.hasGamePlayerJoinedMessage());
		if (!msg.hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
		// Rejoin the game.
		sendMessage(rejoinGameRequestMsg(gameId, false));
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasJoinGameAckMessage() && !msg.hasJoinGameFailedMessage());
		if (!msg.hasJoinGameAckMessage()) {
			fail("User " + AuthUser + " could not rejoin normal game.");
		}

		// All other players should have received "player joined".
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasPlayerListMessage());
			assertEquals(firstPlayerIdAfterRejoin, msg.getPlayerListMessage().getPlayerId());
			assertEquals(PlayerListNotification.playerListNew, msg.getPlayerListMessage().getPlayerListNotification());
		}

		// Wait for start event.
		do {
			msg = receiveMessage();
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
		sendMessage(msg);

		// Wait for game start. This may take a while, because rejoin is performed at the beginning of the next hand.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasGameStartRejoinMessage());

		// Check whether we got all necessary data to rejoin.
		assertEquals(gameId, msg.getGameStartRejoinMessage().getGameId());
		// We left at the first hand.
		assertEquals(1, msg.getGameStartRejoinMessage().getHandNum());
		// 10 Players should now be active again.
		assertEquals(10, msg.getGameStartRejoinMessage().getRejoinPlayerDataCount());

		// All other players should have received "player id changed".
		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasPlayerIdChangedMessage());
			assertEquals(firstPlayerId, msg.getPlayerIdChangedMessage().getOldPlayerId());
			assertEquals(firstPlayerIdAfterRejoin, msg.getPlayerIdChangedMessage().getNewPlayerId());
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

		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
	}
}
