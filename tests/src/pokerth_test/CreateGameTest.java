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

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import pokerth_protocol.*;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;

public class CreateGameTest extends TestBase {

	@Test
	public void testJoinGameRequestMessage() throws Exception {
		guestInit();

		Collection<InitialNonZeroAmountOfMoney> l = new ArrayList<InitialNonZeroAmountOfMoney>();
		l.add(new InitialNonZeroAmountOfMoney(250));
		l.add(new InitialNonZeroAmountOfMoney(600));
		l.add(new InitialNonZeroAmountOfMoney(1000));
		NetGameInfo gameInfo = createGameInfo(8, EndRaiseModeEnumType.EnumType.raiseByEndValue, 1000, 100, GuestUser + " create test game", l, 10, 0, 7, 2000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				NetGameTypeEnumType.EnumType.normalGame,
				20,
				7,
				GamePassword,
				false));

		PokerTHMessage msg;
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
		if (msg.isJoinGameReplyMessageSelected())
		{
			if (msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected())
			{
				assertTrue(msg.getJoinGameReplyMessage().getValue().getGameId().getValue() != 0);
				NetGameInfo receivedGameInfo = msg.getJoinGameReplyMessage().getValue().getJoinGameResult().getJoinGameAck().getGameInfo();
				assertEquals(receivedGameInfo.getDelayBetweenHands(), gameInfo.getDelayBetweenHands());
				assertEquals(receivedGameInfo.getEndRaiseMode().getValue(), gameInfo.getEndRaiseMode().getValue());
				assertEquals(receivedGameInfo.getEndRaiseSmallBlindValue().getValue(), gameInfo.getEndRaiseSmallBlindValue().getValue());
				assertEquals(receivedGameInfo.getFirstSmallBlind(), gameInfo.getFirstSmallBlind());
				assertEquals(receivedGameInfo.getGameName(), gameInfo.getGameName());
				assertEquals(receivedGameInfo.getManualBlinds(), gameInfo.getManualBlinds());
				assertEquals(receivedGameInfo.getMaxNumPlayers(), gameInfo.getMaxNumPlayers());
				assertEquals(receivedGameInfo.getNetGameType().getValue(), gameInfo.getNetGameType().getValue());
				assertEquals(receivedGameInfo.getPlayerActionTimeout(), gameInfo.getPlayerActionTimeout());
				assertEquals(receivedGameInfo.getProposedGuiSpeed(), gameInfo.getProposedGuiSpeed());
				assertEquals(receivedGameInfo.getRaiseIntervalMode().getRaiseEveryHands(), gameInfo.getRaiseIntervalMode().getRaiseEveryHands());
				assertEquals(receivedGameInfo.getRaiseIntervalMode().getRaiseEveryMinutes(), gameInfo.getRaiseIntervalMode().getRaiseEveryMinutes());
				assertEquals(receivedGameInfo.getStartMoney(), gameInfo.getStartMoney());
			}
			else
			{
				fail("Game creation failed!");
			}
		}
		else {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
	}

}
