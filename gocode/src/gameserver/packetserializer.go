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
	"encoding/binary"
	"pokerth"
)

const NET_HEADER_SIZE uint32 = 4

type PacketSerializer interface {
	ReadPacket(buf []byte, num uint32) (uint32, *pokerth.PokerTHMessage)
	WritePacket(buf []byte, packet *pokerth.PokerTHMessage) uint32
}

func readPacketRaw(buf []byte, num uint32) (uint32, *pokerth.PokerTHMessage) {
	packet := &pokerth.PokerTHMessage{}
	err := packet.Unmarshal(buf[0:num])
	if err != nil {
		packet = nil
	}
	return num, packet
}

func readPacketWithHeader(buf []byte, num uint32) (uint32, *pokerth.PokerTHMessage) {
	var bytesScanned uint32 = 0
	var packetSize uint32 = 0
	packet := &pokerth.PokerTHMessage{}
	if num >= NET_HEADER_SIZE {
		packetSize = binary.BigEndian.Uint32(buf[0:NET_HEADER_SIZE])
		if packetSize <= 0 {
			bytesScanned = NET_HEADER_SIZE
		} else if num >= NET_HEADER_SIZE+packetSize {
			bytesScanned, packet = readPacketRaw(buf[NET_HEADER_SIZE:], NET_HEADER_SIZE+packetSize)
		}
	}
	return bytesScanned, packet
}

func writePacketGeneric(buf []byte, packet *pokerth.PokerTHMessage, headerSize uint32) uint32 {
	var bytesWritten uint32 = 0
	num, err := packet.MarshalTo(buf[headerSize:])
	if err == nil {
		if headerSize > 0 {
			binary.BigEndian.PutUint32(buf[0:headerSize], uint32(num))
			bytesWritten = uint32(num) + headerSize
		} else {
			bytesWritten = uint32(num)
		}
	}
	return bytesWritten
}

type RawPacketSerializer struct {
}

func (RawPacketSerializer) ReadPacket(buf []byte, num uint32) (uint32, *pokerth.PokerTHMessage) {
	return readPacketRaw(buf, num)
}

func (RawPacketSerializer) WritePacket(buf []byte, packet *pokerth.PokerTHMessage) uint32 {
	return writePacketGeneric(buf, packet, 0)
}

type HeaderPacketSerializer struct {
}

func (HeaderPacketSerializer) ReadPacket(buf []byte, num uint32) (uint32, *pokerth.PokerTHMessage) {
	return readPacketWithHeader(buf, num)
}

func (HeaderPacketSerializer) WritePacket(buf []byte, packet *pokerth.PokerTHMessage) uint32 {
	return writePacketGeneric(buf, packet, NET_HEADER_SIZE)
}
