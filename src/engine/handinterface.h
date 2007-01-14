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

#include "guiinterface.h"
#include "boardinterface.h"
#include "playerinterface.h"
#include "preflopinterface.h"
#include "flopinterface.h"
#include "turninterface.h"
#include "riverinterface.h"

class HandInterface{
public:
    
	virtual ~HandInterface();

	virtual PlayerInterface** getPlayerArray() const =0;
	virtual BoardInterface* getBoard() const =0;
	virtual PreflopInterface* getPreflop() const =0;
	virtual FlopInterface* getFlop() const =0;
	virtual TurnInterface* getTurn() const =0;
	virtual RiverInterface* getRiver() const =0;
	virtual GuiInterface* getGuiInterface() const =0;

	virtual void setMyID(const int& theValue) =0;
	virtual int getMyID() const =0;
	
	virtual void setActualQuantityPlayers(const int& theValue) =0;
	virtual int getActualQuantityPlayers() const =0;

	virtual void setActualRound(const int& theValue) =0;
	virtual int getActualRound() const =0;

	virtual void setDealerPosition(const int& theValue) =0;
	virtual int getDealerPosition() const =0;

	virtual void setSmallBlind(const int& theValue) =0;
	virtual int getSmallBlind() const =0;

	virtual void setAllInCondition(bool theValue) =0;
	virtual bool getAllInCondition() const =0;

	virtual void setStartCash(const int& theValue) =0;
	virtual int getStartCash() const =0;

	virtual void assignButtons();

	virtual void highlightRoundLabel();
	virtual void switchRounds();

};

#endif
