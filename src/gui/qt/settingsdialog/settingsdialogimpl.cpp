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
#include "settingsdialogimpl.h"
#include "myavatarlistitem.h"

#include "configfile.h"
#include <net/socket_startup.h>
#include <iostream>


settingsDialogImpl::settingsDialogImpl(QWidget *parent, ConfigFile *c, selectAvatarDialogImpl *s)
    : QDialog(parent), myConfig(c), mySelectAvatarDialogImpl(s)
{

  setupUi(this);
	
	QtHelper *myQtHelper = new QtHelper;

	pushButton_openFlipsidePicture->setIcon(QIcon(QPixmap(myQtHelper->getDataPath()+"gfx/gui/misc/fileopen16.png")));
	pushButton_openLogDir->setIcon(QIcon(QPixmap(myQtHelper->getDataPath()+"gfx/gui/misc/fileopen16.png")));

	if (myConfig->readConfigInt("CLA_NoWriteAccess")) { groupBox_logOnOff->setDisabled(TRUE); }

	connect( buttonBox, SIGNAL( accepted() ), this, SLOT( isAccepted() ) );
	connect( lineEdit_HumanPlayerName, SIGNAL( textChanged( const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent1Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent2Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent3Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent4Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent5Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent6Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( pushButton_openFlipsidePicture, SIGNAL( clicked() ), this, SLOT( setFlipsidePicFileName()) );
	connect( pushButton_openLogDir, SIGNAL( clicked() ), this, SLOT( setLogDir()) );
	connect( pushButton_HumanPlayerAvatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile0()) );
	connect( pushButton_Opponent1Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile1()) );
	connect( pushButton_Opponent2Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile2()) );
	connect( pushButton_Opponent3Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile3()) );
	connect( pushButton_Opponent4Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile4()) );
	connect( pushButton_Opponent5Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile5()) );
	connect( pushButton_Opponent6Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile6()) );

	connect( checkBox_UseInternetGamePassword, SIGNAL( toggled(bool) ), this, SLOT( clearInternetGamePassword(bool)) ); 

	//temporarely unused until ai is enabled in network
// 	label_36->hide();
// 	spinBox_netGameSpeed->hide();
// 
// 	label_18->hide();
// 	comboBox_netEngineVersion->hide();
}

