/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
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
#ifndef CLIENTBOARD_H
#define CLIENTBOARD_H

#include <boardinterface.h>
#include <boost/thread.hpp>
#include <vector>


class PlayerInterface;
class HandInterface;


class ClientBoard : public BoardInterface
{
public:
	ClientBoard();
	~ClientBoard();

	void setPlayer(std::vector<boost::shared_ptr<PlayerInterface> >);
	void setHand(HandInterface*);

	void setMyCards(int* theValue);
	void getMyCards(int* theValue);

	int getPot() const;
	void setPot(int theValue);
	int getSets() const;
	void setSets(int theValue);

	void collectSets();
	void collectPot();

	void distributePot();

private:
	mutable boost::recursive_mutex m_syncMutex;

	std::vector<boost::shared_ptr<PlayerInterface> > playerArray;
	HandInterface *actualHand;

	int myCards[5];
	int pot;
	int sets;
};

#endif
