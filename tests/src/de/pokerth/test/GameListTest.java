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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.net.Socket;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.GameListNewMessage;
import de.pokerth.protocol.ProtoBuf.GameListPlayerJoinedMessage;
import de.pokerth.protocol.ProtoBuf.GameListPlayerLeftMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.NetGameType;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.RaiseIntervalMode;
import de.pokerth.protocol.ProtoBuf.NetGameMode;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;


public class GameListTest extends TestBase {

	protected void checkGameListNewMsg(int myId, GameListNewMessage gameListNew, NetGameMode mode, NetGameInfo gameInfo) {
		assertEquals(NetGameMode.netGameCreated, gameListNew.getGameMode());
		assertTrue(!gameListNew.getIsPrivate());
		assertEquals(myId, gameListNew.getAdminPlayerId());
		NetGameInfo receivedGameInfo = gameListNew.getGameInfo();
		assertEquals(gameInfo.getDelayBetweenHands(), receivedGameInfo.getDelayBetweenHands());
		assertEquals(gameInfo.getEndRaiseMode(), receivedGameInfo.getEndRaiseMode());
		assertEquals(gameInfo.getEndRaiseSmallBlindValue(), receivedGameInfo.getEndRaiseSmallBlindValue());
		assertEquals(gameInfo.getFirstSmallBlind(), receivedGameInfo.getFirstSmallBlind());
		assertEquals(gameInfo.getGameName(), receivedGameInfo.getGameName());
		assertTrue(receivedGameInfo.getManualBlindsCount() == 0);
		assertEquals(gameInfo.getMaxNumPlayers(), receivedGameInfo.getMaxNumPlayers());
		assertEquals(gameInfo.getNetGameType(), receivedGameInfo.getNetGameType());
		assertEquals(gameInfo.getPlayerActionTimeout(), receivedGameInfo.getPlayerActionTimeout());
		assertEquals(gameInfo.getProposedGuiSpeed(), receivedGameInfo.getProposedGuiSpeed());
		assertEquals(RaiseIntervalMode.raiseOnHandNum, gameInfo.getRaiseIntervalMode());
		assertEquals(gameInfo.getStartMoney(), receivedGameInfo.getStartMoney());
	}

	@Test
	public void testGameList() throws Exception {

		int myId = guestInit();

		// Waiting for player list update.
		PokerTHMessage msg;
		msg = receiveMessage();
		assertTrue(msg.hasLobbyMessage() || msg.getLobbyMessage().hasPlayerListMessage());

		// Create a new game.
		Collection<Integer> l = new ArrayList<Integer>();
		NetGameInfo gameInfo = createGameInfo(NetGameType.normalGame, 10, 5, 5, EndRaiseMode.doubleBlinds, 0, 100, GuestUser + " game list normal game", l, 10, 0, 2, 2000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				"",
				false));

		// Game list message is sent before join game ack.
		msg = receiveMessage();
		assertTrue(msg.hasLobbyMessage() || msg.getLobbyMessage().hasGameListNewMessage());
		GameListNewMessage gameListNewMsg = msg.getLobbyMessage().getGameListNewMessage();
		int gameId = gameListNewMsg.getGameId();
		assertTrue(0 != gameListNewMsg.getGameId());
		checkGameListNewMsg(
				myId,
				gameListNewMsg,
				NetGameMode.netGameCreated,
				gameInfo);
		assertTrue(gameListNewMsg.getPlayerIdsCount() == 0);

		// Next message is join game ack.
		msg = receiveMessage();
		assertTrue(msg.hasLobbyMessage() && msg.getLobbyMessage().hasJoinGameAckMessage());
		// Make sure game list id equals join game ack id.
		assertEquals(gameId, msg.getLobbyMessage().getJoinGameAckMessage().getGameId());

		// Next message is game list player joined.
		msg = receiveMessage();
		assertTrue(msg.hasLobbyMessage() && msg.getLobbyMessage().hasGameListPlayerJoinedMessage());
		GameListPlayerJoinedMessage gameListJoinedMsg = msg.getLobbyMessage().getGameListPlayerJoinedMessage();
		assertEquals(gameId, gameListJoinedMsg.getGameId());
		assertEquals(myId, gameListJoinedMsg.getPlayerId());

