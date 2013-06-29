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

import org.junit.After;
import org.junit.Before;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import com.google.protobuf.ByteString;

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

import de.pokerth.protocol.ProtoBuf.AnnounceMessage;
import de.pokerth.protocol.ProtoBuf.AnnounceMessage.ServerType;
import de.pokerth.protocol.ProtoBuf.AuthClientResponseMessage;
import de.pokerth.protocol.ProtoBuf.ErrorMessage;
import de.pokerth.protocol.ProtoBuf.InitAckMessage;
import de.pokerth.protocol.ProtoBuf.InitMessage;
import de.pokerth.protocol.ProtoBuf.JoinExistingGameMessage;
import de.pokerth.protocol.ProtoBuf.JoinNewGameMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;
import de.pokerth.protocol.ProtoBuf.RejoinExistingGameMessage;

public abstract class TestBase {

	public final int PROTOCOL_VERSION_MAJOR = 5;
	public final int PROTOCOL_VERSION_MINOR = 1;
	public final String AuthUser = "user";
	public final String AuthPassword = "pencil";
	public final String GuestUser = "Guest112233";
	public final String GamePassword = "äöü?ßÄÖÜ";

	protected Socket sock;
	protected Connection dbConn;
	protected int lastRejoinGameId = 0;

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
	}

	@After
	public void tearDown() throws IOException {
		sock.close();
	}

	public void sendMessage(PokerTHMessage msg) throws Exception {
		sendMessage(msg, sock);
	}

	public void sendMessage(PokerTHMessage msg, Socket s) throws Exception {
		int size = msg.getSerializedSize();
		byte[] header = new byte[4];
		for (int i = 0; i < 4; i++)
		{
			header[i] = (new Integer(size >>> 24)).byteValue();
			size <<= 8;
		}
		s.getOutputStream().write(header);
		s.getOutputStream().write(msg.toByteArray());
	}

	public PokerTHMessage receiveMessage() throws Exception {
		return receiveMessage(sock);
	}

	public PokerTHMessage receiveMessage(Socket s) throws Exception {
		byte[] header = new byte[4];
		s.getInputStream().read(header);
		int size = 0;
		for (int i = 0; i < 4; i++) {
			size <<= 8;
			size |= (int)header[i];
		}
		byte[] data = new byte[size];
		s.getInputStream().read(data);
		return PokerTHMessage.parseFrom(data);
	}

	public int guestInit() throws Exception {
		return guestInit(sock);
	}

	public int guestInit(Socket s) throws Exception {
		int playerId = 0;
		PokerTHMessage msg = receiveMessage(s);
		assertTrue(msg.hasAnnounceMessage());

		AnnounceMessage.Version requestedVersion = AnnounceMessage.Version.newBuilder()
			.setMajorVersion(PROTOCOL_VERSION_MAJOR)
			.setMinorVersion(PROTOCOL_VERSION_MINOR)
			.build();
		InitMessage init = InitMessage.newBuilder()
			.setBuildId(0)
			.setLogin(InitMessage.LoginType.guestLogin)
			.setRequestedVersion(requestedVersion)
			.setNickName(GuestUser)
			.build();
		msg = PokerTHMessage.newBuilder()
			.setMessageType(PokerTHMessageType.Type_InitMessage)
			.setInitMessage(init)
			.build();
		sendMessage(msg, s);

		msg = receiveMessage(s);
		if (msg.hasInitAckMessage() && msg.getMessageType() == PokerTHMessageType.Type_InitAckMessage) {
			InitAckMessage initAck = msg.getInitAckMessage();
			assertTrue(initAck.getYourPlayerId() != 0L);
			assertTrue(!initAck.hasYourAvatarHash());
			playerId = initAck.getYourPlayerId();
		}
		else {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}
		return playerId;
	}

	public int userInit() throws Exception {
		return userInit(sock, AuthUser, AuthPassword);
	}

	public int userInit(Socket s, String user, String password) throws Exception {
		return userInit(s, user, password, null, null);
	}

	public class Guid {
		public byte[] value;
	}

	public int userInit(Socket s, String user, String password, byte[] avatarData, Guid lastSessionId) throws Exception {
		int playerId = 0;
		PokerTHMessage msg = receiveMessage(s);
		AnnounceMessage announce = msg.getAnnounceMessage();
		assertTrue(announce.getServerType() == ServerType.serverTypeInternetAuth);

		ScramSha1 scramAuth = new ScramSha1();

		// Send challenge.
		AnnounceMessage.Version requestedVersion = AnnounceMessage.Version.newBuilder()
				.setMajorVersion(PROTOCOL_VERSION_MAJOR)
				.setMinorVersion(PROTOCOL_VERSION_MINOR)
				.build();
		InitMessage.Builder initBuilder = InitMessage.newBuilder();
		initBuilder
				.setBuildId(0)
				.setLogin(InitMessage.LoginType.authenticatedLogin)
				.setRequestedVersion(requestedVersion)
				.setClientUserData(ByteString.copyFromUtf8(scramAuth.executeStep1(user)));
		if (avatarData != null) {
			initBuilder.setAvatarHash(ByteString.copyFrom(avatarData));
		}
		if (lastSessionId != null && lastSessionId.value != null) {
			initBuilder.setMyLastSessionId(ByteString.copyFrom(lastSessionId.value));
		}
		InitMessage init = initBuilder.build();
		msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_InitMessage)
				.setInitMessage(init)
				.build();
		sendMessage(msg, s);

		msg = receiveMessage(s);

		if (msg.hasAuthServerChallengeMessage() && msg.getMessageType() == PokerTHMessageType.Type_AuthServerChallengeMessage)
		{
			String serverFirstMessage = new String(msg.getAuthServerChallengeMessage().getServerChallenge().toStringUtf8());
			AuthClientResponseMessage authClient = AuthClientResponseMessage.newBuilder()
				.setClientResponse(ByteString.copyFromUtf8(scramAuth.executeStep2(password, serverFirstMessage)))
				.build();

			msg = PokerTHMessage.newBuilder()
					.setMessageType(PokerTHMessageType.Type_AuthClientResponseMessage)
					.setAuthClientResponseMessage(authClient)
					.build();
			sendMessage(msg, s);
		}
		failOnErrorMessage(msg);

		msg = receiveMessage(s);
		failOnErrorMessage(msg);

		msg = receiveMessage(s);
		if (msg.hasInitAckMessage() && msg.getMessageType() == PokerTHMessageType.Type_InitAckMessage) {
			InitAckMessage initAck = msg.getInitAckMessage();
			assertTrue(initAck.getYourPlayerId() != 0L);
			assertTrue(!initAck.hasYourAvatarHash());
			playerId = initAck.getYourPlayerId();
			if (lastSessionId != null) {
				lastSessionId.value = initAck.getYourSessionId().toByteArray();
			}
			if (initAck.hasRejoinGameId()) {
				lastRejoinGameId = initAck.getRejoinGameId();
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

	public PokerTHMessage createGameRequestMsg(NetGameInfo gameInfo, String password, boolean autoLeave) {
		JoinNewGameMessage.Builder joinBuilder = JoinNewGameMessage.newBuilder();
		joinBuilder.setGameInfo(gameInfo);
		joinBuilder.setAutoLeave(autoLeave);
		if (!password.isEmpty()) {
			joinBuilder.setPassword(password);
		}
		JoinNewGameMessage joinNew = joinBuilder.build();

		PokerTHMessage msg = PokerTHMessage.newBuilder()
			.setMessageType(PokerTHMessageType.Type_JoinNewGameMessage)
			.setJoinNewGameMessage(joinNew)
			.build();
		return msg;
	}

	public PokerTHMessage joinGameRequestMsg(int gameId, String password, boolean autoLeave) {
		return joinGameRequestMsg(gameId, password, autoLeave, false);
	}

	public PokerTHMessage joinGameRequestMsg(int gameId, String password, boolean autoLeave, boolean spectateOnly) {
		JoinExistingGameMessage.Builder joinBuilder = JoinExistingGameMessage.newBuilder();
		joinBuilder.setGameId(gameId);
		joinBuilder.setAutoLeave(autoLeave);
		joinBuilder.setSpectateOnly(spectateOnly);
		if (!password.isEmpty()) {
			joinBuilder.setPassword(password);
		}
		JoinExistingGameMessage joinExisting = joinBuilder.build();

		PokerTHMessage msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_JoinExistingGameMessage)
				.setJoinExistingGameMessage(joinExisting)
				.build();
		return msg;
	}

	public PokerTHMessage rejoinGameRequestMsg(int gameId, boolean autoLeave) {
		RejoinExistingGameMessage rejoinRequest = RejoinExistingGameMessage.newBuilder()
			.setGameId(gameId)
			.setAutoLeave(autoLeave)
			.build();

		PokerTHMessage msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_RejoinExistingGameMessage)
				.setRejoinExistingGameMessage(rejoinRequest)
				.build();
		return msg;
	}

	public NetGameInfo createGameInfo(NetGameInfo.NetGameType gameType, int playerActionTimeout, int proposedGuiSpeed, int delayBetweenHands, NetGameInfo.EndRaiseMode endMode, int endRaiseValue, int sb,
			String gameName, Collection<Integer> manualBlinds, int maxNumPlayers, int raiseEveryMinutes, int raiseEveryHands, int startMoney) {

		NetGameInfo.Builder infoBuilder = NetGameInfo.newBuilder();
		infoBuilder
			.setNetGameType(gameType)
			.setPlayerActionTimeout(playerActionTimeout)
			.setProposedGuiSpeed(proposedGuiSpeed)
			.setDelayBetweenHands(delayBetweenHands)
			.setEndRaiseMode(endMode)
			.setEndRaiseSmallBlindValue(endRaiseValue)
			.setFirstSmallBlind(sb)
			.setGameName(gameName)
			.setMaxNumPlayers(maxNumPlayers)
			.addAllManualBlinds(manualBlinds)
			.setStartMoney(startMoney);

		if (raiseEveryMinutes > 0) {
			infoBuilder
				.setRaiseIntervalMode(NetGameInfo.RaiseIntervalMode.raiseOnMinutes)
				.setRaiseEveryMinutes(raiseEveryMinutes);
		}
		else {
			infoBuilder
				.setRaiseIntervalMode(NetGameInfo.RaiseIntervalMode.raiseOnHandNum)
				.setRaiseEveryHands(raiseEveryHands);
		}

		return infoBuilder.build();
	}

	void failOnErrorMessage(PokerTHMessage msg) {
		if (msg.hasErrorMessage())
		{
			ErrorMessage error = msg.getErrorMessage();
			fail("Received error: " + error.getErrorReason().toString());
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
