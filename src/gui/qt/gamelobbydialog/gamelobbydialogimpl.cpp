//
// C++ Implementation: gamelobbydialogimpl
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "gamelobbydialogimpl.h"

gameLobbyDialogImpl::gameLobbyDialogImpl(QWidget *parent, ConfigFile *c)
 : QDialog(parent), myConfig(c)
{
    setupUi(this);

}

void gameLobbyDialogImpl::exec()
{

	QDialog::exec();
}


gameLobbyDialogImpl::~gameLobbyDialogImpl()
{
}


