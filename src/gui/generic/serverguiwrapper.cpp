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

#include "serverguiwrapper.h"
#include <session.h>


using namespace std;


ServerGuiWrapper::ServerGuiWrapper(ConfigFile *config, ClientCallback *clientcb, ServerCallback *servercb)
: myConfig(config), myClientcb(clientcb), myServercb(servercb)
{
}

ServerGuiWrapper::~ServerGuiWrapper()
{
}

void ServerGuiWrapper::initGui(int speed) {}

Session &ServerGuiWrapper::getSession()
{
	assert(mySession.get());
	return *mySession;
}

void ServerGuiWrapper::setSession(boost::shared_ptr<Session> session)
{
	mySession = session;
}

void ServerGuiWrapper::refreshSet() const {}

void ServerGuiWrapper::refreshCash() const {}

void ServerGuiWrapper::refreshAction(int playerID, int playerAction) const {}

void ServerGuiWrapper::refreshChangePlayer() const {}

void ServerGuiWrapper::refreshAll() const {}

void ServerGuiWrapper::refreshPot() const {}
void ServerGuiWrapper::refreshGroupbox(int playerID, int status) const {}
void ServerGuiWrapper::refreshPlayerName() const {}
void ServerGuiWrapper::refreshButton() const {}
void ServerGuiWrapper::refreshGameLabels(GameState state) const {}

void ServerGuiWrapper::dealHoleCards() {}
void ServerGuiWrapper::dealFlopCards() {}
void ServerGuiWrapper::dealTurnCard() {}
void ServerGuiWrapper::dealRiverCard() {}

void ServerGuiWrapper::nextPlayerAnimation() {}

void ServerGuiWrapper::preflopAnimation1() {}
void ServerGuiWrapper::preflopAnimation2() {}

void ServerGuiWrapper::flopAnimation1() {}
void ServerGuiWrapper::flopAnimation2() {}

void ServerGuiWrapper::turnAnimation1() {}
void ServerGuiWrapper::turnAnimation2() {}

void ServerGuiWrapper::riverAnimation1() {}
void ServerGuiWrapper::riverAnimation2() {}

void ServerGuiWrapper::postRiverAnimation1() {}
void ServerGuiWrapper::postRiverRunAnimation1() {}

void ServerGuiWrapper::flipHolecardsAllIn() {}

void ServerGuiWrapper::nextRoundCleanGui() {}

void ServerGuiWrapper::meInAction() {}
void ServerGuiWrapper::disableMyButtons() {}
void ServerGuiWrapper::startTimeoutAnimation(int playerId, int timeoutSec) {}
void ServerGuiWrapper::stopTimeoutAnimation(int playerId) {}

void ServerGuiWrapper::logPlayerActionMsg(string playerName, int action, int setValue) {}
void ServerGuiWrapper::logNewGameHandMsg(int gameID, int handID) {}
void ServerGuiWrapper::logPlayerWinsMsg(int playerID, int pot) {}
void ServerGuiWrapper::logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4, int card5) {}
void ServerGuiWrapper::logFlipHoleCardsMsg(std::string playerName, int card1, int card2, int cardsValueInt, std::string showHas) {}

void ServerGuiWrapper::SignalNetClientConnect(int actionID) { if (myClientcb) myClientcb->SignalNetClientConnect(actionID); }
void ServerGuiWrapper::SignalNetClientGameInfo(int actionID) { if (myClientcb) myClientcb->SignalNetClientGameInfo(actionID); }
void ServerGuiWrapper::SignalNetClientError(int errorID, int osErrorID) { if (myClientcb) myClientcb->SignalNetClientError(errorID, osErrorID); }
void ServerGuiWrapper::SignalNetClientPlayerJoined(const string &playerName) { if (myClientcb) myClientcb->SignalNetClientPlayerJoined(playerName); }
void ServerGuiWrapper::SignalNetClientPlayerLeft(const string &playerName) { if (myClientcb) myClientcb->SignalNetClientPlayerLeft(playerName); }
void ServerGuiWrapper::SignalNetClientGameStart(boost::shared_ptr<Game> game) { if (myClientcb) myClientcb->SignalNetClientGameStart(game); }
void ServerGuiWrapper::SignalNetClientChatMsg(const string &playerName, const string &msg) { if (myClientcb) myClientcb->SignalNetClientChatMsg(playerName, msg); }

void ServerGuiWrapper::SignalNetServerSuccess(int actionID) { if (myServercb) myServercb->SignalNetServerSuccess(actionID); }
void ServerGuiWrapper::SignalNetServerError(int errorID, int osErrorID) { if (myServercb) myServercb->SignalNetServerError(errorID, osErrorID); }
void ServerGuiWrapper::SignalNetServerPlayerJoined(const string &playerName) { if (myServercb) myServercb->SignalNetServerPlayerJoined(playerName); }
void ServerGuiWrapper::SignalNetServerPlayerLeft(const string &playerName) { if (myServercb) myServercb->SignalNetServerPlayerLeft(playerName); }


