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

import java.net.Socket;
import java.util.ArrayList;
import java.util.Collection;

import org.junit.Test;

import pokerth_protocol.ChatRequestMessage;
import pokerth_protocol.ChatRequestTypeGame;
import pokerth_protocol.ChatRequestTypeLobby;
import pokerth_protocol.ChatRequestTypePrivate;
import pokerth_protocol.NetGameInfo;
import pokerth_protocol.NonZeroId;
import pokerth_protocol.PokerTHMessage;
import pokerth_protocol.ChatRequestMessage.ChatRequestMessageSequenceType;
import pokerth_protocol.ChatRequestMessage.ChatRequestMessageSequenceType.ChatRequestTypeChoiceType;
import pokerth_protocol.NetGameInfo.EndRaiseModeEnumType;
import pokerth_protocol.NetGameInfo.NetGameTypeEnumType;


public class ChatTest extends TestBase {

	final String ChatText = "Hello World ÖÄÜöäüẞ€";

	PokerTHMessage createLobbyChatMsg(String chatText) {
		ChatRequestTypeLobby chatLobby = new ChatRequestTypeLobby();
		ChatRequestTypeChoiceType chatType = new ChatRequestTypeChoiceType();
		chatType.selectChatRequestTypeLobby(chatLobby);
		ChatRequestMessageSequenceType chatSeq = new ChatRequestMessageSequenceType();
		chatSeq.setChatRequestType(chatType);
		chatSeq.setChatText(chatText);
		ChatRequestMessage chatRequest = new ChatRequestMessage();
		chatRequest.setValue(chatSeq);
		PokerTHMessage msg = new PokerTHMessage();
		msg.selectChatRequestMessage(chatRequest);
		return msg;
	}

	PokerTHMessage createGameChatMsg(String chatText, long gameId) {
		ChatRequestTypeGame chatGame = new ChatRequestTypeGame();
		chatGame.setGameId(new NonZeroId(gameId));
		ChatRequestTypeChoiceType chatType = new ChatRequestTypeChoiceType();
		chatType.selectChatRequestTypeGame(chatGame);
		ChatRequestMessageSequenceType chatSeq = new ChatRequestMessageSequenceType();
		chatSeq.setChatRequestType(chatType);
		chatSeq.setChatText(chatText);
		ChatRequestMessage chatRequest = new ChatRequestMessage();
		chatRequest.setValue(chatSeq);
		PokerTHMessage msg = new PokerTHMessage();
		msg.selectChatRequestMessage(chatRequest);
		return msg;
	}

	PokerTHMessage createPrivateChatMsg(String chatText, long playerId) {
		ChatRequestTypePrivate chatPrivate = new ChatRequestTypePrivate();
		chatPrivate.setTargetPlayerId(new NonZeroId(playerId));
		ChatRequestTypeChoiceType chatType = new ChatRequestTypeChoiceType();
		chatType.selectChatRequestTypePrivate(chatPrivate);
		ChatRequestMessageSequenceType chatSeq = new ChatRequestMessageSequenceType();
		chatSeq.setChatRequestType(chatType);
		chatSeq.setChatText(chatText);
		ChatRequestMessage chatRequest = new ChatRequestMessage();
		chatRequest.setValue(chatSeq);
		PokerTHMessage msg = new PokerTHMessage();
		msg.selectChatRequestMessage(chatRequest);
		return msg;
	}

