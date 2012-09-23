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
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.net.Socket;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.NetGameType;
import de.pokerth.protocol.ProtoBuf.NetPlayerState;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;
import de.pokerth.protocol.ProtoBuf.StartEventAckMessage;


public class SeatStateTest extends TestBase {

	@Test
	public void testSeatState() throws Exception {
		long firstPlayerId = userInit();

		Collection<Integer> l = new ArrayList<Integer>();
		String gameName = AuthUser + " run normal game for seatState";
		NetGameInfo gameInfo = createGameInfo(NetGameType.normalGame, 5, 7, 5, EndRaiseMode.doubleBlinds, 0, 200, gameName, l, 10, 0, 2, 10000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				"",
				false));

		// Wait for join game ack.
		PokerTHMessage msg;
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasJoinGameAckMessage() && !msg.hasJoinGameFailedMessage());
		if (!msg.hasJoinGameAckMessage()) {
			fail("Could not create game!");
		}
		int gameId = msg.getJoinGameAckMessage().getGameId();

		// Let 9 additional clients join.
		Socket s[] = new Socket[9];
		int playerId[] = new int[9];
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password);
			sendMessage(joinGameRequestMsg(gameId, "", false), s[i]);
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasJoinGameAckMessage() && !msg.hasJoinGameFailedMessage());
			if (!msg.hasJoinGameAckMessage()) {
				fail("Could not join game!");
			}
		}

		// Server should automatically send start event.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasStartEventMessage());
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

		// Wait for game start message.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasGameStartInitialMessage());
		assertEquals(gameId, msg.getGameStartInitialMessage().getGameId());
		Collection<Integer> seats = msg.getGameStartInitialMessage().getPlayerSeatsList();
		assertEquals(10, seats.size());
		int firstPlayerPos = 0;
		for (Iterator<Integer> it = seats.iterator(); it.hasNext(); ) {
			int seat = it.next();
			if (seat == firstPlayerId)
				break;
			firstPlayerPos++;
		}

		// Wait for first seat state list.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasHandStartMessage());
		Collection<NetPlayerState> seatStates = msg.getHandStartMessage().getSeatStatesList();

		// Check whether the correct default seat states are sent.
		assertEquals(10, seatStates.size());
		for (Iterator<NetPlayerState> it = seatStates.iterator(); it.hasNext(); ) {
			NetPlayerState state = it.next();
			assertEquals(NetPlayerState.netPlayerStateNormal, state);
		}
		// All other players leave (and are in autofold state then).
		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
		// Wait for next seat state list.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
			assertTrue(!msg.hasEndOfGameMessage());
		} while (!msg.hasHandStartMessage());
		seatStates = msg.getHandStartMessage().getSeatStatesList();

		// Check whether the correct seat states are sent.
		assertEquals(10, seatStates.size());
		int stateNormalCounter = 0;
		int stateInactiveCounter = 0;
		int seatPos = 0;
		for (Iterator<NetPlayerState> it = seatStates.iterator(); it.hasNext(); ) {
			NetPlayerState state = it.next();
			assertTrue(NetPlayerState.netPlayerStateNoMoney != state);
			if (NetPlayerState.netPlayerStateNormal == state) {
				assertEquals(firstPlayerPos, seatPos);
				stateNormalCounter++;
			}
			if (NetPlayerState.netPlayerStateSessionInactive == state) {
				stateInactiveCounter++;
			}
			seatPos++;
		}
		assertEquals(1, stateNormalCounter);
		assertEquals(9, stateInactiveCounter);
	}
}
