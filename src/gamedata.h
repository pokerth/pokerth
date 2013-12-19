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
/* Game data. */

#ifndef _GAMEDATA_H_
#define _GAMEDATA_H_

#include <list>
#include <string>
#ifndef Q_MOC_RUN
#include <third_party/boost/timers.hpp>
#endif

typedef std::list<unsigned> PlayerIdList;

enum GameMode {
	GAME_MODE_CREATED = 1,
	GAME_MODE_STARTED,
	GAME_MODE_CLOSED
};

enum GameType {
	GAME_TYPE_NORMAL = 1,
	GAME_TYPE_REGISTERED_ONLY,
	GAME_TYPE_INVITE_ONLY,
	GAME_TYPE_RANKING
};

enum RaiseIntervalMode {
	RAISE_ON_HANDNUMBER = 1,
	RAISE_ON_MINUTES
};

enum RaiseMode {
	DOUBLE_BLINDS = 1,
	MANUAL_BLINDS_ORDER
};

enum AfterManualBlindsMode {
	AFTERMB_DOUBLE_BLINDS = 1,
	AFTERMB_RAISE_ABOUT,
	AFTERMB_STAY_AT_LAST_BLIND
};

// For the sake of simplicity, this is a struct.
struct GameData {
	GameData() : gameType(GAME_TYPE_NORMAL), allowSpectators(true),
		maxNumberOfPlayers(0), startMoney(0), firstSmallBlind(0),
		raiseIntervalMode(RAISE_ON_HANDNUMBER), raiseSmallBlindEveryHandsValue(8),
		raiseSmallBlindEveryMinutesValue(1), raiseMode(DOUBLE_BLINDS),
		afterManualBlindsMode(AFTERMB_DOUBLE_BLINDS), afterMBAlwaysRaiseValue(0),
		guiSpeed(4), delayBetweenHandsSec(6), playerActionTimeoutSec(20) {}
	GameType gameType;
	bool allowSpectators;
	int maxNumberOfPlayers;
	int startMoney;
	int firstSmallBlind;
	RaiseIntervalMode raiseIntervalMode;
	int raiseSmallBlindEveryHandsValue;
	int raiseSmallBlindEveryMinutesValue;
	RaiseMode raiseMode;
	std::list<int> manualBlindsList;
	AfterManualBlindsMode afterManualBlindsMode;
	int afterMBAlwaysRaiseValue;
	int guiSpeed;
	int delayBetweenHandsSec;
	int playerActionTimeoutSec;
};

struct GameInfo {
	GameInfo() : mode(GAME_MODE_CREATED), adminPlayerId(0), isPasswordProtected(false) {}
	std::string name;
	GameData data;
	GameMode mode;
	unsigned adminPlayerId;
	PlayerIdList players;
	PlayerIdList spectators;
	PlayerIdList spectatorsDuringGame;
	bool isPasswordProtected;
};

struct StartData {
	StartData() : startDealerPlayerId(0), numberOfPlayers(0) {}
	unsigned startDealerPlayerId;
	int numberOfPlayers;
};

struct VoteKickData {
	VoteKickData()
		: petitionId(0), kickPlayerId(0), numVotesToKick(0),
		  numVotesInFavourOfKicking(0), numVotesAgainstKicking(0), timeLimitSec(0),
		  voteTimer(boost::posix_time::time_duration(0, 0, 0), boost::timers::portable::microsec_timer::auto_start) {}
	unsigned petitionId;
	unsigned kickPlayerId;
	int numVotesToKick;
	int numVotesInFavourOfKicking;
	int numVotesAgainstKicking;
	int timeLimitSec;
	boost::timers::portable::microsec_timer voteTimer;
	PlayerIdList votedPlayerIds;
};

#endif

