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
#include "createinternetgamedialogimpl.h"
#include "session.h"
#include "configfile.h"
#include "gamedata.h"
#include "changecompleteblindsdialogimpl.h"


createInternetGameDialogImpl::createInternetGameDialogImpl(QWidget *parent, ConfigFile *c)
	: QDialog(parent), myConfig(c), currentGuestMode(false), currentPlayerName("")
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
	spinBox_netDelayBetweenHands->installEventFilter(this);
	spinBox_netTimeOutPlayerAction->installEventFilter(this);
	comboBox_gameType->setItemData(0, GAME_TYPE_NORMAL, Qt::UserRole);
	comboBox_gameType->setItemData(1, GAME_TYPE_REGISTERED_ONLY, Qt::UserRole);
	comboBox_gameType->setItemData(2, GAME_TYPE_INVITE_ONLY, Qt::UserRole);
	comboBox_gameType->setItemData(3, GAME_TYPE_RANKING, Qt::UserRole);
	myChangeCompleteBlindsDialog = new changeCompleteBlindsDialogImpl;
	startBlind = new QLabel(tr("<i>First small blind: $%1</i>").arg(RANKING_GAME_START_SBLIND));
	raiseMode = new QLabel(tr("<i>Double blinds every %1'th hand</i>").arg(RANKING_GAME_RAISE_EVERY_HAND));
	startBlind->hide();
	raiseMode->hide();
	gridLayout1->addWidget(startBlind, 0, 0, 1, 1);
	gridLayout1->addWidget(raiseMode, 1, 0, 1, 1);

	connect( radioButton_changeBlindsSettings, SIGNAL( clicked(bool) ), this, SLOT( callChangeBlindsDialog(bool) ) );
#ifndef GUI_800x480
	connect( pushButton_cancel, SIGNAL( clicked() ), this, SLOT( cancel() ) );
#endif
	connect( pushButton_createGame, SIGNAL( clicked() ), this, SLOT( createGame() ) );
	connect( checkBox_Password, SIGNAL( toggled(bool) ), this, SLOT( clearGamePassword(bool)) );
	connect( comboBox_gameType, SIGNAL(currentIndexChanged(int)), this, SLOT( gameTypeChanged() ) );
}


void createInternetGameDialogImpl::exec(bool guestMode, QString playerName)
{

	currentGuestMode = guestMode;
	currentPlayerName = playerName;
	fillFormular(guestMode, playerName);
	QDialog::exec();
}

void createInternetGameDialogImpl::createGame()
{

}

void createInternetGameDialogImpl::cancel()
{

}

void createInternetGameDialogImpl::fillFormular(bool guestMode, QString playerName)
{

	//Network Game Settings
	spinBox_netDelayBetweenHands->setValue(myConfig->readConfigInt("NetDelayBetweenHands"));
	spinBox_netTimeOutPlayerAction->setValue(myConfig->readConfigInt("NetTimeOutPlayerAction"));
	checkBox_Password->setChecked(myConfig->readConfigInt("UseInternetGamePassword"));
	if(myConfig->readConfigInt("UseInternetGamePassword")) {
		lineEdit_Password->setText(QString::fromUtf8(myConfig->readConfigString("InternetGamePassword").c_str()));
	}


	if(guestMode) {
		comboBox_gameType->setCurrentIndex(0);
		comboBox_gameType->setDisabled(true);
		lineEdit_gameName->setText(tr("%1's game").arg(playerName));
		lineEdit_gameName->setDisabled(true);

	} else {
		comboBox_gameType->setDisabled(false);
		comboBox_gameType->setCurrentIndex(myConfig->readConfigInt("InternetGameType"));
		lineEdit_gameName->setDisabled(false);
		lineEdit_gameName->setText(QString::fromUtf8(myConfig->readConfigString("InternetGameName").c_str()));
	}

	gameTypeChanged();
}

void createInternetGameDialogImpl::keyPressEvent ( QKeyEvent * event )
{
#ifndef ANDROID
	if (event->key() == 16777220) {
		pushButton_createGame->click();    //ENTER
	}
#endif
}

void createInternetGameDialogImpl::clearGamePassword(bool clear)
{

	if(!clear) {
		lineEdit_Password->clear();
	}
}

void createInternetGameDialogImpl::callChangeBlindsDialog(bool show)
{

	if(show) {
		myChangeCompleteBlindsDialog->exec();
		if(myChangeCompleteBlindsDialog->result() == QDialog::Accepted ) {}
		else {
			radioButton_useSavedBlindsSettings->setChecked(true);
		}

	}
}

