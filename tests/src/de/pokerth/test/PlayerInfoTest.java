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

import java.net.Socket;

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;

import org.junit.Test;

import de.pokerth.protocol.ProtoBuf.NetAvatarType;
import de.pokerth.protocol.ProtoBuf.NetPlayerInfoRights;
import de.pokerth.protocol.ProtoBuf.PlayerInfoReplyMessage;
import de.pokerth.protocol.ProtoBuf.PlayerInfoReplyMessage.PlayerInfoData;
import de.pokerth.protocol.ProtoBuf.PlayerInfoRequestMessage;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage;
import de.pokerth.protocol.ProtoBuf.PokerTHMessage.PokerTHMessageType;

public class PlayerInfoTest extends TestBase {

	protected void sendPlayerInfoRequest(Socket s, int playerId) throws Exception {
		Collection<Integer> tmpList = new ArrayList<Integer>();
		tmpList.add(playerId);
		sendPlayerInfoRequest(s, tmpList);
	}

	protected void sendPlayerInfoRequest(Socket s, Collection<Integer> playerIds) throws Exception {
		PlayerInfoRequestMessage request = PlayerInfoRequestMessage.newBuilder()
			.addAllPlayerId(playerIds)
			.build();
		PokerTHMessage msg = PokerTHMessage.newBuilder()
				.setMessageType(PokerTHMessageType.Type_PlayerInfoRequestMessage)
				.setPlayerInfoRequestMessage(request)
				.build();
		sendMessage(msg, s);
	}

	@Test
	public void testPlayerInfoRequest() throws Exception {

		int firstPlayerId = guestInit();
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
		int playerId[] = new int[9];
		int maxPlayerId = 0;
		for (int i = 0; i < 9; i++) {
			s[i] = new Socket("localhost", 7234);
			String username = "test" + (i+1);
			String password = username;
			// Every second player has an avatar.
			if (i % 2 == 0) {
				playerId[i] = userInit(s[i], username, password);
			} else {
				playerId[i] = userInit(s[i], username, password, avatarHash, null);
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
			} while (msg.hasPlayerListMessage());
			assertTrue(msg.hasPlayerInfoReplyMessage() && msg.getMessageType() == PokerTHMessageType.Type_PlayerInfoReplyMessage);
			PlayerInfoReplyMessage reply = msg.getPlayerInfoReplyMessage();
			assertTrue(reply.getPlayerId() == firstPlayerId);
			assertTrue(reply.hasPlayerInfoData());
			PlayerInfoData info = reply.getPlayerInfoData();
			assertEquals(GuestUser, info.getPlayerName());
			assertFalse(info.hasCountryCode());
			assertTrue(info.getIsHuman());
			assertEquals(NetPlayerInfoRights.netPlayerRightsGuest, info.getPlayerRights());
			assertFalse(info.hasAvatarData());
		}
		// Request other players' info (one request containing 9 queries).
		Collection<Integer> tmpList = new ArrayList<Integer>();
		for (Integer id : playerId) { tmpList.add(id); }
		sendPlayerInfoRequest(sock, tmpList);

		for (int i = 0; i < 9; i++) {
			do {
				msg = receiveMessage();
			} while (msg.hasPlayerListMessage());
			assertTrue(msg.hasPlayerInfoReplyMessage() && msg.getMessageType() == PokerTHMessageType.Type_PlayerInfoReplyMessage);
			PlayerInfoReplyMessage reply = msg.getPlayerInfoReplyMessage();
			assertTrue(reply.getPlayerId() == playerId[i]);
			assertTrue(reply.hasPlayerInfoData());
			PlayerInfoData info = reply.getPlayerInfoData();
			assertEquals("test" + (i+1), info.getPlayerName());
			assertFalse(info.hasCountryCode());
			assertTrue(info.getIsHuman());
			assertEquals(NetPlayerInfoRights.netPlayerRightsNormal, info.getPlayerRights());
			// Every second player has an avatar, see above.
			if (i % 2 == 0) {
				assertFalse(info.hasAvatarData());
			} else {
				assertTrue(info.hasAvatarData());
				assertTrue(Arrays.equals(info.getAvatarData().getAvatarHash().toByteArray(), avatarHash));
				assertEquals(NetAvatarType.netAvatarImagePng, info.getAvatarData().getAvatarType());
			}
		}
		// Request invalid player info.
		sendPlayerInfoRequest(sock, maxPlayerId + 1);
		msg = receiveMessage();
		assertTrue(msg.hasPlayerInfoReplyMessage() && msg.getMessageType() == PokerTHMessageType.Type_PlayerInfoReplyMessage);
		PlayerInfoReplyMessage reply = msg.getPlayerInfoReplyMessage();
		assertTrue(reply.getPlayerId() == maxPlayerId + 1);
		assertFalse(reply.hasPlayerInfoData());

		for (int i = 0; i < 9; i++) {
			s[i].close();
		}
	}
}
