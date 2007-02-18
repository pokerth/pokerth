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
#include "joinnetworkgamedialogimpl.h"
#include "session.h"
// #include "configfile.h"

joinNetworkGameDialogImpl::joinNetworkGameDialogImpl(QWidget *parent)
      : QDialog(parent)
{

    setupUi(this);

	connect( pushButton_connect, SIGNAL( clicked() ), this, SLOT( startClient() ) );
	
}

void joinNetworkGameDialogImpl::setSession(Session* s)
{
	mySession = s;
}

void joinNetworkGameDialogImpl::startClient() {

	// TODO: Check input values!
	if (mySession)
		mySession->startNetworkClient(lineEdit_ipAddress->text().toUtf8().constData(), spinBox_port->value(), checkBox_ipv6->isChecked(), lineEdit_password->text().toUtf8().constData());
}