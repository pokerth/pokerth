/*****************************************************************************
 * PokerTH dedicated server                                                  *
 * Copyright (C) 2014 Lothar May                                             *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
package gameserver

import (
	"code.google.com/p/goprotobuf/proto"
	"container/list"
	"log"
	"pokerth"
)

type Lobby struct {
	receiver chan SessionLobbyMessage
	sessions *list.List
}

func NewLobby() *Lobby {
	return &Lobby{make(chan SessionLobbyMessage, RECV_LOBBY_NUM_PACKET_BUF), list.New()}
}

func (l *Lobby) GetReceiver() chan SessionLobbyMessage {
	return l.receiver
}

func (l *Lobby) AddSession(session *Session) {
	log.Print("New session")
	l.sessions.PushBack(session)
	announce := &pokerth.PokerTHMessage{
		MessageType: pokerth.PokerTHMessage_PokerTHMessageType.Enum(pokerth.PokerTHMessage_Type_AnnounceMessage),
		AnnounceMessage: &pokerth.AnnounceMessage{
			ProtocolVersion: &pokerth.AnnounceMessage_Version{
				MajorVersion: proto.Uint32(NET_VERSION_MAJOR),
				MinorVersion: proto.Uint32(NET_VERSION_MINOR),
			},
			LatestGameVersion: &pokerth.AnnounceMessage_Version{
				MajorVersion: proto.Uint32(POKERTH_VERSION_MAJOR),
				MinorVersion: proto.Uint32(POKERTH_VERSION_MINOR),
			},
			LatestBetaRevision: proto.Uint32(0),
			ServerType:         pokerth.AnnounceMessage_ServerType.Enum(pokerth.AnnounceMessage_serverTypeInternetAuth),
			NumPlayersOnServer: proto.Uint32(0),
		},
	}
	session.sender <- announce
}

func (l *Lobby) Run() {
	var lobbyMsg SessionLobbyMessage
	for {
		select {
		case lobbyMsg = <-l.receiver:
			log.Printf("Lobby packet %d", lobbyMsg.packet.GetMessageType())
		}
	}
}
