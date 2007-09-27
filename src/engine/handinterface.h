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
#ifndef HANDINTERFACE_H
#define HANDINTERFACE_H

#include <vector>

#include <boost/shared_ptr.hpp>

#include "guiinterface.h"
#include "boardinterface.h"
#include "playerinterface.h"
#include "berointerface.h"

class HandInterface{
public:
    
	virtual ~HandInterface();

	virtual void start() = 0;

	virtual std::vector<boost::shared_ptr<PlayerInterface> > getPlayerArray() const =0;
	virtual PlayerList getActivePlayerList() const =0;
	virtual PlayerList getRunningPlayerList() const =0;

	virtual PlayerListIterator getSeatIt(unsigned) const =0;
	virtual PlayerListIterator getActivePlayerIt(unsigned) const =0;
	virtual PlayerListIterator getRunningPlayerIt(unsigned) const =0;

	virtual BoardInterface* getBoard() const =0;
	virtual boost::shared_ptr<BeRoInterface> getPreflop() const =0;
	virtual boost::shared_ptr<BeRoInterface> getFlop() const =0;
	virtual boost::shared_ptr<BeRoInterface> getTurn() const =0;
	virtual boost::shared_ptr<BeRoInterface> getRiver() const =0;
	virtual GuiInterface* getGuiInterface() const =0;
	virtual boost::shared_ptr<BeRoInterface> getCurrentBeRo() const =0;

	virtual void setMyID(int theValue) =0;
	virtual int getMyID() const =0;
	
	virtual void setStartQuantityPlayers(int theValue) =0;
	virtual int getStartQuantityPlayers() const =0;

	virtual void setActualRound(int theValue) =0;
	virtual int getActualRound() const =0;

	virtual void setDealerPosition(int theValue) =0;
	virtual int getDealerPosition() const =0;

	virtual void setSmallBlind(int theValue) =0;
	virtual int getSmallBlind() const =0;

	virtual void setAllInCondition(bool theValue) =0;
	virtual bool getAllInCondition() const =0;

	virtual void setStartCash(int theValue) =0;
	virtual int getStartCash() const =0;

	virtual void setBettingRoundsPlayed(int theValue) =0;
	virtual int getBettingRoundsPlayed() const =0;

	virtual void setLastPlayersTurn(int theValue) =0;
	virtual int getLastPlayersTurn() const =0;

	virtual void setCardsShown(bool theValue) =0;
	virtual bool getCardsShown() const =0;

	virtual void switchRounds() =0;

};

#endif
