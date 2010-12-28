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
#ifndef LOG_H
#define LOG_H

#include <boost/shared_ptr.hpp>
#include <list>

struct sqlite3;
class ConfigFile;
class PlayerInterface;

typedef boost::shared_ptr<std::list<boost::shared_ptr<PlayerInterface> > > PlayerList;
typedef std::list<boost::shared_ptr<PlayerInterface> >::iterator PlayerListIterator;
typedef std::list<boost::shared_ptr<PlayerInterface> >::const_iterator PlayerListConstIterator;

class Log
{

public:
    Log(ConfigFile *c);

    ~Log();

    void logNewGameMsg(int gameID, int startCash, int startSmallBlind, unsigned dealerPosition, PlayerList seatsList);
//    void closeLogDbAtExit();


private:
    ConfigFile *myConfig;
    sqlite3 *mySqliteLogDb;

};

#endif // LOG_H
