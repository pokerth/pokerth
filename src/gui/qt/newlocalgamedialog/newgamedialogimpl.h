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
#ifndef NEWGAMEDIALOGIMPL_H
#define NEWGAMEDIALOGIMPL_H

#include "ui_newgamedialog.h"

#include <QtGui>
#include <QtCore>


class ConfigFile;
class changeCompleteBlindsDialogImpl;

class newGameDialogImpl: public QDialog, public Ui::newGameDialog
{
	Q_OBJECT
public:
	newGameDialogImpl(QMainWindow *parent = 0, ConfigFile* = 0);

	void exec();
	changeCompleteBlindsDialogImpl* getChangeCompleteBlindsDialog() {
		return myChangeCompleteBlindsDialog;
	}

public slots:

	void callChangeBlindsDialog(bool);

private:

	ConfigFile *myConfig;
	changeCompleteBlindsDialogImpl *myChangeCompleteBlindsDialog;

};

#endif
