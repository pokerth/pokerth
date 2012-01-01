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
import static org.junit.Assert.fail;

import java.net.Socket;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

import org.junit.Test;

import pokerth_protocol.InitialNonZeroAmountOfMoney;
import pokerth_protocol.NetGameInfo;
import pokerth_protocol.NetPlayerState;
import pokerth_protocol.NonZeroId;
import pokerth_protocol.PokerTHMessage;
import pokerth_protocol.StartEventAckMessage;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;
import pokerth_protocol.StartEventAckMessage.StartEventAckMessageSequenceType;


public class SeatStateTest extends TestBase {

	@Test
	public void testSeatState() throws Exception {
		long firstPlayerId = userInit();

		Collection<InitialNonZeroAmountOfMoney> l = new ArrayList<InitialNonZeroAmountOfMoney>();
		String gameName = AuthUser + " run normal game for seatState";
		NetGameInfo gameInfo = createGameInfo(5, EndRaiseModeEnumType.EnumType.doubleBlinds, 0, 200, gameName, l, 10, 0, 2, 10000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				NetGameTypeEnumType.EnumType.normalGame,
				5,
				7,
				"",
				false));

		// Wait for join game ack.
		PokerTHMessage msg;
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.isJoinGameReplyMessageSelected());
		if (!msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected()) {
			fail("Could not create game!");
		}
		long gameId = msg.getJoinGameReplyMessage().getValue().getGameId().getValue();

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
				fail("Could not join game!");
			}
		}

		// Server should automatically send start event.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.isStartEventMessageSelected());
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

		// Wait for first seat state list.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.isHandStartMessageSelected());
		Collection<NetPlayerState> seatStates = msg.getHandStartMessage().getValue().getSeatStates();

		// Check whether the correct default seat states are sent.
		assertEquals(10, seatStates.size());
		for (Iterator<NetPlayerState> it = seatStates.iterator(); it.hasNext(); ) {
			NetPlayerState state = it.next();
			assertEquals(NetPlayerState.EnumType.playerStateNormal, state.getValue());
		}
		// All other players leave (and are in autofold state then).
		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
		// Wait for next seat state list.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
			assertTrue(!msg.isEndOfGameMessageSelected());
		} while (!msg.isHandStartMessageSelected());
		seatStates = msg.getHandStartMessage().getValue().getSeatStates();

		// Check whether the correct seat states are sent.
		assertEquals(10, seatStates.size());
		int stateNormalCounter = 0;
		int stateInactiveCounter = 0;
		for (Iterator<NetPlayerState> it = seatStates.iterator(); it.hasNext(); ) {
			NetPlayerState state = it.next();
			assertTrue(NetPlayerState.EnumType.playerStateNoMoney != state.getValue());
			if (NetPlayerState.EnumType.playerStateNormal == state.getValue()) {
				stateNormalCounter++;
			}
			if (NetPlayerState.EnumType.playerStateSessionInactive == state.getValue()) {
				stateInactiveCounter++;
			}
		}
		assertEquals(1, stateNormalCounter);
		assertEquals(9, stateInactiveCounter);
	}
}
