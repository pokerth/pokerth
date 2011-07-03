/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/
#include "changecontentdialogimpl.h"
// #include "session.h"
#include "configfile.h"

changeContentDialogImpl::changeContentDialogImpl(QWidget *parent, ConfigFile *config, DialogType t)
	: QDialog(parent), myConfig(config), myType(t)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	setupUi(this);

	switch (myType) {
	case CHANGE_HUMAN_PLAYER_NAME: {

		label_Message->setText(tr("You cannot join Internet-Game-Lobby with \"Human Player\" as nickname.\nPlease choose another one."));
		label_lineLabel->setText(tr("Nick name:"));
		lineEdit->setText(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()));
		lineEdit->setMaxLength(12);
		checkBox->hide();
		this->setGeometry(this->x(), this->y(), this->width(), this->height()-20 );
	}
	break;
	case CHANGE_NICK_ALREADY_IN_USE: {
		label_Message->setText(tr("Your player name is already used by another player.\nPlease choose a different name."));
		label_lineLabel->setText(tr("Nick name:"));
		lineEdit->setText(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()));
		lineEdit->setMaxLength(12);
	}
	break;
	case CHANGE_NICK_INVALID: {
		label_Message->setText(tr("The player name is too short, too long or invalid. Please choose another one."));
		label_lineLabel->setText(tr("Nick name:"));
		lineEdit->setText(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()));
		lineEdit->setMaxLength(12);
	}
	break;
	case CHANGE_INET_GAME_NAME_IN_USE: {
		label_Message->setText(tr("There is already a game with your chosen game name.\nPlease choose another one!"));
		label_lineLabel->setText(tr("Game name:"));
		lineEdit->setText(QString::fromUtf8(myConfig->readConfigString("InternetGameName").c_str()));
		lineEdit->setMaxLength(48);
	}
	break;
	case CHANGE_INET_BAD_GAME_NAME: {
		label_Message->setText(tr("There is a forbidden word in your chosen game name.\nPlease choose another one!"));
		label_lineLabel->setText(tr("Game name:"));
		lineEdit->setText(QString::fromUtf8(myConfig->readConfigString("InternetGameName").c_str()));
		lineEdit->setMaxLength(48);
	}
	break;
	}

	connect(this, SIGNAL(accepted ()), this, SLOT(saveContent()));

}

void changeContentDialogImpl::saveContent()
{

	switch (myType) {
	case CHANGE_HUMAN_PLAYER_NAME: {
		myConfig->writeConfigString("MyName", lineEdit->text().toUtf8().constData());
	}
	break;
	case CHANGE_NICK_ALREADY_IN_USE: {
		if(checkBox->isChecked())               {
			myConfig->writeConfigString("MyName", lineEdit->text().toUtf8().constData());
		}
	}
	break;
	case CHANGE_NICK_INVALID: {
		if(checkBox->isChecked()) {
			myConfig->writeConfigString("MyName", lineEdit->text().toUtf8().constData());
		}
	}
	break;
	case CHANGE_INET_BAD_GAME_NAME:
	case CHANGE_INET_GAME_NAME_IN_USE: {
		if(checkBox->isChecked()) {
			myConfig->writeConfigString("InternetGameName", lineEdit->text().toUtf8().constData());
		}
	}
	break;
	}

	//write buffer to disc
	myConfig->writeBuffer();
}

