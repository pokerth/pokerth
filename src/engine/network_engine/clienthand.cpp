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
#include "clienthand.h"
#include <game_defs.h>

using namespace std;

ClientHand::ClientHand(boost::shared_ptr<EngineFactory> f, GuiInterface *g, BoardInterface *b, PlayerInterface **p, int id, int sP, int aP, int dP, int sB,int sC)
: myFactory(f), myGui(g),  myBoard(b), playerArray(p), myPreflop(0), myFlop(0), myTurn(0), myRiver(0), myID(id), actualQuantityPlayers(aP), startQuantityPlayers(sP), dealerPosition(dP), actualRound(0), smallBlind(sB), startCash(sC), allInCondition(0), bettingRoundsPlayed(0)
{
}



ClientHand::~ClientHand()
{
}

void
ClientHand::start()
{
}

void
ClientHand::assignButtons()
{
}

void
ClientHand::switchRounds()
{
}

void
ClientHand::highlightRoundLabel()
{
}

