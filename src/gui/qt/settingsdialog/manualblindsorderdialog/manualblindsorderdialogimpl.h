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
#ifndef MANUALBLINDSORDERDIALOGIMPL_H
#define MANUALBLINDSORDERDIALOGIMPL_H

#include "ui_manualblindsorderdialog.h"

#include <QtCore>
#include <QtGui>

class ConfigFile;

class manualBlindsOrderDialogImpl: public QDialog, public Ui::manualBlindsOrderDialog
{
	Q_OBJECT
public:
	manualBlindsOrderDialogImpl(QWidget *parent = 0, ConfigFile *c = 0);

	void exec();


public slots:
	bool getSettingsCorrect() const	{
		return settingsCorrect;
	}

	void addBlindValueToList();
	void removeBlindFromList();
	void sortBlindsList();

private:

	ConfigFile* myConfig;

	bool settingsCorrect;
};

#endif
