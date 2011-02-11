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
#ifndef CLIENTBERO_H
#define CLIENTBERO_H

#include <boost/thread.hpp>
#include "berointerface.h"

class HandInterface;

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class ClientBeRo : public BeRoInterface
{
public:
	ClientBeRo(HandInterface* hi, int id, unsigned dP, int sB, GameState gS);
	~ClientBeRo();

	GameState getMyBeRoID() const;

	int getHighestCardsValue() const;
	void setHighestCardsValue(int theValue);

	void setLastActionPlayer ( unsigned theValue );
	unsigned getLastActionPlayer() const;

	void setSmallBlindPositionId(unsigned theValue);
	unsigned getSmallBlindPositionId() const;

	void setBigBlindPositionId(unsigned theValue);
	unsigned getBigBlindPositionId() const;

	void setCurrentPlayersTurnId(unsigned theValue);
	unsigned getCurrentPlayersTurnId() const;

	void setFirstRoundLastPlayersTurnId(unsigned theValue);
	unsigned getFirstRoundLastPlayersTurnId() const;

	void setCurrentPlayersTurnIt(PlayerListIterator theValue);
	PlayerListIterator getCurrentPlayersTurnIt() const;

	void setLastPlayersTurnIt(PlayerListIterator theValue);
	PlayerListIterator getLastPlayersTurnIt() const;

	void setHighestSet(int theValue);
	int getHighestSet() const;

	void setFirstRound(bool theValue);
	bool getFirstRound() const;

	void setSmallBlindPosition(int theValue);
	int getSmallBlindPosition() const;

	void setSmallBlind(int theValue);
	int getSmallBlind() const;

	void setMinimumRaise ( int theValue );
	int getMinimumRaise() const;

	void setFullBetRule ( bool theValue );
	bool getFullBetRule() const;

	void skipFirstRunGui();

	void nextPlayer();
	void run();

	void postRiverRun();

private:

	const GameState myBeRoID;

	mutable boost::recursive_mutex m_syncMutex;

	HandInterface *myHand;

	int highestCardsValue;

	PlayerListIterator currentPlayersTurnIt; // iterator for runningPlayerList
	PlayerListIterator lastPlayersTurnIt; // iterator for runningPlayerList

	unsigned smallBlindPositionId;
	unsigned bigBlindPositionId;

	unsigned currentPlayersTurnId;
	unsigned firstRoundLastPlayersTurnId;


	int highestSet;
	bool firstRound;
	int smallBlindPosition;
	int smallBlind;

	int minimumRaise;
	bool fullBetRule;

	int lastActionPlayer;
};

#endif
