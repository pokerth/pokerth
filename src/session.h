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
#ifndef STDSESSION_H
#define STDSESSION_H
#include <string>

class GuiInterface;
class Game;
class ConfigFile;
class ClientThread;

class Session{
public:

    Session(GuiInterface*);

    ~Session();

	void startGame(int, int, int);
	void deleteGame();

	void startNetworkClient(const std::string &serverAddress, unsigned serverPort, bool ipv6, const std::string &pwd);
	void terminateNetworkClient();

	void setActualGameID(const int& theValue) { actualGameID = theValue; }
	int getActualGameID() const { return actualGameID; }

// 	void createConfig();

private:

	int actualGameID;

	ClientThread *myNetClient;

	Game *actualGame;
	GuiInterface *myGui;
	ConfigFile *myConfig;

};



#endif
