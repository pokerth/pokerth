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
import java.util.Iterator;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;

public class CreateGameTest extends TestBase {

	@Test
	public void testJoinGameRequestMessage() throws Exception {
		guestInit();

		Collection<Integer> l = new ArrayList<Integer>();
		l.add(250);
		l.add(600);
		l.add(1000);
		NetGameInfo gameInfo = createGameInfo(NetGameInfo.NetGameType.normalGame, 20, 7, 8, EndRaiseMode.raiseByEndValue, 1000, 100, GuestUser + " create test game", l, 10, 0, 7, 2000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				GamePassword,
				false));

		PokerTHMessage msg;
		msg = receiveMessage();
		if (!msg.hasPlayerListMessage() || msg.getMessageType() != PokerTHMessageType.Type_PlayerListMessage) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		msg = receiveMessage();
		if (!msg.hasGameListNewMessage() || msg.getMessageType() != PokerTHMessageType.Type_GameListNewMessage) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		msg = receiveMessage();
		if (msg.hasJoinGameAckMessage() && msg.getMessageType() == PokerTHMessageType.Type_JoinGameAckMessage)
		{
			assertTrue(msg.getJoinGameAckMessage().getGameId() != 0);
			NetGameInfo receivedGameInfo = msg.getJoinGameAckMessage().getGameInfo();
			assertEquals(receivedGameInfo.getDelayBetweenHands(), gameInfo.getDelayBetweenHands());
			assertEquals(receivedGameInfo.getEndRaiseMode(), gameInfo.getEndRaiseMode());
			assertEquals(receivedGameInfo.getEndRaiseSmallBlindValue(), gameInfo.getEndRaiseSmallBlindValue());
			assertEquals(receivedGameInfo.getFirstSmallBlind(), gameInfo.getFirstSmallBlind());
			assertEquals(receivedGameInfo.getGameName(), gameInfo.getGameName());
			assertEquals(receivedGameInfo.getManualBlindsCount(), gameInfo.getManualBlindsCount());
			for (Iterator<Integer> rec_it = receivedGameInfo.getManualBlindsList().iterator(),
					game_it = gameInfo.getManualBlindsList().iterator();
					rec_it.hasNext() && game_it.hasNext();)
			{
				assertEquals(rec_it.next(), game_it.next());
			}
			assertEquals(receivedGameInfo.getMaxNumPlayers(), gameInfo.getMaxNumPlayers());
			assertEquals(receivedGameInfo.getNetGameType(), gameInfo.getNetGameType());
			assertEquals(receivedGameInfo.getPlayerActionTimeout(), gameInfo.getPlayerActionTimeout());
			assertEquals(receivedGameInfo.getProposedGuiSpeed(), gameInfo.getProposedGuiSpeed());
			assertEquals(receivedGameInfo.getRaiseIntervalMode(), gameInfo.getRaiseIntervalMode());
			assertEquals(receivedGameInfo.getRaiseEveryHands(), gameInfo.getRaiseEveryHands());
			assertEquals(receivedGameInfo.getRaiseEveryMinutes(), gameInfo.getRaiseEveryMinutes());
			assertEquals(receivedGameInfo.getStartMoney(), gameInfo.getStartMoney());
		}
		else {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
	}

}
