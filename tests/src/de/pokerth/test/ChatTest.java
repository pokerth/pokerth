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

import java.net.Socket;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.ChatMessage.ChatType;
import de.pokerth.protocol.ProtoBuf.ChatRequestMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo;
import de.pokerth.protocol.ProtoBuf.StartEventAckMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.EndRaiseMode;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;
import de.pokerth.protocol.ProtoBuf.NetGameInfo.NetGameType;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;
import de.pokerth.protocol.ProtoBuf.StartEventMessage;
import de.pokerth.protocol.ProtoBuf.StartEventMessage.StartEventType;


public class ChatTest extends TestBase {

	final String ChatText = "Hello World ÖÄÜöäüẞ€";

	PokerTHMessage createLobbyChatMsg(String chatText) {
		ChatRequestMessage chatLobby = ChatRequestMessage.newBuilder()
			.setChatText(chatText)
			.build();
		PokerTHMessage msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_ChatRequestMessage)
				.setChatRequestMessage(chatLobby)
				.build();
		return msg;
	}

	PokerTHMessage createGameChatMsg(String chatText, int gameId) {
		ChatRequestMessage chatGame = ChatRequestMessage.newBuilder()
				.setChatText(chatText)
				.setTargetGameId(gameId)
				.build();
		PokerTHMessage msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_ChatRequestMessage)
				.setChatRequestMessage(chatGame)
				.build();
		return msg;
	}

	PokerTHMessage createPrivateChatMsg(String chatText, int playerId) {
		ChatRequestMessage chatPrivate = ChatRequestMessage.newBuilder()
				.setChatText(chatText)
				.setTargetPlayerId(playerId)
				.build();
		PokerTHMessage msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_ChatRequestMessage)
				.setChatRequestMessage(chatPrivate)
				.build();
		return msg;
	}

	@Test
	public void testChat() throws Exception {
		guestInit();

		Socket s[] = new Socket[8];
		int playerId[] = new int[8];
		for (int i = 0; i < 8; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			playerId[i] = userInit(s[i], username, password);
		}

		PokerTHMessage msg = createLobbyChatMsg(ChatText + 1);
		// Message as guest user should be rejected.
		sendMessage(msg);
		do {
			msg = receiveMessage();
		} while (msg.hasPlayerListMessage());
		assertTrue(msg.hasChatRejectMessage());
		assertEquals(ChatText + 1, msg.getChatRejectMessage().getChatText());

		// Message as registered user should be sent to other users and guests.
		msg = createLobbyChatMsg(ChatText + 2);
		sendMessage(msg, s[0]);

		msg = receiveMessage();
		assertTrue(msg.hasChatMessage() && msg.getMessageType() == PokerTHMessageType.Type_ChatMessage);
		assertEquals(ChatText + 2, msg.getChatMessage().getChatText());
		assertEquals(ChatType.chatTypeLobby, msg.getChatMessage().getChatType());
		assertEquals(playerId[0], msg.getChatMessage().getPlayerId());

		for (int i = 0; i < 8; i++) {
			do {
				msg = receiveMessage(s[i]);
			} while (msg.hasPlayerListMessage());
			assertTrue(msg.hasChatMessage() && msg.getMessageType() == PokerTHMessageType.Type_ChatMessage);
			assertEquals(ChatText + 2, msg.getChatMessage().getChatText());
			assertEquals(ChatType.chatTypeLobby, msg.getChatMessage().getChatType());
			assertEquals(playerId[0], msg.getChatMessage().getPlayerId());
		}

		// A game chat message, if not within a game, should be rejected.
		msg = createGameChatMsg(ChatText + 3, 1);
		sendMessage(msg);

		msg = receiveMessage();
		assertTrue(msg.hasChatRejectMessage() && msg.getMessageType() == PokerTHMessageType.Type_ChatRejectMessage);
		assertEquals(ChatText + 3, msg.getChatRejectMessage().getChatText());

		msg = createGameChatMsg(ChatText + 4, 1);
		sendMessage(msg, s[0]);

		msg = receiveMessage(s[0]);
		assertTrue(msg.hasChatRejectMessage() && msg.getMessageType() == PokerTHMessageType.Type_ChatRejectMessage);
		assertEquals(ChatText + 4, msg.getChatRejectMessage().getChatText());

		// Guests are not allowed to send private messages in the lobby.
		msg = createPrivateChatMsg(ChatText + 5, playerId[1]);
		sendMessage(msg);

		msg = receiveMessage();
		assertTrue(msg.hasChatRejectMessage() && msg.getMessageType() == PokerTHMessageType.Type_ChatRejectMessage);
		assertEquals(ChatText + 5, msg.getChatRejectMessage().getChatText());

		// Registered users are allowed to send private messages in the lobby.
		msg = createPrivateChatMsg(ChatText + 6, playerId[1]);
		sendMessage(msg, s[0]);

		msg = receiveMessage(s[1]);
		assertTrue(msg.hasChatMessage() && msg.getMessageType() == PokerTHMessageType.Type_ChatMessage);
		assertEquals(ChatText + 6, msg.getChatMessage().getChatText());
		assertEquals(ChatType.chatTypePrivate, msg.getChatMessage().getChatType());
		assertEquals(playerId[0], msg.getChatMessage().getPlayerId());

		// Game messages can be sent by registered users within a game.
		Collection<Integer> l = new ArrayList<Integer>();
		NetGameInfo gameInfo = createGameInfo(NetGameType.normalGame, 10, 5, 5, EndRaiseMode.doubleBlinds, 0, 100, GuestUser + " game list normal game", l, 10, 0, 2, 2000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				"",
				false));
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasJoinGameAckMessage() && !msg.hasJoinGameFailedMessage());
		assertTrue(msg.hasJoinGameAckMessage() && msg.getMessageType() == PokerTHMessageType.Type_JoinGameAckMessage);
		int gameId = msg.getJoinGameAckMessage().getGameId();

		// Let 8 players join the game, and test game chat.
		for (int i = 0; i < 8; i++) {
			sendMessage(joinGameRequestMsg(gameId, "", false), s[i]);
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.hasJoinGameAckMessage() && !msg.hasJoinGameFailedMessage());
			assertTrue(msg.hasJoinGameAckMessage() && msg.getMessageType() == PokerTHMessageType.Type_JoinGameAckMessage);
		}

		StartEventMessage startEvent = StartEventMessage.newBuilder()
				.setGameId(gameId)
				.setFillWithComputerPlayers(false)
				.setStartEventType(StartEventType.startEvent)
				.build();
		msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_StartEventMessage)
				.setStartEventMessage(startEvent)
				.build();
		sendMessage(msg);

		// Server should confirm start event.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasStartEventMessage());

		// Acknowledge start event.
		StartEventAckMessage startAck = StartEventAckMessage.newBuilder()
				.setGameId(gameId)
				.build();
		msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_StartEventAckMessage)
				.setStartEventAckMessage(startAck)
				.build();
		sendMessage(msg);
		
		for (int i = 0; i < 8; i++) {
			sendMessage(msg, s[i]);
		}

		// Server should game start.
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.hasGameStartInitialMessage());


		// Guest user: not allowed.
		msg = createGameChatMsg(ChatText + 7, gameId);
		sendMessage(msg);
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
			assertFalse(msg.hasChatMessage());
		} while (!msg.hasChatRejectMessage());
		assertEquals(ChatText + 7, msg.getChatRejectMessage().getChatText());

		// Other users: allowed.
		for (int c = 0; c < 8; c++) {
			msg = createGameChatMsg(ChatText + "c" + c, gameId);
			sendMessage(msg, s[c]);
			do {
				msg = receiveMessage(s[c]);
				failOnErrorMessage(msg);
				assertFalse(msg.hasChatRejectMessage() || msg.getMessageType() == PokerTHMessageType.Type_ChatRejectMessage);
			} while (!msg.hasChatMessage());
	
			assertEquals(ChatText + "c" + c, msg.getChatMessage().getChatText());
			assertEquals(ChatType.chatTypeGame, msg.getChatMessage().getChatType());
			assertEquals(playerId[c], msg.getChatMessage().getPlayerId());
			assertEquals(gameId, msg.getChatMessage().getGameId());

			for (int i = 0; i < 8; i++) {
				if (i != c) {
					do {
						msg = receiveMessage(s[i]);
						failOnErrorMessage(msg);
						assertFalse(msg.hasChatRejectMessage() || msg.getMessageType() == PokerTHMessageType.Type_ChatRejectMessage);
					} while (!msg.hasChatMessage());
					assertEquals(ChatText + "c" + c, msg.getChatMessage().getChatText());
					assertEquals(ChatType.chatTypeGame, msg.getChatMessage().getChatType());
					assertEquals(playerId[c], msg.getChatMessage().getPlayerId());
					assertEquals(gameId, msg.getChatMessage().getGameId());
				}
			}
		}

		// Private chat message should now be rejected.
		msg = createPrivateChatMsg(ChatText + 8, playerId[1]);
		sendMessage(msg, s[0]);
		do {
			msg = receiveMessage(s[0]);
			failOnErrorMessage(msg);
			assertFalse(msg.hasChatMessage() || msg.getMessageType() == PokerTHMessageType.Type_ChatMessage);
		} while (!msg.hasChatRejectMessage());

		for (int i = 0; i < 8; i++) {
			s[i].close();
		}
	}
}
