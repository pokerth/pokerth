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

#ifndef CARDSVALUE_H
#define CARDSVALUE_H

#include "game_defs.h"
#include "engine_defs.h"

#include <iostream>
#include <vector>

struct KickerValue {
	int factorValue;
	int select;
	int remain;
};

class CardsValue
{
public:
	static int holeCardsClass(int, int);
	static int cardsValueShort(int[4]);
	static int cardsValue(int[4], int[4] = 0);
	static KickerValue determineKickerValue(int, int, int);
	static int cardsValueOld(int[7], int[5]);
	static std::string determineHandName(int myCardsValueInt, PlayerList activePlayerList);
	static std::list<std::string> translateCardsValueCode(int cardsValueCode);

	static int holeCardsToIntCode(int[2]);

	static std::vector< std::vector<int> > calcCardsChance(GameState, int[2], int[5]);

	static int bitcount(int in);
	static int bestHandToPosition(int bestHand[4], int cardArray[7], int position[5]);
	//static int** showdown(GameState, int**, int);

};

#endif
