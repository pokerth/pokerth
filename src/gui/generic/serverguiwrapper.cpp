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
#include "genericservergui.h"


using namespace std;


ServerGuiWrapper::ServerGuiWrapper(ConfigFile *config, ClientCallback *clientcb, ServerCallback *servercb)
: myClientcb(clientcb), myServercb(servercb)
{
	myGui.reset(new GenericServerGui(config));
}

ServerGuiWrapper::~ServerGuiWrapper()
{
}

void ServerGuiWrapper::initGui(int speed) {}

Session &ServerGuiWrapper::getSession()
{
	return myGui->getSession();
}

void ServerGuiWrapper::setSession(boost::shared_ptr<Session> session)
{
	myGui->setSession(session);
}

bool
ServerGuiWrapper::isNetworkServer() const
{
	/* TODO hack */
	return true;
}

void
ServerGuiWrapper::waitForNetworkAction(GameState state, unsigned uniquePlayerId)
{
	/* TODO hack */
	myGui->waitForNetworkAction(state, uniquePlayerId);
}

void ServerGuiWrapper::refreshSet() const {}

void ServerGuiWrapper::refreshCash() const {}

void ServerGuiWrapper::refreshAction(int playerID, int playerAction) const {}

void ServerGuiWrapper::refreshChangePlayer() const {}

void ServerGuiWrapper::refreshAll() const {}

void ServerGuiWrapper::refreshPot() const {}
void ServerGuiWrapper::refreshGroupbox(int playerID, int status) const {}
void ServerGuiWrapper::refreshPlayerName() const {}

void ServerGuiWrapper::dealHoleCards() const {}
void ServerGuiWrapper::dealFlopCards() const {}
void ServerGuiWrapper::dealTurnCard() const {}
void ServerGuiWrapper::dealRiverCard() const {}

void ServerGuiWrapper::highlightRoundLabel(string round) const {}


void ServerGuiWrapper::nextPlayerAnimation() const {}

void ServerGuiWrapper::preflopAnimation1() const {}
void ServerGuiWrapper::preflopAnimation2() const {}

void ServerGuiWrapper::flopAnimation1() const {}
void ServerGuiWrapper::flopAnimation2() const {}

void ServerGuiWrapper::turnAnimation1() const {}
void ServerGuiWrapper::turnAnimation2() const {}

void ServerGuiWrapper::riverAnimation1() const {}
void ServerGuiWrapper::riverAnimation2() const {}

void ServerGuiWrapper::postRiverAnimation1() const {}
void ServerGuiWrapper::postRiverRunAnimation1() const {}

void ServerGuiWrapper::flipHolecardsAllIn() const {}

void ServerGuiWrapper::nextRoundCleanGui() const {}

void ServerGuiWrapper::meInAction() const {}

void ServerGuiWrapper::logPlayerActionMsg(string playerName, int action, int setValue) {}
void ServerGuiWrapper::logNewGameHandMsg(int gameID, int handID) {}

void ServerGuiWrapper::SignalNetClientConnect(int actionID) { if (myClientcb) myClientcb->SignalNetClientConnect(actionID); }
void ServerGuiWrapper::SignalNetClientGameInfo(int actionID) { if (myClientcb) myClientcb->SignalNetClientGameInfo(actionID); }
void ServerGuiWrapper::SignalNetClientError(int errorID, int osErrorID) { if (myClientcb) myClientcb->SignalNetClientError(errorID, osErrorID); }
void ServerGuiWrapper::SignalNetClientPlayerJoined(const string &playerName) { if (myClientcb) myClientcb->SignalNetClientPlayerJoined(playerName); }
void ServerGuiWrapper::SignalNetClientPlayerLeft(const string &playerName) { if (myClientcb) myClientcb->SignalNetClientPlayerLeft(playerName); }
void ServerGuiWrapper::SignalNetClientGameStart(const GameData &gameData) { if (myClientcb) myClientcb->SignalNetClientGameStart(gameData); }

void ServerGuiWrapper::SignalNetServerSuccess(int actionID) { if (myServercb) myServercb->SignalNetServerSuccess(actionID); }
void ServerGuiWrapper::SignalNetServerError(int errorID, int osErrorID) { if (myServercb) myServercb->SignalNetServerError(errorID, osErrorID); }
void ServerGuiWrapper::SignalNetServerPlayerJoined(const string &playerName) { if (myServercb) myServercb->SignalNetServerPlayerJoined(playerName); }
void ServerGuiWrapper::SignalNetServerPlayerLeft(const string &playerName) { if (myServercb) myServercb->SignalNetServerPlayerLeft(playerName); }


