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
#include "changecompleteblindsdialogimpl.h"


createInternetGameDialogImpl::createInternetGameDialogImpl(QWidget *parent, ConfigFile *c)
      : QDialog(parent), myConfig(c)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif	
    	setupUi(this);

	myChangeCompleteBlindsDialog = new changeCompleteBlindsDialogImpl;

	connect( radioButton_changeBlindsSettings, SIGNAL( clicked(bool) ), this, SLOT( callChangeBlindsDialog(bool) ) );
	connect( pushButton_cancel, SIGNAL( clicked() ), this, SLOT( cancel() ) );
	connect( pushButton_createGame, SIGNAL( clicked() ), this, SLOT( createGame() ) );
	connect( checkBox_Password, SIGNAL( toggled(bool) ), this, SLOT( clearGamePassword(bool)) ); 
        connect( comboBox_gameType, SIGNAL(currentIndexChanged(int)), this, SLOT( gameTypeChanged() ) );
        
	//temporarely unused until ai is enabled in network
// 	label_5->hide();
// 	spinBox_gameSpeed->hide();


}


void createInternetGameDialogImpl::exec(bool guestMode, QString playerName) {
	
        fillFormular(guestMode, playerName);
	QDialog::exec();
}

void createInternetGameDialogImpl::createGame() {
	
}

void createInternetGameDialogImpl::cancel() {
	
}

void createInternetGameDialogImpl::fillFormular(bool guestMode, QString playerName) {
	
	//Network Game Settings
	spinBox_quantityPlayers->setValue(myConfig->readConfigInt("NetNumberOfPlayers"));
	spinBox_startCash->setValue(myConfig->readConfigInt("NetStartCash"));
	spinBox_netDelayBetweenHands->setValue(myConfig->readConfigInt("NetDelayBetweenHands"));
	spinBox_netTimeOutPlayerAction->setValue(myConfig->readConfigInt("NetTimeOutPlayerAction"));
	checkBox_Password->setChecked(myConfig->readConfigInt("UseInternetGamePassword"));
	if(myConfig->readConfigInt("UseInternetGamePassword")) {
		lineEdit_Password->setText(QString::fromUtf8(myConfig->readConfigString("InternetGamePassword").c_str()));
	}

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
	
	for(it1= myBlindsList.begin(); it1 != myBlindsList.end(); it1++) {
		myChangeCompleteBlindsDialog->listWidget_blinds->addItem(QString::number(*it1,10));
	}
	myChangeCompleteBlindsDialog->sortBlindsList();
	
	myChangeCompleteBlindsDialog->radioButton_afterThisAlwaysDoubleBlinds->setChecked(myConfig->readConfigInt("NetAfterMBAlwaysDoubleBlinds"));
	myChangeCompleteBlindsDialog->radioButton_afterThisAlwaysRaiseAbout->setChecked(myConfig->readConfigInt("NetAfterMBAlwaysRaiseAbout"));
	myChangeCompleteBlindsDialog->spinBox_afterThisAlwaysRaiseValue->setValue(myConfig->readConfigInt("NetAfterMBAlwaysRaiseValue"));
	myChangeCompleteBlindsDialog->radioButton_afterThisStayAtLastBlind->setChecked(myConfig->readConfigInt("NetAfterMBStayAtLastBlind"));

        if(guestMode) {
            comboBox_gameType->setCurrentIndex(0);
            comboBox_gameType->setDisabled(true);
            lineEdit_gameName->setText(tr("%1's game").arg(playerName));
            lineEdit_gameName->setDisabled(true);

        }
        else {
            comboBox_gameType->setDisabled(false);
            comboBox_gameType->setCurrentIndex(myConfig->readConfigInt("InternetGameType"));
            lineEdit_gameName->setDisabled(false);
            lineEdit_gameName->setText(QString::fromUtf8(myConfig->readConfigString("InternetGameName").c_str()));
        }

        gameTypeChanged();
}

void createInternetGameDialogImpl::keyPressEvent ( QKeyEvent * event ) {


	if (event->key() == 16777220) { pushButton_createGame->click(); } //ENTER 
	
}

void createInternetGameDialogImpl::clearGamePassword(bool clear) {

	if(!clear) { lineEdit_Password->clear(); }
}

void createInternetGameDialogImpl::callChangeBlindsDialog(bool show) {

	if(show) {
		myChangeCompleteBlindsDialog->exec();
		if(myChangeCompleteBlindsDialog->result() == QDialog::Accepted ) {}
		else {
			radioButton_useSavedBlindsSettings->setChecked(TRUE);
		}

	}
}

void createInternetGameDialogImpl::gameTypeChanged() {

    switch (comboBox_gameType->currentIndex()) {

    case 0: { checkBox_Password->setDisabled(FALSE); }
    break;
    case 1: { checkBox_Password->setDisabled(FALSE); }
    break;
    case 2: { checkBox_Password->setChecked(FALSE);
              checkBox_Password->setDisabled(TRUE);
            }
    break;
    case 3: { checkBox_Password->setChecked(FALSE);
              checkBox_Password->setDisabled(TRUE);
            }
    break;
    }

}
