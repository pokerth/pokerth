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
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.Statement;

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import pokerth_protocol.*;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;

import org.junit.Before;

import java.io.File;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import pokerth_protocol.MyActionRequestMessage.MyActionRequestMessageSequenceType;
import pokerth_protocol.StartEventAckMessage.StartEventAckMessageSequenceType;


public class RunRankingGameTest extends TestBase {

	Connection dbConn;

	@Before
	public void dbInit() throws Exception {
		String configFileName = System.getProperty("user.home") + "/.pokerth/config.xml";
		File file = new File(configFileName);
		DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
		DocumentBuilder db = dbf.newDocumentBuilder();
		Document doc = db.parse(file);
		doc.getDocumentElement().normalize();
		Element configNode = (Element)doc.getElementsByTagName("Configuration").item(0);

		Element dbAddressNode = (Element)configNode.getElementsByTagName("DBServerAddress").item(0);
		String dbAddress = dbAddressNode.getAttribute("value");

		Element dbUserNode = (Element)configNode.getElementsByTagName("DBServerUser").item(0);
		String dbUser = dbUserNode.getAttribute("value");

		Element dbPasswordNode = (Element)configNode.getElementsByTagName("DBServerPassword").item(0);
		String dbPassword = dbPasswordNode.getAttribute("value");

		Element dbNameNode = (Element)configNode.getElementsByTagName("DBServerDatabaseName").item(0);
		String dbName = dbNameNode.getAttribute("value");

		final String dbUrl = "jdbc:mysql://" + dbAddress + ":3306/" + dbName;
		Class.forName("com.mysql.jdbc.Driver").newInstance ();
		dbConn = DriverManager.getConnection(dbUrl, dbUser, dbPassword);
	}

	@Test
	public void testRunRankingGame() throws Exception {

		Statement dbStatement = dbConn.createStatement();
		ResultSet countBeforeResult = dbStatement.executeQuery("SELECT COUNT(idgame) FROM game");
		countBeforeResult.first();
		long countBefore = countBeforeResult.getLong(1);

		long firstPlayerId = userInit();

		Collection<Integer> l = new ArrayList<Integer>();
		String gameName = AuthUser + " run ranking game";
		NetGameInfo gameInfo = createGameInfo(5, EndRaiseModeEnumType.EnumType.doubleBlinds, 0, 50, gameName, l, 10, 0, 11, 10000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				NetGameTypeEnumType.EnumType.rankingGame,
				5,
				7,
				"",
				false));

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
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password);
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
		String dbGameName = gameResult.getString(2);
		assertEquals(gameName, dbGameName);
		java.sql.Timestamp gameStart = gameResult.getTimestamp(3);
		java.sql.Timestamp gameEnd = gameResult.getTimestamp(4);
		assertTrue(gameEnd.after(gameStart));
		// Do not consider daylight saving time, just calculate the raw difference.
		long gameDurationMsec = gameEnd.getTime() - gameStart.getTime();
		assertTrue(gameDurationMsec > 10 * 1000); // game duration should be larger than 10 seconds.
		assertTrue(gameDurationMsec < 60 * 60 * 1000); // game duration should be smaller than 1 hour.

		// Check database entries for the players in the game.
		ResultSet gamePlayerResult = dbStatement.executeQuery("SELECT COUNT(DISTINCT player_idplayer) FROM game_has_player WHERE game_idgame = " + idgame);
		gamePlayerResult.first();
		assertEquals(gamePlayerResult.getLong(1), 10);
		// The one who always went all in should have won!
		ResultSet winnerResult = dbStatement.executeQuery(
				"SELECT place FROM game_has_player LEFT JOIN player_login on (game_has_player.player_idplayer = player_login.id) WHERE game_idgame = " + idgame + " AND username = '" + AuthUser + "'");
		winnerResult.first();
		assertEquals(winnerResult.getLong(1), 1);
	}
}
