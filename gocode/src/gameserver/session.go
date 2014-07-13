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
	"log"
	"net"
	"pokerth"
)

const POKERTH_VERSION_MAJOR = 1
const POKERTH_VERSION_MINOR = 11

const NET_VERSION_MAJOR = 5
const NET_VERSION_MINOR = 1

const MAX_PACKET_SIZE uint32 = 384
const RECV_BUF_SIZE uint32 = 4 * MAX_PACKET_SIZE
const SEND_BUF_SIZE uint32 = 2 * MAX_PACKET_SIZE
const SEND_NUM_PACKET_BUF = 2048
const RECV_DISPATCHER_NUM_PACKET_BUF = 2048
const RECV_LOBBY_NUM_PACKET_BUF = 2048

type Session struct {
	id uint32
	PacketSerializer
	Connection net.Conn
	sender     chan *pokerth.PokerTHMessage
	receiver   chan SessionPokerTHMessage
}

type SessionPokerTHMessage struct {
	session *Session
	packet  *pokerth.PokerTHMessage
}

type SessionAuthMessage struct {
	session *Session
	packet  *pokerth.AuthMessage
}

type SessionLobbyMessage struct {
	session *Session
	packet  *pokerth.LobbyMessage
}

type SessionGameMessage struct {
	session *Session
	packet  *pokerth.GameMessage
}

func NewSession(id uint32, serializer PacketSerializer, conn net.Conn, receiver chan SessionPokerTHMessage) *Session {
	return &Session{id, serializer, conn, make(chan *pokerth.PokerTHMessage, SEND_NUM_PACKET_BUF), receiver}
}

func (s *Session) Run() {
	// run sender as separate goroutine
	go s.handleSend()
	s.handleReceive()
}

func (s *Session) handleReceive() {
	// close connection on exit
	defer s.Connection.Close()

	var buf [RECV_BUF_SIZE]byte
	var bufPos uint32 = 0
	for {
		// read upto RECV_BUF_SIZE bytes
		num, err := s.Connection.Read(buf[bufPos:])
		bufPos += uint32(num)
		if err != nil {
			log.Printf("Read error:  %s\n", err.Error())
			return
		}

		bytesScanned, packet := s.ReadPacket(buf[0:bufPos], bufPos)

		if bytesScanned > 0 {
			if packet == nil {
				log.Print("Invalid packet")
			} else {
				log.Printf("Packet in: %d\n", packet.GetMessageType())
				s.receiver <- SessionPokerTHMessage{s, packet}
				remainingBytes := bufPos - bytesScanned
				if remainingBytes > 0 {
					copy(buf[0:], buf[bytesScanned:remainingBytes])
					bufPos = remainingBytes
				} else {
					bufPos = 0
				}
			}
		}
	}
}

func (s *Session) handleSend() {
	var buf [RECV_BUF_SIZE]byte
	var packet *pokerth.PokerTHMessage
	for {
		select {
		case packet = <-s.sender:
			packetSize := s.WritePacket(buf[0:RECV_BUF_SIZE], packet)
			if packetSize > 0 {
				var bufStart uint32 = 0
				for bufStart < packetSize {
					num, err := s.Connection.Write(buf[bufStart:packetSize])
					if err != nil {
						log.Printf("Write error:  %s\n", err.Error())
						return
					}
					bufStart += uint32(num)
				}
				log.Printf("Packet out: %d, size: %d\n", packet.GetMessageType(), packetSize)
			}
		}
	}
}
