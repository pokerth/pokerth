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
#include "manualblindsorderdialogimpl.h"
#include "configfile.h"
#include <iostream>


manualBlindsOrderDialogImpl::manualBlindsOrderDialogImpl(QWidget *parent, ConfigFile *c)
    : QDialog(parent), myConfig(c), settingsCorrect(TRUE)
{

	 setupUi(this);

	connect( pushButton_add, SIGNAL( clicked() ), this, SLOT( addBlindValueToList() ) );
	connect( pushButton_delete, SIGNAL( clicked() ), this, SLOT( removeBlindFromList() ) );
	
}

void manualBlindsOrderDialogImpl::exec() {


	QDialog::exec();
}


void manualBlindsOrderDialogImpl::addBlindValueToList() {

	listWidget_blinds->addItem(QString::number(spinBox_input->value(),10));
	sortBlindsList();
}

void manualBlindsOrderDialogImpl::removeBlindFromList() {

	listWidget_blinds->takeItem(listWidget_blinds->currentRow());
	sortBlindsList();
}

void manualBlindsOrderDialogImpl::sortBlindsList() {

	int i;
	QList<int> tempIntList;
	QStringList tempStringList;
	bool ok = TRUE;
		
	for(i=0; i<listWidget_blinds->count(); i++) {
// 		std::cout << listWidget_blinds->item(i)->text().toInt(&ok,10) << "\n";
		tempIntList << listWidget_blinds->item(i)->text().toInt(&ok,10);		
	}

	qStableSort(tempIntList.begin(), tempIntList.end());
// 
	for(i=0; i<tempIntList.count(); i++) {
// 		
// 		std::cout << tempIntList[i] << "\n";
		tempStringList << QString::number(tempIntList[i],10);
	}
// 	
	listWidget_blinds->clear();
	listWidget_blinds->addItems(tempStringList);
}
