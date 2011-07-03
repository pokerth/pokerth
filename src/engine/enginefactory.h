/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *****************************************************************************/

#ifndef ENGINEFACTORY_H
#define ENGINEFACTORY_H

#include "boardinterface.h"
#include "handinterface.h"
#include "playerinterface.h"
#include "berointerface.h"
#include "log.h"

class EngineFactory
{
public:

	virtual ~EngineFactory();

	virtual boost::shared_ptr<HandInterface> createHand(boost::shared_ptr<EngineFactory> f, GuiInterface *g, boost::shared_ptr<BoardInterface> b, Log *l, PlayerList sl, PlayerList apl, PlayerList rpl, int id, int sP, int dP, int sB,int sC) =0;
	virtual boost::shared_ptr<BoardInterface> createBoard(unsigned dp) =0;
	virtual boost::shared_ptr<PlayerInterface> createPlayer(int id, unsigned uniqueId, PlayerType type, std::string name, std::string avatar, int sC, bool aS, int mB) =0;
	virtual std::vector<boost::shared_ptr<BeRoInterface> > createBeRo(HandInterface *hi, int id, unsigned dP, int sB) =0;
};

#endif
