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
#ifndef PREFLOPINTERFACE_H
#define PREFLOPINTERFACE_H

class PreflopInterface{
public:
    
	virtual ~PreflopInterface();

	virtual void setPlayersTurn(int theValue) =0;
	virtual int getPlayersTurn() const =0;
	
	virtual void setHighestSet(int theValue) =0;
	virtual int getHighestSet() const =0;

	virtual void setPreflopFirstRound(bool theValue) =0;
	virtual bool setPreflopFirstRound() const =0;

	virtual void preflopRun() =0;
	virtual void nextPlayer2() =0; 

	

};

#endif
