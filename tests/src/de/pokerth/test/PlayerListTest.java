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

import java.net.Socket;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.PlayerListMessage;
import de.pokerth.protocol.ProtoBuf.PlayerListMessage.PlayerListNotification;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;


public class PlayerListTest extends TestBase {

	@Test
	public void testPlayerList() throws Exception {

		int myId = guestInit();

		// Waiting for player list update.
		PokerTHMessage msg;
		msg = receiveMessage();
		assertTrue(msg.hasPlayerListMessage());

		// This should be a "player list new" notification with correct player id.
		PlayerListMessage listMsg = msg.getPlayerListMessage();
		assertEquals(myId, listMsg.getPlayerId());
		assertEquals(PlayerListNotification.playerListNew, listMsg.getPlayerListNotification());

		Socket s[] = new Socket[9];
		long playerId[] = new long[9];
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password);

			msg = receiveMessage();
			assertTrue(msg.hasPlayerListMessage());
			listMsg = msg.getPlayerListMessage();
			// Id should be different from first id.
			assertTrue(myId != playerId[i]);
			// This should be a "player list new" notification with correct player id.
			assertEquals(playerId[i], listMsg.getPlayerId());
			assertEquals(PlayerListNotification.playerListNew, listMsg.getPlayerListNotification());

			s[i].close();

			// After the connection is closed, a "player list left" notification should be received.
			msg = receiveMessage();
			assertTrue(msg.hasPlayerListMessage());
			listMsg = msg.getPlayerListMessage();
			assertEquals(playerId[i], listMsg.getPlayerId());
			assertEquals(PlayerListNotification.playerListLeft, listMsg.getPlayerListNotification());
		}
	}
}
