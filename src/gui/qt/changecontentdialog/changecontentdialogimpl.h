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
#ifndef CHANGECONTENTDIALOGIMPL_H
#define CHANGECONTENTDIALOGIMPL_H

#include "ui_changecontentdialog.h"

#include <QtCore>

enum DialogType { CHANGE_HUMAN_PLAYER_NAME=0, CHANGE_NICK_ALREADY_IN_USE, CHANGE_NICK_INVALID, CHANGE_INET_BAD_GAME_NAME, CHANGE_INET_GAME_NAME_IN_USE };

class ConfigFile;

class changeContentDialogImpl: public QDialog, public Ui::changeContentDialog {
    Q_OBJECT
public:
    changeContentDialogImpl(QWidget *parent, ConfigFile *config, DialogType t);

public slots:

    void saveContent();

private: 
    ConfigFile *myConfig;
    DialogType myType;

};

#endif