		// Check game list for newly connected players.
		Socket s[] = new Socket[9];
		long playerId[] = new long[9];
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password);
			msg = receiveMessage();
			assertTrue(msg.hasLobbyMessage() && msg.getLobbyMessage().hasPlayerListMessage());

			do {
				msg = receiveMessage(s[i]);
			} while (msg.hasLobbyMessage() && msg.getLobbyMessage().hasPlayerListMessage());
			assertTrue(msg.hasLobbyMessage() && msg.getLobbyMessage().hasGameListNewMessage());
			gameListNewMsg = msg.getLobbyMessage().getGameListNewMessage();
			assertEquals(gameId, gameListNewMsg.getGameId());
			assertTrue(0 != gameListNewMsg.getGameId());
			checkGameListNewMsg(
					myId,
					gameListNewMsg,
					NetGameMode.netGameCreated,
					gameInfo);
			assertEquals(1, gameListNewMsg.getPlayerIdsCount());
			assertEquals(myId, gameListNewMsg.getPlayerIds(0));
		}

		// Let 9 players join the game.
		for (int i = 0; i < 9; i++) {
			sendMessage(joinGameRequestMsg(gameId, "", false), s[i]);
			do {
				msg = receiveMessage(s[i]);
			} while (msg.hasLobbyMessage() && msg.getLobbyMessage().hasPlayerListMessage());
			for (int j = 0; j < i; j++) {
				assertTrue(msg.hasLobbyMessage() && msg.getLobbyMessage().hasGameListPlayerJoinedMessage());
				gameListJoinedMsg = msg.getLobbyMessage().getGameListPlayerJoinedMessage();
				assertEquals(gameId, gameListJoinedMsg.getGameId());
				assertEquals(playerId[j], gameListJoinedMsg.getPlayerId());
				msg = receiveMessage(s[i]);
			}
			failOnErrorMessage(msg);
			// Next message is join game ack.
			assertTrue(msg.hasLobbyMessage() && msg.getLobbyMessage().hasJoinGameAckMessage());
			// Make sure game list id equals join game ack id.
			assertEquals(gameId, msg.getLobbyMessage().getJoinGameAckMessage().getGameId());

			// Next message is game list player joined.
			do {
				msg = receiveMessage(s[i]);
			} while (msg.hasGameMessage() && msg.getGameMessage().hasGameManagementMessage() && msg.getGameMessage().getGameManagementMessage().hasGamePlayerJoinedMessage());
			assertTrue(msg.hasLobbyMessage() && msg.getLobbyMessage().hasGameListPlayerJoinedMessage());
			gameListJoinedMsg = msg.getLobbyMessage().getGameListPlayerJoinedMessage();
			assertEquals(gameId, gameListJoinedMsg.getGameId());
			assertEquals(playerId[i], gameListJoinedMsg.getPlayerId());
		}

		// Wait for game list update which marks start of game.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!(msg.hasLobbyMessage() && msg.getLobbyMessage().hasGameListUpdateMessage()));

		assertEquals(NetGameMode.netGameStarted, msg.getLobbyMessage().getGameListUpdateMessage().getGameMode());

		// Wait for player left messages.
		for (int i = 0; i < 9; i++) {
			s[i].close();
			do {
				msg = receiveMessage();
				failOnErrorMessage(msg);
			} while (!(msg.hasLobbyMessage() && msg.getLobbyMessage().hasGameListPlayerLeftMessage()));
			GameListPlayerLeftMessage gameListLeftMsg = msg.getLobbyMessage().getGameListPlayerLeftMessage();
			assertEquals(gameId, gameListLeftMsg.getGameId());
			assertEquals(playerId[i], gameListLeftMsg.getPlayerId());
		}

		// Wait for game list update which marks close of game.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!(msg.hasLobbyMessage() && msg.getLobbyMessage().hasGameListUpdateMessage()));

		assertEquals(NetGameMode.netGameClosed, msg.getLobbyMessage().getGameListUpdateMessage().getGameMode());
	}
}
