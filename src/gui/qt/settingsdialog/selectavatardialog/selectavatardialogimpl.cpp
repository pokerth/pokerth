/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
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
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#include "selectavatardialogimpl.h"
#include "myavatarlistitem.h"
#include "mymessagebox.h"
#include "configfile.h"
#include <iostream>


selectAvatarDialogImpl::selectAvatarDialogImpl(QWidget *parent, ConfigFile *c)
	: QDialog(parent), myConfig(c), settingsCorrect(true), avatarDir("")
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	setupUi(this);
#ifdef ANDROID
	this->setWindowState(Qt::WindowFullScreen);
#endif
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
	listWidget->setDragEnabled(false);
	listWidget->setResizeMode(QListView::Adjust);


// 	int i;
// 	for (i=0; i<30; i++) {
// 		MyAvatarListItem *myItem = new MyAvatarListItem(listWidget);
// // 		myItem->setIcon(QIcon(QPixmap(myAppDataPath +"gfx/gui/table/default/genereticAvatar.png").scaled(50,50,Qt::IgnoreAspectRatio,Qt::SmoothTransformation)));
// 		myItem->setMyLink(myAppDataPath +"gfx/gui/table/default/genereticAvatar.png");
// 		myItem->setText("PokerTH");
// 		listWidget->addItem(myItem);
// 	};

	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/95df673e5ee4d02f0f95d5c7ff091cfb.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/490ed3a748f6f4d6c261436198f2368f.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/e3e1ae15f7d808fce16c26c049e2cbe9.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/237c518f2bb6ac99f6eb1012e18460b9.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/9f49fc3d6062ddc08dc6bbd9ef9b399d.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/bed234c6672d512bdb4c2768f0b3f90c.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/d6f8a45e9243b7e7e4e2c8e41df1c9f5.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/ef0975f38a367a45cb8042b438957304.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/bc28d08c451a775f9e332c14a397096b.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/cbd2e0c6ee579fb8e0f3942639cf988d.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/d66940c138c207c68a338769d683e691.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/16b702a87f681c4343486523a1a867e8.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/45e16bc214942321a3814fb43a84a88e.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/d21268e1d2ec969367ee5b54bd34738b.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/14e3f39cc775608bcecf5597248b3558.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/0c04318e6fa54534935e3e6cfe12c4a1.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/6ecea83d9fac6b4cae97e2fccd09a5f6.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/74672ebfa2ab02968386c4e441a08668.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/e54e67ebd5265cf549362a2cda5a999c.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/da34195981d255656f39c05481de3b6a.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/6ad16e2c3e4b8e4e6da47032d4372f4b.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/c018b15b8164a303b9395b1e89c51004.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/0c24b06358f7b308d6b47c59ebace73d.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/c6d57064672b9a6c78a06248d2a60770.png";
	peopleAvatarList << myAppDataPath +"gfx/avatars/default/people/39fa73528e8dc1a5a478938affe79100.png";

	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/d9a4712147692ace05d5b87a5f76223f.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/cd78743ae833cb838395e126ddc4b298.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/e604d933cf5dcbef120c516c66083b13.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/3036f43bab1f56a3c3dd693c0551bbd8.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/663f3d7a9ad330b2c77aa5f542fedf38.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/c10142a7b42f9e5c89ad4c644c570b74.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/e6e6b9af8c7bea68e359538e1298efd6.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/ed1dc23e53b713ab69850f808e39498e.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/9ff203623c75d8570126599d45922aa3.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/0f2ca195ebaec613ffc8626185415b18.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/32955c19e24eba10fd63f1dcc75b5a25.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/276648a9e4397177ba6e065bb8392a0f.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/82487a6a11cfd4174d3c56c13f47c03e.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/7b7d565bf61d15722085e0489c474078.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/aafbbe402e1883657b2db3e2f7574e5a.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/00a0b3d26a67841239b8883183b7a8f0.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/f4893bf7b611d72231e94fa487cc74e9.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/f12b7ce0222d0303a0de70c7dfa9ac0f.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/2d5248926bda0c674f31e25981de33a4.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/71348dd09e29dbe4ee6accc25d74bc94.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/192f2abb785082aa9cbd745f763c7afb.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/9ae421286ad12b4a5e7d2ca767e3d9a3.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/08e18c4e25a5052575e6a409e71959f5.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/dccda20e75901d151e199cf0d74451d4.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/f08cff60124b7690f13319fcf67a0b26.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/eb427f89e4773e87afb87505885cf698.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/f48c98f3814cafa82e1c444915ceb0a7.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/88fdee1614fdbf125d0a82bc349da0ff.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/a8c3d13377c17b06341b3904185ca30d.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/e6a7b4c2d32f5c5ff8cd895b928b5594.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/8426dadd34be5db4f4cdfa12ec1e8c53.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/53d7f2bda9b904adf62153a9341d48db.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/030a14c105d3fd164cb4bae54bad6191.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/d0b3c777001d2548fc87c112a181ddbf.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/d2e74be7f80276f90091f90b2a78ddfb.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/70bf201de8b46b19a30b94d6ea717043.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/20f0819f145c3a6e7272adc2909f35e7.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/20e7e95df017855a44a16ff54bb8252a.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/d7204e29016ca8963c02f46d4d41dc7d.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/5858abb9942a4d7faabfb6e7ff2520fd.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/c824fc964b0f051569d961d24d0897cd.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/778eca12f78519ac598f8858bdaf5ffa.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/e4e0c2850b68a65dcce6e83512e60665.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/5e802a524d92f339ad1adedf5675c467.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/9a106f3f5f9647c77174da1977dcb3c1.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/6b1ae8bfc14ffb2ae3247c7327b1f5ed.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/44d23de146bd9d753571915226c44cfd.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/bb2ae3396e8523545a955ceec97a273b.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/76fcf5f16951e17a63e4ff50d09278f7.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/41c06d77495fbcd69cfbb4457ebc11dc.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/92875006f8786c9fa082ca13ae188a28.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/d2dfc050bd61af58f05853864d5f9c4e.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/2b0fca47b50e2cc4f910eba9ffa858a9.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/85de4941160836a15745859dfcd9828e.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/fee2595e8a7be7ea50a39604e8c72e5e.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/f69440dd7d5483cc33d93c52cbb2c64d.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/f3e4763d3f5ba1a4bde813667efa82a1.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/150bfd890394dceb33c66ec8d033b7f4.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/7ee6c7e39bdecf872f782f7fefeffc6c.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/3956df277e41217641829542a4591c5f.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/910800128fa4afacd15c61a67e6d01a5.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/eb7612a2b515bd826babec8f649b44cf.png";
	miscAvatarList << myAppDataPath +"gfx/avatars/default/misc/08ced517579b3d258c947aec6c0a0dd4.png";



