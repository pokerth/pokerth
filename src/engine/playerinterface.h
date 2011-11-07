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
 *****************************************************************************/

#ifndef PLAYERINTERFACE_H
#define PLAYERINTERFACE_H

#include "handinterface.h"
#include <playerdata.h>

class SessionData;

class PlayerInterface
{
public:

	virtual ~PlayerInterface() =0;

	virtual void setHand(HandInterface *) =0;

	virtual int getMyID() const =0;
	virtual void setMyUniqueID(unsigned newId) =0;
	virtual unsigned getMyUniqueID() const =0;

	virtual void setMyGuid(const std::string &theValue) =0;
	virtual std::string getMyGuid() const =0;

	virtual PlayerType getMyType() const =0;

	virtual void setMyDude(int theValue) =0;
	virtual int getMyDude() const =0;

	virtual void setMyDude4(int theValue) =0;
	virtual int getMyDude4() const =0;

	virtual void setMyName(const std::string& theValue) =0;
	virtual std::string getMyName() const =0;

	virtual void setMyAvatar(const std::string& theValue) =0;
	virtual std::string getMyAvatar() const =0;

	virtual void setMyCash(int theValue) =0;
	virtual int getMyCash() const =0;

	virtual void setMySet(int theValue) =0;
	virtual void setMySetAbsolute(int theValue) =0;
	virtual void setMySetNull() =0;
	virtual int getMySet() const =0;
	virtual int getMyLastRelativeSet() const =0;

	virtual void setMyAction(PlayerAction theValue, bool blind=0) =0;
	virtual PlayerAction getMyAction() const	=0;

	virtual void setMyButton(int theValue) =0;
	virtual int getMyButton() const	=0;

	virtual void setMyActiveStatus(bool theValue) =0;
	virtual bool getMyActiveStatus() const =0;

	virtual void setMyStayOnTableStatus(bool theValue) =0;
	virtual bool getMyStayOnTableStatus() const =0;

	virtual void setMyCards(int* theValue) =0;
	virtual void getMyCards(int* theValue) const =0;

	virtual void setMyTurn(bool theValue) =0;
	virtual bool getMyTurn() const =0;

	virtual void setMyCardsFlip(bool theValue, int state) =0;
	virtual bool getMyCardsFlip() const =0;

	virtual void setMyCardsValueInt(int theValue) =0;
	virtual int getMyCardsValueInt() const =0;

	virtual void setMyBestHandPosition(int* theValue) =0;
	virtual void getMyBestHandPosition(int* theValue) const =0;

	virtual void setMyRoundStartCash(int theValue) =0;
	virtual int getMyRoundStartCash() const =0;

	virtual void setMyAverageSets(int theValue) =0;
	virtual int getMyAverageSets() const =0;

	virtual void setLastMoneyWon ( int theValue ) =0;
	virtual int getLastMoneyWon() const =0;

	virtual void setMyAggressive(bool theValue) =0;
	virtual int getMyAggressive() const =0;

	virtual void setSBluff ( int theValue ) =0;
	virtual int getSBluff() const =0;

	virtual void setSBluffStatus ( bool theValue ) =0;
	virtual bool getSBluffStatus() const =0;

	virtual void action() =0;
	virtual int checkMyAction(int targetAction, int targetBet, int highestSet, int minimumRaise, int smallBlind) = 0;

	virtual void preflopEngine() =0;
	virtual void flopEngine() =0;
	virtual void turnEngine() =0;
	virtual void riverEngine() =0;

	virtual void setIsConnected(bool connected) =0;
	virtual bool isConnected() const=0;

	virtual bool checkIfINeedToShowCards() =0;
};

#endif
