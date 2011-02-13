package pokerth_test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.net.Socket;

import org.junit.Test;

import pokerth_protocol.PlayerListMessage;
import pokerth_protocol.PokerTHMessage;
import pokerth_protocol.PlayerListMessage.PlayerListMessageSequenceType.PlayerListNotificationEnumType;


public class PlayerListTest extends TestBase {

	@Test
	public void testPlayerList() throws Exception {

		long myId = guestInit();

		// Waiting for player list update.
		PokerTHMessage msg;
		msg = receiveMessage();
		assertTrue(msg.isPlayerListMessageSelected());

		// This should be a "player list new" notification with correct player id.
		PlayerListMessage listMsg = msg.getPlayerListMessage();
		assertEquals(myId, listMsg.getValue().getPlayerId().getValue().longValue());
		assertEquals(PlayerListNotificationEnumType.EnumType.playerListNew, listMsg.getValue().getPlayerListNotification().getValue());

		Socket s[] = new Socket[9];
		long playerId[] = new long[9];
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password);

			msg = receiveMessage();
			assertTrue(msg.isPlayerListMessageSelected());
			listMsg = msg.getPlayerListMessage();
			// Id should be different from first id.
			assertTrue(myId != playerId[i]);
			// This should be a "player list new" notification with correct player id.
			assertEquals(playerId[i], listMsg.getValue().getPlayerId().getValue().longValue());
			assertEquals(PlayerListNotificationEnumType.EnumType.playerListNew, listMsg.getValue().getPlayerListNotification().getValue());

			s[i].close();

			// After the connection is closed, a "player list left" notification should be received.
			msg = receiveMessage();
			assertTrue(msg.isPlayerListMessageSelected());
			listMsg = msg.getPlayerListMessage();
			assertEquals(playerId[i], listMsg.getValue().getPlayerId().getValue().longValue());
			assertEquals(PlayerListNotificationEnumType.EnumType.playerListLeft, listMsg.getValue().getPlayerListNotification().getValue());
		}
	}
}
