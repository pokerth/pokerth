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

#ifndef CLIENTHAND_H
#define CLIENTHAND_H

#include <enginefactory.h>
#include <guiinterface.h>
#include <boardinterface.h>
#include <playerinterface.h>
#include <handinterface.h>
#include <berointerface.h>
#include <log.h>
#include <boost/thread.hpp>

#include <vector>

class ClientHand : public HandInterface
{
public:
	ClientHand ( boost::shared_ptr<EngineFactory> f, GuiInterface*, boost::shared_ptr<BoardInterface>, Log*, PlayerList, PlayerList, PlayerList , int, int, int, int, int );
	~ClientHand();

	void start();

	PlayerList getSeatsList() const;
	PlayerList getActivePlayerList() const;
	PlayerList getRunningPlayerList() const;

	boost::shared_ptr<BoardInterface> getBoard() const;
	boost::shared_ptr<BeRoInterface> getPreflop() const;
	boost::shared_ptr<BeRoInterface> getFlop() const;
	boost::shared_ptr<BeRoInterface> getTurn() const;
	boost::shared_ptr<BeRoInterface> getRiver() const;
	GuiInterface* getGuiInterface() const;
	boost::shared_ptr<BeRoInterface> getCurrentBeRo() const;

	Log* getLog() const
	{
		return myLog;
	}

	void setMyID ( int theValue );
	int getMyID() const;

	void setCurrentQuantityPlayers ( int theValue );
	int getCurrentQuantityPlayers() const;

	void setStartQuantityPlayers ( int theValue );
	int getStartQuantityPlayers() const;

	void setCurrentRound ( GameState theValue );
	GameState getCurrentRound() const;
	GameState getRoundBeforePostRiver() const;

	void setDealerPosition ( int theValue );
	int getDealerPosition() const;

	void setSmallBlind ( int theValue );
	int getSmallBlind() const;

	void setAllInCondition ( bool theValue );
	bool getAllInCondition() const;

	void setStartCash ( int theValue );
	int getStartCash() const;

	void setBettingRoundsPlayed ( int theValue );
	int getBettingRoundsPlayed() const;

	void setPreviousPlayerID ( int theValue );
	int getPreviousPlayerID() const;

	void setLastActionPlayerID ( unsigned theValue );
	unsigned getLastActionPlayerID() const;

	void setCardsShown ( bool theValue );
	bool getCardsShown() const;

	void switchRounds();

protected:
	PlayerListIterator getSeatIt(unsigned) const;
	PlayerListIterator getActivePlayerIt(unsigned) const;
	PlayerListIterator getRunningPlayerIt(unsigned) const;


private:
	mutable boost::recursive_mutex m_syncMutex;

	boost::shared_ptr<EngineFactory> myFactory;
	GuiInterface *myGui;
	boost::shared_ptr<BoardInterface> myBoard;
	Log *myLog;

	PlayerList seatsList;
	PlayerList activePlayerList;
	PlayerList runningPlayerList;

	std::vector<boost::shared_ptr<BeRoInterface> > myBeRo;

	int myID;
	int startQuantityPlayers;
	unsigned dealerPosition;
	GameState currentRound;
	GameState roundBeforePostRiver;
	int smallBlind;
	int startCash;

	int previousPlayerID;
	unsigned lastActionPlayerID;

	bool allInCondition;
	bool cardsShown;
};

#endif


