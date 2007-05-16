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
#include "configfile.h"
#include <iostream>


settingsDialogImpl::settingsDialogImpl(QWidget *parent, ConfigFile *c)
    : QDialog(parent), myConfig(c)
{

	 setupUi(this);

	if (myConfig->readConfigInt("CLA_NoWriteAccess")) { groupBox_logOnOff->setDisabled(TRUE); }

	connect( buttonBox, SIGNAL( accepted() ), this, SLOT( isAccepted() ) );
	connect( lineEdit_humanPlayerName, SIGNAL( textChanged( const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent1Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent2Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent3Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent4Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent5Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent6Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( pushButton_openFlipsidePicture, SIGNAL( clicked() ), this, SLOT( setFlipsidePicFileName()) );
	connect( pushButton_openLogDir, SIGNAL( clicked() ), this, SLOT( setLogDir()) );
	connect( pushButton_openAvatarFile0, SIGNAL( clicked() ), this, SLOT( setAvatarFile0()) );
	connect( pushButton_openAvatarFile1, SIGNAL( clicked() ), this, SLOT( setAvatarFile1()) );
	connect( pushButton_openAvatarFile2, SIGNAL( clicked() ), this, SLOT( setAvatarFile2()) );
	connect( pushButton_openAvatarFile3, SIGNAL( clicked() ), this, SLOT( setAvatarFile3()) );
	connect( pushButton_openAvatarFile4, SIGNAL( clicked() ), this, SLOT( setAvatarFile4()) );
	connect( pushButton_openAvatarFile5, SIGNAL( clicked() ), this, SLOT( setAvatarFile5()) );
	connect( pushButton_openAvatarFile6, SIGNAL( clicked() ), this, SLOT( setAvatarFile6()) );

}

void settingsDialogImpl::exec() {

	
// 	stackedWidget->removeWidget(page_4);
// 	listWidget->takeItem(2);
// 	page_4->hide();

	playerNickIsChanged = FALSE;

	//Player Nicks
	lineEdit_humanPlayerName->setText(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()));
	lineEdit_humanPlayerAvatar->setText(QString::fromUtf8(myConfig->readConfigString("MyAvatar").c_str()));
	lineEdit_Opponent1Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent1Name").c_str()));
	lineEdit_Opponent1Avatar->setText(QString::fromUtf8(myConfig->readConfigString("Opponent1Avatar").c_str()));
	lineEdit_Opponent2Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent2Name").c_str()));
	lineEdit_Opponent2Avatar->setText(QString::fromUtf8(myConfig->readConfigString("Opponent2Avatar").c_str()));
	lineEdit_Opponent3Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent3Name").c_str()));
	lineEdit_Opponent3Avatar->setText(QString::fromUtf8(myConfig->readConfigString("Opponent3Avatar").c_str()));
	lineEdit_Opponent4Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent4Name").c_str()));
	lineEdit_Opponent4Avatar->setText(QString::fromUtf8(myConfig->readConfigString("Opponent4Avatar").c_str()));
	lineEdit_Opponent5Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent5Name").c_str()));
	lineEdit_Opponent5Avatar->setText(QString::fromUtf8(myConfig->readConfigString("Opponent5Avatar").c_str()));
	lineEdit_Opponent6Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent6Name").c_str()));
	lineEdit_Opponent6Avatar->setText(QString::fromUtf8(myConfig->readConfigString("Opponent6Avatar").c_str()));

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
	spinBox_serverPort->setValue(myConfig->readConfigInt("ServerPort"));
	lineEdit_serverPassword->setText(QString::fromUtf8(myConfig->readConfigString("ServerPassword").c_str()));
	checkBox_useIpv6->setChecked(myConfig->readConfigInt("ServerUseIpv6"));
	

	//Interface
	checkBox_showLeftToolbox->setChecked(myConfig->readConfigInt("ShowLeftToolBox"));
	checkBox_showRightToolbox->setChecked(myConfig->readConfigInt("ShowRightToolBox"));
	checkBox_showStatusbarMessages->setChecked(myConfig->readConfigInt("ShowStatusbarMessages"));
	checkBox_showIntro->setChecked(myConfig->readConfigInt("ShowIntro"));
	checkBox_showFadeOutCardsAnimation->setChecked(myConfig->readConfigInt("ShowFadeOutCardsAnimation"));
	checkBox_showFlipCardsAnimation->setChecked(myConfig->readConfigInt("ShowFlipCardsAnimation"));
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

	QDialog::exec();

}

void settingsDialogImpl::isAccepted() {

	settingsCorrect = TRUE;

// 	Player Nicks
	myConfig->writeConfigString("MyName", lineEdit_humanPlayerName->text().toUtf8().constData());
	if(QFile::QFile(lineEdit_humanPlayerAvatar->text()).exists() || lineEdit_humanPlayerAvatar->text() == "") { 
		myConfig->writeConfigString("MyAvatar", lineEdit_humanPlayerAvatar->text().toUtf8().constData());  
	}
	else {	QMessageBox::warning(this, tr("Settings Error"),
		tr("The entered human player avatar picture doesn't exists.\n"
		"Please enter an valid picture!"),
		QMessageBox::Ok);
		settingsCorrect = FALSE; 
	}
	
	myConfig->writeConfigString("Opponent1Name", lineEdit_Opponent1Name->text().toUtf8().constData());
	if(QFile::QFile(lineEdit_Opponent1Avatar->text()).exists() || lineEdit_Opponent1Avatar->text() == "") { 
		myConfig->writeConfigString("Opponent1Avatar", lineEdit_Opponent1Avatar->text().toUtf8().constData());  
	}
	else {	QMessageBox::warning(this, tr("Settings Error"),
		tr("The entered \"opponent 1\" avatar picture doesn't exists.\n"
		"Please enter an valid picture!"),
		QMessageBox::Ok);
		settingsCorrect = FALSE; 
	}

	myConfig->writeConfigString("Opponent2Name", lineEdit_Opponent2Name->text().toUtf8().constData());
	if(QFile::QFile(lineEdit_Opponent2Avatar->text()).exists() || lineEdit_Opponent2Avatar->text() == "") { 
		myConfig->writeConfigString("Opponent2Avatar", lineEdit_Opponent2Avatar->text().toUtf8().constData());  
	}
	else {	QMessageBox::warning(this, tr("Settings Error"),
		tr("The entered \"opponent 2\" avatar picture doesn't exists.\n"
		"Please enter an valid picture!"),
		QMessageBox::Ok);
		settingsCorrect = FALSE; 
	}

	myConfig->writeConfigString("Opponent3Name", lineEdit_Opponent3Name->text().toUtf8().constData());
	if(QFile::QFile(lineEdit_Opponent3Avatar->text()).exists() || lineEdit_Opponent3Avatar->text() == "") { 
		myConfig->writeConfigString("Opponent3Avatar", lineEdit_Opponent3Avatar->text().toUtf8().constData());  
	}
	else {	QMessageBox::warning(this, tr("Settings Error"),
		tr("The entered \"opponent 3\" avatar picture doesn't exists.\n"
		"Please enter an valid picture!"),
		QMessageBox::Ok);
		settingsCorrect = FALSE; 
	}

	myConfig->writeConfigString("Opponent4Name", lineEdit_Opponent4Name->text().toUtf8().constData());
	if(QFile::QFile(lineEdit_Opponent4Avatar->text()).exists() || lineEdit_Opponent4Avatar->text() == "") { 
		myConfig->writeConfigString("Opponent4Avatar", lineEdit_Opponent4Avatar->text().toUtf8().constData());  
	}
	else {	QMessageBox::warning(this, tr("Settings Error"),
		tr("The entered \"opponent 4\" avatar picture doesn't exists.\n"
		"Please enter an valid picture!"),
		QMessageBox::Ok);
		settingsCorrect = FALSE; 
	}

	myConfig->writeConfigString("Opponent5Name", lineEdit_Opponent5Name->text().toUtf8().constData());
	if(QFile::QFile(lineEdit_Opponent5Avatar->text()).exists() || lineEdit_Opponent5Avatar->text() == "") { 
		myConfig->writeConfigString("Opponent5Avatar", lineEdit_Opponent5Avatar->text().toUtf8().constData());  
	}
	else {	QMessageBox::warning(this, tr("Settings Error"),
		tr("The entered \"opponent 5\" avatar picture doesn't exists.\n"
		"Please enter an valid picture!"),
		QMessageBox::Ok);
		settingsCorrect = FALSE; 
	}

	myConfig->writeConfigString("Opponent6Name", lineEdit_Opponent6Name->text().toUtf8().constData());
	if(QFile::QFile(lineEdit_Opponent6Avatar->text()).exists() || lineEdit_Opponent6Avatar->text() == "") { 
		myConfig->writeConfigString("Opponent6Avatar", lineEdit_Opponent6Avatar->text().toUtf8().constData());  
	}
	else {	QMessageBox::warning(this, tr("Settings Error"),
		tr("The entered \"opponent 6\" avatar picture doesn't exists.\n"
		"Please enter an valid picture!"),
		QMessageBox::Ok);
		settingsCorrect = FALSE; 
	}

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
	myConfig->writeConfigInt("ServerPort", spinBox_serverPort->value());
	myConfig->writeConfigString("ServerPassword", lineEdit_serverPassword->text().toUtf8().constData());
	myConfig->writeConfigInt("ServerUseIpv6", checkBox_useIpv6->isChecked());
	
// 	Interface
	myConfig->writeConfigInt("ShowLeftToolBox", checkBox_showLeftToolbox->isChecked());
	myConfig->writeConfigInt("ShowRightToolBox", checkBox_showRightToolbox->isChecked());
	myConfig->writeConfigInt("ShowStatusbarMessages", checkBox_showStatusbarMessages->isChecked());	
	myConfig->writeConfigInt("ShowIntro", checkBox_showIntro->isChecked());
	myConfig->writeConfigInt("ShowFadeOutCardsAnimation", checkBox_showFadeOutCardsAnimation->isChecked());
	myConfig->writeConfigInt("ShowFlipCardsAnimation", checkBox_showFlipCardsAnimation->isChecked());
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

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select your avatar picture"),
                                                QDir::homePath(),
                                                tr("Images (*.png)"));

     if (!fileName.isEmpty())
         lineEdit_humanPlayerAvatar->setText(fileName);
}

void settingsDialogImpl::setAvatarFile1() {

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select computer opponents avatar picture"),
                                                QDir::homePath(),
                                                tr("Images (*.png)"));

     if (!fileName.isEmpty())
     	lineEdit_Opponent1Avatar->setText(fileName);
}

