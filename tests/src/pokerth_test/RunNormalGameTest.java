package pokerth_test;

import static org.junit.Assert.fail;

import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import pokerth_protocol.NetGameInfo;
import pokerth_protocol.NonZeroId;
import pokerth_protocol.PokerTHMessage;
import pokerth_protocol.StartEventMessage;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;
import pokerth_protocol.StartEventMessage.StartEventMessageSequenceType;


public class RunNormalGameTest extends TestBase {

	@Test
	public void testRunNormalGameAsGuest() throws Exception {
		guestInit();

		Collection<Integer> l = new ArrayList<Integer>();
		NetGameInfo gameInfo = createGameInfo(5, EndRaiseModeEnumType.EnumType.doubleBlinds, 0, 100, GuestUser + " run normal game", l, 10, 0, 2, 2000);
		sendMessage(createJoinGameRequestMsg(
				gameInfo,
				NetGameTypeEnumType.EnumType.normalGame,
				10,
				5,
				""));

		PokerTHMessage msg;
		do {
			msg = receiveMessage();
		} while (msg.isPlayerListMessageSelected() || msg.isGameListMessageSelected());

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

		StartEventMessageSequenceType gameStartType = new StartEventMessageSequenceType();
		gameStartType.setGameId(new NonZeroId(gameId));
		gameStartType.setFillWithComputerPlayers(true);
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

		do {
			msg = receiveMessage();
		} while (
				msg.isGameListMessageSelected()
				|| msg.isGamePlayerMessageSelected()
				|| msg.isStartEventMessageSelected()
				);

		do {
			msg = receiveMessage();
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
	}
}
