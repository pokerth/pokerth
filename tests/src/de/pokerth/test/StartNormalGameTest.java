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

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.GameManagementMessage.GameManagementMessageType;
import de.pokerth.protocol.ProtoBuf.GameMessage.GameMessageType;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.NetGameType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;
import de.pokerth.protocol.ProtoBuf.GameManagementMessage;
import de.pokerth.protocol.ProtoBuf.GameMessage;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;
import de.pokerth.protocol.ProtoBuf.StartEventAckMessage;
import de.pokerth.protocol.ProtoBuf.StartEventMessage;
import de.pokerth.protocol.ProtoBuf.StartEventMessage.StartEventType;

public class StartNormalGameTest extends TestBase {

	@Test
	public void testGameStartMessage() throws Exception {
		guestInit();

		Collection<Integer> l = new ArrayList<Integer>();
		NetGameInfo gameInfo = createGameInfo(NetGameType.normalGame, 10, 7, 10, EndRaiseMode.doubleBlinds, 0, 100, GuestUser + " start normal game", l, 10, 0, 11, 20000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				"",
				false));

		PokerTHMessage msg;
		// Waiting for player list update.
		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasGameListNewMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		msg = receiveMessage();
		if (!msg.hasLobbyMessage() || !msg.getLobbyMessage().hasJoinGameAckMessage()) {
			failOnErrorMessage(msg);
			fail("Could not create game!");
		}
		int gameId = msg.getLobbyMessage().getJoinGameAckMessage().getGameId();

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

		do {
			msg = receiveMessage();
		} while ((msg.hasLobbyMessage() && (msg.getLobbyMessage().hasGameListNewMessage() || msg.getLobbyMessage().hasGameListPlayerJoinedMessage()))
				|| (msg.hasGameMessage() && msg.getGameMessage().hasGameManagementMessage() && msg.getGameMessage().getGameManagementMessage().hasGamePlayerJoinedMessage()));

		assertTrue(msg.hasGameMessage() && msg.getGameMessage().hasGameManagementMessage() && msg.getGameMessage().getGameManagementMessage().hasStartEventMessage() && msg.getGameMessage().getGameManagementMessage().getMessageType() == GameManagementMessageType.Type_StartEventMessage);

		StartEventAckMessage startAck = StartEventAckMessage.newBuilder()
				.build();
		gameManagement = GameManagementMessage.newBuilder()
				.setMessageType(GameManagementMessageType.Type_StartEventAckMessage)
				.setStartEventAckMessage(startAck)
				.build();
		game = GameMessage.newBuilder()
				.setMessageType(GameMessageType.Type_GameManagementMessage)
				.setGameManagementMessage(gameManagement)
				.setGameId(gameId)
				.build();
		msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_GameMessage)
				.setGameMessage(game)
				.build();
		sendMessage(msg);

		do {
			msg = receiveMessage();
		} while (msg.hasLobbyMessage() && msg.getLobbyMessage().hasGameListUpdateMessage());

		if (!msg.hasGameMessage() || !msg.getGameMessage().hasGameManagementMessage() || !msg.getGameMessage().getGameManagementMessage().hasGameStartInitialMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
	}

}
