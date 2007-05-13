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
#ifndef LOCALENGINEFACTORY_H
#define LOCALENGINEFACTORY_H

#include <enginefactory.h>

#include "localhand.h"
#include "localboard.h"
#include "localplayer.h"
#include "localpreflop.h"
#include "localflop.h"
#include "localturn.h"
#include "localriver.h"

#include "configfile.h"

class LocalEngineFactory : public EngineFactory
{
public:
    LocalEngineFactory(ConfigFile*);

    ~LocalEngineFactory();

	HandInterface* createHand(EngineFactory *f, GuiInterface *g, BoardInterface *b, PlayerInterface **p, int id, int sP, int aP, int dP, int sB,int sC);
	BoardInterface* createBoard();
	PlayerInterface* createPlayer(BoardInterface *b, int id, unsigned uniqueId, PlayerType type, std::string name, std::string avatar, int sC, bool aS, int mB);
	PreflopInterface* createPreflop(HandInterface* hi, int id, int aP, int dP, int sB);
	FlopInterface* createFlop(HandInterface* hi, int id, int aP, int dP, int sB);
	TurnInterface* createTurn(HandInterface* hi, int id, int aP, int dP, int sB);
	RiverInterface* createRiver(HandInterface* hi, int id, int aP, int dP, int sB);
private:

	ConfigFile *myConfig;
};

#endif
