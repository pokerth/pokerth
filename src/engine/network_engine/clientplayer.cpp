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
#include "clientplayer.h"
#include "handinterface.h"

using namespace std;


ClientPlayer::ClientPlayer(ConfigFile *c, BoardInterface *b, int id, unsigned uniqueId, PlayerType type, std::string name, std::string avatar, int sC, bool aS, int mB)
: PlayerInterface(), myConfig(c), actualHand(0), actualBoard(b), myCardsValue(0), myID(id), myUniqueID(uniqueId), myType(type), myName(name), myAvatar(avatar), myDude(0), myDude4(0), myCardsValueInt(0), myOdds(-1.0), myCash(sC), mySet(0), myAction(0), myButton(mB), myActiveStatus(aS), myTurn(0), myRoundStartCash(0), sBluff(0), sBluffStatus(0)
{
}


ClientPlayer::~ClientPlayer()
{
}


void
ClientPlayer::setHand(HandInterface* br)
{
	actualHand = br;
}


void
ClientPlayer::action()
{
}


void
ClientPlayer::preflopEngine()
{
}


void
ClientPlayer::flopEngine()
{
}


void
ClientPlayer::turnEngine()
{
}


void
ClientPlayer::riverEngine()
{
}


void
ClientPlayer::evaluation(int bet, int raise)
{
}


int
ClientPlayer::preflopCardsValue(int* cards)
{
	return 0;
}


int
ClientPlayer::flopCardsValue(int* cards)
{
	return 0;
}


void
ClientPlayer::readFile()
{
}

int
ClientPlayer::turnCardsValue(int* cards)
{
	return 0;
}

void
ClientPlayer::preflopEngine3()
{
}

void
ClientPlayer::flopEngine3()
{
}

void
ClientPlayer::turnEngine3()
{
}

void
ClientPlayer::riverEngine3()
{
}

void ClientPlayer::setNetSessionData(boost::shared_ptr<SessionData> session)
{
	myNetSessionData = session;
}

boost::shared_ptr<SessionData> ClientPlayer::getNetSessionData()
{
	return myNetSessionData;
}