void createInternetGameDialogImpl::gameTypeChanged()
{

	switch (comboBox_gameType->currentIndex()) {

	case GAME_TYPE_NORMAL-1: {
		checkBox_Password->setDisabled(false);
		spinBox_startCash->setDisabled(false);
		spinBox_quantityPlayers->setDisabled(false);
		spinBox_quantityPlayers->setValue(myConfig->readConfigInt("NetNumberOfPlayers"));
		spinBox_startCash->setValue(myConfig->readConfigInt("NetStartCash"));
		radioButton_useSavedBlindsSettings->show();
		radioButton_changeBlindsSettings->show();
		startBlind->hide();
		raiseMode->hide();
		checkBox_allowSpectators->setEnabled(true);
		checkBox_allowSpectators->setChecked(myConfig->readConfigInt("InternetGameAllowSpectators"));
	}

	break;
	case GAME_TYPE_REGISTERED_ONLY-1: {
		checkBox_Password->setDisabled(false);
		spinBox_startCash->setDisabled(false);
		spinBox_quantityPlayers->setDisabled(false);
		spinBox_quantityPlayers->setValue(myConfig->readConfigInt("NetNumberOfPlayers"));
		spinBox_startCash->setValue(myConfig->readConfigInt("NetStartCash"));
		radioButton_useSavedBlindsSettings->show();
		radioButton_changeBlindsSettings->show();
		startBlind->hide();
		raiseMode->hide();
		checkBox_allowSpectators->setEnabled(true);
		checkBox_allowSpectators->setChecked(myConfig->readConfigInt("InternetGameAllowSpectators"));
	}
	break;
	case GAME_TYPE_INVITE_ONLY-1: {
		checkBox_Password->setChecked(false);
		checkBox_Password->setDisabled(true);
		spinBox_startCash->setDisabled(false);
		spinBox_quantityPlayers->setDisabled(false);
		spinBox_quantityPlayers->setValue(myConfig->readConfigInt("NetNumberOfPlayers"));
		spinBox_startCash->setValue(myConfig->readConfigInt("NetStartCash"));
		radioButton_useSavedBlindsSettings->show();
		radioButton_changeBlindsSettings->show();
		startBlind->hide();
		raiseMode->hide();
		checkBox_allowSpectators->setEnabled(true);
		checkBox_allowSpectators->setChecked(myConfig->readConfigInt("InternetGameAllowSpectators"));
	}
	break;
	case GAME_TYPE_RANKING-1: {
		checkBox_Password->setChecked(false);
		checkBox_Password->setDisabled(true);

		spinBox_startCash->setValue(RANKING_GAME_START_CASH);
		spinBox_startCash->setDisabled(true);
		spinBox_quantityPlayers->setValue(RANKING_GAME_NUMBER_OF_PLAYERS);
		spinBox_quantityPlayers->setDisabled(true);
		radioButton_useSavedBlindsSettings->hide();
		radioButton_changeBlindsSettings->hide();
		startBlind->show();
		raiseMode->show();
		checkBox_allowSpectators->setDisabled(true);
		checkBox_allowSpectators->setChecked(true);
	}
	break;
	}

	if(comboBox_gameType->currentIndex() == GAME_TYPE_RANKING-1) {
		//set static values
		myChangeCompleteBlindsDialog->spinBox_firstSmallBlind->setValue(RANKING_GAME_START_SBLIND);
		myChangeCompleteBlindsDialog->radioButton_raiseBlindsAtHands->setChecked(true);
		myChangeCompleteBlindsDialog->radioButton_raiseBlindsAtMinutes->setChecked(false);
		myChangeCompleteBlindsDialog->spinBox_raiseSmallBlindEveryHands->setValue(11);
		myChangeCompleteBlindsDialog->radioButton_alwaysDoubleBlinds->setChecked(true);
		myChangeCompleteBlindsDialog->radioButton_manualBlindsOrder->setChecked(false);
		myChangeCompleteBlindsDialog->listWidget_blinds->clear();
	} else {
		//read config values
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
}

bool createInternetGameDialogImpl::eventFilter(QObject *obj, QEvent *event)
{
#ifdef ANDROID
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

	//androi changes for return key behavior (hopefully useless from necessitas beta2)
	if (event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Return) {

		if(spinBox_startCash->hasFocus()) {
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
		if(lineEdit_gameName->hasFocus()) {
			lineEdit_gameName->clearFocus();
		}
		if(lineEdit_Password->hasFocus()) {
			lineEdit_Password->clearFocus();
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
