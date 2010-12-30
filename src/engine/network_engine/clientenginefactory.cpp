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
#include "clientbero.h"



ClientEngineFactory::ClientEngineFactory()
{
}


ClientEngineFactory::~ClientEngineFactory()
{
}


HandInterface* ClientEngineFactory::createHand(boost::shared_ptr<EngineFactory> f, GuiInterface *g, BoardInterface *b, Log *l, PlayerList sl, PlayerList apl, PlayerList rpl, int id, int sP, int dP, int sB,int sC)
{
    return new ClientHand(f, g, b, l, sl, apl, rpl, id, sP, dP, sB, sC);
}

BoardInterface* ClientEngineFactory::createBoard()
{
	return new ClientBoard;
}

boost::shared_ptr<PlayerInterface>  ClientEngineFactory::createPlayer(BoardInterface *b, int id, unsigned uniqueId, PlayerType type, std::string name, std::string avatar, int sC, bool aS, int mB)
{
	return boost::shared_ptr<PlayerInterface> (new ClientPlayer(NULL, b, id, uniqueId, type, name, avatar, sC, aS, mB));
}

std::vector<boost::shared_ptr<BeRoInterface> > ClientEngineFactory::createBeRo(HandInterface* hi, int id, unsigned dP, int sB) {

	std::vector<boost::shared_ptr<BeRoInterface> > myBeRo;

	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new ClientBeRo(hi, id, dP, sB, GAME_STATE_PREFLOP)));
	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new ClientBeRo(hi, id, dP, sB, GAME_STATE_FLOP)));
	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new ClientBeRo(hi, id, dP, sB, GAME_STATE_TURN)));
	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new ClientBeRo(hi, id, dP, sB, GAME_STATE_RIVER)));
	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new ClientBeRo(hi, id, dP, sB, GAME_STATE_POST_RIVER)));

	return myBeRo;

}

