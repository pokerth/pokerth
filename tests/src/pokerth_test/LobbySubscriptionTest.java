package pokerth_test;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.net.Socket;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import pokerth_protocol.NetGameInfo;
import pokerth_protocol.PokerTHMessage;
import pokerth_protocol.SubscriptionRequestMessage;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;
import pokerth_protocol.SubscriptionRequestMessage.SubscriptionRequestMessageSequenceType;
import pokerth_protocol.SubscriptionRequestMessage.SubscriptionRequestMessageSequenceType.SubscriptionActionEnumType;


public class LobbySubscriptionTest extends TestBase {

	@Test
	public void testLobbySubscription() throws Exception {
		guestInit();

		PokerTHMessage msg;
		msg = receiveMessage();
		if (!msg.isPlayerListMessageSelected()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		SubscriptionRequestMessageSequenceType subscriptionType = new SubscriptionRequestMessageSequenceType();
		SubscriptionActionEnumType action = new SubscriptionActionEnumType();
		action.setValue(SubscriptionActionEnumType.EnumType.unsubscribeGameList);
		subscriptionType.setSubscriptionAction(action);
		SubscriptionRequestMessage subscriptionRequest = new SubscriptionRequestMessage();
		subscriptionRequest.setValue(subscriptionType);
		msg = new PokerTHMessage();
		msg.selectSubscriptionRequestMessage(subscriptionRequest);
		sendMessage(msg);

		// Create a new game.
		Collection<Integer> l = new ArrayList<Integer>();
		NetGameInfo gameInfo = createGameInfo(5, EndRaiseModeEnumType.EnumType.doubleBlinds, 0, 100, GuestUser + " game list normal game", l, 10, 0, 2, 2000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				NetGameTypeEnumType.EnumType.normalGame,
				10,
				5,
				"",
				false));

		// No game list message should be sent by the server.
		// Next message is join game ack.
		msg = receiveMessage();
		assertTrue(msg.isJoinGameReplyMessageSelected());
		assertTrue(msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected());
		long gameId = msg.getJoinGameReplyMessage().getValue().getGameId().getValue().longValue();

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
			if (msg.isGameListMessageSelected() || msg.isPlayerListMessageSelected()) {
				fail("Game/player list messages are switched off!");
			}
		} while (!msg.isStartEventMessageSelected());

		// Resubscribe game list
		subscriptionType = new SubscriptionRequestMessageSequenceType();
		action = new SubscriptionActionEnumType();
		action.setValue(SubscriptionActionEnumType.EnumType.resubscribeGameList);
		subscriptionType.setSubscriptionAction(action);
		subscriptionRequest = new SubscriptionRequestMessage();
		subscriptionRequest.setValue(subscriptionType);
		msg = new PokerTHMessage();
		msg.selectSubscriptionRequestMessage(subscriptionRequest);
		sendMessage(msg);

		// Next messages should player list messages for all 10 players.
		for (int i = 0; i < 10; i++) {
			msg = receiveMessage();
			assertTrue(msg.isPlayerListMessageSelected());
		}
		// Now there should be one game list message.
		msg = receiveMessage();
		assertTrue(msg.isGameListMessageSelected());

		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
	}
}