// 	peopleAvatarList << myAppDataPath +"gfx/avatars/default/misc/

	connect(groupBox, SIGNAL(toggled(bool)), this, SLOT(toggleGroupBox1(bool)));
	connect(groupBox_2, SIGNAL(toggled(bool)), this, SLOT(toggleGroupBox2(bool)));
	connect( buttonBox, SIGNAL( accepted() ), this, SLOT( isAccepted() ) );
	connect( buttonBox, SIGNAL( rejected() ), this, SLOT( isRejected() ) );
	connect( pushButton_OpenAvatarFile, SIGNAL( clicked() ), this, SLOT( setExternalAvatar() ) );
	connect( listWidget, SIGNAL( doubleClicked(QModelIndex) ), this, SLOT( isAccepted() ) );
	connect( comboBox_avatarViewCategorie, SIGNAL( currentIndexChanged(int) ), this, SLOT( refreshAvatarView() ) );


}

int selectAvatarDialogImpl::exec()
{
	//clear
	lineEdit->setText("");
	refreshAvatarView();
	return QDialog::exec();
}

void selectAvatarDialogImpl::refreshAvatarView()
{

	listWidget->clear();

	int i;
	QStringList currentViewList;

	switch(comboBox_avatarViewCategorie->currentIndex()) {

	case 0: {
		currentViewList = peopleAvatarList;
	}
	break;

	case 1: {
		currentViewList = miscAvatarList;
	}
	break;
	}

	for (i=0; i<currentViewList.size(); i++) {

		MyAvatarListItem *myItem = new MyAvatarListItem(listWidget);
		myItem->setIcon(QIcon(QPixmap(currentViewList.at(i)).scaled(50,50,Qt::IgnoreAspectRatio,Qt::SmoothTransformation)));
		myItem->setMyLink(currentViewList.at(i));
		myItem->setText(QString("No. ")+QString::number(i+1,10));
		listWidget->addItem(myItem);
	}
}

void selectAvatarDialogImpl::toggleGroupBox1(bool /*toggleState*/)
{
	if(groupBox->isChecked()) groupBox_2->setChecked(false);
}

void selectAvatarDialogImpl::toggleGroupBox2(bool /*toggleState*/)
{
	if(groupBox_2->isChecked()) groupBox->setChecked(false);
}

QString selectAvatarDialogImpl::getAvatarLink()
{

	QList<QListWidgetItem *> myItemList = listWidget->selectedItems();

	if(groupBox->isChecked()) {
		if(myItemList.size() == 1) {
			return static_cast<MyAvatarListItem*>(listWidget->currentItem())->getMyLink();
		} else return QString("");
	}

	QFile lineEditFile(lineEdit->text());
	if(groupBox_2->isChecked() && lineEditFile.exists() ) return externalAvatar;
	else return QString("");
}


void selectAvatarDialogImpl::isAccepted()
{

	QList<QListWidgetItem *> myItemList = listWidget->selectedItems();

	settingsCorrect = true;

	if(groupBox->isChecked()) {
		if(myItemList.size() == 0) {
			MyMessageBox::warning(this, tr("Avatar File Error"),
								  tr("Please select an avatar from the list!"),
								  QMessageBox::Ok);
			settingsCorrect = false;
		} else settingsCorrect = true;;
	}

	if(groupBox_2->isChecked()) {
		QFile lineEditFile(lineEdit->text());
		if(lineEditFile.exists()) {

			if(lineEditFile.size() <= 30720 ) {
				externalAvatar = lineEdit->text();
				settingsCorrect = true;
			} else {
				MyMessageBox::warning(this, tr("Avatar File Error"),
									  tr("The file size of the chosen picture is too big. (max. 30KB)\n"
										 "Please choose a smaller picture!"),
									  QMessageBox::Ok);
				settingsCorrect = false;
				externalAvatar = "";

			}
		} else {
			MyMessageBox::warning(this, tr("Avatar File Error"),
								  tr("The entered avatar picture doesn't exist.\n"
									 "Please enter an valid picture!"),
								  QMessageBox::Ok);
			settingsCorrect = false;
			externalAvatar = "";
		}
	}

	//Wenn alles richtig eingegeben wurde --> Dialog schließen
	if(settingsCorrect) {
		this->hide();
	}
}

void selectAvatarDialogImpl::isRejected()
{
	settingsCorrect = false;
}

void selectAvatarDialogImpl::setExternalAvatar()
{

	if (avatarDir == "") avatarDir = QDir::homePath();
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select external avatar picture"), avatarDir, tr("Images (*.png *.jpg *.gif)"));

	if (!fileName.isEmpty()) {
		lineEdit->setText(fileName);
		avatarDir =  fileName;
	}
}

