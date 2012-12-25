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

#ifndef CLIENTPLAYER_H
#define CLIENTPLAYER_H

#include <playerinterface.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <string>

class ConfigFile;
class HandInterface;

class ClientPlayer : public PlayerInterface
{
public:
	ClientPlayer(ConfigFile*, int id, unsigned uniqueId, PlayerType type, std::string name, std::string avatar, int sC, bool aS, bool sotS, int mB);
	~ClientPlayer();

	void setHand(HandInterface *);

	int getMyID() const;
	void setMyUniqueID(unsigned newId);
	unsigned getMyUniqueID() const;
	void setMyGuid(const std::string &theValue);
	std::string getMyGuid() const;
	PlayerType getMyType() const;

	void setMyDude(int theValue);
	int getMyDude() const;

	void setMyDude4(int theValue);
	int getMyDude4() const;

	void setMyName(const std::string& theValue);
	std::string getMyName() const;

	void setMyAvatar(const std::string& theValue);
	std::string getMyAvatar() const;

	void setMyCash(int theValue);
	int getMyCash() const;

	void setMySet(int theValue);
	void setMySetAbsolute(int theValue);
	void setMySetNull();
	int getMySet() const;
	int getMyLastRelativeSet() const;

	void setMyAction(PlayerAction theValue, bool human);
	PlayerAction getMyAction() const;

	void setMyButton(int theValue);
	int getMyButton() const;

	void setMyActiveStatus(bool theValue);
	bool getMyActiveStatus() const;

	void setMyStayOnTableStatus(bool theValue);
	bool getMyStayOnTableStatus() const;

	void setMyCards(int* theValue);
	void getMyCards(int* theValue) const;

	void setMyTurn(bool theValue);
	bool getMyTurn() const;

	void setMyCardsFlip(bool theValue, int state);
	bool getMyCardsFlip() const;

	void setMyCardsValueInt(int theValue);
	int getMyCardsValueInt() const;

	void setLogHoleCardsDone(bool theValue);
	bool getLogHoleCardsDone() const;

	void setMyBestHandPosition(int* theValue);
	void getMyBestHandPosition(int* theValue) const;

	void setMyRoundStartCash(int theValue);
	int getMyRoundStartCash() const;

	void setLastMoneyWon ( int theValue );
	int getLastMoneyWon() const;

	void setMyAverageSets(int theValue);
	int getMyAverageSets() const;

	void setMyAggressive(bool theValue);
	int getMyAggressive() const;

	void setSBluff (int theValue);
	int getSBluff() const;

	void setSBluffStatus (bool theValue);
	bool getSBluffStatus() const;

	void action();
	int checkMyAction(int targetAction, int targetBet, int highestSet, int minimumRaise, int smallBlind);

	void preflopEngine();
	void flopEngine();
	void turnEngine();
	void riverEngine();

	void preflopEngine3();
	void flopEngine3();
	void turnEngine3();
	void riverEngine3();

	int preflopCardsValue(int*);
	int flopCardsValue(int*);
	int turnCardsValue(int*);

	void readFile();

	void evaluation(int, int);

	void setIsSessionActive(bool active);
	bool isSessionActive() const;
	void setIsKicked(bool kicked);
	bool isKicked() const;
	void setIsMuted(bool muted);
	bool isMuted() const;

	bool checkIfINeedToShowCards();

	void markRemoteAction() {}
	unsigned getTimeSecSinceLastRemoteAction() const {
		return 0;
	}

private:
	mutable boost::recursive_mutex m_syncMutex;

	ConfigFile *myConfig;
	HandInterface *currentHand;

	// Konstanten
	const int myID;
	unsigned myUniqueID;
	std::string myGuid;
	const PlayerType myType;
	std::string myName;
	std::string myAvatar;
	int myDude;
	int myDude4;


	// Laufvariablen
	int myCardsValueInt;
	int myBestHandPosition[5];
	double myOdds;
	int myNiveau[3];
	bool logHoleCardsDone;

	int myCards[2];
	int myCash;
	int mySet;
	int myLastRelativeSet;
	PlayerAction myAction;
	int myButton; // 0 = none, 1 = dealer, 2 =small, 3 = big
	bool myActiveStatus; // 0 = inactive, 1 = active
	bool myStayOnTableStatus; // 0 = left, 1 = stay
	bool myTurn; // 0 = no, 1 = yes
	bool myCardsFlip; // 0 = cards are not fliped, 1 = cards are already flipped,
	int myRoundStartCash;
	int lastMoneyWon;

	int myAverageSets[4];
	bool myAggressive[7];

	int sBluff;
	bool sBluffStatus;

	bool m_isSessionActive;
	bool m_isKicked;
	bool m_isMuted;
};

#endif
