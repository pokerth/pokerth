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
package main

import (
	"code.google.com/p/go.net/websocket"
	"gameserver"
	"log"
	"net"
	"net/http"
)

var lobby *gameserver.Lobby
var dispatcher *gameserver.Dispatcher

func main() {
	lobby = gameserver.NewLobby()
	dispatcher = gameserver.NewDispatcher(lobby)
	go dispatcher.Run()

	listener, err := net.Listen("tcp", ":7234")
	if err != nil {
		log.Fatalf("Listen error: %s", err.Error())
	}
	go acceptSockets(listener)

	server := websocket.Server{Handler: handleWebsocketConn}
	http.Handle("/pokerthwebsocket", server)
	http.ListenAndServe(":7233", nil)
}

func acceptSockets(listener net.Listener) {
	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Fatalf("Accept error: %s\n", err.Error())
		}
		s := gameserver.NewSession(dispatcher.GetNextSessionId(), gameserver.HeaderPacketSerializer{}, conn, dispatcher.GetReceiver())
		lobby.AddSession(s)
		go s.Run()
	}
}

func handleWebsocketConn(ws *websocket.Conn) {
	ws.PayloadType = websocket.BinaryFrame
	s := gameserver.NewSession(dispatcher.GetNextSessionId(), gameserver.RawPacketSerializer{}, ws, dispatcher.GetReceiver())
	lobby.AddSession(s)
	s.Run()
}
