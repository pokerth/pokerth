/*****************************************************************************
 * Independent Chip Model (ICM) calculation                                  *
 * Copyright (C) 2015 Daniel Steinhauer                                      *
 *                                                                           *
 * This file is part of:                                                     *
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

#include "gametableimpl_ICM.h"
#include "session.h"
#include "game.h"
#include "playerinterface.h"
#include "configfile.h"

gameTableImpl_ICM::gameTableImpl_ICM (ConfigFile *c, QMainWindow *parent)
  : gameTableImpl (c, parent)
{
  icm_calc = new independent_chip_model::Table ();
}

gameTableImpl_ICM::~gameTableImpl_ICM ()
{
  delete icm_calc;
}

void gameTableImpl_ICM::refreshCash ()
{
  if (myConfig->readConfigInt ("ShowICM"))  {
    boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();

    PlayerListConstIterator it_c;
    PlayerList seatsList = currentGame->getSeatsList();
  
    for (it_c = seatsList->begin (); it_c != seatsList->end (); ++it_c)  {
      icm_calc->setStack ((*it_c)->getMyID (), (*it_c)->getMyCash ());
    }
  
    icm_calc->calc_ICM ();
  
    for (it_c = seatsList->begin (); it_c != seatsList->end (); ++it_c)  {
      cashLabelArray[(*it_c)->getMyID ()]->setToolTip (
        QString ("ICM: %1 P").arg (icm_calc->get_EV ((*it_c)->getMyID ()))
      );
    }
  }
  
  gameTableImpl::refreshCash ();
}

void gameTableImpl_ICM::provideMyActions (int mode)
{
  gameTableImpl::provideMyActions (mode);
  
  boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
  boost::shared_ptr<PlayerInterface> humanPlayer = currentHand->getSeatsList()->front();
  PlayerList activePlayerList = currentHand->getActivePlayerList();
  
  pushButton_CallCheck->setToolTip ("");
  if (!(
    (mode && (humanPlayer->getMyAction() == PLAYER_ACTION_ALLIN || humanPlayer->getMyAction() == PLAYER_ACTION_FOLD || (humanPlayer->getMySet() == currentHand->getCurrentBeRo()->getHighestSet() && (humanPlayer->getMyAction() != PLAYER_ACTION_NONE)))) || !humanPlayer->isSessionActive()
  ) && (myConfig->readConfigInt ("ShowICM")))  {
    if (!(humanPlayer->getMySet()== currentHand->getCurrentBeRo()->getHighestSet()))  {
      pushButton_CallCheck->setToolTip (
	QString ("ICM: %1 P").arg (icm_calc->get_Call_EV (
	  humanPlayer->getMyID (), getMyCallAmount ()
	))
      );
    }
  }
}