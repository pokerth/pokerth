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
#ifndef INTERNETGAMELOGINDIALOGIMPL_H
#define INTERNETGAMELOGINDIALOGIMPL_H

#include "ui_internetgamelogindialog.h"

class ConfigFile;

class internetGameLoginDialogImpl : public QDialog, public Ui::internetGameLoginDialog
{
	Q_OBJECT

public:
	internetGameLoginDialogImpl(QWidget *parent = 0, ConfigFile *c =0);

	void exec();
	void accept();

public slots:
	void regUserToggled(bool);
	void guestUserToggled(bool);
	void okButtonCheck();

private:

	ConfigFile *myConfig;
};

#endif // INTERNETGAMELOGINDIALOGIMPL_H
