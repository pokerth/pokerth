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
    : QDialog(parent), myConfig(c), settingsCorrect(TRUE)
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

		
// 	int i;
// 	for (i=0; i<30; i++) {
// 		MyAvatarListItem *myItem = new MyAvatarListItem(listWidget);
// // 		myItem->setIcon(QIcon(QPixmap(":/guiv2/resources/guiv2/genereticAvatar.png").scaled(50,50,Qt::IgnoreAspectRatio,Qt::SmoothTransformation)));
// 		myItem->setMyLink(":/guiv2/resources/guiv2/genereticAvatar.png");
// 		myItem->setText("PokerTH");
// 		listWidget->addItem(myItem);
// 	};

	QStringList avatarList;

	avatarList << ":avatar/resources/avatar/Mona_Lisa.png";
        avatarList << ":avatar/resources/avatar/Catherine_russia.png";
        avatarList << ":avatar/resources/avatar/Elisabeth1.png";
	avatarList << ":avatar/resources/avatar/George_Washington.png";	
	avatarList << ":avatar/resources/avatar/Louis_XIV.png";
	avatarList << ":avatar/resources/avatar/Sissi.png";
	avatarList << ":avatar/resources/avatar/Columbus.png";
        avatarList << ":avatar/resources/avatar/Napoleon.png";
	avatarList << ":avatar/resources/avatar/Martin_Luther_King.png";
        avatarList << ":avatar/resources/avatar/PopeInnocentX.png";
        avatarList << ":avatar/resources/avatar/Pope_Clement_VII.png";
        avatarList << ":avatar/resources/avatar/James_Watt.png";
        avatarList << ":avatar/resources/avatar/albert_einstein.png";
        avatarList << ":avatar/resources/avatar/bach.png";
	avatarList << ":avatar/resources/avatar/mozart.png";
        avatarList << ":avatar/resources/avatar/goethe.png";
        avatarList << ":avatar/resources/avatar/linus.png";
        avatarList << ":avatar/resources/avatar/marc.png";
	avatarList << ":avatar/resources/avatar/florian.png";
	avatarList << ":avatar/resources/avatar/benedikt.png";
	avatarList << ":avatar/resources/avatar/Kapitan.png";
        avatarList << ":avatar/resources/avatar/clipart1.png";
	
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
	
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select external avatar picture"), QDir::homePath(), tr("Images (*.png)"));

     	if (!fileName.isEmpty())
     	lineEdit->setText(fileName);
}

