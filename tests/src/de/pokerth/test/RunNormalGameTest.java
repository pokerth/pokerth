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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.fail;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.GameManagementMessage.GameManagementMessageType;
import de.pokerth.protocol.ProtoBuf.GameMessage.GameMessageType;
import de.pokerth.protocol.ProtoBuf.LobbyMessage.LobbyMessageType;
import de.pokerth.protocol.ProtoBuf.GameManagementMessage;
import de.pokerth.protocol.ProtoBuf.GameMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.PlayerResult;
import de.pokerth.protocol.ProtoBuf.StartEventAckMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.NetGameType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;
import de.pokerth.protocol.ProtoBuf.StartEventMessage.StartEventType;
import de.pokerth.protocol.ProtoBuf.StartEventMessage;


public class RunNormalGameTest extends TestBase {

	@Test
	public void testRunNormalGameAsGuest() throws Exception {
		guestInit();

		Collection<Integer> l = new ArrayList<Integer>();
		NetGameInfo gameInfo = createGameInfo(NetGameType.normalGame, 10, 5, 5, EndRaiseMode.doubleBlinds, 0, 100, GuestUser + " run normal game", l, 10, 0, 2, 2000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				"",
				false));

		PokerTHMessage msg;

		// Waiting for player list update.
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage() || msg.getLobbyMessage().getMessageType() != LobbyMessageType.Type_PlayerListMessage) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		// Game list update (new game)
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListNewMessage() || msg.getLobbyMessage().getMessageType() != LobbyMessageType.Type_GameListNewMessage) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		// Join game ack.
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasJoinGameAckMessage() || msg.getLobbyMessage().getMessageType() != LobbyMessageType.Type_JoinGameAckMessage) {
			failOnErrorMessage(msg);
			fail("Could not create game!");
		}
		int gameId = msg.getLobbyMessage().getJoinGameAckMessage().getGameId();

		// Game list update (player joined).
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListPlayerJoinedMessage() || msg.getLobbyMessage().getMessageType() != LobbyMessageType.Type_GameListPlayerJoinedMessage) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		StartEventMessage startEvent = StartEventMessage.newBuilder()
				.setStartEventType(StartEventType.startEvent)
				.setFillWithComputerPlayers(true)
				.build();

		GameManagementMessage gameManagement = GameManagementMessage.newBuilder()
				.setMessageType(GameManagementMessageType.Type_StartEventMessage)
				.setStartEventMessage(startEvent)
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

		// Now the computer players should join.
		for (int i = 0; i < 9; i++) {
			msg = receiveMessage();
			if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasGamePlayerJoinedMessage() || msg.getGameMessage().getGameManagementMessage().getMessageType() != GameManagementMessageType.Type_GamePlayerJoinedMessage) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			msg = receiveMessage();
			if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListPlayerJoinedMessage() || msg.getLobbyMessage().getMessageType() != LobbyMessageType.Type_GameListPlayerJoinedMessage) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
		}

		// Server should confirm start event.
		msg = receiveMessage();
		if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasStartEventMessage() || msg.getGameMessage().getGameManagementMessage().getMessageType() != GameManagementMessageType.Type_StartEventMessage) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
		// Acknowledge start event.
		StartEventAckMessage startAck = StartEventAckMessage.newBuilder()
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

		// Game list update (game now running).
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListUpdateMessage() || msg.getLobbyMessage().getMessageType() != LobbyMessageType.Type_GameListUpdateMessage) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		msg = receiveMessage();
		if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasGameStartInitialMessage() || msg.getGameMessage().getGameManagementMessage().getMessageType() != GameManagementMessageType.Type_GameStartInitialMessage) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		long lastPlayerMoney = 0;
		do {
			msg = receiveMessage();
			if (msg.hasGameMessage() && msg.getGameMessage().hasGameEngineMessage() && msg.getGameMessage().getGameEngineMessage().hasEndOfHandHideCardsMessage()) {
				lastPlayerMoney = msg.getGameMessage().getGameEngineMessage().getEndOfHandHideCardsMessage().getPlayerMoney();
			} else if (msg.getGameMessage().getGameEngineMessage().hasEndOfHandShowCardsMessage()) {
				Collection<PlayerResult> result = msg.getGameMessage().getGameEngineMessage().getEndOfHandShowCardsMessage().getPlayerResultsList();
				assertFalse(result.isEmpty());
				long maxPlayerMoney = 0;
				for (Iterator<PlayerResult> it = result.iterator(); it.hasNext(); ) {
					PlayerResult r = it.next();
					int curMoney = r.getPlayerMoney();
					if (curMoney > maxPlayerMoney) {
						maxPlayerMoney = curMoney;
					}
				}
				lastPlayerMoney = maxPlayerMoney;
			}
		} while (
				msg.hasGameMessage()
				&& ((msg.getGameMessage().hasGameEngineMessage() &&
				(msg.getGameMessage().getGameEngineMessage().hasHandStartMessage()
						|| msg.getGameMessage().getGameEngineMessage().hasDealFlopCardsMessage()
						|| msg.getGameMessage().getGameEngineMessage().hasDealRiverCardMessage()
						|| msg.getGameMessage().getGameEngineMessage().hasDealTurnCardMessage()
						|| msg.getGameMessage().getGameEngineMessage().hasPlayersTurnMessage()
						|| msg.getGameMessage().getGameEngineMessage().hasPlayersActionDoneMessage()
						|| msg.getGameMessage().getGameEngineMessage().hasEndOfHandHideCardsMessage()
						|| msg.getGameMessage().getGameEngineMessage().hasEndOfHandShowCardsMessage()
						|| msg.getGameMessage().getGameEngineMessage().hasAllInShowCardsMessage()))
				|| ((msg.getGameMessage().hasGameManagementMessage() &&
					msg.getGameMessage().getGameManagementMessage().hasTimeoutWarningMessage()))));
		if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasEndOfGameMessage() || msg.getGameMessage().getGameManagementMessage().getMessageType() != GameManagementMessageType.Type_EndOfGameMessage) {
			fail("No end of game received.");
		}
		// Last player money should be sum of all money.
		assertEquals(2000 * 10, lastPlayerMoney);

		// Now the computer players should leave.
		for (int i = 0; i < 9; i++) {
			msg = receiveMessage();
			if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasGamePlayerLeftMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
			msg = receiveMessage();
			if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListPlayerLeftMessage()) {
				failOnErrorMessage(msg);
				fail("Invalid message.");
			}
		}
	}
}
