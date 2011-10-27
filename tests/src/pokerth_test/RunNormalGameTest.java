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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.fail;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

import org.junit.Test;

import pokerth_protocol.NetGameInfo;
import pokerth_protocol.NonZeroId;
import pokerth_protocol.PlayerResult;
import pokerth_protocol.PokerTHMessage;
import pokerth_protocol.StartEvent;
import pokerth_protocol.StartEventAckMessage;
import pokerth_protocol.StartEventMessage;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;
import pokerth_protocol.StartEventAckMessage.StartEventAckMessageSequenceType;
import pokerth_protocol.StartEventMessage.StartEventMessageSequenceType;
import pokerth_protocol.InitialNonZeroAmountOfMoney;
import pokerth_protocol.StartEventMessage.StartEventMessageSequenceType.StartEventTypeChoiceType;


public class RunNormalGameTest extends TestBase {

	@Test
	public void testRunNormalGameAsGuest() throws Exception {
		guestInit();

		Collection<InitialNonZeroAmountOfMoney> l = new ArrayList<InitialNonZeroAmountOfMoney>();
		NetGameInfo gameInfo = createGameInfo(5, EndRaiseModeEnumType.EnumType.doubleBlinds, 0, 100, GuestUser + " run normal game", l, 10, 0, 2, 2000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				NetGameTypeEnumType.EnumType.normalGame,
				10,
				5,
				"",
				false));

		PokerTHMessage msg;

		// Waiting for player list update.
		msg = receiveMessage();
		if (!msg.isPlayerListMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

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

		// Now the computer players should join.
		for (int i = 0; i < 9; i++) {
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

		// Server should confirm start event.
		msg = receiveMessage();
		if (!msg.isStartEventMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
		// Acknowledge start event.
		StartEventAckMessageSequenceType startType = new StartEventAckMessageSequenceType();
		startType.setGameId(new NonZeroId(gameId));
		StartEventAckMessage startAck = new StartEventAckMessage();
		startAck.setValue(startType);
		msg = new PokerTHMessage();
		msg.selectStartEventAckMessage(startAck);
		sendMessage(msg);

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

		long lastPlayerMoney = 0;
		do {
			msg = receiveMessage();
			if (msg.isEndOfHandMessageSelected()) {
				if (msg.getEndOfHandMessage().getValue().getEndOfHandType().isEndOfHandHideCardsSelected()) {
					lastPlayerMoney = msg.getEndOfHandMessage().getValue().getEndOfHandType().getEndOfHandHideCards().getPlayerMoney().getValue();
				} else if (msg.getEndOfHandMessage().getValue().getEndOfHandType().isEndOfHandShowCardsSelected()) {
					Collection<PlayerResult> result = msg.getEndOfHandMessage().getValue().getEndOfHandType().getEndOfHandShowCards().getPlayerResults();
					assertFalse(result.isEmpty());
					long maxPlayerMoney = 0;
					for (Iterator<PlayerResult> it = result.iterator(); it.hasNext(); ) {
						PlayerResult r = it.next();
						long curMoney = r.getPlayerMoney().getValue();
						if (curMoney > maxPlayerMoney) {
							maxPlayerMoney = curMoney;
						}
					}
					lastPlayerMoney = maxPlayerMoney;
				}
			}
		} while (
				msg.isHandStartMessageSelected()
				|| msg.isDealFlopCardsMessageSelected()
				|| msg.isDealRiverCardMessageSelected()
				|| msg.isDealTurnCardMessageSelected()
				|| msg.isPlayersTurnMessageSelected()
				|| msg.isPlayersActionDoneMessageSelected()
				|| msg.isEndOfHandMessageSelected()
				|| msg.isAllInShowCardsMessageSelected()
				);
		if (!msg.isEndOfGameMessageSelected()) {
			fail("No end of game received.");
		}
		// Last player money should be sum of all money.
		assertEquals(2000 * 10, lastPlayerMoney);

		// Now the computer players should leave.
		for (int i = 0; i < 9; i++) {
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
	}
}
