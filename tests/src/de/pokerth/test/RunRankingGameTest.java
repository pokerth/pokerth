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
import java.sql.ResultSet;
import java.sql.Statement;

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.MyActionRequestMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.NetPlayerAction;
import de.pokerth.protocol.ProtoBuf.PlayerResult;
import de.pokerth.protocol.ProtoBuf.StartEventAckMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.NetGameType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;


public class RunRankingGameTest extends TestBase {

	@Test
	public void testRunRankingGame() throws Exception {

		Statement dbStatement = dbConn.createStatement();
		ResultSet countBeforeResult = dbStatement.executeQuery("SELECT COUNT(idgame) FROM game");
		countBeforeResult.first();
		long countBefore = countBeforeResult.getLong(1);

		int firstPlayerId = userInit();

		// Waiting for player list update.
		PokerTHMessage msg;
		msg = receiveMessage();
		if (!msg.hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		Collection<Integer> l = new ArrayList<Integer>();
		String gameName = AuthUser + " run ranking game";
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
		long playerId[] = new long[9];
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password);
			// Waiting for player list update.
			do {
				msg = receiveMessage(s[i]);
			} while (msg.hasGameListNewMessage() || msg.hasGamePlayerJoinedMessage());
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

		int handNum = 0;
		int lastPlayerMoney = 0;
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
			if (msg.hasHandStartMessage()) {
				handNum++;
				// Cards should be encrypted for registered users.
				assertTrue(msg.getHandStartMessage().hasEncryptedCards());
				byte[] encData = msg.getHandStartMessage().getEncryptedCards().toByteArray();
				byte[] cardData = decryptCards(AuthPassword, encData);
				int size = cardData.length;
				while (size > 0 && cardData[size-1] == 0) {
					size--;
				}
				String cardStr = new String(cardData, 0, size);
				String[] cardTok = cardStr.split("\\s");
				// First token is player id.
				assertEquals(String.valueOf(firstPlayerId), cardTok[0]);
				// Second token is game id.
				assertEquals(String.valueOf(gameId), cardTok[1]);
				// Third token is hand num.
				assertEquals(String.valueOf(handNum), cardTok[2]);
				// Fourth and fifth tokens are cards.
				int card1 = Integer.valueOf(cardTok[3]);
				int card2 = Integer.valueOf(cardTok[4]);
				assertTrue(card1 < 52);
				assertTrue(card1 >= 0);
				assertTrue(card2 < 52);
				assertTrue(card2 >= 0);
			}
			else if (msg.hasPlayersTurnMessage()) {
				if (msg.getPlayersTurnMessage().getPlayerId() == firstPlayerId) {
					MyActionRequestMessage myRequest = MyActionRequestMessage.newBuilder()
						.setGameId(gameId)
						.setGameState(msg.getPlayersTurnMessage().getGameState())
						.setHandNum(handNum)
						.setMyAction(NetPlayerAction.netActionAllIn)
						.setMyRelativeBet(0)
						.build();
					PokerTHMessage outMsg = PokerTHMessage.newBuilder()
						.setMessageType(PokerTHMessageType.Type_MyActionRequestMessage)
						.setMyActionRequestMessage(myRequest)
						.build();
					sendMessage(outMsg);
				}
			}
			for (int i = 0; i < 9; i++) {
				while (s[i].getInputStream().available() > 0) {
					PokerTHMessage inMsg = receiveMessage(s[i]);
					failOnErrorMessage(inMsg);
					if (inMsg.hasPlayersTurnMessage()) {
						if (inMsg.getPlayersTurnMessage().getPlayerId() == playerId[i]) {

							MyActionRequestMessage myRequest = MyActionRequestMessage.newBuilder()
								.setGameId(gameId)
								.setGameState(inMsg.getPlayersTurnMessage().getGameState())
								.setHandNum(handNum)
								.setMyAction(NetPlayerAction.netActionFold)
								.setMyRelativeBet(0)
								.build();
							PokerTHMessage outMsg = PokerTHMessage.newBuilder()
								.setMessageType(PokerTHMessageType.Type_MyActionRequestMessage)
								.setMyActionRequestMessage(myRequest)
								.build();
							sendMessage(outMsg, s[i]);
						}
					}
					else if (inMsg.hasEndOfHandHideCardsMessage()) {
						lastPlayerMoney = inMsg.getEndOfHandHideCardsMessage().getPlayerMoney();
					} else if (inMsg.hasEndOfHandShowCardsMessage()) {
						Collection<PlayerResult> result = inMsg.getEndOfHandShowCardsMessage().getPlayerResultsList();
						assertFalse(result.isEmpty());
						int maxPlayerMoney = 0;
						for (Iterator<PlayerResult> it = result.iterator(); it.hasNext(); ) {
							PlayerResult r = it.next();
							int curMoney = r.getPlayerMoney();
							if (curMoney > maxPlayerMoney) {
								maxPlayerMoney = curMoney;
							}
						}
						lastPlayerMoney = maxPlayerMoney;
					}
				}
			}
		} while (!msg.hasEndOfGameMessage());

		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
		Thread.sleep(4000);

		// Last player money should be sum of all money.
		assertEquals(10000 * 10, lastPlayerMoney);

		// Check database entry for the game.
		ResultSet countAfterResult = dbStatement.executeQuery("SELECT COUNT(idgame) FROM game");
		countAfterResult.first();
		long countAfter = countAfterResult.getLong(1);
		assertEquals(countBefore + 1, countAfter);

		// Select the latest game.
		ResultSet gameResult = dbStatement.executeQuery("SELECT idgame, name, start_time, end_time FROM game WHERE start_time = (SELECT MAX(start_time) from game)");
		gameResult.first();
		long idgame = gameResult.getLong(1);
		String dbGameName = gameResult.getString(2);
		assertEquals(dbGameName, gameName);
		java.sql.Timestamp gameStart = gameResult.getTimestamp(3);
		java.sql.Timestamp gameEnd = gameResult.getTimestamp(4);
		assertTrue(gameEnd.after(gameStart));
		// Do not consider daylight saving time, just calculate the raw difference.
		long gameDurationMsec = gameEnd.getTime() - gameStart.getTime();
		assertTrue(gameDurationMsec > 10 * 1000); // game duration should be larger than 10 seconds.
		assertTrue(gameDurationMsec < 60 * 60 * 1000); // game duration should be smaller than 1 hour.

		// Check database entries for the players in the game.
		ResultSet gamePlayerResult = dbStatement.executeQuery("SELECT COUNT(*) FROM game_has_player WHERE game_idgame = " + idgame);
		gamePlayerResult.first();
		assertEquals(10, gamePlayerResult.getLong(1));
		// The one who always went all in should have won!
		ResultSet winnerResult = dbStatement.executeQuery(
				"SELECT place FROM game_has_player LEFT JOIN player_login on (game_has_player.player_idplayer = player_login.id) WHERE game_idgame = " + idgame + " AND username = '" + AuthUser + "'");
		winnerResult.first();
		assertEquals(1, winnerResult.getLong(1));
	}
}
