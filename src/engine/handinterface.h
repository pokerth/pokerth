/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/

#ifndef HANDINTERFACE_H
#define HANDINTERFACE_H

#include "guiinterface.h"
#include "boardinterface.h"
#include "playerinterface.h"
#include "berointerface.h"
#include "log.h"

class HandInterface
{
public:

	virtual ~HandInterface();

	virtual void start() = 0;

	virtual PlayerList getSeatsList() const =0;
	virtual PlayerList getActivePlayerList() const =0;
	virtual PlayerList getRunningPlayerList() const =0;

	virtual boost::shared_ptr<BoardInterface> getBoard() const =0;
	virtual boost::shared_ptr<BeRoInterface> getPreflop() const =0;
	virtual boost::shared_ptr<BeRoInterface> getFlop() const =0;
	virtual boost::shared_ptr<BeRoInterface> getTurn() const =0;
	virtual boost::shared_ptr<BeRoInterface> getRiver() const =0;
	virtual GuiInterface* getGuiInterface() const =0;
	virtual boost::shared_ptr<BeRoInterface> getCurrentBeRo() const =0;
	virtual Log* getLog() const =0;

	virtual void setMyID(int theValue) =0;
	virtual int getMyID() const =0;

	virtual void setStartQuantityPlayers(int theValue) =0;
	virtual int getStartQuantityPlayers() const =0;

	virtual void setCurrentRound(GameState theValue) =0;
	virtual GameState getCurrentRound() const =0;
	virtual GameState getRoundBeforePostRiver() const =0;

	virtual void setDealerPosition(int theValue) =0;
	virtual int getDealerPosition() const =0;

	virtual void setSmallBlind(int theValue) =0;
	virtual int getSmallBlind() const =0;

	virtual void setAllInCondition(bool theValue) =0;
	virtual bool getAllInCondition() const =0;

	virtual void setStartCash(int theValue) =0;
	virtual int getStartCash() const =0;

	virtual void setPreviousPlayerID(int theValue) =0;
	virtual int getPreviousPlayerID() const =0;

	virtual void setLastActionPlayerID( unsigned theValue ) =0;
	virtual unsigned getLastActionPlayerID() const =0;

	virtual void setCardsShown(bool theValue) =0;
	virtual bool getCardsShown() const =0;

	virtual void switchRounds() =0;

protected:
	virtual PlayerListIterator getSeatIt(unsigned) const =0;
	virtual PlayerListIterator getActivePlayerIt(unsigned) const =0;
	virtual PlayerListIterator getRunningPlayerIt(unsigned) const =0;

	friend class Game;
	friend class LocalBeRo;
	friend class LocalBeRoPreflop;
	friend class LocalBeRoFlop;
	friend class LocalBeRoTurn;
	friend class LocalBeRoRiver;
	friend class LocalBeRoPostRiver;
};

#endif
