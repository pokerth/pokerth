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

#include "clientplayer.h"
#include <handinterface.h>

using namespace std;


ClientPlayer::ClientPlayer(ConfigFile *c, int id, unsigned uniqueId, PlayerType type, std::string name, std::string avatar, int sC, bool aS, bool sotS, int mB)
	: PlayerInterface(), myConfig(c), currentHand(0), myID(id), myUniqueID(uniqueId), myType(type),
	  myName(name), myAvatar(avatar), myDude(0), myDude4(0), myCardsValueInt(0), myOdds(-1.0), logHoleCardsDone(false), myCash(sC), mySet(0), myLastRelativeSet(0),
	  myAction(PLAYER_ACTION_NONE), myButton(mB), myActiveStatus(aS), myStayOnTableStatus(sotS), myTurn(false), myCardsFlip(false), myRoundStartCash(0),
	  lastMoneyWon(0), sBluff(0), sBluffStatus(false), m_isSessionActive(false), m_isKicked(false), m_isMuted(false)
{
	myBestHandPosition[0] = myBestHandPosition[1] = myBestHandPosition[2] = myBestHandPosition[3] = myBestHandPosition[4] = 0;
	myNiveau[0] = myNiveau[1] = myNiveau[2] = 0;
	myCards[0] = myCards[1] = 0;
	myAverageSets[0] = myAverageSets[1] = myAverageSets[2] = myAverageSets[3] = 0;
	myAggressive[0] = myAggressive[1] = myAggressive[2] = myAggressive[3] = myAggressive[4] = myAggressive[5] = myAggressive[6] = false;
}


ClientPlayer::~ClientPlayer()
{
}


void
ClientPlayer::setHand(HandInterface* br)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	currentHand = br;
}

int
ClientPlayer::getMyID() const
{
	return myID;
}

void
ClientPlayer::setMyUniqueID(unsigned newId)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myUniqueID = newId;
}

unsigned
ClientPlayer::getMyUniqueID() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myUniqueID;
}

void
ClientPlayer::setMyGuid(const std::string &theValue)
{
	myGuid = theValue;
}

std::string
ClientPlayer::getMyGuid() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myGuid;
}

PlayerType
ClientPlayer::getMyType() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myType;
}

void
ClientPlayer::setMyDude(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myDude = theValue;
}

int
ClientPlayer::getMyDude() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myDude;
}

void
ClientPlayer::setMyDude4(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myDude4 = theValue;
}

int
ClientPlayer::getMyDude4() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myDude4;
}

void
ClientPlayer::setMyName(const std::string& theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myName = theValue;
}

std::string
ClientPlayer::getMyName() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myName;
}

void
ClientPlayer::setMyAvatar(const std::string& theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myAvatar = theValue;
}

std::string
ClientPlayer::getMyAvatar() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myAvatar;
}

void
ClientPlayer::setMyCash(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myCash = theValue;
}

int
ClientPlayer::getMyCash() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myCash;
}

void
ClientPlayer::setMySet(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myLastRelativeSet = theValue;
}

void
ClientPlayer::setMySetAbsolute(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	mySet = theValue;
}

void
ClientPlayer::setMySetNull()
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	mySet = 0;
	myLastRelativeSet = 0;
}

int
ClientPlayer::getMySet() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return mySet;
}

int
ClientPlayer::getMyLastRelativeSet() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myLastRelativeSet;
}

void
ClientPlayer::setMyAction(PlayerAction theValue, bool /*human*/)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myAction = theValue;
}

PlayerAction
ClientPlayer::getMyAction() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myAction;
}

void
ClientPlayer::setMyButton(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myButton = theValue;
}

int
ClientPlayer::getMyButton() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myButton;
}

void
ClientPlayer::setMyActiveStatus(bool theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myActiveStatus = theValue;
}

bool
ClientPlayer::getMyActiveStatus() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myActiveStatus;
}

void
ClientPlayer::setMyStayOnTableStatus(bool theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myStayOnTableStatus = theValue;
}

bool
ClientPlayer::getMyStayOnTableStatus() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myStayOnTableStatus;
}

void
ClientPlayer::setMyCards(int* theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	for (int i = 0; i < 2; i++)
		myCards[i] = theValue[i];
}

void
ClientPlayer::getMyCards(int* theValue) const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	for (int i = 0; i < 2; i++)
		theValue[i] = myCards[i];
}

void
ClientPlayer::setMyTurn(bool theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myTurn = theValue;
}

bool
ClientPlayer::getMyTurn() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myTurn;
}

