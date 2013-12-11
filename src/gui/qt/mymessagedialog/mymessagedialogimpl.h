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
#ifndef MYMESSAGEDIALOGIMPL_H
#define MYMESSAGEDIALOGIMPL_H

#ifdef GUI_800x480
#include "ui_mymessagedialog_800x480.h"
#else
#include "ui_mymessagedialog.h"
#endif

enum MESSAGE_CONTENT {
	NONE=0,                           //0: not set ;)
	BACKTO_LOBBY_QUESTION,          //1: leave lobby during online game: Question(Do you really wanna leave?)
	INFO_AFTER_JOIN_INVITE_GAME,    //2: join invite only game: Info(You can invite people with right click ...)
	JOIN_INVITE_GAME_QUESTION,      //3: reciev invite to game: Question(You've been invited to the game <b>%1</b> by <b>%2</b>.<br>Do you want to join this game?)
	IGNORE_PLAYER_QUESTION,        //4: click ignore player: Question(Do you really want to put this player on ignore List?)
	GT_VALUES_MISSING,              //5: Selected game table style \"%1\" seems to be incomplete or defective. \n\nThe value(s) of: \n%2 \nis/are missing.
	CLOSE_GAMETABLE_QUESTION,       //6: close gametable: Do you really wanna quit?
	GT_PICS_MISSING,                //7: One or more pictures from current game table style \"%1\" were not found:
	GT_OUTDATED,                    //8: Selected game table style \"%1\" seems to be outdated. \nThe current PokerTH game table style version is
	CD_VALUES_MISSING,              //9:
	CD_PICS_MISSING,                //10:
	CD_OUTDATED,                    //11:
	UNIGNORE_PLAYER_QUESTION       //12: click unignore player: Question(Do you really want to remove this player from ignore List?)
};

class ConfigFile;

class myMessageDialogImpl: public QDialog, public Ui::myMessageDialog
{
	Q_OBJECT
public:

	myMessageDialogImpl(ConfigFile *, QWidget *parent = 0);

public slots:

	bool checkIfMesssageWillBeDisplayed(int id);
	int exec(int messageId, QString msg, QString title, QPixmap pix, QDialogButtonBox::StandardButtons buttons, bool showCheckBox = false);
	void show(int messageId, QString msg, QString title, QPixmap pix, QDialogButtonBox::StandardButtons buttons, bool showCheckBox = false);
	void accept();
	void reject();
	void writeConfig();

private:

	ConfigFile *myConfig;
	int currentMsgId;
	std::list<std::string> currentMsgShowList;

};

#endif

