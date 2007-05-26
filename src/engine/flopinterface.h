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
#ifndef FLOPINTERFACE_H
#define FLOPINTERFACE_H

class FlopInterface{
public:


	virtual ~FlopInterface();

	virtual void setPlayersTurn(const int& theValue) =0;
	virtual int getPlayersTurn() const =0;
	
	virtual void setHighestSet(const int& theValue) =0;
	virtual int getHighestSet() const =0;

	virtual void setFirstFlopRound(bool theValue) =0;
	virtual bool getFirstFlopRound() const =0;

	virtual void setSmallBlindPosition(const int& theValue) =0;
	virtual int getSmallBlindPosition() const =0;

	virtual void setSmallBlind(const int& theValue) =0;
	virtual int getSmallBlind() const =0;

	virtual void resetFirstRun() =0;

	virtual void flopRun() =0;
	virtual void nextPlayer2() =0;

};

#endif
