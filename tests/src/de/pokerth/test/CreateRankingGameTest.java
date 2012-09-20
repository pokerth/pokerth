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

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.NetGameType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;


public class CreateRankingGameTest extends TestBase {

	static private int counter = 0;

	private void createRankingGame(String password) throws Exception {
		counter++;
		Collection<Integer> l = new ArrayList<Integer>();
		NetGameInfo gameInfo = createGameInfo(NetGameType.rankingGame, 20, 7, 8, EndRaiseMode.doubleBlinds, 0, 50, GuestUser + " create ranking game " + counter, l, 10, 0, 11, 10000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				password,
				false));
	}

	@Test
	public void testCreateRankingGameAsGuest() throws Exception {
		guestInit();

		PokerTHMessage msg;
		msg = receiveMessage();
		if (!msg.hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		createRankingGame("");
		msg = receiveMessage();

		if (!msg.hasJoinGameFailedMessage())
		{
			failOnErrorMessage(msg);
			fail("Guest user could create ranking game!");
		}
	}

	@Test
	public void testCreateRankingGameNoPasswordAsUser() throws Exception {
		userInit();

		// Waiting for player list update.
		PokerTHMessage msg;
		msg = receiveMessage();
		if (!msg.hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		createRankingGame("");
		msg = receiveMessage();

		if (msg.hasGameListNewMessage())
		{
			msg = receiveMessage();
			if (msg.hasJoinGameFailedMessage())
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

		// Waiting for player list update.
		PokerTHMessage msg;
		msg = receiveMessage();
		if (!msg.hasPlayerListMessage()) {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		createRankingGame(GamePassword);
		msg = receiveMessage();

		if (!msg.hasJoinGameFailedMessage())
		{
			failOnErrorMessage(msg);
			fail("Registered user should not be allowed to create ranking game with password!");
		}
	}
}
