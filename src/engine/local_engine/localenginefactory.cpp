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


LocalEngineFactory::LocalEngineFactory()
 : EngineFactory()
{
}


LocalEngineFactory::~LocalEngineFactory()
{
}


HandInterface* LocalEngineFactory::createHand(EngineFactory *f, GuiInterface *g, BoardInterface *b, PlayerInterface **p, int id, int qP, int dP, int sB,int sC) { return new LocalHand(f, g, b, p, id, qP, dP, sB, sC); }

BoardInterface* LocalEngineFactory::createBoard() { return new LocalBoard; }

PlayerInterface* LocalEngineFactory::createPlayer(BoardInterface *b, int id, std::string name, int sC, bool aS, int mB) { return new LocalPlayer(b, id, name, sC, aS, mB); }

PreflopInterface* LocalEngineFactory::createPreflop(HandInterface* hi, int id, int qP, int dP, int sB) { return new LocalPreflop(hi, id, qP, dP, sB); }

FlopInterface* LocalEngineFactory::createFlop(HandInterface* hi, int id, int qP, int dP, int sB) { return new LocalFlop(hi, id, qP, dP, sB); }

TurnInterface* LocalEngineFactory::createTurn(HandInterface* hi, int id, int qP, int dP, int sB) { return new LocalTurn(hi, id, qP, dP, sB); }

RiverInterface* LocalEngineFactory::createRiver(HandInterface* hi, int id, int qP, int dP, int sB) { return new LocalRiver(hi, id, qP, dP, sB); }
