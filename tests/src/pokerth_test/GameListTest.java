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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.net.Socket;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import pokerth_protocol.GameListMessage;
import pokerth_protocol.GameListNew;
import pokerth_protocol.NetGameInfo;
import pokerth_protocol.NetGameMode;
import pokerth_protocol.PokerTHMessage;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;
import pokerth_protocol.InitialNonZeroAmountOfMoney;


public class GameListTest extends TestBase {

	protected void checkGameListNewMsg(long myId, GameListNew gameListNew, NetGameMode.EnumType mode, NetGameInfo gameInfo) {
		assertEquals(NetGameMode.EnumType.gameCreated, gameListNew.getGameMode().getValue());
		assertTrue(!gameListNew.getIsPrivate());
		assertEquals(myId, gameListNew.getAdminPlayerId().getValue().longValue());
		NetGameInfo receivedGameInfo = gameListNew.getGameInfo();
		assertEquals(gameInfo.getDelayBetweenHands(), receivedGameInfo.getDelayBetweenHands());
		assertEquals(gameInfo.getEndRaiseMode().getValue(), receivedGameInfo.getEndRaiseMode().getValue());
		assertEquals(gameInfo.getEndRaiseSmallBlindValue().getValue(), receivedGameInfo.getEndRaiseSmallBlindValue().getValue());
		assertEquals(gameInfo.getFirstSmallBlind(), receivedGameInfo.getFirstSmallBlind());
		assertEquals(gameInfo.getGameName(), receivedGameInfo.getGameName());
		assertTrue(receivedGameInfo.getManualBlinds().isEmpty());
		assertEquals(gameInfo.getMaxNumPlayers(), receivedGameInfo.getMaxNumPlayers());
		assertEquals(gameInfo.getNetGameType().getValue(), receivedGameInfo.getNetGameType().getValue());
		assertEquals(gameInfo.getPlayerActionTimeout(), receivedGameInfo.getPlayerActionTimeout());
		assertEquals(gameInfo.getProposedGuiSpeed(), receivedGameInfo.getProposedGuiSpeed());
		assertTrue(gameInfo.getRaiseIntervalMode().isRaiseEveryHandsSelected());
		assertEquals(gameInfo.getStartMoney().getValue(), receivedGameInfo.getStartMoney().getValue());
	}

	@Test
	public void testGameList() throws Exception {

		long myId = guestInit();

		// Waiting for player list update.
		PokerTHMessage msg;
		msg = receiveMessage();
		assertTrue(msg.isPlayerListMessageSelected());

		// Create a new game.
		Collection<InitialNonZeroAmountOfMoney> l = new ArrayList<InitialNonZeroAmountOfMoney>();
		NetGameInfo gameInfo = createGameInfo(5, EndRaiseModeEnumType.EnumType.doubleBlinds, 0, 100, GuestUser + " game list normal game", l, 10, 0, 2, 2000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				NetGameTypeEnumType.EnumType.normalGame,
				10,
				5,
				"",
				false));

		// Game list message is sent before join game ack.
		msg = receiveMessage();
		assertTrue(msg.isGameListMessageSelected());
		GameListMessage gameListMsg = msg.getGameListMessage();
		long gameId = gameListMsg.getValue().getGameId().getValue();
		assertTrue(0 != gameListMsg.getValue().getGameId().getValue());
		assertTrue(gameListMsg.getValue().getGameListNotification().isGameListNewSelected());
		checkGameListNewMsg(
				myId,
				gameListMsg.getValue().getGameListNotification().getGameListNew(),
				NetGameMode.EnumType.gameCreated,
				gameInfo);
		assertTrue(gameListMsg.getValue().getGameListNotification().getGameListNew().getPlayerIds().isEmpty());

		// Next message is join game ack.
		msg = receiveMessage();
		assertTrue(msg.isJoinGameReplyMessageSelected());
		assertTrue(msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected());
		// Make sure game list id equals join game ack id.
		assertEquals(gameId, msg.getJoinGameReplyMessage().getValue().getGameId().getValue().longValue());

