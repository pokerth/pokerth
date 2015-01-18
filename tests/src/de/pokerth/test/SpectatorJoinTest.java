/*  PokerTH automated tests.
	Copyright (C) 2013 Lothar May

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

import static org.junit.Assert.*;

import java.net.Socket;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.GameManagementMessage;
import de.pokerth.protocol.ProtoBuf.GameMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;
import de.pokerth.protocol.ProtoBuf.StartEventAckMessage;
import de.pokerth.protocol.ProtoBuf.GameManagementMessage.GameManagementMessageType;
import de.pokerth.protocol.ProtoBuf.GameMessage.GameMessageType;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.NetGameType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;

public class SpectatorJoinTest extends TestBase {

	@Test
	public void testSpectatorJoinGameBeforeStart() throws Exception {
		Guid firstPlayerSession = new Guid();
		int firstPlayerId = userInit(sock, AuthUser, AuthPassword, null, firstPlayerSession);

		// Waiting for player list update.
		PokerTHMessage msg;
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
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
			fail("Invalid message: " + msg.getMessageType());
		}

		// Join game ack.
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasJoinGameAckMessage()) {
			failOnErrorMessage(msg);
			fail("Could not create game!");
		}
		int gameId = msg.getLobbyMessage().getJoinGameAckMessage().getGameId();

		// Game list update (player joined).
		msg = receiveMessage();
		if (!msg.getLobbyMessage().hasGameListPlayerJoinedMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
		}

		// Let a spectator join.
		Socket spectatorSock = new Socket("localhost", 7234);
		int spectatorId = userInit(spectatorSock, "test20", "test20");

		msg = receiveMessage(spectatorSock);
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
		}
		msg = receiveMessage(spectatorSock);
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListNewMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
		}
		msg = receiveMessage(spectatorSock);
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
		}
		// Player List should also be updated for first player.
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
		}

		sendMessage(joinGameRequestMsg(gameId, "", false, true), spectatorSock);
		msg = receiveMessage(spectatorSock);
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasJoinGameAckMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
		}
		assertTrue(msg.getLobbyMessage().getJoinGameAckMessage().getSpectateOnly());

		msg = receiveMessage(spectatorSock);
		if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasGamePlayerJoinedMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
		}
		assertEquals(firstPlayerId, msg.getGameMessage().getGameManagementMessage().getGamePlayerJoinedMessage().getPlayerId());
		// TODO Spectator joined message is missing here!

		msg = receiveMessage(spectatorSock);
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListSpectatorJoinedMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
		}
		assertEquals(gameId, msg.getLobbyMessage().getGameListSpectatorJoinedMessage().getGameId());
		assertEquals(spectatorId, msg.getLobbyMessage().getGameListSpectatorJoinedMessage().getPlayerId());

		// Spectator should be visible for first player.
		msg = receiveMessage();
		if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasGameSpectatorJoinedMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
		}
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListSpectatorJoinedMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
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
			} while (msg.hasLobbyMessage() && msg.getLobbyMessage().hasPlayerListMessage());

			if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListNewMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message: " + msg.getMessageType());
			}
			assertEquals(1, msg.getLobbyMessage().getGameListNewMessage().getSpectatorIdsCount());
			assertEquals(spectatorId, msg.getLobbyMessage().getGameListNewMessage().getSpectatorIds(0));
			do {
				msg = receiveMessage(s[i]);
			} while ((msg.hasLobbyMessage() && (msg.getLobbyMessage().hasGameListNewMessage() || msg.getLobbyMessage().hasGameListPlayerJoinedMessage()))
				|| (msg.hasGameMessage() && msg.getGameMessage().hasGameManagementMessage() && msg.getGameMessage().getGameManagementMessage().hasGamePlayerJoinedMessage()));
			sendMessage(joinGameRequestMsg(gameId, "", false), s[i]);
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!(msg.hasLobbyMessage() && msg.getLobbyMessage().hasJoinGameAckMessage()) && !(msg.hasLobbyMessage() && msg.getLobbyMessage().hasJoinGameFailedMessage()));
			if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasJoinGameAckMessage()) {
				fail("User " + username + " could not join ranking game.");
			}

			// The player should have joined the game.
			msg = receiveMessage();
			if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message: " + msg.getMessageType());
			}
			msg = receiveMessage();
			if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasGamePlayerJoinedMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message: " + msg.getMessageType());
			}
			msg = receiveMessage();
			if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListPlayerJoinedMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message: " + msg.getMessageType());
			}
			// The spectator should also receive the updates.
			msg = receiveMessage(spectatorSock);
			if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message: " + msg.getMessageType());
			}
			msg = receiveMessage(spectatorSock);
			if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasGamePlayerJoinedMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message: " + msg.getMessageType());
			}
			msg = receiveMessage(spectatorSock);
			if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListPlayerJoinedMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message: " + msg.getMessageType());
			}
		}

		// Server should automatically send start event.
		msg = receiveMessage();
		if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasStartEventMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
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
		msg = receiveMessage(spectatorSock);
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListUpdateMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
		}

		msg = receiveMessage(spectatorSock);
		if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasGameStartInitialMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
		}

		// Spectator should receive a hand start message without cards.
		msg = receiveMessage(spectatorSock);
		if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameEngineMessage() || !msg.getGameMessage().getGameEngineMessage().hasHandStartMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message: " + msg.getMessageType());
		}
		assertFalse(msg.getGameMessage().getGameEngineMessage().getHandStartMessage().hasEncryptedCards());
		assertFalse(msg.getGameMessage().getGameEngineMessage().getHandStartMessage().hasPlainCards());
		assertEquals(gameId, msg.getGameMessage().getGameId());

		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
		spectatorSock.close();
	}

}
