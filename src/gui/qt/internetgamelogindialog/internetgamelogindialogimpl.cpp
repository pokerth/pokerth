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
#include "internetgamelogindialogimpl.h"
#include "configfile.h"
#include <tools.h>
#include <QtCore>
#include <QtGui>

internetGameLoginDialogImpl::internetGameLoginDialogImpl(QWidget *parent, ConfigFile *c) :
	QDialog(parent), myConfig(c)
{
	setupUi(this);
	this->installEventFilter(this);
#ifdef ANDROID
	this->setWindowState(Qt::WindowFullScreen);
#endif
	//html stuff
	QString createAccount(QString("<a href='http://create-gaming-account.pokerth.net'>%1</a>").arg(tr("Create new user account")));
	label_createAnAccount->setText(createAccount);


	connect(groupBox_reguser, SIGNAL(toggled(bool)), this, SLOT(regUserToggled(bool)));
	connect(checkBox_guest, SIGNAL(toggled(bool)), this, SLOT(guestUserToggled(bool)));
	connect(lineEdit_password, SIGNAL(textEdited(QString)), this, SLOT(okButtonCheck()));
	connect(lineEdit_username, SIGNAL(textEdited(QString)), this, SLOT(okButtonCheck()));
}

void internetGameLoginDialogImpl::regUserToggled(bool b)
{

	checkBox_guest->setChecked(!b);

	if(b) {
		if(QString::fromUtf8(myConfig->readConfigString("MyRememberedNameDuringGuestLogin").c_str()).isEmpty()) {
			lineEdit_username->setText(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()));
		} else {
			lineEdit_username->setText(QString::fromUtf8(myConfig->readConfigString("MyRememberedNameDuringGuestLogin").c_str()));
		}
		if(myConfig->readConfigInt("InternetSavePassword")) {
			checkBox_rememberPassword->setChecked(true);
			lineEdit_password->setText(QString::fromUtf8(QByteArray::fromBase64(myConfig->readConfigString("InternetLoginPassword").c_str())));
		} else {
			checkBox_rememberPassword->setChecked(false);
			lineEdit_password->clear();
		}
	}

	okButtonCheck();
}

void internetGameLoginDialogImpl::guestUserToggled(bool b)
{

	groupBox_reguser->setChecked(!b);

	if(b) {

		if(!lineEdit_username->text().isEmpty()) {
			myConfig->writeConfigString("MyRememberedNameDuringGuestLogin", lineEdit_username->text().toUtf8().constData());
		}
		checkBox_rememberPassword->setChecked(false);
		lineEdit_password->clear();
		lineEdit_username->clear();

		myConfig->writeBuffer();
	}

	okButtonCheck();
}

int internetGameLoginDialogImpl::exec()
{

	if(myConfig->readConfigInt("InternetLoginMode") == 0) {
		groupBox_reguser->setChecked(true);
		lineEdit_username->setText(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()));
		if(myConfig->readConfigInt("InternetSavePassword")) {
			checkBox_rememberPassword->setChecked(true);
			lineEdit_password->setText(QString::fromUtf8(QByteArray::fromBase64(myConfig->readConfigString("InternetLoginPassword").c_str())));
		} else {
			checkBox_rememberPassword->setChecked(false);
			lineEdit_password->clear();
		}
	} else {
		checkBox_guest->setChecked(true);
	}

	okButtonCheck();

	if(groupBox_reguser->isChecked() && !checkBox_rememberPassword->isChecked() && lineEdit_password->text().isEmpty()) {
		lineEdit_password->setFocus();
	}

	return QDialog::exec();
}

void internetGameLoginDialogImpl::accept()
{

	if(groupBox_reguser->isChecked()) {
		myConfig->writeConfigInt("InternetLoginMode", 0);
		myConfig->writeConfigString("MyName", lineEdit_username->text().toUtf8().constData());
		if(checkBox_rememberPassword->isChecked()) {
			myConfig->writeConfigInt("InternetSavePassword", 1);
			myConfig->writeConfigString("InternetLoginPassword", lineEdit_password->text().toUtf8().toBase64().constData());
		} else {
			myConfig->writeConfigInt("InternetSavePassword", 0);
		}

	} else if(checkBox_guest->isChecked()) {
		myConfig->writeConfigInt("InternetLoginMode", 1);
		// Generate a valid guest name.
		QString guestName;
		int guestId;
		Tools::GetRand(1, 99999, 1, &guestId);
		guestName.sprintf("Guest%05d", guestId);
		myConfig->writeConfigString("MyName", guestName.toUtf8().constData());
	}

	myConfig->writeBuffer();

	QDialog::accept();
}

void internetGameLoginDialogImpl::okButtonCheck()
{

	if(groupBox_reguser->isChecked()) {
		if(!lineEdit_password->text().isEmpty() && !lineEdit_username->text().isEmpty()) {
			pushButton_login->setEnabled(true);
		} else {
			pushButton_login->setEnabled(false);
		}
	} else {
		pushButton_login->setEnabled(true);
	}
}


bool internetGameLoginDialogImpl::eventFilter(QObject *obj, QEvent *event)
{
#ifdef ANDROID
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

	//androi changes for return key behavior (hopefully useless from necessitas beta2)
	if (event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Return) {
		if(lineEdit_username->hasFocus()) {
			lineEdit_password->setFocus();
		}
		if(lineEdit_password->hasFocus()) {
			QTimer::singleShot(1000, this, SLOT(clickLoginButton()));
		}
		event->ignore();
		return false;
	} else {
		// pass the event on to the parent class
		return QDialog::eventFilter(obj, event);
	}
#else
	return QDialog::eventFilter(obj, event);
#endif
}
