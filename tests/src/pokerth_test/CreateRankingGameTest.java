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


public class CreateRankingGameTest extends TestBase {

	static private int counter = 0;

	private void createRankingGame(String password) throws Exception {
		counter++;
		Collection<Integer> l = new ArrayList<Integer>();
		NetGameInfo gameInfo = createGameInfo(8, EndRaiseModeEnumType.EnumType.doubleBlinds, 0, 50, GuestUser + " create ranking game " + counter, l, 10, 0, 11, 10000);
		sendMessage(createJoinGameRequestMsg(
				gameInfo,
				NetGameTypeEnumType.EnumType.rankingGame,
				20,
				7,
				password));
	}

	@Test
	public void testCreateRankingGameAsGuest() throws Exception {
		guestInit();

		createRankingGame("");
		PokerTHMessage msg;
		msg = receiveMessage();

		if (msg.isJoinGameReplyMessageSelected())
		{
			if (!msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameFailedSelected())
			{
				fail("Guest user could create ranking game!");
			}
		}
		else {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
	}

	@Test
	public void testCreateRankingGameNoPasswordAsUser() throws Exception {
		userInit();

		createRankingGame("");
		PokerTHMessage msg;
		msg = receiveMessage();

		if (msg.isGameListMessageSelected())
		{
			msg = receiveMessage();
			if (msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameFailedSelected())
			{
				fail("Registered user could not join ranking game!");
			}
		}
		else {
			failOnErrorMessage(msg);
			fail("Registered user could not create ranking game!");
		}
	}

	@Test
	public void testCreateRankingGameWithPasswordAsUser() throws Exception {
		userInit();

		createRankingGame(GamePassword);
		PokerTHMessage msg;
		msg = receiveMessage();

		if (msg.isJoinGameReplyMessageSelected())
		{
			if (!msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameFailedSelected())
			{
				fail("Registered user should not be allowed to create ranking game with password!");
			}
		}
		else {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
	}
}
