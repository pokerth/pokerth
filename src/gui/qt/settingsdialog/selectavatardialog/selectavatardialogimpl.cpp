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
#include "selectavatardialogimpl.h"
#include "configfile.h"
#include <iostream>


selectAvatarDialogImpl::selectAvatarDialogImpl(QWidget *parent, ConfigFile *c)
    : QDialog(parent), myConfig(c)
{

	 setupUi(this);
	

// 	listWidget->setViewMode(QListView::IconMode);
// 	listWidget->setIconSize(QSize(50,50));
// 	listWidget->setLayoutDirection(Qt::LeftToRight);
    	listWidget->setViewMode(QListView::IconMode);
	listWidget->setIconSize(QSize(50, 50));
//     	listWidget->setFlow(QListView::TopToBottom);
//       	listWidget->setWrapping(true);
	listWidget->setSpacing(20);
	listWidget->setDragEnabled(FALSE);
	listWidget->setResizeMode(QListView::Adjust);

	int i;
	for (i=0; i<30; i++) {
		QListWidgetItem *myItem = new QListWidgetItem;
		myItem->setIcon(QIcon(QPixmap(":/guiv2/resources/guiv2/genereticAvatar.png")));
		listWidget->addItem(myItem);
	};

	QObject::connect(groupBox, SIGNAL(toggled(bool)), this, SLOT(toggleGroupBox1(bool)));
	QObject::connect(groupBox_2, SIGNAL(toggled(bool)), this, SLOT(toggleGroupBox2(bool)));

}

void selectAvatarDialogImpl::exec() {

	QDialog::exec();
}

void selectAvatarDialogImpl::toggleGroupBox1(bool toogleState) { if(groupBox->isChecked()) groupBox_2->setChecked(FALSE); }

void selectAvatarDialogImpl::toggleGroupBox2(bool toogleState) { if(groupBox_2->isChecked()) groupBox->setChecked(FALSE); }

