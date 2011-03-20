/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
 *   f.thauer@web.de   *
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
#ifndef CARDSVALUE_H
#define CARDSVALUE_H

#include "game_defs.h"
#include "engine_defs.h"

#include <iostream>
#include <vector>

class CardsValue
{
public:
	static int holeCardsClass(int, int);
	static int cardsValue(int*, int*);
	static std::string determineHandName(int myCardsValueInt, PlayerList activePlayerList);
	static std::list<std::string> translateCardsValueCode(int cardsValueCode);

	static int holeCardsToIntCode(int*);
	static int* intCodeToHoleCards(int);

	static std::vector< std::vector<int> > calcCardsChance(GameState, int*, int*);
	//static int** showdown(GameState, int**, int);

};

#endif
