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
	"pokerth"
	"sync/atomic"
)

type Dispatcher struct {
	receiver      chan SessionPokerTHMessage
	authChan      chan SessionAuthMessage
	lobbyChan     chan SessionLobbyMessage
	lastSessionId uint32
}

func NewDispatcher(authChan chan SessionAuthMessage, lobbyChan chan SessionLobbyMessage) *Dispatcher {
	return &Dispatcher{make(chan SessionPokerTHMessage, RECV_DISPATCHER_NUM_PACKET_BUF), authChan, lobbyChan, 0}
}

func (d *Dispatcher) GetReceiver() chan SessionPokerTHMessage {
	return d.receiver
}

func (d *Dispatcher) GetNextSessionId() uint32 {
	return atomic.AddUint32(&d.lastSessionId, 1)
}

func (d *Dispatcher) Run() {
	var sessionMsg SessionPokerTHMessage
	for {
		select {
		case sessionMsg = <-d.receiver:
			log.Printf("Packet in dispatcher session %d type %d", sessionMsg.session.id, sessionMsg.packet.GetMessageType())
			switch sessionMsg.packet.GetMessageType() {
			case pokerth.PokerTHMessage_Type_AuthMessage:
				d.authChan <- SessionAuthMessage{sessionMsg.session, sessionMsg.packet.GetAuthMessage()}
			case pokerth.PokerTHMessage_Type_LobbyMessage:
				d.lobbyChan <- SessionLobbyMessage{sessionMsg.session, sessionMsg.packet.GetLobbyMessage()}
			default:
				log.Print("Unknown packet type")
			}
		}
	}
}
