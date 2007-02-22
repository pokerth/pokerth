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
#include "settingsdialogimpl.h"
#include "configfile.h"
#include <iostream>


settingsDialogImpl::settingsDialogImpl(QWidget *parent)
    : QDialog(parent)
{

	 setupUi(this);

	playerNickIsChanged = FALSE;

	//Formulare Füllen
	ConfigFile myConfig;	

	//Player Nicks
	lineEdit_humanPlayerName->setText(QString::fromStdString(myConfig.readConfigString("MyName")));
	lineEdit_Opponent1Name->setText(QString::fromStdString(myConfig.readConfigString("Opponent1Name")));
	lineEdit_Opponent2Name->setText(QString::fromStdString(myConfig.readConfigString("Opponent2Name")));
	lineEdit_Opponent3Name->setText(QString::fromStdString(myConfig.readConfigString("Opponent3Name")));
	lineEdit_Opponent4Name->setText(QString::fromStdString(myConfig.readConfigString("Opponent4Name")));

	//Local Game Settings
	spinBox_quantityPlayers->setValue(myConfig.readConfigInt("NumberOfPlayers"));
	spinBox_startCash->setValue(myConfig.readConfigInt("StartCash"));
	spinBox_smallBlind->setValue(myConfig.readConfigInt("SmallBlind"));
	spinBox_handsBeforeRaiseSmallBlind->setValue(myConfig.readConfigInt("HandsBeforeRaiseSmallBlind"));
	spinBox_gameSpeed->setValue(myConfig.readConfigInt("GameSpeed"));
	checkBox_pauseBetweenHands->setChecked(myConfig.readConfigInt("PauseBetweenHands"));
	checkBox_showGameSettingsDialogOnNewGame->setChecked(myConfig.readConfigInt("ShowGameSettingsDialogOnNewGame"));
	
	//Network Game Settings
	spinBox_netQuantityPlayers->setValue(myConfig.readConfigInt("NetNumberOfPlayers"));
	spinBox_netStartCash->setValue(myConfig.readConfigInt("NetStartCash"));
	spinBox_netSmallBlind->setValue(myConfig.readConfigInt("NetSmallBlind"));
	spinBox_netHandsBeforeRaiseSmallBlind->setValue(myConfig.readConfigInt("NetHandsBeforeRaiseSmallBlind"));
	spinBox_netGameSpeed->setValue(myConfig.readConfigInt("NetGameSpeed"));
	lineEdit_serverPassword->setText(QString::fromStdString(myConfig.readConfigString("ServerPassword")));

	//Interface
	checkBox_showLeftToolbox->setChecked(myConfig.readConfigInt("ShowLeftToolBox"));
	checkBox_showRightToolbox->setChecked(myConfig.readConfigInt("ShowRightToolBox"));
	checkBox_showIntro->setChecked(myConfig.readConfigInt("ShowIntro"));
	checkBox_showFadeOutCardsAnimation->setChecked(myConfig.readConfigInt("ShowFadeOutCardsAnimation"));
	checkBox_showFlipCardsAnimation->setChecked(myConfig.readConfigInt("ShowFlipCardsAnimation"));
	radioButton_flipsideTux->setChecked(myConfig.readConfigInt("FlipsideTux"));
	radioButton_flipsideOwn->setChecked(myConfig.readConfigInt("FlipsideOwn"));
	if(radioButton_flipsideOwn->isChecked()) { 
		lineEdit_OwnFlipsideFilename->setEnabled(TRUE);
		pushButton_openFlipsidePicture->setEnabled(TRUE);
	}
	lineEdit_OwnFlipsideFilename->setText(QString::fromStdString(myConfig.readConfigString("FlipsideOwnFile")));

	//Log 
	lineEdit_logDir->setText(QString::fromStdString(myConfig.readConfigString("LogDir")));
	spinBox_logStoreDuration->setValue(myConfig.readConfigInt("LogStoreDuration"));
	
	connect( buttonBox, SIGNAL( accepted() ), this, SLOT( isAccepted() ) );
	connect( lineEdit_humanPlayerName, SIGNAL( textChanged( const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent1Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent2Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent3Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent4Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( pushButton_openFlipsidePicture, SIGNAL( clicked() ), this, SLOT( setFlipsidePicFileName()) );
	connect( pushButton_openLogDir, SIGNAL( clicked() ), this, SLOT( setLogDir()) );
	

}


void settingsDialogImpl::isAccepted() {

	settingsCorrect = TRUE;

	//Daten speichern
	ConfigFile myConfig;

// 	Player Nicks
	myConfig.writeConfigString("MyName", lineEdit_humanPlayerName->text().toStdString());
	myConfig.writeConfigString("Opponent1Name", lineEdit_Opponent1Name->text().toStdString());
	myConfig.writeConfigString("Opponent2Name", lineEdit_Opponent2Name->text().toStdString());
	myConfig.writeConfigString("Opponent3Name", lineEdit_Opponent3Name->text().toStdString());
	myConfig.writeConfigString("Opponent4Name", lineEdit_Opponent4Name->text().toStdString());

// 	Local Game Settings
	myConfig.writeConfigInt("NumberOfPlayers", spinBox_quantityPlayers->value());
	myConfig.writeConfigInt("StartCash", spinBox_startCash->value());
	myConfig.writeConfigInt("SmallBlind", spinBox_smallBlind->value());
	myConfig.writeConfigInt("HandsBeforeRaiseSmallBlind", spinBox_handsBeforeRaiseSmallBlind->value());
	myConfig.writeConfigInt("GameSpeed", spinBox_gameSpeed->value());
	myConfig.writeConfigInt("PauseBetweenHands", checkBox_pauseBetweenHands->isChecked());
	myConfig.writeConfigInt("ShowGameSettingsDialogOnNewGame", checkBox_showGameSettingsDialogOnNewGame->isChecked());

	//Network Game Settings
	myConfig.writeConfigInt("NetNumberOfPlayers", spinBox_netQuantityPlayers->value());
	myConfig.writeConfigInt("NetStartCash", spinBox_netStartCash->value());
	myConfig.writeConfigInt("NetSmallBlind", spinBox_netSmallBlind->value());
	myConfig.writeConfigInt("NetHandsBeforeRaiseSmallBlind", spinBox_netHandsBeforeRaiseSmallBlind->value());
	myConfig.writeConfigInt("NetGameSpeed", spinBox_netGameSpeed->value());
	myConfig.writeConfigString("ServerPassword", lineEdit_serverPassword->text().toStdString());
	
// 	Interface
	myConfig.writeConfigInt("ShowLeftToolBox", checkBox_showLeftToolbox->isChecked());
	myConfig.writeConfigInt("ShowRightToolBox", checkBox_showRightToolbox->isChecked());
	myConfig.writeConfigInt("ShowIntro", checkBox_showIntro->isChecked());
	myConfig.writeConfigInt("ShowFadeOutCardsAnimation", checkBox_showFadeOutCardsAnimation->isChecked());
	myConfig.writeConfigInt("ShowFlipCardsAnimation", checkBox_showFlipCardsAnimation->isChecked());
	myConfig.writeConfigInt("FlipsideTux", radioButton_flipsideTux->isChecked());
	myConfig.writeConfigInt("FlipsideOwn", radioButton_flipsideOwn->isChecked());

	if(radioButton_flipsideOwn->isChecked()) {
		if(QFile::QFile(lineEdit_OwnFlipsideFilename->text()).exists() && lineEdit_OwnFlipsideFilename->text() != "") {myConfig.writeConfigString("FlipsideOwnFile", lineEdit_OwnFlipsideFilename->text().toStdString()); }
		else {	QMessageBox::warning(this, tr("Settings Error"),
			tr("The entered Flipside Picture doesn't exists.\n"
			"Please enter an valid Picture!"),
			QMessageBox::Ok);
			settingsCorrect = FALSE; 
		}
	}

//	Log
	if(QDir::QDir(lineEdit_logDir->text()).exists() && lineEdit_logDir->text() != "") { myConfig.writeConfigString("LogDir", lineEdit_logDir->text().toStdString());	}
	else { 
		QMessageBox::warning(this, tr("Settings Error"),
                   tr("The Log File Directory doesn't exists.\n"
                      "Please select an valid Directory!"),
                   QMessageBox::Ok);
		settingsCorrect = FALSE; 
	}

	myConfig.writeConfigInt("LogStoreDuration", spinBox_logStoreDuration->value());


	//Wenn alles richtig eingegeben wurde --> Dialog schließen
	if(settingsCorrect) { this->hide(); }
}

void settingsDialogImpl::setFlipsidePicFileName()
 {
 	QString fileName = QFileDialog::getOpenFileName(this, tr("Select your Flipside Picture"),
                                                QDir::homePath(),
                                                tr("Images (*.bmp *.png *.xpm)"));

     if (!fileName.isEmpty())
         lineEdit_OwnFlipsideFilename->setText(fileName);
 }

void settingsDialogImpl::setLogDir()
 {
	 QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 QDir::homePath(),
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
		
     	if (!dir.isEmpty()) lineEdit_logDir->setText(dir);
 }
