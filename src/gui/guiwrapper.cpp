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
#include "guiwrapper.h"

#include "log.h"
#include "mainwindowimpl.h"

using namespace std;


GuiWrapper::GuiWrapper(mainWindowImpl* w) : myW(w)
{

// Am Ende soll GuiWrapper mainWindowImpl
// 	myW = new mainWindowImpl();	
// 	myW->show();

// Jetzt muss myW erstmal aus Session kommen!

	myLog = new Log(myW);


}


GuiWrapper::~GuiWrapper()
{
}


void GuiWrapper::showPlayerActionLogMsg(string playerName, int &action, int &setValue) const { 		myLog->showPlayerActionLogMsg(playerName, action, setValue); 
}

void GuiWrapper::refreshSet() const { myW->refreshSet(); }
void GuiWrapper::refreshChangePlayer() const { myW->refreshChangePlayer(); }


void GuiWrapper::dealFlopCards() const { myW->dealFlopCards0(); }
void GuiWrapper::dealTurnCard() const { myW->dealTurnCards0(); }
void GuiWrapper::dealRiverCard() const { myW->dealRiverCards0(); }

void GuiWrapper::nextPlayerAnimation() const { myW->nextPlayerAnimation(); }

void GuiWrapper::preflopAnimation1() const { myW->preflopAnimation1(); }
void GuiWrapper::preflopAnimation2() const { myW->preflopAnimation2(); }

void GuiWrapper::flopAnimation1() const { myW->flopAnimation1(); }
void GuiWrapper::flopAnimation2() const { myW->flopAnimation2(); }

void GuiWrapper::turnAnimation1() const { myW->turnAnimation1(); }
void GuiWrapper::turnAnimation2() const { myW->turnAnimation2(); }

void GuiWrapper::riverAnimation1() const { myW->riverAnimation1(); }
void GuiWrapper::riverAnimation2() const { myW->riverAnimation2(); }

void GuiWrapper::postRiverAnimation1() const { myW->postRiverAnimation1(); }
