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

	progressBar->setValue(actionID*(100/MSG_SOCK_LAST));

	if (actionID == MSG_SOCK_LAST)
		QTimer::singleShot(1000, this, SLOT(accept()));
}

void connectToServerDialogImpl::error(int errorID, int osErrorID) {

	switch (errorID) {

		case ERR_SOCK_SERVERADDR_NOT_SET:
			{QMessageBox::warning(this, tr("Network Error"),
				tr("Server address was not set."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_INVALID_PORT:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("An invalid port was set (ports 0-1023 are not allowed)."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_CREATION_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Could not create a socket for TCP communication."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_SET_ADDR_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Could not set the IP address."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_SET_PORT_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Could not set the port for this type of address."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_RESOLVE_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("The server name could not be resolved."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_BIND_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Bind failed - please choose a different port."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_LISTEN_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Internal network error: \"listen\" failed."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_ACCEPT_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Server execution was terminated."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_CONNECT_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Could not connect to the server."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_SELECT_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Internal network error: \"select\" failed."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_RECV_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Internal network error: \"recv\" failed."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_SEND_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Internal network error: \"send\" failed."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_CONN_RESET:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Connection was closed by server."),
				QMessageBox::Close); }
		break;
		default:  { QMessageBox::warning(this, tr("Network Error"),
				tr("DEFAULT ERROR"),
				QMessageBox::Close); }
	}

	this->reject();

}
