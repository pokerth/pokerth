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

import java.net.URI;
import java.nio.ByteBuffer;
import java.util.concurrent.ArrayBlockingQueue;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.AnnounceMessage;
import de.pokerth.protocol.ProtoBuf.AuthClientRequestMessage;
import de.pokerth.protocol.ProtoBuf.AuthMessage;
import de.pokerth.protocol.ProtoBuf.InitDoneMessage;
import de.pokerth.protocol.ProtoBuf.AnnounceMessage.ServerType;
import de.pokerth.protocol.ProtoBuf.AuthMessage.AuthMessageType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;
import de.pokerth.protocol.ProtoBuf.LobbyMessage.LobbyMessageType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;

import org.java_websocket.client.WebSocketClient;
import org.java_websocket.handshake.ServerHandshake;

public class WebSocketTest extends TestBase {

	protected static final String POKERTHWEBSOCKET = "ws://localhost:7233/pokerthwebsocket";

	class PokerTHWebSocketClient extends WebSocketClient {
		
		public ArrayBlockingQueue<ByteBuffer> messageQueue;

		public PokerTHWebSocketClient(URI serverUri) {
			super(serverUri);
			messageQueue = new ArrayBlockingQueue<ByteBuffer>(100);
		}

		@Override
		public void onOpen(ServerHandshake handshakedata) {
		}

		@Override
		public void onMessage(String message) {
			fail("All messages should be binary.");
		}

		@Override
		public void onMessage(ByteBuffer message) {
			try {
				messageQueue.put(message);
			} catch (InterruptedException e) {
				fail("Message interrupted");
			}
		}

		@Override
		public void onClose(int code, String reason, boolean remote) {
		}

		@Override
		public void onError(Exception ex) {
			fail("WebSocket error: " + ex.getStackTrace());
		}
	}

	protected void TestAnnounceMsg(AnnounceMessage announce, int numPlayersOnServer) {
		assertEquals(PROTOCOL_VERSION_MAJOR, announce.getProtocolVersion().getMajorVersion());
		assertEquals(PROTOCOL_VERSION_MINOR, announce.getProtocolVersion().getMinorVersion());
		assertEquals(ServerType.serverTypeInternetAuth, announce.getServerType());
		assertEquals(numPlayersOnServer, announce.getNumPlayersOnServer());
	}

	@Test
	public void testWebSocketGuestLogin() throws Exception {

		PokerTHWebSocketClient webClient = new PokerTHWebSocketClient(new URI(POKERTHWEBSOCKET));
		
		webClient.connectBlocking();
		ByteBuffer buffer = webClient.messageQueue.take();
		PokerTHMessage msg = PokerTHMessage.parseFrom(buffer.array());
		assertTrue(msg.hasAnnounceMessage());
		TestAnnounceMsg(msg.getAnnounceMessage(), 0);

		AnnounceMessage.Version requestedVersion = AnnounceMessage.Version.newBuilder()
			.setMajorVersion(PROTOCOL_VERSION_MAJOR)
			.setMinorVersion(PROTOCOL_VERSION_MINOR)
			.build();
		AuthClientRequestMessage init = AuthClientRequestMessage.newBuilder()
			.setBuildId(0)
			.setLogin(AuthClientRequestMessage.LoginType.guestLogin)
			.setRequestedVersion(requestedVersion)
			.setNickName(GuestUser)
			.build();
		AuthMessage auth = AuthMessage.newBuilder()
			.setMessageType(AuthMessageType.Type_AuthClientRequestMessage)
			.setAuthClientRequestMessage(init)
			.build();
		msg = PokerTHMessage.newBuilder()
			.setMessageType(PokerTHMessageType.Type_AuthMessage)
			.setAuthMessage(auth)
			.build();
		webClient.send(msg.toByteArray());

		buffer = webClient.messageQueue.take();
		msg = PokerTHMessage.parseFrom(buffer.array());

		if (msg.hasLobbyMessage() && msg.getMessageType() == PokerTHMessageType.Type_LobbyMessage
			&& msg.getLobbyMessage().hasInitDoneMessage() && msg.getLobbyMessage().getMessageType() == LobbyMessageType.Type_InitDoneMessage) {
			InitDoneMessage initDone = msg.getLobbyMessage().getInitDoneMessage();
			assertTrue(initDone.getYourPlayerId() != 0L);
			assertTrue(!initDone.hasYourAvatarHash());
		}
		else {
			failOnErrorMessage(msg);
			fail("Invalid message.");
		}

		webClient.close();
	}
}
