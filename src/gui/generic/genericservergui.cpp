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

#include "genericservergui.h"
#include <session.h>
#include <game.h>

using namespace std;


GenericServerGui::GenericServerGui(ConfigFile *config)
: myConfig(config)
{
}


GenericServerGui::~GenericServerGui()
{
}

void GenericServerGui::initGui(int speed)
{
}

Session &GenericServerGui::getSession()
{
	assert(mySession.get());
	return *mySession;
}

void GenericServerGui::setSession(boost::shared_ptr<Session> session)
{
	mySession = session;
}

void GenericServerGui::waitForNetworkAction(GameState state, unsigned uniquePlayerId)
{
	getSession().waitForNetworkServerAction(state, uniquePlayerId);
}


void GenericServerGui::dealHoleCards()
{
}

void GenericServerGui::dealFlopCards()
{
}

void GenericServerGui::dealTurnCard()
{
}

void GenericServerGui::dealRiverCard()
{
}

void GenericServerGui::nextPlayerAnimation()
{
}

void GenericServerGui::preflopAnimation1()
{
}

void GenericServerGui::preflopAnimation2()
{
}

void GenericServerGui::flopAnimation1()
{
}

void GenericServerGui::flopAnimation2()
{
}

void GenericServerGui::turnAnimation1()
{
}

void GenericServerGui::turnAnimation2()
{
}

void GenericServerGui::riverAnimation1()
{
}

void GenericServerGui::riverAnimation2()
{
}

void GenericServerGui::postRiverAnimation1()
{
}

void GenericServerGui::postRiverRunAnimation1()
{
}

void GenericServerGui::flipHolecardsAllIn()
{
}

