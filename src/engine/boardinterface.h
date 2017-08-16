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

#ifndef BOARDINTERFACE_H
#define BOARDINTERFACE_H

#include <engine_defs.h>

class HandInterface;

class BoardInterface
{

public:

	virtual ~BoardInterface();
//
	virtual void setPlayerLists(PlayerList, PlayerList, PlayerList) =0;
//
	virtual void setMyCards(int* theValue) =0;
	virtual void getMyCards(int* theValue) =0;
//
	virtual int getPot() const=0;
	virtual void setPot(int theValue) =0;
	virtual int getSets() const=0;
	virtual void setSets(int theValue) =0;

	virtual void setAllInCondition(bool theValue) =0;
	virtual void setLastActionPlayerID(unsigned theValue) =0;
//
	virtual void collectSets() =0;
	virtual void collectPot() =0;

	virtual void distributePot(unsigned) =0;
	virtual void determinePlayerNeedToShowCards() =0;

	virtual std::list<unsigned> getWinners() const =0;
	virtual void setWinners(const std::list<unsigned> &winners) =0;

	virtual std::list<unsigned> getPlayerNeedToShowCards() const =0;
	virtual void setPlayerNeedToShowCards(const std::list<unsigned> &playerNeedToShowCards) =0;

};

#endif
