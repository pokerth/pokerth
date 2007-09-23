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
#include "clientboard.h"

#include <handinterface.h>
#include <game_defs.h>

using namespace std;

ClientBoard::ClientBoard()
: playerArray(0), actualHand(0), pot(0), sets(0)
{
}


ClientBoard::~ClientBoard()
{
}

void
ClientBoard::setPlayerLists(std::vector<boost::shared_ptr<PlayerInterface> > sl, PlayerList apl, PlayerList rpl)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	playerArray = sl;
	activePlayerList = apl;
	runningPlayerList = rpl;
}

void
ClientBoard::setHand(HandInterface* br)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	actualHand = br;
}

void
ClientBoard::setMyCards(int* theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	for (int i = 0; i < 5; i++)
		myCards[i] = theValue[i];
}

void
ClientBoard::getMyCards(int* theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	for (int i = 0; i < 5; i++)
		theValue[i] = myCards[i];
}

int
ClientBoard::getPot() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return pot;
}

void
ClientBoard::setPot(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	pot = theValue;
}

int
ClientBoard::getSets() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return sets;
}

void
ClientBoard::setSets(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	sets = theValue;
}

void
ClientBoard::collectSets()
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	sets = 0;
	for (int i = 0; i < MAX_NUMBER_OF_PLAYERS; i++)
		sets += playerArray[i]->getMySet();
}

void
ClientBoard::collectPot()
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	pot += sets; 
	sets = 0;
}

void
ClientBoard::distributePot()
{

}