		// Next message is game list player joined.
		msg = receiveMessage();
		assertTrue(msg.isGameListMessageSelected());
		gameListMsg = msg.getGameListMessage();
		assertTrue(gameListMsg.getValue().getGameListNotification().isGameListPlayerJoinedSelected());

		// Check game list for newly connected players.
		Socket s[] = new Socket[9];
		long playerId[] = new long[9];
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password);
			msg = receiveMessage();
			assertTrue(msg.isPlayerListMessageSelected());

			do {
				msg = receiveMessage(s[i]);
			} while (msg.isPlayerListMessageSelected());
			assertTrue(msg.isGameListMessageSelected());
			gameListMsg = msg.getGameListMessage();
			assertEquals(gameId, gameListMsg.getValue().getGameId().getValue().longValue());
			assertTrue(0 != gameListMsg.getValue().getGameId().getValue());
			assertTrue(gameListMsg.getValue().getGameListNotification().isGameListNewSelected());
			checkGameListNewMsg(
					myId,
					gameListMsg.getValue().getGameListNotification().getGameListNew(),
					NetGameMode.EnumType.gameCreated,
					gameInfo);
			assertEquals(1, gameListMsg.getValue().getGameListNotification().getGameListNew().getPlayerIds().size());
			assertEquals(myId, gameListMsg.getValue().getGameListNotification().getGameListNew().getPlayerIds().iterator().next().getValue().longValue());
		}

		// Let 9 players join the game.
		for (int i = 0; i < 9; i++) {
			sendMessage(joinGameRequestMsg(gameId, "", false), s[i]);
			do {
				msg = receiveMessage(s[i]);
			} while (msg.isPlayerListMessageSelected());
			for (int j = 0; j < i; j++) {
				assertTrue(msg.isGameListMessageSelected());
				gameListMsg = msg.getGameListMessage();
				assertTrue(gameListMsg.getValue().getGameListNotification().isGameListPlayerJoinedSelected());
				assertEquals(playerId[j], gameListMsg.getValue().getGameListNotification().getGameListPlayerJoined().getPlayerId().getValue().longValue());
				msg = receiveMessage(s[i]);
			}
			failOnErrorMessage(msg);
			// Next message is join game ack.
			assertTrue(msg.isJoinGameReplyMessageSelected());
			assertTrue(msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected());
			// Make sure game list id equals join game ack id.
			assertEquals(gameId, msg.getJoinGameReplyMessage().getValue().getGameId().getValue().longValue());

			// Next message is game list player joined.
			do {
				msg = receiveMessage(s[i]);
			} while (msg.isGamePlayerMessageSelected());
			assertTrue(msg.isGameListMessageSelected());
			gameListMsg = msg.getGameListMessage();
			assertTrue(gameListMsg.getValue().getGameListNotification().isGameListPlayerJoinedSelected());
			assertEquals(playerId[i], gameListMsg.getValue().getGameListNotification().getGameListPlayerJoined().getPlayerId().getValue().longValue());
		}

		// Wait for game list update which marks start of game.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!(msg.isGameListMessageSelected() && msg.getGameListMessage().getValue().getGameListNotification().isGameListUpdateSelected()));

		assertEquals(NetGameMode.EnumType.gameStarted, msg.getGameListMessage().getValue().getGameListNotification().getGameListUpdate().getGameMode().getValue());

		// Wait for player left messages.
		for (int i = 0; i < 9; i++) {
			s[i].close();
			do {
				msg = receiveMessage();
				failOnErrorMessage(msg);
			} while (!msg.isGameListMessageSelected());
			gameListMsg = msg.getGameListMessage();
			assertTrue(gameListMsg.getValue().getGameListNotification().isGameListPlayerLeftSelected());
			assertEquals(playerId[i], gameListMsg.getValue().getGameListNotification().getGameListPlayerLeft().getPlayerId().getValue().longValue());
		}

		// Wait for game list update which marks close of game.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!(msg.isGameListMessageSelected() && msg.getGameListMessage().getValue().getGameListNotification().isGameListUpdateSelected()));

		assertEquals(NetGameMode.EnumType.gameClosed, msg.getGameListMessage().getValue().getGameListNotification().getGameListUpdate().getGameMode().getValue());
	}
}
