/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
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
/* Callback interface for network client gui. */

#ifndef _CLIENTCALLBACK_H_
#define _CLIENTCALLBACK_H_

#include <boost/shared_ptr.hpp>
#include <string>

#include <game_defs.h>
#include <gamedata.h>
#include <serverdata.h>

class Game;

class ClientCallback
{
public:
	virtual ~ClientCallback();

	virtual void SignalNetClientConnect(int actionID) = 0;
	virtual void SignalNetClientGameInfo(int actionID) = 0;
	virtual void SignalNetClientError(int errorID, int osErrorID) = 0;
	virtual void SignalNetClientNotification(int notificationId) = 0;
	virtual void SignalNetClientStatsUpdate(const ServerStats &stats) = 0;
	virtual void SignalNetClientPingUpdate(unsigned minPing, unsigned avgPing, unsigned maxPing) = 0;
	virtual void SignalNetClientShowTimeoutDialog(NetTimeoutReason reason, unsigned remainingSec) = 0;
	virtual void SignalNetClientRemovedFromGame(int notificationId) = 0;

	virtual void SignalNetClientGameListNew(unsigned gameId) = 0;
	virtual void SignalNetClientGameListRemove(unsigned gameId) = 0;
	virtual void SignalNetClientGameListUpdateMode(unsigned gameId, GameMode mode) = 0;
	virtual void SignalNetClientGameListUpdateAdmin(unsigned gameId, unsigned adminPlayerId) = 0;
	virtual void SignalNetClientGameListPlayerJoined(unsigned gameId, unsigned playerId) = 0;
	virtual void SignalNetClientGameListPlayerLeft(unsigned gameId, unsigned playerId) = 0;
	virtual void SignalNetClientGameListSpectatorJoined(unsigned gameId, unsigned playerId) = 0;
	virtual void SignalNetClientGameListSpectatorLeft(unsigned gameId, unsigned playerId) = 0;

	virtual void SignalNetClientGameStart(boost::shared_ptr<Game> game) = 0;
	virtual void SignalNetClientSelfJoined(unsigned playerId, const std::string &playerName, bool isGameAdmin) = 0;
	virtual void SignalNetClientPlayerJoined(unsigned playerId, const std::string &playerName, bool isGameAdmin) = 0;
	virtual void SignalNetClientPlayerChanged(unsigned playerId, const std::string &newPlayerName) = 0;
	virtual void SignalNetClientPlayerLeft(unsigned playerId, const std::string &playerName, int removeReason) = 0;
	virtual void SignalNetClientSpectatorJoined(unsigned playerId, const std::string &playerName) = 0;
	virtual void SignalNetClientSpectatorLeft(unsigned playerId, const std::string &playerName, int removeReason) = 0;
	virtual void SignalNetClientNewGameAdmin(unsigned playerId, const std::string &playerName) = 0;

	virtual void SignalNetClientGameChatMsg(const std::string &playerName, const std::string &msg) = 0;
	virtual void SignalNetClientLobbyChatMsg(const std::string &playerName, const std::string &msg) = 0;
	virtual void SignalNetClientPrivateChatMsg(const std::string &playerName, const std::string &msg) = 0;
	virtual void SignalNetClientMsgBox(const std::string &msg) = 0;
	virtual void SignalNetClientMsgBox(unsigned msgId) = 0;
	virtual void SignalNetClientWaitDialog() = 0;

	virtual void SignalNetClientServerListAdd(unsigned serverId) = 0;
	virtual void SignalNetClientServerListClear() = 0;
	virtual void SignalNetClientServerListShow() = 0;

	virtual void SignalNetClientLoginShow() = 0;
	virtual void SignalNetClientRejoinPossible(unsigned gameId) = 0;
	virtual void SignalNetClientPostRiverShowCards(unsigned playerId) = 0;

	virtual void SignalLobbyPlayerJoined(unsigned playerId, const std::string &nickName) = 0;
	virtual void SignalLobbyPlayerKicked(const std::string &nickName, const std::string &byWhom, const std::string &reason) = 0;
	virtual void SignalLobbyPlayerLeft(unsigned playerId) = 0;

	virtual void SignalSelfGameInvitation(unsigned gameId, unsigned playerIdFrom) = 0;
	virtual void SignalPlayerGameInvitation(unsigned gameId, unsigned playerIdWho, unsigned playerIdFrom) = 0;
	virtual void SignalRejectedGameInvitation(unsigned gameId, unsigned playerIdWho, DenyGameInvitationReason reason) = 0;
};

#endif