	@Test
	public void testChat() throws Exception {
		guestInit();

		Socket s[] = new Socket[9];
		long playerId[] = new long[9];
		for (int i = 0; i < 9; i++) {
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
		} while (msg.isPlayerListMessageSelected());
		assertTrue(msg.isChatRejectMessageSelected());
		assertEquals(ChatText + 1, msg.getChatRejectMessage().getValue().getChatText());

		// Message as registered user should be sent to other users and guests.
		msg = createLobbyChatMsg(ChatText + 2);
		sendMessage(msg, s[0]);

		msg = receiveMessage();
		assertTrue(msg.isChatMessageSelected());
		assertEquals(ChatText + 2, msg.getChatMessage().getValue().getChatText());
		assertTrue(msg.getChatMessage().getValue().getChatType().isChatTypeLobbySelected());
		assertEquals(playerId[0], msg.getChatMessage().getValue().getChatType().getChatTypeLobby().getPlayerId().getValue().longValue());

		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage(s[i]);
			} while (msg.isPlayerListMessageSelected());
			assertTrue(msg.isChatMessageSelected());
			assertEquals(ChatText + 2, msg.getChatMessage().getValue().getChatText());
			assertTrue(msg.getChatMessage().getValue().getChatType().isChatTypeLobbySelected());
			assertEquals(playerId[0], msg.getChatMessage().getValue().getChatType().getChatTypeLobby().getPlayerId().getValue().longValue());
		}

		// A game chat message, if not within a game, should be rejected.
		msg = createGameChatMsg(ChatText + 3, 1);
		sendMessage(msg);

		msg = receiveMessage();
		assertTrue(msg.isChatRejectMessageSelected());
		assertEquals(ChatText + 3, msg.getChatRejectMessage().getValue().getChatText());

		msg = createGameChatMsg(ChatText + 4, 1);
		sendMessage(msg, s[0]);

		msg = receiveMessage(s[0]);
		assertTrue(msg.isChatRejectMessageSelected());
		assertEquals(ChatText + 4, msg.getChatRejectMessage().getValue().getChatText());

		// Guests are not allowed to send private messages in the lobby.
		msg = createPrivateChatMsg(ChatText + 5, playerId[1]);
		sendMessage(msg);

		msg = receiveMessage();
		assertTrue(msg.isChatRejectMessageSelected());
		assertEquals(ChatText + 5, msg.getChatRejectMessage().getValue().getChatText());

		// Registered users are allowed to send private messages in the lobby.
		msg = createPrivateChatMsg(ChatText + 6, playerId[1]);
		sendMessage(msg, s[0]);

		msg = receiveMessage(s[1]);
		assertTrue(msg.isChatMessageSelected());
		assertEquals(ChatText + 6, msg.getChatMessage().getValue().getChatText());
		assertTrue(msg.getChatMessage().getValue().getChatType().isChatTypePrivateSelected());
		assertEquals(playerId[0], msg.getChatMessage().getValue().getChatType().getChatTypePrivate().getPlayerId().getValue().longValue());

		// Game messages can be sent by registered users within a game.
		Collection<Integer> l = new ArrayList<Integer>();
		NetGameInfo gameInfo = createGameInfo(5, EndRaiseModeEnumType.EnumType.doubleBlinds, 0, 100, GuestUser + " game list normal game", l, 10, 0, 2, 2000);
		sendMessage(createGameRequestMsg(
				gameInfo,
				NetGameTypeEnumType.EnumType.normalGame,
				10,
				5,
				"",
				false));
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.isJoinGameReplyMessageSelected());
		assertTrue(msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected());
		long gameId = msg.getJoinGameReplyMessage().getValue().getGameId().getValue().longValue();

		// Let 8 players join the game, and test game chat.
		for (int i = 0; i < 8; i++) {
			sendMessage(joinGameRequestMsg(gameId, "", false), s[i]);
			do {
				msg = receiveMessage(s[i]);
				failOnErrorMessage(msg);
			} while (!msg.isJoinGameReplyMessageSelected());
			assertTrue(msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected());
		}

		// Guest user: not allowed.
		msg = createGameChatMsg(ChatText + 7, gameId);
		sendMessage(msg);
		do {
			msg = receiveMessage();
			failOnErrorMessage(msg);
		} while (!msg.isChatRejectMessageSelected());
		assertEquals(ChatText + 7, msg.getChatRejectMessage().getValue().getChatText());

		// Other users: allowed.
		for (int c = 0; c < 8; c++) {
			msg = createGameChatMsg(ChatText + "c" + c, gameId);
			sendMessage(msg, s[c]);
			do {
				msg = receiveMessage();
				failOnErrorMessage(msg);
				assertFalse(msg.isChatRejectMessageSelected());
			} while (!msg.isChatMessageSelected());
	
			assertEquals(ChatText + "c" + c, msg.getChatMessage().getValue().getChatText());
			assertTrue(msg.getChatMessage().getValue().getChatType().isChatTypeGameSelected());
			assertEquals(playerId[c], msg.getChatMessage().getValue().getChatType().getChatTypeGame().getPlayerId().getValue().longValue());
			assertEquals(gameId, msg.getChatMessage().getValue().getChatType().getChatTypeGame().getGameId().getValue().longValue());
	
			for (int i = 0; i < 8; i++) {
				do {
					msg = receiveMessage(s[i]);
					failOnErrorMessage(msg);
					assertFalse(msg.isChatRejectMessageSelected());
				} while (!msg.isChatMessageSelected());
				assertEquals(ChatText + "c" + c, msg.getChatMessage().getValue().getChatText());
				assertTrue(msg.getChatMessage().getValue().getChatType().isChatTypeGameSelected());
				assertEquals(playerId[c], msg.getChatMessage().getValue().getChatType().getChatTypeGame().getPlayerId().getValue().longValue());
				assertEquals(gameId, msg.getChatMessage().getValue().getChatType().getChatTypeGame().getGameId().getValue().longValue());
			}
		}

		// Private and lobby messages are forbidden once the game is running.
		sendMessage(joinGameRequestMsg(gameId, "", false), s[8]);
		do {
			msg = receiveMessage(s[8]);
			failOnErrorMessage(msg);
			// This player was not in the game and should not have received chat messages.
			assertFalse(msg.isChatMessageSelected());
		} while (!msg.isJoinGameReplyMessageSelected());
		assertTrue(msg.getJoinGameReplyMessage().getValue().getJoinGameResult().isJoinGameAckSelected());


		// Server should confirm start event.
		do {
			msg = receiveMessage(s[0]);
			failOnErrorMessage(msg);
		} while (!msg.isGameStartMessageSelected());

		// Private chat message should now be rejected.
		msg = createPrivateChatMsg(ChatText + 8, playerId[1]);
		sendMessage(msg, s[0]);
		do {
			msg = receiveMessage(s[0]);
			failOnErrorMessage(msg);
			assertFalse(msg.isChatMessageSelected());
		} while (!msg.isChatRejectMessageSelected());

		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
	}
}
