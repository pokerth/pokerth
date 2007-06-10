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

#include "guiwrapper.h"
#include "session.h"
#include "log.h"
#include "chat.h"
#include "mainwindowimpl.h"
#include "configfile.h"

using namespace std;


GuiWrapper::GuiWrapper(ConfigFile *c) : myLog(0), myW(0), myConfig(c)
{


	myW = new mainWindowImpl(myConfig);
	myW->show();
	myLog = new Log(myW, myConfig);
	myChat = new Chat(myW, myConfig);

}


GuiWrapper::~GuiWrapper()
{
}

void GuiWrapper::initGui(int speed) { myW->signalInitGui(speed); }

Session &GuiWrapper::getSession() { return myW->getSession(); }
void GuiWrapper::setSession(boost::shared_ptr<Session> session) { myW->setSession(session); }

void GuiWrapper::refreshSet() const { myW->signalRefreshSet(); }
void GuiWrapper::refreshCash() const { myW->signalRefreshCash(); }
void GuiWrapper::refreshAction(int playerID, int playerAction) const { myW->signalRefreshAction(playerID, playerAction); }
void GuiWrapper::refreshChangePlayer() const { myW->signalRefreshChangePlayer(); }
void GuiWrapper::refreshAll() const { myW->signalRefreshAll(); }
void GuiWrapper::refreshPot() const { myW->signalRefreshPot(); }
void GuiWrapper::refreshGroupbox(int playerID, int status) const { myW->signalRefreshGroupbox(playerID, status); }
void GuiWrapper::refreshPlayerName() const { myW->signalRefreshPlayerName(); }
void GuiWrapper::refreshButton() const { myW->signalRefreshButton(); }
void GuiWrapper::refreshGameLabels(GameState state) const { myW->signalRefreshGameLabels(state); }

void GuiWrapper::waitForGuiUpdateDone() const
{
	myW->signalGuiUpdateDone();
	myW->waitForGuiUpdateDone();
}

void GuiWrapper::dealHoleCards() { myW->signalDealHoleCards(); }
void GuiWrapper::dealFlopCards() { myW->signalDealFlopCards0(); }
void GuiWrapper::dealTurnCard() { myW->signalDealTurnCards0(); }
void GuiWrapper::dealRiverCard() { myW->signalDealRiverCards0(); }

void GuiWrapper::nextPlayerAnimation() { myW->signalNextPlayerAnimation(); }

void GuiWrapper::preflopAnimation1() { myW->signalPreflopAnimation1(); }
void GuiWrapper::preflopAnimation2() { myW->signalPreflopAnimation2(); }

void GuiWrapper::flopAnimation1() { myW->signalFlopAnimation1(); }
void GuiWrapper::flopAnimation2() { myW->signalFlopAnimation2(); }

void GuiWrapper::turnAnimation1() { myW->signalTurnAnimation1(); }
void GuiWrapper::turnAnimation2() { myW->signalTurnAnimation2(); }

void GuiWrapper::riverAnimation1() { myW->signalRiverAnimation1(); }
void GuiWrapper::riverAnimation2() { myW->signalRiverAnimation2(); }

void GuiWrapper::postRiverAnimation1() { myW->signalPostRiverAnimation1(); }
void GuiWrapper::postRiverRunAnimation1() { myW->signalPostRiverRunAnimation1(); }

void GuiWrapper::flipHolecardsAllIn() { myW->signalFlipHolecardsAllIn(); }

void GuiWrapper::nextRoundCleanGui() { myW->signalNextRoundCleanGui(); }

void GuiWrapper::meInAction() { myW->signalMeInAction(); }
void GuiWrapper::disableMyButtons() { myW->signalDisableMyButtons(); }
void GuiWrapper::startTimeoutAnimation(int playerId, int timeoutSec) { myW->signalStartTimeoutAnimation(playerId, timeoutSec); }
void GuiWrapper::stopTimeoutAnimation(int playerId) { myW->signalStopTimeoutAnimation(playerId); }

void GuiWrapper::logPlayerActionMsg(string playerName, int action, int setValue) { myLog->signalLogPlayerActionMsg(QString::fromUtf8(playerName.c_str()), action, setValue); }
void GuiWrapper::logNewGameHandMsg(int gameID, int handID) { myLog->signalLogNewGameHandMsg(gameID, handID); }
void GuiWrapper::logPlayerWinsMsg(int playerID, int pot) { myLog->signalLogPlayerWinsMsg(playerID, pot); }
void GuiWrapper::logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4, int card5) { myLog->signalLogDealBoardCardsMsg(roundID, card1, card2, card3, card4, card5); }
void GuiWrapper::logFlipHoleCardsMsg(string playerName, int card1, int card2, int cardsValueInt, string showHas) { myLog->signalLogFlipHoleCardsMsg(QString::fromUtf8(playerName.c_str()), card1, card2, cardsValueInt, QString::fromUtf8(showHas.c_str())); }

void GuiWrapper::SignalNetClientConnect(int actionID) { myW->signalNetClientConnect(actionID); }
void GuiWrapper::SignalNetClientGameInfo(int actionID) { myW->signalNetClientGameInfo(actionID); }
void GuiWrapper::SignalNetClientError(int errorID, int osErrorID) { myW->signalNetClientError(errorID, osErrorID); }
void GuiWrapper::SignalNetClientPlayerJoined(const string &playerName) { myW->signalNetClientPlayerJoined(QString::fromUtf8(playerName.c_str())); }
void GuiWrapper::SignalNetClientPlayerLeft(const string &playerName) { myW->signalNetClientPlayerLeft(QString::fromUtf8(playerName.c_str())); }
void GuiWrapper::SignalNetClientGameStart(boost::shared_ptr<Game> game) { myW->signalNetClientGameStart(game); }
void GuiWrapper::SignalNetClientChatMsg(const string &playerName, const string &msg) { myChat->signalChatMessage(QString::fromUtf8(playerName.c_str()), QString::fromUtf8(msg.c_str())); }

void GuiWrapper::SignalNetServerSuccess(int actionID) { }
void GuiWrapper::SignalNetServerError(int errorID, int osErrorID) { myW->signalNetServerError(errorID, osErrorID); }
void GuiWrapper::SignalNetServerPlayerJoined(const string &playerName) { myW->signalNetServerPlayerJoined(QString::fromUtf8(playerName.c_str())); }
void GuiWrapper::SignalNetServerPlayerLeft(const string &playerName)
{
	QString tmpName(QString::fromUtf8(playerName.c_str()));
	myW->signalNetServerPlayerLeft(tmpName);
	myLog->signalLogPlayerLeftMsg(tmpName);
}


