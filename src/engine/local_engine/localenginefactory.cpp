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
#include "localenginefactory.h"

#include "localhand.h"
#include "localboard.h"
#include "localplayer.h"
#include "localpreflop.h"
#include "localflop.h"
#include "localturn.h"
#include "localriver.h"

LocalEngineFactory::LocalEngineFactory()
 : EngineFactory()
{
}


LocalEngineFactory::~LocalEngineFactory()
{
}


HandInterface* LocalEngineFactory::createHand() { /*return new LocalHand;*/ }

BoardInterface* LocalEngineFactory::createBoard() { return new LocalBoard; }

PlayerInterface* LocalEngineFactory::createPlayer() { /*return new LocalPlayer;*/ }

PreflopInterface* LocalEngineFactory::createPreflop() {/* return new LocalPreflop;*/ }

FlopInterface* LocalEngineFactory::createFlop() { /*return new LocalFlop;*/ }

TurnInterface* LocalEngineFactory::createTurn() { /*return new LocalTurn;*/ }

RiverInterface* LocalEngineFactory::createRiver() { /*return new LocalRiver;*/ }
