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
#include "createinternetgamedialogimpl.h"
#include "session.h"
#include "configfile.h"

createInternetGameDialogImpl::createInternetGameDialogImpl(QWidget *parent, ConfigFile *c)
      : QDialog(parent), myConfig(c)
{

    setupUi(this);

	fillFormular();

	connect( pushButton_cancel, SIGNAL( clicked() ), this, SLOT( cancel() ) );
	connect( pushButton_createGame, SIGNAL( clicked() ), this, SLOT( createGame() ) );

	//temporarely unused until ai is enabled in network
// 	label_5->hide();
// 	spinBox_gameSpeed->hide();


}


void createInternetGameDialogImpl::exec() { 
	
	fillFormular();
	QDialog::exec();
}

void createInternetGameDialogImpl::createGame() {
	
}

void createInternetGameDialogImpl::cancel() {
	
}

void createInternetGameDialogImpl::fillFormular() {
	
	//Network Game Settings
	spinBox_quantityPlayers->setValue(myConfig->readConfigInt("NetNumberOfPlayers"));
	spinBox_startCash->setValue(myConfig->readConfigInt("NetStartCash"));
	spinBox_smallBlind->setValue(myConfig->readConfigInt("NetSmallBlind"));
	spinBox_handsBeforeRaiseSmallBlind->setValue(myConfig->readConfigInt("NetHandsBeforeRaiseSmallBlind"));
	//temporarely unused until ai is enabled in network
	spinBox_gameSpeed->setValue(myConfig->readConfigInt("NetGameSpeed"));
	spinBox_netTimeOutPlayerAction->setValue(myConfig->readConfigInt("NetTimeOutPlayerAction"));
}

void createInternetGameDialogImpl::showDialog() { 
	
	fillFormular();
	exec();
}

void createInternetGameDialogImpl::keyPressEvent ( QKeyEvent * event ) {


	if (event->key() == 16777220) { pushButton_createGame->click(); } //ENTER 
	
}
