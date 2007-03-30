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
#ifndef PLAYERINTERFACE_H
#define PLAYERINTERFACE_H

#include "handinterface.h"

class PlayerInterface{
public:

	virtual ~PlayerInterface() =0;

	virtual void setHand(HandInterface*) =0;

	virtual void setMyID(const int& theValue) =0;
	virtual int getMyID() const =0;

	virtual void setMyDude(const int& theValue) =0;
	virtual int getMyDude() const =0;

	virtual void setMyName(const std::string& theValue) =0;
	virtual std::string getMyName() const =0;

	virtual void setMyAvatar(const std::string& theValue) =0;
	virtual std::string getMyAvatar() const =0;

	virtual void setMyCash(const int& theValue) =0;
	virtual int getMyCash() const =0;

	virtual void setMySet(const int& theValue) =0;
	virtual void setMySetNull() =0;
	virtual int getMySet() const =0;

	virtual void setMyAction(const int& theValue) =0;
	virtual int getMyAction() const	=0;

	virtual void setMyButton(const int& theValue) =0;
	virtual int getMyButton() const	=0;

	virtual void setMyActiveStatus(bool theValue) =0;
	virtual bool getMyActiveStatus() const =0;

	virtual void setMyCards(int* theValue) =0;
	virtual void getMyCards(int* theValue) =0;

	virtual void setMyTurn(bool theValue) =0;
	virtual bool getMyTurn() const =0;

	virtual void setMyCardsFlip(bool theValue) =0;
	virtual bool getMyCardsFlip() const =0;

	virtual void setMyCardsValueInt(const int& theValue) =0;
	virtual int getMyCardsValueInt() const =0;

	virtual int* getMyBestHandPosition() =0;

	virtual void setMyRoundStartCash(const int& theValue) =0;
	virtual int getMyRoundStartCash() const =0;

	virtual void setMyAverageSets(const int& theValue) =0;
	virtual int getMyAverageSets() const =0;

	virtual void setMyAggressive(const bool& theValue) =0;
	virtual int getMyAggressive() const =0;

	virtual void setSBluff ( int theValue ) =0;
	virtual int getSBluff() const =0;
	
	virtual void action() =0;
	
	virtual void preflopEngine() =0;
	virtual void flopEngine() =0;
	virtual void turnEngine() =0;
	virtual void riverEngine() =0;

};

#endif
