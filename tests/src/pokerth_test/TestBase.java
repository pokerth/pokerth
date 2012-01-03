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
import org.w3c.dom.Document;
import org.w3c.dom.Element;

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
import java.security.MessageDigest;
import java.sql.Connection;
import java.sql.DriverManager;
import java.util.Collection;

import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

public abstract class TestBase {

	public final int PROTOCOL_VERSION_MAJOR = 5;
	public final int PROTOCOL_VERSION_MINOR = 0;
	public final String AuthUser = "user";
	public final String AuthPassword = "pencil";
	public final String GuestUser = "Guest112233";
	public final String GamePassword = "äöü?ßÄÖÜ";

	protected IEncoder<PokerTHMessage> encoder;
	protected IDecoder decoder;
	protected Socket sock;
	protected Connection dbConn;
	protected long lastRejoinGameId = 0;

	@Before
	public void dbInit() throws Exception {
		String configFileName = System.getProperty("user.home");
		if (System.getProperty("os.name").toLowerCase().indexOf("linux") > -1) {
			configFileName += "/.pokerth/config.xml";
		} else {
			configFileName += "/AppData/Roaming/pokerth/config.xml";
		}
		File file = new File(configFileName);
		DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
		DocumentBuilder db = dbf.newDocumentBuilder();
		Document doc = db.parse(file);
		doc.getDocumentElement().normalize();
		Element configNode = (Element)doc.getElementsByTagName("Configuration").item(0);

		Element dbAddressNode = (Element)configNode.getElementsByTagName("DBServerAddress").item(0);
		String dbAddress = dbAddressNode.getAttribute("value");

		Element dbUserNode = (Element)configNode.getElementsByTagName("DBServerUser").item(0);
		String dbUser = dbUserNode.getAttribute("value");

		Element dbPasswordNode = (Element)configNode.getElementsByTagName("DBServerPassword").item(0);
		String dbPassword = dbPasswordNode.getAttribute("value");

		Element dbNameNode = (Element)configNode.getElementsByTagName("DBServerDatabaseName").item(0);
		String dbName = dbNameNode.getAttribute("value");

		final String dbUrl = "jdbc:mysql://" + dbAddress + ":3306/" + dbName;
		Class.forName("com.mysql.jdbc.Driver").newInstance ();
		dbConn = DriverManager.getConnection(dbUrl, dbUser, dbPassword);
	}

	@After
	public void dbClose() throws Exception {
		dbConn.close();
	}

