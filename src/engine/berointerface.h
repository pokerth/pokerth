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
#ifndef BEROINTERFACE_H
#define BEROINTERFACE_H

#include <boost/shared_ptr.hpp>
#include <list>

#include <game_defs.h>

class PlayerInterface;

typedef boost::shared_ptr<std::list<boost::shared_ptr<PlayerInterface> > > PlayerList;
typedef std::list<boost::shared_ptr<PlayerInterface> >::iterator PlayerListIterator;

class BeRoInterface{
public:

	virtual ~BeRoInterface();
	
	virtual GameState getMyBeRoID() const =0;

	virtual void setPlayersTurn(int) =0;
	virtual int getPlayersTurn() const =0;

	virtual void setCurrentPlayersTurnId(unsigned) =0;
	virtual unsigned getCurrentPlayersTurnId() const =0;

	virtual void setCurrentPlayersTurnIt(PlayerListIterator) =0;
	virtual PlayerListIterator getCurrentPlayersTurnIt() const =0;

	virtual void setSmallBlindPositionId(unsigned) =0;
	virtual unsigned getSmallBlindPositionId() const =0;

	virtual void setBigBlindPositionId(unsigned) =0;
	virtual unsigned getBigBlindPositionId() const =0;

	virtual void setHighestSet(int) =0;
	virtual int getHighestSet() const =0;

	virtual void setHighestCardsValue(int theValue) =0;
	virtual int getHighestCardsValue() const =0;

        virtual void setLastActionPlayer( unsigned theValue ) =0;
        virtual unsigned getLastActionPlayer() const =0;

	virtual void setMinimumRaise (int) =0;
	virtual int getMinimumRaise() const =0;

        virtual void setFullBetRule (bool) =0;
        virtual bool getFullBetRule() const =0;

	virtual bool getFirstRound() const =0;

	virtual void skipFirstRunGui() =0;

	virtual void nextPlayer() =0;
	virtual void run() =0;

	virtual void postRiverRun() =0;

};

#endif