void settingsDialogImpl::exec() {

	
// 	stackedWidget->removeWidget(page_4);
// 	listWidget->takeItem(2);
// 	page_4->hide();

	playerNickIsChanged = FALSE;

	//Player Nicks
	lineEdit_HumanPlayerName->setText(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()));
	pushButton_HumanPlayerAvatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("MyAvatar").c_str()));
	lineEdit_Opponent1Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent1Name").c_str()));
	pushButton_Opponent1Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent1Avatar").c_str()));
	lineEdit_Opponent2Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent2Name").c_str()));
	pushButton_Opponent2Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent2Avatar").c_str()));
	lineEdit_Opponent3Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent3Name").c_str()));
	pushButton_Opponent3Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent3Avatar").c_str()));
	lineEdit_Opponent4Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent4Name").c_str()));
	pushButton_Opponent4Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent4Avatar").c_str()));
	lineEdit_Opponent5Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent5Name").c_str()));
	pushButton_Opponent5Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent5Avatar").c_str()));
	lineEdit_Opponent6Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent6Name").c_str()));
	pushButton_Opponent6Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent6Avatar").c_str()));

	//refresh AvatarIcons
	pushButton_HumanPlayerAvatar->setIcon(QIcon(pushButton_HumanPlayerAvatar->getMyLink()));
	pushButton_Opponent1Avatar->setIcon(QIcon(pushButton_Opponent1Avatar->getMyLink()));
	pushButton_Opponent2Avatar->setIcon(QIcon(pushButton_Opponent2Avatar->getMyLink()));
	pushButton_Opponent3Avatar->setIcon(QIcon(pushButton_Opponent3Avatar->getMyLink()));
	pushButton_Opponent4Avatar->setIcon(QIcon(pushButton_Opponent4Avatar->getMyLink()));
	pushButton_Opponent5Avatar->setIcon(QIcon(pushButton_Opponent5Avatar->getMyLink()));
	pushButton_Opponent6Avatar->setIcon(QIcon(pushButton_Opponent6Avatar->getMyLink()));

	//Local Game Settings
	spinBox_quantityPlayers->setValue(myConfig->readConfigInt("NumberOfPlayers"));
	spinBox_startCash->setValue(myConfig->readConfigInt("StartCash"));
	spinBox_smallBlind->setValue(myConfig->readConfigInt("SmallBlind"));
	spinBox_handsBeforeRaiseSmallBlind->setValue(myConfig->readConfigInt("HandsBeforeRaiseSmallBlind"));
	spinBox_gameSpeed->setValue(myConfig->readConfigInt("GameSpeed"));
	checkBox_pauseBetweenHands->setChecked(myConfig->readConfigInt("PauseBetweenHands"));
	checkBox_showGameSettingsDialogOnNewGame->setChecked(myConfig->readConfigInt("ShowGameSettingsDialogOnNewGame"));
	comboBox_engineVersion->setCurrentIndex(myConfig->readConfigInt("EngineVersion"));
	
	//Network Game Settings
	spinBox_netQuantityPlayers->setValue(myConfig->readConfigInt("NetNumberOfPlayers"));
	spinBox_netStartCash->setValue(myConfig->readConfigInt("NetStartCash"));
	spinBox_netSmallBlind->setValue(myConfig->readConfigInt("NetSmallBlind"));
	spinBox_netHandsBeforeRaiseSmallBlind->setValue(myConfig->readConfigInt("NetHandsBeforeRaiseSmallBlind"));
	spinBox_netGameSpeed->setValue(myConfig->readConfigInt("NetGameSpeed"));
	comboBox_netEngineVersion->setCurrentIndex(myConfig->readConfigInt("NetEngineVersion"));
	spinBox_netTimeOutPlayerAction->setValue(myConfig->readConfigInt("NetTimeOutPlayerAction"));
	spinBox_serverPort->setValue(myConfig->readConfigInt("ServerPort"));
	lineEdit_serverPassword->setText(QString::fromUtf8(myConfig->readConfigString("ServerPassword").c_str()));
	checkBox_useIpv6->setChecked(myConfig->readConfigInt("ServerUseIpv6"));
	checkBox_useSctp->setChecked(myConfig->readConfigInt("ServerUseSctp"));

	//Internet Game Settings
	lineEdit_InternetServerAddress->setText(QString::fromUtf8(myConfig->readConfigString("InternetServerAddress").c_str()));
	spinBox_InternetServerPort->setValue(myConfig->readConfigInt("InternetServerPort"));
	lineEdit_InternetServerPassword->setText(QString::fromUtf8(myConfig->readConfigString("InternetServerPassword").c_str()));
	checkBox_InternetServerUseIpv6->setChecked(myConfig->readConfigInt("InternetServerUseIpv6"));
	checkBox_InternetServerUseSctp->setChecked(myConfig->readConfigInt("InternetServerUseSctp"));
	checkBox_UseInternetGamePassword->setChecked(myConfig->readConfigInt("UseInternetGamePassword"));
	if(myConfig->readConfigInt("UseInternetGamePassword")) {
		lineEdit_InternetGamePassword->setText(QString::fromUtf8(myConfig->readConfigString("InternetGamePassword").c_str()));
	}
	lineEdit_IRCServerAddress->setText(QString::fromUtf8(myConfig->readConfigString("IRCServerAddress").c_str()));
	spinBox_IRCServerPort->setValue(myConfig->readConfigInt("IRCServerPort"));
	lineEdit_IRCChannel->setText(QString::fromUtf8(myConfig->readConfigString("IRCChannel").c_str()));
	checkBox_IRCServerUseIpv6->setChecked(myConfig->readConfigInt("IRCServerUseIpv6"));

	//Interface
	checkBox_showLeftToolbox->setChecked(myConfig->readConfigInt("ShowLeftToolBox"));
	checkBox_showRightToolbox->setChecked(myConfig->readConfigInt("ShowRightToolBox"));
	checkBox_showStatusbarMessages->setChecked(myConfig->readConfigInt("ShowStatusbarMessages"));
	checkBox_showIntro->setChecked(myConfig->readConfigInt("ShowIntro"));
	checkBox_showFadeOutCardsAnimation->setChecked(myConfig->readConfigInt("ShowFadeOutCardsAnimation"));
	checkBox_showFlipCardsAnimation->setChecked(myConfig->readConfigInt("ShowFlipCardsAnimation"));
	checkBox_showBlindButtons->setChecked(myConfig->readConfigInt("ShowBlindButtons"));
	checkBox_antiPeekMode->setChecked(myConfig->readConfigInt("AntiPeekMode"));
	checkBox_playSoundEffects->setChecked(myConfig->readConfigInt("PlaySoundEffects"));
	horizontalSlider_soundVolume->setValue(myConfig->readConfigInt("SoundVolume"));
	radioButton_flipsideTux->setChecked(myConfig->readConfigInt("FlipsideTux"));
	radioButton_flipsideOwn->setChecked(myConfig->readConfigInt("FlipsideOwn"));
	if(radioButton_flipsideOwn->isChecked()) { 
		lineEdit_OwnFlipsideFilename->setEnabled(TRUE);
		pushButton_openFlipsidePicture->setEnabled(TRUE);
	}
	lineEdit_OwnFlipsideFilename->setText(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str()));

	//Log 
	groupBox_logOnOff->setChecked(myConfig->readConfigInt("LogOnOff"));
	lineEdit_logDir->setText(QString::fromUtf8(myConfig->readConfigString("LogDir").c_str()));
	spinBox_logStoreDuration->setValue(myConfig->readConfigInt("LogStoreDuration"));
	comboBox_logInterval->setCurrentIndex(myConfig->readConfigInt("LogInterval"));

	checkBox_useIpv6->setEnabled(socket_has_ipv6());
	checkBox_useSctp->setEnabled(socket_has_sctp());

	QDialog::exec();

}

