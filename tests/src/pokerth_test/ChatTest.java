package pokerth_test;

import java.net.Socket;

import org.junit.Test;

import pokerth_protocol.ChatRequestMessage;
import pokerth_protocol.ChatRequestTypeLobby;
import pokerth_protocol.PokerTHMessage;
import pokerth_protocol.ChatRequestMessage.ChatRequestMessageSequenceType;
import pokerth_protocol.ChatRequestMessage.ChatRequestMessageSequenceType.ChatRequestTypeChoiceType;


public class ChatTest extends TestBase {

	@Test
	public void testLobbyChat() throws Exception {
		guestInit();

		Socket s[] = new Socket[9];
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			userInit(s[i], username, password);
		}

		ChatRequestTypeLobby chatLobby = new ChatRequestTypeLobby();
		ChatRequestTypeChoiceType chatType = new ChatRequestTypeChoiceType();
		chatType.selectChatRequestTypeLobby(chatLobby);
		ChatRequestMessageSequenceType chatSeq = new ChatRequestMessageSequenceType();
		chatSeq.setChatRequestType(chatType);
		chatSeq.setChatText("Hello World");
		ChatRequestMessage chatRequest = new ChatRequestMessage();
		chatRequest.setValue(chatSeq);
		PokerTHMessage msg = new PokerTHMessage();
		msg.selectChatRequestMessage(chatRequest);
		sendMessage(msg);

		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
	}
}
