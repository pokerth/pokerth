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

#ifndef BEROINTERFACE_H
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
