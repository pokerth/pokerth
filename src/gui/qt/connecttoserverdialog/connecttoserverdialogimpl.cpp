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
#include "connecttoserverdialogimpl.h"
// #include "configfile.h"

connectToServerDialogImpl::connectToServerDialogImpl(QWidget *parent)
      : QDialog(parent)
{

    setupUi(this);

	

}

void connectToServerDialogImpl::refresh(int actionID) {

	int maxStateNumber = 4;

	switch (actionID) {

	case 0: { label_actionMessage->setText("msg0"); }
	break;
	case 1: { label_actionMessage->setText("msg1"); }
	break;
	case 2: { label_actionMessage->setText("msg2"); }
	break;
	case 3: { label_actionMessage->setText("msg3"); }
	break;
	case 4: { label_actionMessage->setText("msg4"); }
	break;
	default:  { label_actionMessage->setText("ERROR"); }
	}

	progressBar->setValue(actionID*(100/maxStateNumber));

	if (actionID*(100/maxStateNumber == 100)) { QTimer::singleShot(500, this, SLOT(hide())); }
}

void connectToServerDialogImpl::error(int errorID, int osErrorID) {

	if(osErrorID) {
		QMessageBox::warning(this, tr("Connection Error"),
			tr("An Operating System Error occured during Connection"),
			QMessageBox::Close);
	}

	switch (errorID) {

		case 0: { QMessageBox::warning(this, tr("Connection Error"),
				tr("Error0 occured during Connection"),
				QMessageBox::Close); }
		break;
		case 1: {QMessageBox::warning(this, tr("Connection Error"),
				tr("Error1 occured during Connection"),
				QMessageBox::Close); }
		break;
		case 2: { QMessageBox::warning(this, tr("Connection Error"),
				tr("Error2 occured during Connection"),
				QMessageBox::Close); }
		break;
		case 3: { QMessageBox::warning(this, tr("Connection Error"),
				tr("Error3 occured during Connection"),
				QMessageBox::Close); }
		break;
		case 4: { QMessageBox::warning(this, tr("Connection Error"),
				tr("Error4 occured during Connection"),
				QMessageBox::Close); }
		break;
		default:  { QMessageBox::warning(this, tr("Connection Error"),
				tr("DEFAULT ERROR"),
				QMessageBox::Close); }
	}

	this->reject();

}
