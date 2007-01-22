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


settingsDialogImpl::settingsDialogImpl(QWidget *parent, const char *name)
    : QDialog(parent, name)
{

	 setupUi(this);

	//Formulare Füllen
	ConfigFile myConfig;	

	//Player Nicks
	lineEdit_humanPlayerName->setText(QString::fromStdString(myConfig.readConfigString("MyName", "Human Player")));
	lineEdit_Opponent1Name->setText(QString::fromStdString(myConfig.readConfigString("Opponent1Name", "Player 1")));
	lineEdit_Opponent2Name->setText(QString::fromStdString(myConfig.readConfigString("Opponent2Name", "Player 2")));
	lineEdit_Opponent3Name->setText(QString::fromStdString(myConfig.readConfigString("Opponent3Name", "Player 3")));
	lineEdit_Opponent4Name->setText(QString::fromStdString(myConfig.readConfigString("Opponent4Name", "Player 4")));

	//Game Settings
	spinBox_quantityPlayers->setValue(myConfig.readConfigInt("NumberOfPlayers", 5));
	spinBox_startCash->setValue(myConfig.readConfigInt("StartCash", 2000));
	spinBox_smallBlind->setValue(myConfig.readConfigInt("SmallBlind", 10));
	spinBox_gameSpeed->setValue(myConfig.readConfigInt("GameSpeed", 4));
	checkBox_showGameSettingsDialogOnNewGame->setChecked(myConfig.readConfigInt("ShowGameSettingsDialogOnNewGame", 1));

	//Interface
	checkBox_showToolbox->setChecked(myConfig.readConfigInt("ShowToolBox", 1));
	
	connect( buttonBox, SIGNAL( accepted() ), this, SLOT( isAccepted() ) );

}


void settingsDialogImpl::isAccepted() {

	//Daten speichern
	ConfigFile myConfig;

// 	Player Nicks
	myConfig.writeConfigString("MyName", lineEdit_humanPlayerName->text().toStdString());
	myConfig.writeConfigString("Opponent1Name", lineEdit_Opponent1Name->text().toStdString());
	myConfig.writeConfigString("Opponent2Name", lineEdit_Opponent2Name->text().toStdString());
	myConfig.writeConfigString("Opponent3Name", lineEdit_Opponent3Name->text().toStdString());
	myConfig.writeConfigString("Opponent4Name", lineEdit_Opponent4Name->text().toStdString());

// 	Game Settings
	myConfig.writeConfigInt("NumberOfPlayers", spinBox_quantityPlayers->value());
	myConfig.writeConfigInt("StartCash", spinBox_startCash->value());
	myConfig.writeConfigInt("SmallBlind", spinBox_smallBlind->value());
	myConfig.writeConfigInt("GameSpeed", spinBox_gameSpeed->value());
	myConfig.writeConfigInt("ShowGameSettingsDialogOnNewGame", checkBox_showGameSettingsDialogOnNewGame->isChecked());

// 	Interface
	myConfig.writeConfigInt("ShowToolBox", checkBox_showToolbox->isChecked());

}
