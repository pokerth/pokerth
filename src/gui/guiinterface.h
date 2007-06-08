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
#ifndef GUIINTERFACE_H
#define GUIINTERFACE_H

#include <net/clientcallback.h>
#include <net/servercallback.h>
#include <game_defs.h>
#include <string>
#include <boost/shared_ptr.hpp>


class Session;


class GuiInterface : public ClientCallback, public ServerCallback {
public:
	virtual ~GuiInterface();

	virtual void initGui(int speed) =0;

	virtual Session &getSession() =0;
	virtual void setSession(boost::shared_ptr<Session> session) =0;

	//refresh-Funktionen
	virtual void refreshSet() const=0;
	virtual void refreshCash() const=0;
	virtual void refreshAction(int =-1, int =-1) const=0;
	virtual void refreshChangePlayer() const=0;
	virtual void refreshPot() const=0;
	virtual void refreshGroupbox(int =-1, int =-1) const=0;
	virtual void refreshAll() const=0;
	virtual void refreshPlayerName() const=0;
	virtual void refreshButton() const =0;
	virtual void refreshGameLabels(GameState state) const=0;

// 	// Karten-Funktionen
	virtual void dealHoleCards()=0;
	virtual void dealFlopCards()=0;
	virtual void dealTurnCard()=0;
	virtual void dealRiverCard()=0;

	virtual void nextPlayerAnimation()=0;

	virtual void preflopAnimation1()=0;
	virtual void preflopAnimation2()=0;

	virtual void flopAnimation1()=0;
	virtual void flopAnimation2()=0;

	virtual void turnAnimation1()=0;
	virtual void turnAnimation2()=0;

	virtual void riverAnimation1()=0;
	virtual void riverAnimation2()=0;

	virtual void postRiverAnimation1()=0;
	virtual void postRiverRunAnimation1()=0;
// 	virtual void postRiverAnimation2()=0;
	virtual void flipHolecardsAllIn()=0;

// 	virtual void startNewHand() const=0;
	virtual void nextRoundCleanGui()=0;
// 	
// 	virtual void userWidgetsBackgroudColor() const=0;
// 	virtual void timerBlockerFalse() const=0;
	virtual void meInAction()=0;
	virtual void disableMyButtons()=0;
	virtual void startTimeoutAnimation(int playerId, int timeoutSec) =0;
	virtual void stopTimeoutAnimation(int playerId) =0;

	//log.cpp
	virtual void logPlayerActionMsg(std::string playName, int action, int setValue) =0;
	virtual void logNewGameHandMsg(int gameID, int HandID) =0;
	virtual	void logPlayerWinsMsg(int playerID, int pot) = 0;
	virtual void logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4 = -1, int card5 = -1) = 0;
	virtual void logFlipHoleCardsMsg(std::string playerName, int card1, int card2, int cardsValueInt = -1, std::string showHas = "shows") = 0;
};

#endif
