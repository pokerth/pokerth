/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/* Game data. */

#ifndef _GAMEDATA_H_
#define _GAMEDATA_H_

#include <playerdata.h>

enum GameMode
{
	GAME_MODE_CREATED = 1,
	GAME_MODE_STARTED,
	GAME_MODE_CLOSED
};

enum RaiseIntervalMode
{
	RAISE_ON_HANDNUMBER = 1,
	RAISE_ON_MINUTES
};

enum RaiseMode
{
	DOUBLE_BLINDS = 1,
	MANUAL_BLINDS_ORDER
};

enum AfterManualBlindsMode
{
	AFTERMB_DOUBLE_BLINDS = 1,
	AFTERMB_RAISE_ABOUT,
	AFTERMB_STAY_AT_LAST_BLIND
};

// For the sake of simplicity, this is a struct.
struct GameData
{
	GameData() : maxNumberOfPlayers(0), startMoney(0), smallBlind(0), firstSmallBlind(0), raiseIntervalMode(RAISE_ON_HANDNUMBER), raiseSmallBlindEveryHandsValue(8), raiseSmallBlindEveryMinutesValue(0), raiseMode(DOUBLE_BLINDS), afterManualBlindsMode(AFTERMB_DOUBLE_BLINDS), afterMBAlwaysRaiseValue(0), handsBeforeRaise(1), guiSpeed(4), playerActionTimeoutSec(20) {}
	int maxNumberOfPlayers;
	int startMoney;
		int smallBlind; // deprecated
	int firstSmallBlind;
	RaiseIntervalMode raiseIntervalMode;
	int raiseSmallBlindEveryHandsValue;
	int raiseSmallBlindEveryMinutesValue;
	RaiseMode raiseMode;
	std::list<int> manualBlindsList;
	AfterManualBlindsMode afterManualBlindsMode;
	int afterMBAlwaysRaiseValue;
		int handsBeforeRaise; // deprecated
	int guiSpeed;
	int playerActionTimeoutSec;
};

struct GameInfo
{
	GameInfo() : mode(GAME_MODE_CREATED), isPasswordProtected(false) {}
	std::string name;
	GameData data;
	GameMode mode;
	PlayerIdList players;
	bool isPasswordProtected;
};

struct StartData
{
	StartData() : startDealerPlayerId(0), numberOfPlayers(0) {}
	unsigned startDealerPlayerId;
	int numberOfPlayers;
};

#endif

