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

import org.bn.*;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import pokerth_protocol.AnnounceMessage;
import pokerth_protocol.ErrorMessage;
import pokerth_protocol.GuestLogin;
import pokerth_protocol.InitAckMessage;
import pokerth_protocol.InitMessage;
import pokerth_protocol.PokerTHMessage;
import pokerth_protocol.Version;
import pokerth_protocol.AnnounceMessage.AnnounceMessageSequenceType.ServerTypeEnumType;
import pokerth_protocol.InitMessage.InitMessageSequenceType;
import pokerth_protocol.InitMessage.InitMessageSequenceType.LoginChoiceType;

import java.io.*;
import java.net.*;

public abstract class TestBase {

	public final int PROTOCOL_VERSION_MAJOR = 1;
	public final int PROTOCOL_VERSION_MINOR = 0;
	public final String AuthUser = "user";
	public final String AuthPassword = "pencil";
	public final String GuestUser = "Guest112233";
	public final String GamePassword = "äöü?ßÄÖÜ";

	protected IEncoder<PokerTHMessage> encoder;
	protected IDecoder decoder;
	private Socket sock;

	@Before
	public void setUp() throws Exception {
		InetAddress localaddr = InetAddress.getLocalHost();
		sock = new Socket(localaddr, 7234);
		encoder = CoderFactory.getInstance().newEncoder("BER");
		decoder = CoderFactory.getInstance().newDecoder("BER");
	}

	@After
	public void tearDown() throws IOException {
		sock.close();
	}

	public void sendMessage(PokerTHMessage msg) throws Exception {
		sendMessage(msg, sock);
	}

	public void sendMessage(PokerTHMessage msg, Socket s) throws Exception {
		encoder.encode(msg, s.getOutputStream());
	}

	public PokerTHMessage receiveMessage() throws Exception {
		return receiveMessage(sock);
	}

	public PokerTHMessage receiveMessage(Socket s) throws Exception {
		return decoder.decode(s.getInputStream(), PokerTHMessage.class);
	}

	public void guestInit() throws Exception {
		guestInit(sock);
	}

	public void guestInit(Socket s) throws Exception {
		PokerTHMessage msg = receiveMessage(s);
		AnnounceMessage announce = msg.getAnnounceMessage();
		assertTrue(announce.getValue().getServerType().getValue() == ServerTypeEnumType.EnumType.serverTypeInternetAuth);

		Version requestedVersion = new Version();
		requestedVersion.setMajor(PROTOCOL_VERSION_MAJOR);
		requestedVersion.setMinor(PROTOCOL_VERSION_MINOR);
		GuestLogin guestLogin = new GuestLogin();
		guestLogin.setNickName(GuestUser);
		LoginChoiceType loginType = new LoginChoiceType();
		loginType.selectGuestLogin(guestLogin);
		InitMessageSequenceType msgType = new InitMessageSequenceType();
		msgType.setBuildId(0L);
		msgType.setLogin(loginType);
		msgType.setRequestedVersion(requestedVersion);
		InitMessage init = new InitMessage();
		init.setValue(msgType);
		msg = new PokerTHMessage();
		msg.selectInitMessage(init);
		sendMessage(msg, s);

		msg = receiveMessage(s);
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
