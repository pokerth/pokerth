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

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.net.Socket;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;
import de.pokerth.protocol.ProtoBuf.SubscriptionRequestMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.NetGameType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;
import de.pokerth.protocol.ProtoBuf.SubscriptionRequestMessage.SubscriptionAction;


public class LobbySubscriptionTest extends TestBase {

	@Test
	public void testLobbySubscription() throws Exception {
		guestInit();

		PokerTHMessage msg;
		msg = receiveMessage();
		if (!msg.hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		SubscriptionRequestMessage subscriptionRequest = SubscriptionRequestMessage.newBuilder()
			.setSubscriptionAction(SubscriptionAction.unsubscribeGameList)
			.build();
		msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_SubscriptionRequestMessage)
				.setSubscriptionRequestMessage(subscriptionRequest)
				.build();
		sendMessage(msg);

		// Create a new game.
		Collection<Integer> l = new ArrayList<Integer>();
		NetGameInfo gameInfo = createGameInfo(NetGameType.normalGame, 10, 5, 5, EndRaiseMode.doubleBlinds, 0, 100, GuestUser + " game list normal game", l, 10, 0, 2, 2000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				"",
				false));

		// No game list message should be sent by the server.
		// Next message is join game ack.
		msg = receiveMessage();
		assertTrue(msg.hasJoinGameAckMessage());
		int gameId = msg.getJoinGameAckMessage().getGameId();

		Socket s[] = new Socket[9];
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			userInit(s[i], username, password);
			sendMessage(joinGameRequestMsg(gameId, "", false), s[i]);
		}

		// No game list message should be received.
		do {
			msg = receiveMessage();
			if (msg.hasGameListNewMessage() || msg.hasPlayerListMessage()) {
				fail("Game/player list messages are switched off!");
			}
		} while (!msg.hasStartEventMessage());

		// Resubscribe game list
		subscriptionRequest = SubscriptionRequestMessage.newBuilder()
				.setSubscriptionAction(SubscriptionAction.resubscribeGameList)
				.build();
		msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_SubscriptionRequestMessage)
				.setSubscriptionRequestMessage(subscriptionRequest)
				.build();
		sendMessage(msg);

		// Next messages should player list messages for all 10 players.
		for (int i = 0; i < 10; i++) {
			msg = receiveMessage();
			assertTrue(msg.hasPlayerListMessage());
		}
		// Now there should be one game list message.
		msg = receiveMessage();
		assertTrue(msg.hasGameListNewMessage());

		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
	}
}
