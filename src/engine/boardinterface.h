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
	virtual void setLastActionPlayer(unsigned theValue) =0;
//
	virtual void collectSets() =0;
	virtual void collectPot() =0;

	virtual void distributePot() =0;
	virtual void determinePlayerNeedToShowCards() =0;

	virtual std::list<unsigned> getWinners() const =0;
	virtual void setWinners(const std::list<unsigned> &winners) =0;

	virtual std::list<unsigned> getPlayerNeedToShowCards() const =0;
	virtual void setPlayerNeedToShowCards(const std::list<unsigned> &playerNeedToShowCards) =0;

};

#endif
