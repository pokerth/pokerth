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
#include <net/socket_msg.h>

connectToServerDialogImpl::connectToServerDialogImpl(QWidget *parent)
      : QDialog(parent)
{

    setupUi(this);
}

void connectToServerDialogImpl::refresh(int actionID) {

	switch (actionID) {

	case MSG_SOCK_INIT_DONE: { label_actionMessage->setText("Resolving address..."); }
	break;
	case MSG_SOCK_RESOLVE_DONE: { label_actionMessage->setText("Connecting to server..."); }
	break;
	case MSG_SOCK_CONNECT_DONE: { label_actionMessage->setText("Starting session..."); }
	break;
	case MSG_SOCK_SESSION_DONE: { label_actionMessage->setText("Connection established!"); }
	break;

	default:  { label_actionMessage->setText("Please wait..."); }
	}

	progressBar->setValue(actionID*(100/MSG_SOCK_LIMIT_CONNECT));

	if (actionID == MSG_SOCK_LIMIT_CONNECT)
		QTimer::singleShot(1000, this, SLOT(accept()));
}

