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
#include "myavatarlistitem.h"
#include "configfile.h"
#include <iostream>


selectAvatarDialogImpl::selectAvatarDialogImpl(QWidget *parent, ConfigFile *c)
    : QDialog(parent), myConfig(c), settingsCorrect(TRUE), avatarDir("")
{

	 setupUi(this);
	
	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());

	pushButton_OpenAvatarFile->setIcon(QIcon(QPixmap(myAppDataPath+"gfx/gui/misc/fileopen16.png")));

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

		
// 	int i;
// 	for (i=0; i<30; i++) {
// 		MyAvatarListItem *myItem = new MyAvatarListItem(listWidget);
// // 		myItem->setIcon(QIcon(QPixmap(myAppDataPath +"gfx/gui/table/default/genereticAvatar.png").scaled(50,50,Qt::IgnoreAspectRatio,Qt::SmoothTransformation)));
// 		myItem->setMyLink(myAppDataPath +"gfx/gui/table/default/genereticAvatar.png");
// 		myItem->setText("PokerTH");
// 		listWidget->addItem(myItem);
// 	};

	QStringList avatarList;

	avatarList << myAppDataPath +"gfx/avatars/default/95df673e5ee4d02f0f95d5c7ff091cfb.png";
	avatarList << myAppDataPath +"gfx/avatars/default/490ed3a748f6f4d6c261436198f2368f.png";
	avatarList << myAppDataPath +"gfx/avatars/default/e3e1ae15f7d808fce16c26c049e2cbe9.png";
	avatarList << myAppDataPath +"gfx/avatars/default/237c518f2bb6ac99f6eb1012e18460b9.png";	
	avatarList << myAppDataPath +"gfx/avatars/default/9f49fc3d6062ddc08dc6bbd9ef9b399d.png";
	avatarList << myAppDataPath +"gfx/avatars/default/bed234c6672d512bdb4c2768f0b3f90c.png";
	avatarList << myAppDataPath +"gfx/avatars/default/d6f8a45e9243b7e7e4e2c8e41df1c9f5.png";
	avatarList << myAppDataPath +"gfx/avatars/default/ef0975f38a367a45cb8042b438957304.png";
	avatarList << myAppDataPath +"gfx/avatars/default/bc28d08c451a775f9e332c14a397096b.png";
	avatarList << myAppDataPath +"gfx/avatars/default/cbd2e0c6ee579fb8e0f3942639cf988d.png";
	avatarList << myAppDataPath +"gfx/avatars/default/d66940c138c207c68a338769d683e691.png";
	avatarList << myAppDataPath +"gfx/avatars/default/16b702a87f681c4343486523a1a867e8.png";
	avatarList << myAppDataPath +"gfx/avatars/default/45e16bc214942321a3814fb43a84a88e.png";
	avatarList << myAppDataPath +"gfx/avatars/default/d21268e1d2ec969367ee5b54bd34738b.png";
	avatarList << myAppDataPath +"gfx/avatars/default/14e3f39cc775608bcecf5597248b3558.png";
	avatarList << myAppDataPath +"gfx/avatars/default/0c04318e6fa54534935e3e6cfe12c4a1.png";
	avatarList << myAppDataPath +"gfx/avatars/default/6ecea83d9fac6b4cae97e2fccd09a5f6.png";
	avatarList << myAppDataPath +"gfx/avatars/default/d90bd3fbbe3671ebac5f9f223d0c6d65.png";
	avatarList << myAppDataPath +"gfx/avatars/default/93dec21823a314fb8ef2261b7aa75ae5.png";
	avatarList << myAppDataPath +"gfx/avatars/default/74672ebfa2ab02968386c4e441a08668.png";
	avatarList << myAppDataPath +"gfx/avatars/default/e54e67ebd5265cf549362a2cda5a999c.png";
	avatarList << myAppDataPath +"gfx/avatars/default/da34195981d255656f39c05481de3b6a.png";	
	avatarList << myAppDataPath +"gfx/avatars/default/6ad16e2c3e4b8e4e6da47032d4372f4b.png";
	avatarList << myAppDataPath +"gfx/avatars/default/c018b15b8164a303b9395b1e89c51004.png";
	avatarList << myAppDataPath +"gfx/avatars/default/0c24b06358f7b308d6b47c59ebace73d.png";
	
	int i;
	for (i=0; i<avatarList.size(); i++) {
	
		MyAvatarListItem *myItem = new MyAvatarListItem(listWidget);
		myItem->setIcon(QIcon(QPixmap(avatarList.at(i)).scaled(50,50,Qt::IgnoreAspectRatio,Qt::SmoothTransformation)));
		myItem->setMyLink(avatarList.at(i));
		myItem->setText(QString("No. ")+QString::number(i+1,10));
		listWidget->addItem(myItem);
	}

	connect(groupBox, SIGNAL(toggled(bool)), this, SLOT(toggleGroupBox1(bool)));
	connect(groupBox_2, SIGNAL(toggled(bool)), this, SLOT(toggleGroupBox2(bool)));
	connect( buttonBox, SIGNAL( accepted() ), this, SLOT( isAccepted() ) );
	connect( buttonBox, SIGNAL( rejected() ), this, SLOT( isRejected() ) );
	connect( pushButton_OpenAvatarFile, SIGNAL( clicked() ), this, SLOT( setExternalAvatar() ) );
	connect( listWidget, SIGNAL( doubleClicked(QModelIndex) ), this, SLOT( isAccepted() ) );

}