	@Before
	public void setUp() throws Exception {
		Thread.sleep(2000);
		sock = new Socket("localhost", 7234);
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

	public long guestInit() throws Exception {
		return guestInit(sock);
	}

	public long guestInit(Socket s) throws Exception {
		long playerId = 0;
		PokerTHMessage msg = receiveMessage(s);
		assertTrue(msg.isAnnounceMessageSelected());

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
		if (msg.isInitAckMessageSelected()) {
			InitAckMessage initAck = msg.getInitAckMessage();
			assertTrue(initAck.getValue().getYourPlayerId().getValue() != 0L);
			assertTrue(!initAck.getValue().isYourAvatarPresent());
			playerId = initAck.getValue().getYourPlayerId().getValue();
		}
		else {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
		return playerId;
	}

	public long userInit() throws Exception {
		return userInit(sock, AuthUser, AuthPassword);
	}

	public long userInit(Socket s, String user, String password) throws Exception {
		return userInit(s, user, password, null, null);
	}

	public long userInit(Socket s, String user, String password, byte[] avatarData, Guid lastSessionId) throws Exception {
		long playerId = 0;
		PokerTHMessage msg = receiveMessage(s);
		AnnounceMessage announce = msg.getAnnounceMessage();
		assertTrue(announce.getValue().getServerType().getValue() == ServerTypeEnumType.EnumType.serverTypeInternetAuth);

		ScramSha1 scramAuth = new ScramSha1();

		// Send challenge.
		Version requestedVersion = new Version();
		requestedVersion.setMajor(PROTOCOL_VERSION_MAJOR);
		requestedVersion.setMinor(PROTOCOL_VERSION_MINOR);
		AuthenticatedLogin authLogin = new AuthenticatedLogin();
		AvatarHash avatar = null;
		if (avatarData != null) {
			avatar = new AvatarHash(avatarData);
		}
		authLogin.setAvatar(avatar);
		authLogin.setClientUserData(scramAuth.executeStep1(user).getBytes());
		LoginChoiceType loginType = new LoginChoiceType();
		loginType.selectAuthenticatedLogin(authLogin);
		InitMessageSequenceType msgType = new InitMessageSequenceType();
		msgType.setBuildId(0L);
		msgType.setLogin(loginType);
		msgType.setRequestedVersion(requestedVersion);
		if (lastSessionId != null && lastSessionId.getValue() != null)
		{
			msgType.setMyLastSessionId(lastSessionId);
		}
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
			authClient.setClientResponse(scramAuth.executeStep2(password, serverFirstMessage).getBytes());
			AuthMessageChoiceType authChoice = new AuthMessageChoiceType();
			authChoice.selectAuthClientResponse(authClient);
			AuthMessage authResponse = new AuthMessage();
			authResponse.setValue(authChoice);

			msg = new PokerTHMessage();
			msg.selectAuthMessage(authResponse);
			sendMessage(msg, s);
		}
		failOnErrorMessage(msg);

		msg = receiveMessage(s);
		failOnErrorMessage(msg);

		msg = receiveMessage(s);
		if (msg.isInitAckMessageSelected()) {
			InitAckMessage initAck = msg.getInitAckMessage();
			assertTrue(initAck.getValue().getYourPlayerId().getValue() != 0L);
			assertTrue(!initAck.getValue().isYourAvatarPresent());
			playerId = initAck.getValue().getYourPlayerId().getValue();
			if (lastSessionId != null) {
				lastSessionId.setValue(initAck.getValue().getYourSessionId().getValue());
			}
			if (initAck.getValue().isRejoinGameIdPresent()) {
				lastRejoinGameId = initAck.getValue().getRejoinGameId().getValue();
			}
			else {
				lastRejoinGameId = 0;
			}
		}
		else {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
		return playerId;
	}

	public PokerTHMessage createGameRequestMsg(NetGameInfo gameInfo, NetGameTypeEnumType.EnumType type, int playerActionTimeout, int guiSpeed, String password, boolean autoLeave) {
		NetGameTypeEnumType gameType = new NetGameTypeEnumType();
		gameType.setValue(type);
		gameInfo.setNetGameType(gameType);
		gameInfo.setPlayerActionTimeout(playerActionTimeout);
		gameInfo.setProposedGuiSpeed(guiSpeed);
		JoinNewGame joinNew = new JoinNewGame();
		joinNew.setGameInfo(gameInfo);
		if (!password.isEmpty()) {
			joinNew.setPassword(password);
		}
		JoinGameActionChoiceType joinAction = new JoinGameActionChoiceType();
		joinAction.selectJoinNewGame(joinNew);
		JoinGameRequestMessageSequenceType joinType = new JoinGameRequestMessageSequenceType();
		joinType.setJoinGameAction(joinAction);
		joinType.setAutoLeave(autoLeave);
		JoinGameRequestMessage joinRequest = new JoinGameRequestMessage();
		joinRequest.setValue(joinType);

		PokerTHMessage msg = new PokerTHMessage();
		msg.selectJoinGameRequestMessage(joinRequest);
		return msg;
	}

	public PokerTHMessage joinGameRequestMsg(long gameId, String password, boolean autoLeave) {
		JoinExistingGame joinExisting = new JoinExistingGame();
		joinExisting.setGameId(new NonZeroId(gameId));
		if (!password.isEmpty()) {
			joinExisting.setPassword(password);
		}
		JoinGameActionChoiceType joinAction = new JoinGameActionChoiceType();
		joinAction.selectJoinExistingGame(joinExisting);
		JoinGameRequestMessageSequenceType joinType = new JoinGameRequestMessageSequenceType();
		joinType.setJoinGameAction(joinAction);
		joinType.setAutoLeave(autoLeave);

		JoinGameRequestMessage joinRequest = new JoinGameRequestMessage();
		joinRequest.setValue(joinType);

		PokerTHMessage msg = new PokerTHMessage();
		msg.selectJoinGameRequestMessage(joinRequest);
		return msg;
	}

	public PokerTHMessage rejoinGameRequestMsg(long gameId, boolean autoLeave) {
		RejoinExistingGame rejoinExisting = new RejoinExistingGame();
		rejoinExisting.setGameId(new NonZeroId(gameId));
		JoinGameActionChoiceType joinAction = new JoinGameActionChoiceType();
		joinAction.selectRejoinExistingGame(rejoinExisting);
		JoinGameRequestMessageSequenceType joinType = new JoinGameRequestMessageSequenceType();
		joinType.setJoinGameAction(joinAction);
		joinType.setAutoLeave(autoLeave);

		JoinGameRequestMessage joinRequest = new JoinGameRequestMessage();
		joinRequest.setValue(joinType);

		PokerTHMessage msg = new PokerTHMessage();
		msg.selectJoinGameRequestMessage(joinRequest);
		return msg;
	}

	public NetGameInfo createGameInfo(int delayBetweenHands, EndRaiseModeEnumType.EnumType endMode, int endRaiseValue, int sb, String gameName,
			Collection<InitialNonZeroAmountOfMoney> manualBlinds, int maxNumPlayers, int raiseEveryMinutes, int raiseEveryHands, int startMoney) {
		NetGameInfo gameInfo = new NetGameInfo();
		EndRaiseModeEnumType endRaise = new EndRaiseModeEnumType();
		endRaise.setValue(endMode);
		gameInfo.setDelayBetweenHands(delayBetweenHands);
		gameInfo.setEndRaiseMode(endRaise);
		gameInfo.setEndRaiseSmallBlindValue(new InitialAmountOfMoney(endRaiseValue));
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
		gameInfo.setStartMoney(new InitialNonZeroAmountOfMoney(startMoney));

		return gameInfo;
	}

	void failOnErrorMessage(PokerTHMessage msg) {
		if (msg.isErrorMessageSelected())
		{
			ErrorMessage error = msg.getErrorMessage();
			fail("Received error: " + error.getValue().getErrorReason().getValue().toString());
		}
	}

	public byte[] decryptCards(final String password, final byte[] ciphertext) throws Exception
	{
		final MessageDigest shaDigest = MessageDigest.getInstance("SHA-1");
		byte[] pw1 = password.getBytes("UTF-8");

		byte[] keyHash1 = shaDigest.digest(pw1);
		keyHash1 = shaDigest.digest(keyHash1);
		byte[] pw2 = new byte[keyHash1.length + pw1.length];
		System.arraycopy(keyHash1, 0, pw2, 0, keyHash1.length);
		System.arraycopy(pw1, 0, pw2, keyHash1.length, pw1.length);
		byte[] keyHash2 = shaDigest.digest(pw2);
		keyHash2 = shaDigest.digest(keyHash2);

		byte[] key = new byte[16];
		System.arraycopy(keyHash1, 0, key, 0, key.length);
		byte[] iv = new byte[16];
		System.arraycopy(keyHash1, 16, iv, 0, 4);
		System.arraycopy(keyHash2, 0, iv, 4, 12);

		Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");

		cipher.init(
			Cipher.DECRYPT_MODE,
			new SecretKeySpec(key, "AES"),
			new IvParameterSpec(iv));

		return cipher.doFinal(ciphertext);
	}
}
