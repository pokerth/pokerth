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
#include "localberopreflop.h"
#include "localberoflop.h"
#include "localberoturn.h"
#include "localberoriver.h"
#include "localberopostriver.h"

#include <configfile.h>


LocalEngineFactory::LocalEngineFactory(ConfigFile *c)
: myConfig(c)
{
}


LocalEngineFactory::~LocalEngineFactory()
{
}


HandInterface* LocalEngineFactory::createHand(boost::shared_ptr<EngineFactory> f, GuiInterface *g, BoardInterface *b, PlayerInterface **p, int id, int sP, int aP, int dP, int sB,int sC) { return new LocalHand(f, g, b, p, id, sP, aP, dP, sB, sC); }

BoardInterface* LocalEngineFactory::createBoard() { return new LocalBoard; }

PlayerInterface* LocalEngineFactory::createPlayer(BoardInterface *b, int id, unsigned uniqueId, PlayerType type, std::string name, std::string avatar, int sC, bool aS, int mB) { return new LocalPlayer(myConfig, b, id, uniqueId, type, name, avatar, sC, aS, mB); }

std::vector<boost::shared_ptr<BeRoInterface> > LocalEngineFactory::createBeRo(HandInterface* hi, int id, int aP, int dP, int sB) {

	std::vector<boost::shared_ptr<BeRoInterface> > myBeRo;

	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new LocalBeRoPreflop(hi, id, aP, dP, sB)));

	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new LocalBeRoFlop(hi, id, aP, dP, sB)));

	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new LocalBeRoTurn(hi, id, aP, dP, sB)));

	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new LocalBeRoRiver(hi, id, aP, dP, sB)));

	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new LocalBeRoPostRiver(hi, id, aP, dP, sB)));

	return myBeRo;

}
