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

import java.net.Socket;

import static org.junit.Assert.*;

import java.util.Arrays;

import org.junit.Test;

import pokerth_protocol.*;
import pokerth_protocol.PlayerInfoRequestMessage.PlayerInfoRequestMessageSequenceType;

public class PlayerInfoTest extends TestBase {

	protected void sendPlayerInfoRequest(Socket s, long playerId) throws Exception {
		PlayerInfoRequestMessageSequenceType type = new PlayerInfoRequestMessageSequenceType();
		type.setPlayerId(new NonZeroId(playerId));
		PlayerInfoRequestMessage request = new PlayerInfoRequestMessage();
		request.setValue(type);
		PokerTHMessage msg = new PokerTHMessage();
		msg.selectPlayerInfoRequestMessage(request);
		sendMessage(msg, s);
	}

	@Test
	public void testPlayerInfoRequest() throws Exception {

		long firstPlayerId = guestInit();
		byte[] avatarHash =
		{
				// one of the builtin avatars.
				(byte)0x00, (byte)0xa0, (byte)0xb3, (byte)0xd2,
				(byte)0x6a, (byte)0x67, (byte)0x84, (byte)0x12,
				(byte)0x39, (byte)0xb8, (byte)0x88, (byte)0x31,
				(byte)0x83, (byte)0xb7, (byte)0xa8, (byte)0xf0
		};

		// Let 9 additional clients join.
		Socket s[] = new Socket[9];
		long playerId[] = new long[9];
		long maxPlayerId = 0;
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			// Every second player has an avatar.
			if (i % 2 == 0) {
				playerId[i] = userInit(s[i], username, password);
			} else {
				playerId[i] = userInit(s[i], username, password, avatarHash);
			}
			if (playerId[i] > maxPlayerId) {
				maxPlayerId = playerId[i];
			}
		}
		PokerTHMessage msg;

		// Request player info, for guest first.
		for (int i = 0; i < 9; i++) {
			sendPlayerInfoRequest(s[i], firstPlayerId);
			do {
				msg = receiveMessage(s[i]);
			} while (msg.isPlayerListMessageSelected());
			assertTrue(msg.isPlayerInfoReplyMessageSelected());
			PlayerInfoReplyMessage reply = msg.getPlayerInfoReplyMessage();
			assertTrue(reply.getValue().getPlayerId().getValue() == firstPlayerId);
			assertTrue(reply.getValue().getPlayerInfoResult().isPlayerInfoDataSelected());
			PlayerInfoData info = reply.getValue().getPlayerInfoResult().getPlayerInfoData();
			assertEquals(GuestUser, info.getPlayerName());
			assertEquals(null, info.getCountryCode());
			assertTrue(info.getIsHuman());
			assertEquals(PlayerInfoRights.EnumType.playerRightsGuest, info.getPlayerRights().getValue());
			assertEquals(null, info.getAvatarData());
		}
		// Request other players' info.
		for (int i = 0; i < 9; i++) {
			sendPlayerInfoRequest(sock, playerId[i]);
			do {
				msg = receiveMessage();
			} while (msg.isPlayerListMessageSelected());
			assertTrue(msg.isPlayerInfoReplyMessageSelected());
			PlayerInfoReplyMessage reply = msg.getPlayerInfoReplyMessage();
			assertTrue(reply.getValue().getPlayerId().getValue() == playerId[i]);
			assertTrue(reply.getValue().getPlayerInfoResult().isPlayerInfoDataSelected());
			PlayerInfoData info = reply.getValue().getPlayerInfoResult().getPlayerInfoData();
			assertEquals("test" + (i+1), info.getPlayerName());
			assertEquals(null, info.getCountryCode());
			assertTrue(info.getIsHuman());
			assertEquals(PlayerInfoRights.EnumType.playerRightsNormal, info.getPlayerRights().getValue());
			// Every second player has an avatar, see above.
			if (i % 2 == 0) {
				assertEquals(null, info.getAvatarData());
			} else {
				assertTrue(Arrays.equals(info.getAvatarData().getAvatar().getValue(), avatarHash));
				assertEquals(NetAvatarType.EnumType.avatarImagePng, info.getAvatarData().getAvatarType().getValue());
			}
		}
		// Request invalid player info.
		sendPlayerInfoRequest(sock, maxPlayerId + 1);
		msg = receiveMessage();
		assertTrue(msg.isPlayerInfoReplyMessageSelected());
		PlayerInfoReplyMessage reply = msg.getPlayerInfoReplyMessage();
		assertTrue(reply.getValue().getPlayerId().getValue() == maxPlayerId + 1);
		assertTrue(reply.getValue().getPlayerInfoResult().isUnknownPlayerInfoSelected());

		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
	}
}
