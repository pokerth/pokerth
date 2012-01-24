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
#ifndef CONNECTTOSERVERDIALOGIMPL_H
#define CONNECTTOSERVERDIALOGIMPL_H

#ifdef GUI_800x480
#include "ui_connecttoserverdialog_800x480.h"
#else
#include "ui_connecttoserverdialog.h"
#endif

#include <QtGui>
#include <QtCore>


class connectToServerDialogImpl: public QDialog, public Ui::connectToServerDialog
{
	Q_OBJECT
public:
	connectToServerDialogImpl(QWidget *parent = 0);

	void exec();

public slots:

	void refresh(int actionID);
};

#endif
