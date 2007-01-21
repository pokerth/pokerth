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
#include "tinyxml.h"
#include <cstdlib>

#ifdef _WIN32
#include <direct.h>
#endif

using namespace std;

Session::Session(GuiInterface* g) : actualGame(0), myGui(g)
{

	

	// Session an mainwindowimpl bergeben
	myGui->setSession(this);
	
	// Konfiguration erstellen

	string configFileName;
#ifdef _WIN32
	const char *appDataPath = getenv("AppData");
	if (appDataPath)
	{
		configFileName = appDataPath;
		configFileName += "\\pokerth\\";
		mkdir(configFileName.c_str());
	}
#else
	// hier Linux/Mac Code zur Basispfadbestimmung, z.B.
	configFileName = "~/pokerth";
	// wenn nicht existiert, erzeugen!
#endif

	configFileName += "config.xml";
}


Session::~Session()
{
}


void Session::startGame(int qP, int sC, int sB) {

	actualGame = new Game(myGui, qP, sC, sB);

}

void Session::deleteGame() {

	delete actualGame;
	actualGame = 0;

}
