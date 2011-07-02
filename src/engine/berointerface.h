/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/ #ifndef BEROINTERFACE_H
#define BEROINTERFACE_H

#include <game_defs.h>
#include <engine_defs.h>

class BeRoInterface
{
public:

	virtual ~BeRoInterface();

	virtual GameState getMyBeRoID() const =0;

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
