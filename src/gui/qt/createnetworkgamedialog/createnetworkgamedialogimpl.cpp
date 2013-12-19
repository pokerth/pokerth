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
#include "createnetworkgamedialogimpl.h"
#include "session.h"
#include "configfile.h"
#include "changecompleteblindsdialogimpl.h"

createNetworkGameDialogImpl::createNetworkGameDialogImpl(QWidget *parent, ConfigFile *c)
	: QDialog(parent), myConfig(c)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	setupUi(this);
	this->installEventFilter(this);
#ifdef ANDROID
	this->setWindowState(Qt::WindowFullScreen);
#endif
	myChangeCompleteBlindsDialog = new changeCompleteBlindsDialogImpl;
	fillFormular();

	connect( radioButton_changeBlindsSettings, SIGNAL( clicked(bool) ), this, SLOT( callChangeBlindsDialog(bool) ) );
#ifndef GUI_800x480
	connect( pushButton_cancel, SIGNAL( clicked() ), this, SLOT( cancel() ) );
#endif
	connect( pushButton_createGame, SIGNAL( clicked() ), this, SLOT( createGame() ) );
}


int createNetworkGameDialogImpl::exec()
{
	fillFormular();
	return QDialog::exec();
}

void createNetworkGameDialogImpl::createGame()
{

}

void createNetworkGameDialogImpl::cancel()
{

}

void createNetworkGameDialogImpl::fillFormular()
{

	//Network Game Settings
	spinBox_quantityPlayers->setValue(myConfig->readConfigInt("NetNumberOfPlayers"));
	spinBox_startCash->setValue(myConfig->readConfigInt("NetStartCash"));
	spinBox_netDelayBetweenHands->setValue(myConfig->readConfigInt("NetDelayBetweenHands"));
	spinBox_netTimeOutPlayerAction->setValue(myConfig->readConfigInt("NetTimeOutPlayerAction"));

	//fill changeCompleteBlindsDialog
	myChangeCompleteBlindsDialog->spinBox_firstSmallBlind->setValue(myConfig->readConfigInt("NetFirstSmallBlind"));
	myChangeCompleteBlindsDialog->radioButton_raiseBlindsAtHands->setChecked(myConfig->readConfigInt("NetRaiseBlindsAtHands"));
	myChangeCompleteBlindsDialog->radioButton_raiseBlindsAtMinutes->setChecked(myConfig->readConfigInt("NetRaiseBlindsAtMinutes"));
	myChangeCompleteBlindsDialog->spinBox_raiseSmallBlindEveryHands->setValue(myConfig->readConfigInt("NetRaiseSmallBlindEveryHands"));
	myChangeCompleteBlindsDialog->spinBox_raiseSmallBlindEveryMinutes->setValue(myConfig->readConfigInt("NetRaiseSmallBlindEveryMinutes"));
	myChangeCompleteBlindsDialog->radioButton_alwaysDoubleBlinds->setChecked(myConfig->readConfigInt("NetAlwaysDoubleBlinds"));
	myChangeCompleteBlindsDialog->radioButton_manualBlindsOrder->setChecked(myConfig->readConfigInt("NetManualBlindsOrder"));

	myChangeCompleteBlindsDialog->listWidget_blinds->clear();
	myChangeCompleteBlindsDialog->spinBox_input->setMinimum(myChangeCompleteBlindsDialog->spinBox_firstSmallBlind->value()+1);

	std::list<int> myBlindsList = myConfig->readConfigIntList("NetManualBlindsList");
	std::list<int>::iterator it1;

	for(it1= myBlindsList.begin(); it1 != myBlindsList.end(); ++it1) {
		myChangeCompleteBlindsDialog->listWidget_blinds->addItem(QString::number(*it1,10));
	}
	myChangeCompleteBlindsDialog->sortBlindsList();

	myChangeCompleteBlindsDialog->radioButton_afterThisAlwaysDoubleBlinds->setChecked(myConfig->readConfigInt("NetAfterMBAlwaysDoubleBlinds"));
	myChangeCompleteBlindsDialog->radioButton_afterThisAlwaysRaiseAbout->setChecked(myConfig->readConfigInt("NetAfterMBAlwaysRaiseAbout"));
	myChangeCompleteBlindsDialog->spinBox_afterThisAlwaysRaiseValue->setValue(myConfig->readConfigInt("NetAfterMBAlwaysRaiseValue"));
	myChangeCompleteBlindsDialog->radioButton_afterThisStayAtLastBlind->setChecked(myConfig->readConfigInt("NetAfterMBStayAtLastBlind"));
}

void createNetworkGameDialogImpl::showDialog()
{

	fillFormular();
	exec();
}

void createNetworkGameDialogImpl::keyPressEvent ( QKeyEvent * event )
{
#ifndef ANDROID
	if (event->key() == 16777220) {
		pushButton_createGame->click();    //ENTER
	}
#endif

}

void createNetworkGameDialogImpl::callChangeBlindsDialog(bool show)
{

	if(show) {
		myChangeCompleteBlindsDialog->exec();
		if(myChangeCompleteBlindsDialog->result() == QDialog::Accepted ) {}
		else {
			radioButton_useSavedBlindsSettings->setChecked(true);
		}

	}
}

bool createNetworkGameDialogImpl::eventFilter(QObject *obj, QEvent *event)
{
#ifdef ANDROID
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

	//androi changes for return key behavior (hopefully useless from necessitas beta2)
	if (event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Return) {
		if(spinBox_startCash ->hasFocus()) {
			spinBox_startCash->clearFocus();
		}
		if(spinBox_quantityPlayers->hasFocus()) {
			spinBox_quantityPlayers->clearFocus();
		}
		if(spinBox_netTimeOutPlayerAction->hasFocus()) {
			spinBox_netTimeOutPlayerAction->clearFocus();
		}
		if(spinBox_netDelayBetweenHands->hasFocus()) {
			spinBox_netDelayBetweenHands->clearFocus();
		}
		event->ignore();
		return false;
	} else if (event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Back) {
		event->ignore();
		this->reject();
		return false;
	} else {
		// pass the event on to the parent class
		return QDialog::eventFilter(obj, event);
	}
#else
	return QDialog::eventFilter(obj, event);
#endif
}
