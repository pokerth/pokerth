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
import pokerth_protocol.JoinGameRequestMessage.JoinGameRequestMessageSequenceType;
import pokerth_protocol.JoinGameRequestMessage.JoinGameRequestMessageSequenceType.JoinGameActionChoiceType;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;
import pokerth_protocol.NetGameInfo.RaiseIntervalModeChoiceType;

public class CreateGameTest extends TestBase {

	@Test
	public void testJoinGameRequestMessage() throws Exception {
		guestInit();

		EndRaiseModeEnumType endRaise = new EndRaiseModeEnumType();
		endRaise.setValue(EndRaiseModeEnumType.EnumType.keepLastBlind);
		NetGameInfo gameInfo = new NetGameInfo();
		gameInfo.setDelayBetweenHands(6);
		gameInfo.setEndRaiseMode(endRaise);
		gameInfo.setEndRaiseSmallBlindValue(1000);
		gameInfo.setFirstSmallBlind(100);
		gameInfo.setGameName(GuestUser + " game test");
		Collection<Integer> l = new ArrayList<Integer>();
		l.add(250);
		l.add(600);
		l.add(1000);
		gameInfo.setManualBlinds(l);
		gameInfo.setMaxNumPlayers(10);
		NetGameTypeEnumType gameType = new NetGameTypeEnumType();
		gameType.setValue(NetGameTypeEnumType.EnumType.normalGame);
		gameInfo.setNetGameType(gameType);
		gameInfo.setPlayerActionTimeout(20);
		gameInfo.setProposedGuiSpeed(8);
		RaiseIntervalModeChoiceType raiseInterval = new RaiseIntervalModeChoiceType();
		raiseInterval.selectRaiseEveryHands(7);
		gameInfo.setRaiseIntervalMode(raiseInterval);
		gameInfo.setStartMoney(2000);
		JoinNewGame joinNew = new JoinNewGame();
		joinNew.setGameInfo(gameInfo);
		JoinGameActionChoiceType joinAction = new JoinGameActionChoiceType();
		joinAction.selectJoinNewGame(joinNew);
		JoinGameRequestMessageSequenceType joinType = new JoinGameRequestMessageSequenceType();
		joinType.setJoinGameAction(joinAction);
		joinType.setPassword(GamePassword);
		JoinGameRequestMessage joinRequest = new JoinGameRequestMessage();
		joinRequest.setValue(joinType);
		PokerTHMessage msg = new PokerTHMessage();
		msg.selectJoinGameRequestMessage(joinRequest);
		sendMessage(msg);

		do {
			msg = receiveMessage();
		} while (msg.isPlayerListMessageSelected() || msg.isGameListMessageSelected());
		if (msg.isJoinGameReplyMessageSelected())
		{
			if (msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected())
			{
				assertTrue(msg.getJoinGameReplyMessage().getValue().getGameId().getValue() != 0);
				NetGameInfo receivedGameInfo = msg.getJoinGameReplyMessage().getValue().getJoinGameResult().getJoinGameAck().getGameInfo();
				assertEquals(receivedGameInfo.getDelayBetweenHands(), gameInfo.getDelayBetweenHands());
				assertEquals(receivedGameInfo.getEndRaiseMode().getValue(), gameInfo.getEndRaiseMode().getValue());
				assertEquals(receivedGameInfo.getEndRaiseSmallBlindValue(), gameInfo.getEndRaiseSmallBlindValue());
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
		else if (msg.isErrorMessageSelected())
		{
			ErrorMessage error = msg.getErrorMessage();
			fail("Received error: " + error.getValue().getErrorReason().getValue().toString());
		}
		else
		{
			fail("Invalid response message.");
		}
	}

}
