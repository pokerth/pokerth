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


class LocalEngineFactory : public EngineFactory
{
public:
    LocalEngineFactory();

    ~LocalEngineFactory();

	HandInterface* createHand(GuiInterface *g, BoardInterface *b, PlayerInterface **p, int id, int qP, int dP, int sB,int sC);
	BoardInterface* createBoard();
	PlayerInterface* createPlayer(BoardInterface *b, int id, int sC, bool aS, int mB);
	PreflopInterface* createPreflop(HandInterface* hi, int id, int qP, int dP, int sB);
	FlopInterface* createFlop(HandInterface* hi, int id, int qP, int dP, int sB);
	TurnInterface* createTurn(HandInterface* hi, int id, int qP, int dP, int sB);
	RiverInterface* createRiver(HandInterface* hi, int id, int qP, int dP, int sB);
};

#endif
