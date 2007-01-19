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
	ConfigFile config;	

	bool ok=TRUE;

	//Player Nicks
	lineEdit_humanPlayerName->setText(config.readConfig("myname", "Human Player"));
	lineEdit_Opponent1Name->setText(config.readConfig("opponent1name", "Player 1"));
	lineEdit_Opponent2Name->setText(config.readConfig("opponent2name", "Player 2"));
	lineEdit_Opponent3Name->setText(config.readConfig("opponent3name", "Player 3"));
	lineEdit_Opponent4Name->setText(config.readConfig("opponent4name", "Player 4"));

	//Game Settings
	spinBox_quantityPlayers->setValue(config.readConfig("numberofplayers", "5").toInt(&ok,10));
	spinBox_startCash->setValue(config.readConfig("startcash", "2000").toInt(&ok,10));
	spinBox_smallBlind->setValue(config.readConfig("smallblind", "10").toInt(&ok,10));
	spinBox_gameSpeed->setValue(config.readConfig("gamespeed", "4").toInt(&ok,10));
	checkBox_showGameSettingsDialogOnNewGame->setChecked(config.readConfig("showgamesettingsdialogonnewgame", "1").toInt(&ok,10));
	

	connect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );

}


void settingsDialogImpl::accept() {

	//Daten speichern
	ConfigFile config;

	//Player Nicks
	config.writeConfig("myname", lineEdit_humanPlayerName->text());
	config.writeConfig("opponent1name", lineEdit_Opponent1Name->text());
	config.writeConfig("opponent2name", lineEdit_Opponent2Name->text());
	config.writeConfig("opponent3name", lineEdit_Opponent3Name->text());
	config.writeConfig("opponent4name", lineEdit_Opponent4Name->text());

	//Game Settings
	config.writeConfig("numberofplayers", QString::number(spinBox_quantityPlayers->value(),10));
	config.writeConfig("startcash", QString::number(spinBox_startCash->value(),10));
	config.writeConfig("smallblind", QString::number(spinBox_smallBlind->value(),10));
	config.writeConfig("gamespeed", QString::number(spinBox_gameSpeed->value(),10));
	config.writeConfig("showgamesettingsdialogonnewgame", QString::number(checkBox_showGameSettingsDialogOnNewGame->isChecked(),10));

}
