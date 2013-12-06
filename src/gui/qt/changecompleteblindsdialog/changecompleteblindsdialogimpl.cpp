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
#include "changecompleteblindsdialogimpl.h"
#include "mymessagebox.h"
#include "configfile.h"
#include <iostream>


changeCompleteBlindsDialogImpl::changeCompleteBlindsDialogImpl(QWidget *parent, ConfigFile *c)
	: QDialog(parent), myConfig(c), settingsCorrect(true)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	setupUi(this);
	this->installEventFilter(this);
#ifdef ANDROID
	this->setWindowState(Qt::WindowFullScreen);
#endif

	connect( pushButton_add, SIGNAL( clicked() ), this, SLOT( addBlindValueToList() ) );
	connect( pushButton_delete, SIGNAL( clicked() ), this, SLOT( removeBlindFromList() ) );
	connect( spinBox_firstSmallBlind, SIGNAL( valueChanged(int) ), this, SLOT( updateSpinBoxInputMinimum(int) ) );
}

int changeCompleteBlindsDialogImpl::exec()
{
	return QDialog::exec();
}

void changeCompleteBlindsDialogImpl::updateSpinBoxInputMinimum(int value)
{
	spinBox_input->setMinimum(value+1);
}

void changeCompleteBlindsDialogImpl::addBlindValueToList()
{

	if(listWidget_blinds->count() == 30) {
		MyMessageBox::warning(this, tr("Manual Blinds Order"),
							  tr("You cannot set more than 30 manual blinds."),
							  QMessageBox::Close);
	} else {
		listWidget_blinds->addItem(QString::number(spinBox_input->value(),10));
		sortBlindsList();
	}
}

void changeCompleteBlindsDialogImpl::removeBlindFromList()
{

	listWidget_blinds->takeItem(listWidget_blinds->currentRow());
	sortBlindsList();
}

void changeCompleteBlindsDialogImpl::sortBlindsList()
{

	int i;
	QList<int> tempIntList;
	QStringList tempStringList;
	bool ok = true;

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


bool changeCompleteBlindsDialogImpl::eventFilter(QObject *obj, QEvent *event)
{
#ifdef ANDROID
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

	//androi changes for return key behavior (hopefully useless from necessitas beta2)
	if (event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Return) {
		if(spinBox_afterThisAlwaysRaiseValue->hasFocus()) {
			spinBox_afterThisAlwaysRaiseValue->clearFocus();
		}
		if(spinBox_firstSmallBlind->hasFocus()) {
			spinBox_firstSmallBlind->clearFocus();
		}
		if(spinBox_input->hasFocus()) {
			spinBox_input->clearFocus();
		}
		if(spinBox_raiseSmallBlindEveryHands->hasFocus()) {
			spinBox_raiseSmallBlindEveryHands->clearFocus();
		}
		if(spinBox_raiseSmallBlindEveryMinutes->hasFocus()) {
			spinBox_raiseSmallBlindEveryMinutes->clearFocus();
		}
		event->ignore();
		return false;
	} else if (event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Back) {
		this->reject();
		return true;
	} else {
		// pass the event on to the parent class
		return QDialog::eventFilter(obj, event);
	}
#else
	return QDialog::eventFilter(obj, event);
#endif
}
