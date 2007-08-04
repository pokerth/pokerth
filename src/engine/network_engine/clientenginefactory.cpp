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
#include "clientenginefactory.h"

#include "clienthand.h"
#include "clientboard.h"
#include "clientplayer.h"
#include "clientpreflop.h"
#include "clientflop.h"
#include "clientturn.h"
#include "clientriver.h"
#include "clientbero.h"
#include "clientberofactory.h"



ClientEngineFactory::ClientEngineFactory()
{
}


ClientEngineFactory::~ClientEngineFactory()
{
}


HandInterface* ClientEngineFactory::createHand(boost::shared_ptr<EngineFactory> f, GuiInterface *g, BoardInterface *b, PlayerInterface **p, int id, int sP, int aP, int dP, int sB,int sC)
{
	return new ClientHand(f, g, b, p, id, sP, aP, dP, sB, sC);
}

BoardInterface* ClientEngineFactory::createBoard()
{
	return new ClientBoard;
}

PlayerInterface* ClientEngineFactory::createPlayer(BoardInterface *b, int id, unsigned uniqueId, PlayerType type, std::string name, std::string avatar, int sC, bool aS, int mB)
{
	return new ClientPlayer(NULL, b, id, uniqueId, type, name, avatar, sC, aS, mB);
}

PreflopInterface* ClientEngineFactory::createPreflop(HandInterface* hi, int id, int aP, int dP, int sB)
{
	return new ClientPreflop(hi, id, aP, dP, sB);
}

FlopInterface* ClientEngineFactory::createFlop(HandInterface* hi, int id, int aP, int dP, int sB)
{
	return new ClientFlop(hi, id, aP, dP, sB);
}

TurnInterface* ClientEngineFactory::createTurn(HandInterface* hi, int id, int aP, int dP, int sB)
{
	return new ClientTurn(hi, id, aP, dP, sB);
}

RiverInterface* ClientEngineFactory::createRiver(HandInterface* hi, int id, int aP, int dP, int sB)
{
	return new ClientRiver(hi, id, aP, dP, sB);
}

// BeRoInterface* ClientEngineFactory::createBeRo() 
// {
// 	return new ClientBeRo();
// }

BeRoFactoryInterface* ClientEngineFactory::createBeRoFactory(HandInterface* hi) 
{
	return new ClientBeRoFactory(hi);
}
