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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.net.Socket;
import java.sql.Statement;

import org.junit.Test;

import pokerth_protocol.AnnounceMessage;
import pokerth_protocol.AuthenticatedLogin;
import pokerth_protocol.InitMessage;
import pokerth_protocol.PokerTHMessage;
import pokerth_protocol.Version;
import pokerth_protocol.AnnounceMessage.AnnounceMessageSequenceType.ServerTypeEnumType;
import pokerth_protocol.ErrorMessage.ErrorMessageSequenceType.ErrorReasonEnumType;
import pokerth_protocol.InitMessage.InitMessageSequenceType;
import pokerth_protocol.InitMessage.InitMessageSequenceType.LoginChoiceType;


public class BlockedPlayerTest extends TestBase {

	void verifyLoginBlocked() throws Exception {
		PokerTHMessage msg = receiveMessage(sock);
		AnnounceMessage announce = msg.getAnnounceMessage();
		assertTrue(announce.getValue().getServerType().getValue() == ServerTypeEnumType.EnumType.serverTypeInternetAuth);

		ScramSha1 scramAuth = new ScramSha1();

		// Send challenge.
		Version requestedVersion = new Version();
		requestedVersion.setMajor(PROTOCOL_VERSION_MAJOR);
		requestedVersion.setMinor(PROTOCOL_VERSION_MINOR);
		AuthenticatedLogin authLogin = new AuthenticatedLogin();
		authLogin.setClientUserData(scramAuth.executeStep1("test1").getBytes());
		LoginChoiceType loginType = new LoginChoiceType();
		loginType.selectAuthenticatedLogin(authLogin);
		InitMessageSequenceType msgType = new InitMessageSequenceType();
		msgType.setBuildId(0L);
		msgType.setLogin(loginType);
		msgType.setRequestedVersion(requestedVersion);
		InitMessage init = new InitMessage();
		init.setValue(msgType);
		msg = new PokerTHMessage();
		msg.selectInitMessage(init);
		sendMessage(msg, sock);

		msg = receiveMessage(sock);
		assertTrue(msg.isErrorMessageSelected());
		assertEquals(ErrorReasonEnumType.EnumType.errorBlockedByServer, msg.getErrorMessage().getValue().getErrorReason().getValue());
	}
	
	@Test
	public void testRunRankingGame() throws Exception {

		Statement dbStatement = dbConn.createStatement();
		dbStatement.executeUpdate("UPDATE player_login SET valid = 0 WHERE username = 'test1'");
		verifyLoginBlocked();
		dbStatement.executeUpdate("UPDATE player_login SET valid = 1 WHERE username = 'test1'");

		sock.close();
		sock = new Socket("localhost", 7234);

		dbStatement.executeUpdate("UPDATE player_login SET valid = 2 WHERE username = 'test1'");
		verifyLoginBlocked();
		dbStatement.executeUpdate("UPDATE player_login SET valid = 1 WHERE username = 'test1'");

		sock.close();
		sock = new Socket("localhost", 7234);

		dbStatement.executeUpdate("UPDATE player_login SET valid = 4 WHERE username = 'test1'");
		verifyLoginBlocked();
		dbStatement.executeUpdate("UPDATE player_login SET valid = 1 WHERE username = 'test1'");

		sock.close();
		sock = new Socket("localhost", 7234);

		dbStatement.executeUpdate("UPDATE player_login SET aktivator = 0 WHERE username = 'test1'");
		verifyLoginBlocked();
		dbStatement.executeUpdate("UPDATE player_login SET aktivator = 1 WHERE username = 'test1'");

		sock.close();
		sock = new Socket("localhost", 7234);

		dbStatement.executeUpdate("UPDATE player_login SET aktivator = 2 WHERE username = 'test1'");
		verifyLoginBlocked();
		dbStatement.executeUpdate("UPDATE player_login SET aktivator = 1 WHERE username = 'test1'");

		sock.close();
		sock = new Socket("localhost", 7234);

		dbStatement.executeUpdate("UPDATE player_login SET aktivator = 4 WHERE username = 'test1'");
		verifyLoginBlocked();
		dbStatement.executeUpdate("UPDATE player_login SET aktivator = 1 WHERE username = 'test1'");

		sock.close();
		sock = new Socket("localhost", 7234);

		userInit(sock, "test1", "test1");
	}
}