void selectAvatarDialogImpl::exec() {

	//clear
	lineEdit->setText("");
	
	QDialog::exec();
}

void selectAvatarDialogImpl::toggleGroupBox1(bool toogleState) { if(groupBox->isChecked()) groupBox_2->setChecked(FALSE); }

void selectAvatarDialogImpl::toggleGroupBox2(bool toogleState) { if(groupBox_2->isChecked()) groupBox->setChecked(FALSE); }

QString selectAvatarDialogImpl::getAvatarLink() {

	QList<QListWidgetItem *> myItemList = listWidget->selectedItems();

	if(groupBox->isChecked()) {
		if(myItemList.size() == 1) {
			return static_cast<MyAvatarListItem*>(listWidget->currentItem())->getMyLink();
		}
		else return QString("");
	}
	
	if(groupBox_2->isChecked() && QFile::QFile(lineEdit->text()).exists() ) return externalAvatar;
	else return QString("");
}


void selectAvatarDialogImpl::isAccepted() {

	QList<QListWidgetItem *> myItemList = listWidget->selectedItems();

	if(groupBox->isChecked()) { 
		if(myItemList.size() == 0) {
			QMessageBox::warning(this, tr("Avatar File Error"),
			tr("Please select an avatar from the list!"),
			QMessageBox::Ok);
			settingsCorrect = FALSE; 
		}
		else settingsCorrect = TRUE;;
	}
	
	if(groupBox_2->isChecked()) {
		if(QFile::QFile(lineEdit->text()).exists()) { 
			externalAvatar = lineEdit->text();
			settingsCorrect = TRUE;
		}
		else { QMessageBox::warning(this, tr("Avatar File Error"),
			tr("The entered avatar picture doesn't exists.\n"
			"Please enter an valid picture!"),
			QMessageBox::Ok);
			settingsCorrect = FALSE; 
			externalAvatar = "";
		}
	}

	//Wenn alles richtig eingegeben wurde --> Dialog schließen
	if(settingsCorrect) { this->hide(); }
}

void selectAvatarDialogImpl::isRejected() { settingsCorrect = FALSE;  }

void selectAvatarDialogImpl::setExternalAvatar() {
	
	if (avatarDir == "") avatarDir = QDir::homePath();
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select external avatar picture"), avatarDir, tr("Images (*.png *.jpg *.gif)"));

     	if (!fileName.isEmpty()) {
     		lineEdit->setText(fileName);	
		avatarDir =  fileName;
	}
}