void settingsDialogImpl::setAvatarFile2() {

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select computer opponents avatar picture"),
                                                QDir::homePath(),
                                                tr("Images (*.png)"));

     if (!fileName.isEmpty())
     	lineEdit_Opponent2Avatar->setText(fileName);
}

void settingsDialogImpl::setAvatarFile3() {

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select computer opponents avatar picture"),
                                                QDir::homePath(),
                                                tr("Images (*.png)"));

     if (!fileName.isEmpty())
     	lineEdit_Opponent3Avatar->setText(fileName);
}

void settingsDialogImpl::setAvatarFile4() {

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select computer opponents avatar picture"),
                                                QDir::homePath(),
                                                tr("Images (*.png)"));

     if (!fileName.isEmpty())
     	lineEdit_Opponent4Avatar->setText(fileName);
}

void settingsDialogImpl::setAvatarFile5() {

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select computer opponents avatar picture"),
                                                QDir::homePath(),
                                                tr("Images (*.png)"));

     if (!fileName.isEmpty())
     	lineEdit_Opponent5Avatar->setText(fileName);
}

void settingsDialogImpl::setAvatarFile6() {

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select computer opponents avatar picture"),
                                                QDir::homePath(),
                                                tr("Images (*.png)"));

     if (!fileName.isEmpty())
     	lineEdit_Opponent6Avatar->setText(fileName);
}

void settingsDialogImpl::setLogDir()
 {
	 QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 QDir::homePath(),
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
		
     	if (!dir.isEmpty()) lineEdit_logDir->setText(dir);
 }
