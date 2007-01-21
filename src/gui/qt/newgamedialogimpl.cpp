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
#include "newgamedialogimpl.h"
#include "configfile.h"

newGameDialogImpl::newGameDialogImpl(QWidget *parent, const char *name)
      : QDialog(parent, name)
{

    setupUi(this);

	//Formulare Füllen
	ConfigFile config;	

	bool ok=TRUE;

	//Game Settings
// 	spinBox_quantityPlayers->setValue(config.readConfig("numberofplayers", "5").toInt(&ok,10));
// 	spinBox_startCash->setValue(config.readConfig("startcash", "2000").toInt(&ok,10));
// 	spinBox_smallBlind->setValue(config.readConfig("smallblind", "10").toInt(&ok,10));
// 	spinBox_gameSpeed->setValue(config.readConfig("gamespeed", "4").toInt(&ok,10));
	

}

