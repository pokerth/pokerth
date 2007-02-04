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
#include "session.h"
#include "game.h"
#include "guiinterface.h"
#include "configfile.h"



using namespace std;

Session::Session(GuiInterface* g) : actualGame(0), myGui(g)
{

	actualGameID = 0;
	// Session an mainwindowimpl bergeben
	myGui->setSession(this);
	
	myConfig = new ConfigFile;
	

}


Session::~Session()
{
}


void Session::startGame(int qP, int sC, int sB) {

	actualGameID++;

	actualGame = new Game(myConfig, myGui, qP, sC, sB, actualGameID);
}

void Session::deleteGame() {

	delete actualGame;
	actualGame = 0;

}
