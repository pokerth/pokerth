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
import java.sql.Statement;

import org.junit.Test;

import com.google.protobuf.ByteString;

import de.pokerth.protocol.ProtoBuf.AnnounceMessage;
import de.pokerth.protocol.ProtoBuf.ErrorMessage.ErrorReason;
import de.pokerth.protocol.ProtoBuf.InitMessage;
import de.pokerth.protocol.ProtoBuf.AnnounceMessage.ServerType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;


public class BlockedPlayerTest extends TestBase {

	void verifyLoginBlocked() throws Exception {
		PokerTHMessage msg = receiveMessage(sock);
		AnnounceMessage announce = msg.getAnnounceMessage();
		assertTrue(announce.getServerType() == ServerType.serverTypeInternetAuth);

		ScramSha1 scramAuth = new ScramSha1();

		// Send challenge.
		AnnounceMessage.Version requestedVersion = AnnounceMessage.Version.newBuilder()
				.setMajorVersion(PROTOCOL_VERSION_MAJOR)
				.setMinorVersion(PROTOCOL_VERSION_MINOR)
				.build();
		InitMessage init = InitMessage.newBuilder()
				.setBuildId(0)
				.setLogin(InitMessage.LoginType.authenticatedLogin)
				.setRequestedVersion(requestedVersion)
				.setClientUserData(ByteString.copyFromUtf8(scramAuth.executeStep1("test9999")))
				.build();

		msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_InitMessage)
				.setInitMessage(init)
				.build();
		sendMessage(msg, sock);

		msg = receiveMessage(sock);
		assertTrue(msg.hasErrorMessage() && msg.getMessageType() == PokerTHMessageType.Type_ErrorMessage);
		assertEquals(ErrorReason.blockedByServer, msg.getErrorMessage().getErrorReason());
	}
	
	@Test
	public void testRunRankingGame() throws Exception {

		Statement dbStatement = dbConn.createStatement();
		dbStatement.executeUpdate("UPDATE player_login SET valid = 0 WHERE username = 'test9999'");
		verifyLoginBlocked();
		dbStatement.executeUpdate("UPDATE player_login SET valid = 1 WHERE username = 'test9999'");

		sock.close();
		sock = new Socket("localhost", 7234);

		dbStatement.executeUpdate("UPDATE player_login SET valid = 2 WHERE username = 'test9999'");
		verifyLoginBlocked();
		dbStatement.executeUpdate("UPDATE player_login SET valid = 1 WHERE username = 'test9999'");

		sock.close();
		sock = new Socket("localhost", 7234);

		dbStatement.executeUpdate("UPDATE player_login SET valid = 4 WHERE username = 'test9999'");
		verifyLoginBlocked();
		dbStatement.executeUpdate("UPDATE player_login SET valid = 1 WHERE username = 'test9999'");

		sock.close();
		sock = new Socket("localhost", 7234);

		dbStatement.executeUpdate("UPDATE player_login SET aktivator = 0 WHERE username = 'test9999'");
		verifyLoginBlocked();
		dbStatement.executeUpdate("UPDATE player_login SET aktivator = 1 WHERE username = 'test9999'");

		sock.close();
		sock = new Socket("localhost", 7234);

		dbStatement.executeUpdate("UPDATE player_login SET aktivator = 2 WHERE username = 'test9999'");
		verifyLoginBlocked();
		dbStatement.executeUpdate("UPDATE player_login SET aktivator = 1 WHERE username = 'test9999'");

		sock.close();
		sock = new Socket("localhost", 7234);

		dbStatement.executeUpdate("UPDATE player_login SET aktivator = 4 WHERE username = 'test9999'");
		verifyLoginBlocked();
		dbStatement.executeUpdate("UPDATE player_login SET aktivator = 1 WHERE username = 'test9999'");

		sock.close();
		sock = new Socket("localhost", 7234);

		userInit(sock, "test9999", "test9999");
	}
}
