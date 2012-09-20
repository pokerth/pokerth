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

public class StartNormalGameTest extends TestBase {

	@Test
	public void testGameStartMessage() throws Exception {
		guestInit();

		Collection<InitialNonZeroAmountOfMoney> l = new ArrayList<InitialNonZeroAmountOfMoney>();
		NetGameInfo gameInfo = createGameInfo(10, EndRaiseModeEnumType.EnumType.doubleBlinds, 0, 100, GuestUser + " start normal game", l, 10, 0, 11, 20000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				NetGameTypeEnumType.EnumType.normalGame,
				10,
				7,
				"",
				false));

		PokerTHMessage msg;
		// Waiting for player list update.
		msg = receiveMessage();
		if (!msg.isPlayerListMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		msg = receiveMessage();
		if (!msg.isGameListMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

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

		StartEvent startEvent = new StartEvent();
		startEvent.setFillWithComputerPlayers(true);
		StartEventTypeChoiceType eventType = new StartEventTypeChoiceType();
		eventType.selectStartEvent(startEvent);
		StartEventMessageSequenceType gameStartType = new StartEventMessageSequenceType();
		gameStartType.setGameId(new NonZeroId(gameId));
		gameStartType.setStartEventType(eventType);
		StartEventMessage startMsg = new StartEventMessage();
		startMsg.setValue(gameStartType);
		msg = new PokerTHMessage();
		msg.selectStartEventMessage(startMsg);
		sendMessage(msg);

		do {
			msg = receiveMessage();
		} while (msg.isGameListMessageSelected());

		if (msg.isGameStartMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
	}

}
