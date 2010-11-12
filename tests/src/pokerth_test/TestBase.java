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

import pokerth_protocol.*;
import pokerth_protocol.AnnounceMessage.AnnounceMessageSequenceType.ServerTypeEnumType;
import pokerth_protocol.AuthMessage.AuthMessageChoiceType;
import pokerth_protocol.InitMessage.InitMessageSequenceType;
import pokerth_protocol.InitMessage.InitMessageSequenceType.LoginChoiceType;
import pokerth_protocol.JoinGameRequestMessage.JoinGameRequestMessageSequenceType;
import pokerth_protocol.JoinGameRequestMessage.JoinGameRequestMessageSequenceType.JoinGameActionChoiceType;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;
import pokerth_protocol.NetGameInfo.RaiseIntervalModeChoiceType;

import java.io.*;
import java.net.*;
import java.util.ArrayList;
import java.util.Collection;

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

	public void userInit() throws Exception {
		userInit(sock);
	}

	public void userInit(Socket s) throws Exception {
		PokerTHMessage msg = receiveMessage(s);
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
		sendMessage(msg, s);

		msg = receiveMessage(s);

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
			sendMessage(msg, s);
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

		msg = receiveMessage(s);
		if (msg.isErrorMessageSelected())
		{
			ErrorMessage error = msg.getErrorMessage();
			fail("Received error: " + error.getValue().getErrorReason().getValue().toString());
		}
		else if (!msg.isAuthMessageSelected() || !msg.getAuthMessage().getValue().isAuthServerVerificationSelected())
		{
			fail("Invalid auth message.");
		}

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

	public PokerTHMessage createJoinGameRequestMsg(NetGameInfo gameInfo, NetGameTypeEnumType.EnumType type, int playerActionTimeout, int guiSpeed, String password) {
		NetGameTypeEnumType gameType = new NetGameTypeEnumType();
		gameType.setValue(type);
		gameInfo.setNetGameType(gameType);
		gameInfo.setPlayerActionTimeout(playerActionTimeout);
		gameInfo.setProposedGuiSpeed(guiSpeed);
		JoinNewGame joinNew = new JoinNewGame();
		joinNew.setGameInfo(gameInfo);
		JoinGameActionChoiceType joinAction = new JoinGameActionChoiceType();
		joinAction.selectJoinNewGame(joinNew);
		JoinGameRequestMessageSequenceType joinType = new JoinGameRequestMessageSequenceType();
		joinType.setJoinGameAction(joinAction);
		if (!password.isEmpty()) {
			joinType.setPassword(password);
		}
		JoinGameRequestMessage joinRequest = new JoinGameRequestMessage();
		joinRequest.setValue(joinType);

		PokerTHMessage msg = new PokerTHMessage();
		msg.selectJoinGameRequestMessage(joinRequest);
		return msg;
	}

	public NetGameInfo createGameInfo(int delayBetweenHands, EndRaiseModeEnumType.EnumType endMode, int endRaiseValue, int sb, String gameName,
			Collection<Integer> manualBlinds, int maxNumPlayers, int raiseEveryMinutes, int raiseEveryHands, int startMoney) {
		NetGameInfo gameInfo = new NetGameInfo();
		EndRaiseModeEnumType endRaise = new EndRaiseModeEnumType();
		endRaise.setValue(endMode);
		gameInfo.setDelayBetweenHands(delayBetweenHands);
		gameInfo.setEndRaiseMode(endRaise);
		gameInfo.setEndRaiseSmallBlindValue(endRaiseValue);
		gameInfo.setFirstSmallBlind(sb);
		gameInfo.setGameName(gameName);
		gameInfo.setManualBlinds(manualBlinds);
		gameInfo.setMaxNumPlayers(maxNumPlayers);
		RaiseIntervalModeChoiceType raiseInterval = new RaiseIntervalModeChoiceType();
		if (raiseEveryMinutes > 0) {
			raiseInterval.selectRaiseEveryMinutes(raiseEveryMinutes);
		}
		else {
			raiseInterval.selectRaiseEveryHands(raiseEveryHands);
		}
		gameInfo.setRaiseIntervalMode(raiseInterval);
		gameInfo.setStartMoney(startMoney);

		return gameInfo;
	}
}
