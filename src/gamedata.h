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

// For the sake of simplicity, this is a struct.

struct GameData
{
	GameData() : numberOfPlayers(0), startMoney(0), smallBlind(0),
		handsBeforeRaise(1), guiSpeed(4) {}
	int numberOfPlayers;
	int startMoney;
	int smallBlind;
	int handsBeforeRaise;
	int guiSpeed;
};

struct StartData
{
	StartData() : startDealerPlayerId(0) {}
	unsigned startDealerPlayerId;
};

#endif

