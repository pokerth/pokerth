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
 *****************************************************************************/ #include "mymessagedialogimpl.h"

#include <QtCore>
#include "configfile.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>

using namespace std;


myMessageDialogImpl::myMessageDialogImpl(ConfigFile *c, QWidget *parent)
	: QDialog(parent), myConfig(c), currentMsgId(0)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	setupUi(this);
}

int myMessageDialogImpl::exec(int messageId, QString msg, QString title, QPixmap pix, QDialogButtonBox::StandardButtons buttons, bool showCheckBox)
{
	if(showCheckBox) checkBox->show();
	else checkBox->hide();

	bool show = false;
	bool found = false;

	currentMsgId = messageId;

	currentMsgShowList = myConfig->readConfigStringList("IfInfoMessageShowList");
	list<std::string>::iterator it1;
	for(it1= currentMsgShowList.begin(); it1 != currentMsgShowList.end(); ++it1) {

		QString tmpString = QString::fromUtf8(it1->c_str());
		if(QString("%1").arg(messageId) ==  tmpString.split(",").at(1)) {

			found = true;
			show = tmpString.split(",").at(0).toInt();

			break;
		}
	}

	if(!found) {
		currentMsgShowList.push_back(QString("1,%1").arg(messageId).toUtf8().constData());
		myConfig->writeConfigStringList("IfInfoMessageShowList", currentMsgShowList);
		myConfig->writeBuffer();
	}

	if(!found || show) {

		setWindowTitle(title);
		label_icon->setPixmap(pix.scaled(50,50,Qt::KeepAspectRatio,Qt::SmoothTransformation));
		label->setText(msg);
		buttonBox->setStandardButtons(buttons);

		return QDialog::exec();
	}

	return -1;
}

void myMessageDialogImpl::accept()
{
	writeConfig();
	QDialog::accept();
}

void myMessageDialogImpl::reject()
{
	writeConfig();
	QDialog::reject();
}

void myMessageDialogImpl::writeConfig()
{
	if(checkBox->isChecked()) {

		list<std::string>::iterator it1;
		for(it1= currentMsgShowList.begin(); it1 != currentMsgShowList.end(); ++it1) {

			QString tmpString = QString::fromUtf8(it1->c_str());
			if(QString("%1").arg(currentMsgId) == tmpString.split(",").at(1)) {

				(*it1) = QString("0,%1").arg(currentMsgId).toUtf8().constData();
				break;
			}
		}

		myConfig->writeConfigStringList("IfInfoMessageShowList", currentMsgShowList);
		myConfig->writeBuffer();
	}
}

bool myMessageDialogImpl::checkIfMesssageWillBeDisplayed(int id)
{
	bool found = false;
	bool show = true;

	currentMsgShowList = myConfig->readConfigStringList("IfInfoMessageShowList");
	list<std::string>::iterator it1;
	for(it1= currentMsgShowList.begin(); it1 != currentMsgShowList.end(); ++it1) {

		QString tmpString = QString::fromUtf8(it1->c_str());
		if(QString("%1").arg(id) ==  tmpString.split(",").at(1)) {

			found = true;
			show = tmpString.split(",").at(0).toInt();

			break;
		}
	}

	if(!found) {
		currentMsgShowList.push_back(QString("1,%1").arg(id).toUtf8().constData());
		myConfig->writeConfigStringList("IfInfoMessageShowList", currentMsgShowList);
		myConfig->writeBuffer();
	}

	return show;
}
