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


using namespace std;


GuiWrapper::GuiWrapper() : myLog(0), myW(0)
{


	myW = new mainWindowImpl;
	myW->show();

	myLog = new Log(myW);
	myConfig = new ConfigFile;

}


GuiWrapper::~GuiWrapper()
{
}

void GuiWrapper::setGame(Game *g) { myW->setGame(g); }
void GuiWrapper::setHand(HandInterface *lh) { myW->setHand(lh); }
void GuiWrapper::setSession(Session *s) { myW->setSession(s); }

int GuiWrapper::getMaxQuantityPlayers() const { return myW->getMaxQuantityPlayers(); }

void GuiWrapper::refreshSet() const { myW->refreshSet(); }
void GuiWrapper::refreshCash() const { myW->refreshCash(); }
void GuiWrapper::refreshAction(int playerID, int playerAction) const { myW->refreshAction(playerID, playerAction); }
void GuiWrapper::refreshChangePlayer() const { myW->refreshChangePlayer(); }
void GuiWrapper::refreshAll() const { myW->refreshAll(); }
void GuiWrapper::refreshPot() const { myW->refreshPot(); }
void GuiWrapper::refreshGroupbox(int playerID, int status) const { myW->refreshGroupbox(playerID, status); }
void GuiWrapper::refreshPlayerName() const { myW->refreshPlayerName(); }

void GuiWrapper::dealHoleCards() const { myW->dealHoleCards(); }
void GuiWrapper::dealFlopCards() const { myW->dealFlopCards0(); }
void GuiWrapper::dealTurnCard() const { myW->dealTurnCards0(); }
void GuiWrapper::dealRiverCard() const { myW->dealRiverCards0(); }

void GuiWrapper::highlightRoundLabel(string round) const { myW->highlightRoundLabel(round); }


void GuiWrapper::nextPlayerAnimation() const { myW->nextPlayerAnimation(); }

void GuiWrapper::preflopAnimation1() const { myW->preflopAnimation1(); }
void GuiWrapper::preflopAnimation2() const { myW->preflopAnimation2(); }

void GuiWrapper::flopAnimation1() const { myW->flopAnimation1(); }
void GuiWrapper::flopAnimation2() const { myW->flopAnimation2(); }

void GuiWrapper::turnAnimation1() const { myW->turnAnimation1(); }
void GuiWrapper::turnAnimation2() const { myW->turnAnimation2(); }

void GuiWrapper::riverAnimation1() const { myW->riverAnimation1(); }
void GuiWrapper::riverAnimation2() const { myW->riverAnimation2(); }

void GuiWrapper::postRiverAnimation1() const { myW->postRiverAnimation1(); }
void GuiWrapper::postRiverRunAnimation1() const { myW->postRiverRunAnimation1(); }

void GuiWrapper::flipHolecardsAllIn() const { myW->flipHolecardsAllIn(); }

void GuiWrapper::nextRoundCleanGui() const { myW->nextRoundCleanGui(); }

void GuiWrapper::meInAction() const { myW->meInAction(); }

void GuiWrapper::logPlayerActionMsg(string playerName, int action, int setValue) { myLog->logPlayerActionMsg(playerName, action, setValue); }
void GuiWrapper::logNewGameHandMsg(int gameID, int handID) { myLog->logNewGameHandMsg(gameID, handID); }

void GuiWrapper::SignalNetClientConnect(int actionID) { myW->SignalNetClientConnect(actionID); }
void GuiWrapper::SignalNetClientGameInfo(int actionID) { myW->SignalNetClientGameInfo(actionID); }
void GuiWrapper::SignalNetClientError(int errorID, int osErrorID) { myW->SignalNetClientError(errorID, osErrorID); }

void GuiWrapper::SignalNetServerSuccess(int actionID) { }
void GuiWrapper::SignalNetServerError(int errorID, int osErrorID) { }

