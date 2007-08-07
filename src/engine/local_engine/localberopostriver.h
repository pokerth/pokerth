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
#ifndef LOCALBEROPOSTRIVER_H
#define LOCALBEROPOSTRIVER_H

#include <iostream>
#include <localbero.h>

class HandInterface;


class LocalBeRoPostRiver : public LocalBeRo{
public:
	LocalBeRoPostRiver(HandInterface*, int, int, int, int);
	~LocalBeRoPostRiver();

	void setHighestCardsValue(int theValue) { highestCardsValue = theValue;}
	int getHighestCardsValue() const { return highestCardsValue;}

	void run();
	void distributePot();
	
private:

	int highestCardsValue;

};

#endif
