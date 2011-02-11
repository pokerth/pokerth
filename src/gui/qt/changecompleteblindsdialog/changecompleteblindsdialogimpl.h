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
#ifndef CHANGECOMPLETEBLINDSDIALOGIMPL_H
#define CHANGECOMPLETEBLINDSDIALOGIMPL_H

#include "ui_changecompleteblindsdialog.h"

#include <QtCore>
#include <QtGui>

class ConfigFile;

class changeCompleteBlindsDialogImpl: public QDialog, public Ui::changeCompleteBlindsDialog
{
	Q_OBJECT
public:
	changeCompleteBlindsDialogImpl(QWidget *parent = 0, ConfigFile *c = 0);

	void exec();


public slots:
	bool getSettingsCorrect() const	{
		return settingsCorrect;
	}

	void updateSpinBoxInputMinimum(int);
	void addBlindValueToList();
	void removeBlindFromList();
	void sortBlindsList();

private:

	ConfigFile* myConfig;

	bool settingsCorrect;
};

#endif
