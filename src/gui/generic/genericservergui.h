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
#ifndef GENERICSERVERGUI_H
#define GENERICSERVERGUI_H

#include <game_defs.h>
#include <boost/shared_ptr.hpp>

class Session;
class ConfigFile;

class GenericServerGui
{
public:
	GenericServerGui(ConfigFile *config);

	~GenericServerGui();

	void initGui(int speed);

	Session &getSession();
	void setSession(boost::shared_ptr<Session> session);

	void waitForNetworkAction(GameState state, unsigned uniquePlayerId);

private: 

	boost::shared_ptr<Session> mySession;
	ConfigFile *myConfig;
};

#endif
