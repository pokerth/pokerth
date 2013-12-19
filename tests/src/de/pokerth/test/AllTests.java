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

import org.junit.internal.TextListener;
import org.junit.runner.JUnitCore;
import org.junit.runner.RunWith;
import org.junit.runners.Suite;

@RunWith(Suite.class)
@Suite.SuiteClasses( {
	AnnounceTest.class,
	GuestLoginTest.class,
	AuthLoginTest.class,
	PlayerListTest.class,
	GameListTest.class,
	PlayerInfoTest.class,
	LobbySubscriptionTest.class,
	ChatTest.class,
	CreateGameTest.class,
	CreateRankingGameTest.class,
	StartNormalGameTest.class,
	RunNormalGameTest.class,
	BlockedPlayerTest.class,
	RunRankingGameTest.class,
	RejoinGameTest.class,
//	RejoinMultiGameTest.class,
	SeatStateTest.class,
	SpectatorJoinTest.class
})
public class AllTests {
	public static void main(String[] args)
	{
		JUnitCore junit = new JUnitCore();
		junit.addListener(new TextListener(System.out));
		if (!junit.run(AllTests.class).wasSuccessful())
			System.exit(1);
	}
}