void
ClientPlayer::setMyCardsFlip(bool theValue, int state)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myCardsFlip = theValue;
	// log flipping cards
	if (myCardsFlip) {
		switch(state) {
		case 1:
			currentHand->getGuiInterface()->logFlipHoleCardsMsg(myName, myCards[0], myCards[1], myCardsValueInt);
			break;
		case 2:
			currentHand->getGuiInterface()->logFlipHoleCardsMsg(myName, myCards[0], myCards[1]);
			break;
		case 3:
			currentHand->getGuiInterface()->logFlipHoleCardsMsg(myName, myCards[0], myCards[1], myCardsValueInt, "has");
			break;
		default:
			;
		}
	}
}

bool
ClientPlayer::getMyCardsFlip() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myCardsFlip;
}

void
ClientPlayer::setMyCardsValueInt(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myCardsValueInt = theValue;
}

int
ClientPlayer::getMyCardsValueInt() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myCardsValueInt;
}

void
ClientPlayer::setLogHoleCardsDone(bool theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	logHoleCardsDone = theValue;
}

bool
ClientPlayer::getLogHoleCardsDone() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return logHoleCardsDone;
}

void
ClientPlayer::setMyBestHandPosition(int* theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	for (int i = 0; i < 5; i++)
		myBestHandPosition[i] = theValue[i];
}

void
ClientPlayer::getMyBestHandPosition(int* theValue) const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	for (int i = 0; i < 5; i++)
		theValue[i] = myBestHandPosition[i];
}

void
ClientPlayer::setMyRoundStartCash(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myRoundStartCash = theValue;
}

int
ClientPlayer::getMyRoundStartCash() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myRoundStartCash;
}

void
ClientPlayer::setLastMoneyWon ( int theValue )
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	lastMoneyWon = theValue;
}

int
ClientPlayer::getLastMoneyWon() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return lastMoneyWon;
}

void
ClientPlayer::setMyAverageSets(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myAverageSets[0] = myAverageSets[1];
	myAverageSets[1] = myAverageSets[2];
	myAverageSets[2] = myAverageSets[3];
	myAverageSets[3] = theValue;
}

int
ClientPlayer::getMyAverageSets() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return (myAverageSets[0]+myAverageSets[1]+myAverageSets[2]+myAverageSets[3])/4;
}

void
ClientPlayer::setMyAggressive(bool theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	for (int i=0; i<6; i++) {
		myAggressive[i] = myAggressive[i+1];
	}
	myAggressive[6] = theValue;
}

int
ClientPlayer::getMyAggressive() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	int sum = 0;
	for (int i=0; i<7; i++) {
		sum += myAggressive[i];
	}
	return sum;
}

void
ClientPlayer::setSBluff(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	sBluff = theValue;
}

int
ClientPlayer::getSBluff() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return sBluff;
}

void
ClientPlayer::setSBluffStatus(bool theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	sBluffStatus = theValue;
}

bool
ClientPlayer::getSBluffStatus() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return sBluffStatus;
}

void
ClientPlayer::action()
{
}

int
ClientPlayer::checkMyAction(int /*targetAction*/, int /*targetBet*/, int /*highestSet*/, int /*minimumRaise*/, int /*smallBlind*/)
{
	return 0;
}

void
ClientPlayer::preflopEngine()
{
}


void
ClientPlayer::flopEngine()
{
}


void
ClientPlayer::turnEngine()
{
}


void
ClientPlayer::riverEngine()
{
}


void
ClientPlayer::evaluation(int /*bet*/, int /*raise*/)
{
}


int
ClientPlayer::preflopCardsValue(int* /*cards*/)
{
	return 0;
}


int
ClientPlayer::flopCardsValue(int* /*cards*/)
{
	return 0;
}


void
ClientPlayer::readFile()
{
}

int
ClientPlayer::turnCardsValue(int* /*cards*/)
{
	return 0;
}

void
ClientPlayer::preflopEngine3()
{
}

void
ClientPlayer::flopEngine3()
{
}

void
ClientPlayer::turnEngine3()
{
}

void
ClientPlayer::riverEngine3()
{
}


void
ClientPlayer::setIsSessionActive(bool active)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	m_isSessionActive = active;
}

bool
ClientPlayer::isSessionActive() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return m_isSessionActive;
}

void
ClientPlayer::setIsKicked(bool kicked)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	m_isKicked = kicked;
}

bool
ClientPlayer::isKicked() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return m_isKicked;
}

void
ClientPlayer::setIsMuted(bool muted)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	m_isMuted = muted;
}

bool
ClientPlayer::isMuted() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return m_isMuted;
}

bool ClientPlayer::checkIfINeedToShowCards()
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	std::list<unsigned> playerNeedToShowCardsList = currentHand->getBoard()->getPlayerNeedToShowCards();
	for(std::list<unsigned>::iterator it = playerNeedToShowCardsList.begin(); it != playerNeedToShowCardsList.end(); ++it) {
		if(*it == myUniqueID) return true;
	}

	return false;
}