void settingsDialogImpl::isAccepted() {

	settingsCorrect = TRUE;

// 	Player Nicks
	myConfig->writeConfigString("MyName", lineEdit_HumanPlayerName->text().toUtf8().constData());
	myConfig->writeConfigString("MyAvatar", pushButton_HumanPlayerAvatar->getMyLink().toUtf8().constData());  
	
	myConfig->writeConfigString("Opponent1Name", lineEdit_Opponent1Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent1Avatar", pushButton_Opponent1Avatar->getMyLink().toUtf8().constData());  

	myConfig->writeConfigString("Opponent2Name", lineEdit_Opponent2Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent2Avatar", pushButton_Opponent2Avatar->getMyLink().toUtf8().constData());  
	
	myConfig->writeConfigString("Opponent3Name", lineEdit_Opponent3Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent3Avatar", pushButton_Opponent3Avatar->getMyLink().toUtf8().constData());  
	
	myConfig->writeConfigString("Opponent4Name", lineEdit_Opponent4Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent4Avatar", pushButton_Opponent4Avatar->getMyLink().toUtf8().constData());  
	
	myConfig->writeConfigString("Opponent5Name", lineEdit_Opponent5Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent5Avatar", pushButton_Opponent5Avatar->getMyLink().toUtf8().constData());  

	myConfig->writeConfigString("Opponent6Name", lineEdit_Opponent6Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent6Avatar", pushButton_Opponent6Avatar->getMyLink().toUtf8().constData());  

// 	Local Game Settings
	myConfig->writeConfigInt("NumberOfPlayers", spinBox_quantityPlayers->value());
	myConfig->writeConfigInt("StartCash", spinBox_startCash->value());
	myConfig->writeConfigInt("SmallBlind", spinBox_smallBlind->value());
	myConfig->writeConfigInt("HandsBeforeRaiseSmallBlind", spinBox_handsBeforeRaiseSmallBlind->value());
	myConfig->writeConfigInt("GameSpeed", spinBox_gameSpeed->value());
	myConfig->writeConfigInt("EngineVersion", comboBox_engineVersion->currentIndex());
	myConfig->writeConfigInt("PauseBetweenHands", checkBox_pauseBetweenHands->isChecked());
	myConfig->writeConfigInt("ShowGameSettingsDialogOnNewGame", checkBox_showGameSettingsDialogOnNewGame->isChecked());

	//Network Game Settings
	myConfig->writeConfigInt("NetNumberOfPlayers", spinBox_netQuantityPlayers->value());
	myConfig->writeConfigInt("NetStartCash", spinBox_netStartCash->value());
	myConfig->writeConfigInt("NetSmallBlind", spinBox_netSmallBlind->value());
	myConfig->writeConfigInt("NetHandsBeforeRaiseSmallBlind", spinBox_netHandsBeforeRaiseSmallBlind->value());
	myConfig->writeConfigInt("NetGameSpeed", spinBox_netGameSpeed->value());
	myConfig->writeConfigInt("NetEngineVersion", comboBox_netEngineVersion->currentIndex());
	myConfig->writeConfigInt("NetTimeOutPlayerAction", spinBox_netTimeOutPlayerAction->value());
	myConfig->writeConfigInt("ServerPort", spinBox_serverPort->value());
	myConfig->writeConfigString("ServerPassword", lineEdit_serverPassword->text().toUtf8().constData());
	myConfig->writeConfigInt("ServerUseIpv6", checkBox_useIpv6->isChecked());
	myConfig->writeConfigInt("ServerUseSctp", checkBox_useSctp->isChecked());

	//Internet Game Settings
	myConfig->writeConfigString("InternetServerAddress", lineEdit_InternetServerAddress->text().toUtf8().constData());
	myConfig->writeConfigInt("InternetServerPort", spinBox_InternetServerPort->value());
	myConfig->writeConfigString("InternetServerPassword", lineEdit_InternetServerPassword->text().toUtf8().constData());
	myConfig->writeConfigInt("InternetServerUseIpv6", checkBox_InternetServerUseIpv6->isChecked());
	myConfig->writeConfigInt("InternetServerUseSctp", checkBox_InternetServerUseSctp->isChecked());
	myConfig->writeConfigInt("UseInternetGamePassword", checkBox_UseInternetGamePassword->isChecked());
	myConfig->writeConfigString("InternetGamePassword", lineEdit_InternetGamePassword->text().toUtf8().constData());
	myConfig->writeConfigString("IRCServerAddress", lineEdit_IRCServerAddress->text().toUtf8().constData());
	myConfig->writeConfigInt("IRCServerPort", spinBox_IRCServerPort->value());
	myConfig->writeConfigString("IRCChannel", lineEdit_IRCChannel->text().toUtf8().constData());
	myConfig->writeConfigInt("IRCServerUseIpv6", checkBox_IRCServerUseIpv6->isChecked());
	
// 	Interface
	myConfig->writeConfigInt("ShowLeftToolBox", checkBox_showLeftToolbox->isChecked());
	myConfig->writeConfigInt("ShowRightToolBox", checkBox_showRightToolbox->isChecked());
	myConfig->writeConfigInt("ShowStatusbarMessages", checkBox_showStatusbarMessages->isChecked());	
	myConfig->writeConfigInt("ShowIntro", checkBox_showIntro->isChecked());
	myConfig->writeConfigInt("ShowFadeOutCardsAnimation", checkBox_showFadeOutCardsAnimation->isChecked());
	myConfig->writeConfigInt("ShowFlipCardsAnimation", checkBox_showFlipCardsAnimation->isChecked());
	myConfig->writeConfigInt("ShowBlindButtons", checkBox_showBlindButtons->isChecked());
	myConfig->writeConfigInt("AntiPeekMode", checkBox_antiPeekMode->isChecked());
	myConfig->writeConfigInt("PlaySoundEffects", checkBox_playSoundEffects->isChecked());
	myConfig->writeConfigInt("SoundVolume", horizontalSlider_soundVolume->value());
	myConfig->writeConfigInt("FlipsideTux", radioButton_flipsideTux->isChecked());
	myConfig->writeConfigInt("FlipsideOwn", radioButton_flipsideOwn->isChecked());

	if(radioButton_flipsideOwn->isChecked()) {
		if(QFile::QFile(lineEdit_OwnFlipsideFilename->text()).exists() && lineEdit_OwnFlipsideFilename->text() != "") {myConfig->writeConfigString("FlipsideOwnFile", lineEdit_OwnFlipsideFilename->text().toUtf8().constData()); }
		else {	QMessageBox::warning(this, tr("Settings Error"),
			tr("The entered flipside picture doesn't exists.\n"
			"Please enter an valid picture!"),
			QMessageBox::Ok);
			settingsCorrect = FALSE; 
		}
	}

//	Log
	myConfig->writeConfigInt("LogOnOff", groupBox_logOnOff->isChecked());
	if (myConfig->readConfigInt("LogOnOff")) {
	// if log On
		if(QDir::QDir(lineEdit_logDir->text()).exists() && lineEdit_logDir->text() != "") { myConfig->writeConfigString("LogDir", lineEdit_logDir->text().toUtf8().constData());	}
		else { 
			QMessageBox::warning(this, tr("Settings Error"),
			tr("The log file directory doesn't exists.\n"
			"Please select an valid directory!"),
			QMessageBox::Ok);
			settingsCorrect = FALSE; 
		}
	
		myConfig->writeConfigInt("LogStoreDuration", spinBox_logStoreDuration->value());
		myConfig->writeConfigInt("LogInterval", comboBox_logInterval->currentIndex());
	}

	//write buffer to disc 
	myConfig->writeBuffer();

	//Wenn alles richtig eingegeben wurde --> Dialog schließen
	if(settingsCorrect) { this->hide(); }
}

void settingsDialogImpl::setFlipsidePicFileName()
 {
 	QString fileName = QFileDialog::getOpenFileName(this, tr("Select your flipside picture"),
                                                QDir::homePath(),
                                                tr("Images (*.png)"));

     if (!fileName.isEmpty())
         lineEdit_OwnFlipsideFilename->setText(fileName);
 }

void settingsDialogImpl::setAvatarFile0() {

	callSelectAvatarDialog();
	pushButton_HumanPlayerAvatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
	
	pushButton_HumanPlayerAvatar->setIcon(QIcon(pushButton_HumanPlayerAvatar->getMyLink()));
	
}

void settingsDialogImpl::setAvatarFile1() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent1Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent1Avatar->setIcon(QIcon(pushButton_Opponent1Avatar->getMyLink()));
	}     	
}

void settingsDialogImpl::setAvatarFile2() {

	callSelectAvatarDialog();
	
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent2Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent2Avatar->setIcon(QIcon(pushButton_Opponent2Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile3() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent3Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent3Avatar->setIcon(QIcon(pushButton_Opponent3Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile4() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent4Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent4Avatar->setIcon(QIcon(pushButton_Opponent4Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile5() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent5Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent5Avatar->setIcon(QIcon(pushButton_Opponent5Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile6() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent6Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent6Avatar->setIcon(QIcon(pushButton_Opponent6Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setLogDir()
 {
	 QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 QDir::homePath(),
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
		
     	if (!dir.isEmpty()) lineEdit_logDir->setText(dir);
 }

void settingsDialogImpl::clearInternetGamePassword(bool clear) {

	if(!clear) { lineEdit_InternetGamePassword->clear(); }
}

