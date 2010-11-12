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
import org.junit.Test;

import pokerth_protocol.*;
import pokerth_protocol.AnnounceMessage.AnnounceMessageSequenceType.ServerTypeEnumType;
import pokerth_protocol.AuthMessage.AuthMessageChoiceType;
import pokerth_protocol.InitMessage.*;
import pokerth_protocol.InitMessage.InitMessageSequenceType.LoginChoiceType;

public class AuthLoginTest extends TestBase {

	@Test
	public void testAuthLogin() throws Exception {
		PokerTHMessage msg = receiveMessage();
		AnnounceMessage announce = msg.getAnnounceMessage();
		assertTrue(announce.getValue().getServerType().getValue() == ServerTypeEnumType.EnumType.serverTypeInternetAuth);

		ScramSha1 scramAuth = new ScramSha1();

		// Send challenge.
		Version requestedVersion = new Version();
		requestedVersion.setMajor(PROTOCOL_VERSION_MAJOR);
		requestedVersion.setMinor(PROTOCOL_VERSION_MINOR);
		AuthenticatedLogin authLogin = new AuthenticatedLogin();
		authLogin.setClientUserData(scramAuth.executeStep1(AuthUser).getBytes());
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
		sendMessage(msg);

		msg = receiveMessage();

		if (msg.isAuthMessageSelected() && msg.getAuthMessage().getValue().isAuthServerChallengeSelected())
		{
			String serverFirstMessage = new String(msg.getAuthMessage().getValue().getAuthServerChallenge().getServerChallenge());
			AuthClientResponse authClient = new AuthClientResponse();
			authClient.setClientResponse(scramAuth.executeStep2(AuthPassword, serverFirstMessage).getBytes());
			AuthMessageChoiceType authChoice = new AuthMessageChoiceType();
			authChoice.selectAuthClientResponse(authClient);
			AuthMessage authResponse = new AuthMessage();
			authResponse.setValue(authChoice);

			msg = new PokerTHMessage();
			msg.selectAuthMessage(authResponse);
			sendMessage(msg);
		}
		else if (msg.isErrorMessageSelected())
		{
			ErrorMessage error = msg.getErrorMessage();
			fail("Received error: " + error.getValue().getErrorReason().getValue().toString());
		}
		else
		{
			fail("Invalid auth message.");
		}

		msg = receiveMessage();
		if (msg.isErrorMessageSelected())
		{
			ErrorMessage error = msg.getErrorMessage();
			fail("Received error: " + error.getValue().getErrorReason().getValue().toString());
		}
		else if (!msg.isAuthMessageSelected() || !msg.getAuthMessage().getValue().isAuthServerVerificationSelected())
		{
			fail("Invalid auth message.");
		}

		msg = receiveMessage();
		if (msg.isInitAckMessageSelected())
		{
			InitAckMessage initAck = msg.getInitAckMessage();
			assertTrue(initAck.getValue().getYourPlayerId().getValue() != 0L);
			assertTrue(!initAck.getValue().isYourAvatarPresent());
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
