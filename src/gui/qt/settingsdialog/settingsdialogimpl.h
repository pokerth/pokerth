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
#ifndef SETTINGSDIALOGIMPL_H
#define SETTINGSDIALOGIMPL_H

#ifdef GUI_800x480
#include "ui_settingsdialog_800x480.h"
#else
#include "ui_settingsdialog.h"
#endif
#include "selectavatardialogimpl.h"
#include "manualblindsorderdialogimpl.h"

#include <QtCore>
#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif

#include <boost/shared_ptr.hpp>
#include <vector>
#include <iostream>

class ConfigFile;
class selectAvatarDialogImpl;
class manualBlindsOrderDialogImpl;
class MyStyleListItem;
class guiLog;
class FlickCharm;

class settingsDialogImpl: public QDialog, public Ui::settingsDialog
{
	Q_OBJECT
public:
	settingsDialogImpl(QWidget *parent = 0, ConfigFile *c = 0, selectAvatarDialogImpl *s = 0);

	~settingsDialogImpl();

	void prepareDialog();
	void exec(bool ingame);

	void setPlayerNickIsChanged(bool theValue)
	{
		playerNickIsChanged = theValue;
	}
	bool getPlayerNickIsChanged() const
	{
		return playerNickIsChanged;
	}

	bool getSettingsCorrect() const
	{
		return settingsCorrect;
	}

	void setGuiLog(guiLog* l)
	{
		myGuiLog = l;
	}

public slots:

	void isAccepted();
	void playerNickChanged()
	{
		setPlayerNickIsChanged(true);
	};
	void setFlipsidePicFileName();
	void setLogDir();
	void setAvatarFile0();
	void setAvatarFile1();
	void setAvatarFile2();
	void setAvatarFile3();
	void setAvatarFile4();
	void setAvatarFile5();
	void setAvatarFile6();
	void setAvatarFile7();
	void setAvatarFile8();
	void setAvatarFile9();

	void callManualBlindsOrderDialog();
	void callNetManualBlindsOrderDialog();

	void callSelectAvatarDialog()
	{
		mySelectAvatarDialogImpl->exec();
	}

	void clearInternetGamePassword(bool);

// 	void checkProperNetFirstSmallBlind(int);
// 	void checkProperFirstSmallBlind(int);
	void setFirstSmallBlindMargin() ;

	void setLanguageChanged(int);

	void toggleGroupBoxAutomaticServerConfig(bool /*toggleState*/);
	void toggleGroupBoxManualServerConfig(bool /*toggleState*/);

	void setSelectedGameTableStyleActivated();
	void showCurrentGameTableStylePreview();
	void addGameTableStyle();
	void removeGameTableStyle();

	void setSelectedCardDeckStyleActivated();
	void showCurrentCardDeckStylePreview();
	void addCardDeckStyle();
	void removeCardDeckStyle();
	void removePlayerFromIgnoredPlayersList();
	void resetSettings();

private:

	bool playerNickIsChanged;
	bool settingsCorrect;

	bool myAfterMBAlwaysDoubleBlinds;
	bool myNetAfterMBAlwaysDoubleBlinds;
	bool myAfterMBAlwaysRaiseAbout;
	bool myNetAfterMBAlwaysRaiseAbout;
	bool myAfterMBStayAtLastBlind;
	bool myNetAfterMBStayAtLastBlind;
	int myAfterMBAlwaysRaiseValue;
	int myNetAfterMBAlwaysRaiseValue;
	std::list<int> myManualBlindsList;
	std::list<int> myNetManualBlindsList;
	std::list<std::string> myGameTableStylesList;
	std::list<std::string> myCardDeckStylesList;

	ConfigFile* myConfig;
	selectAvatarDialogImpl *mySelectAvatarDialogImpl;
	manualBlindsOrderDialogImpl *myManualBlindsOrderDialog;

	QString myAppDataPath;

	bool languageIsChanged;
	bool calledIngame;
	int changedLanguageIndex;
	guiLog *myGuiLog;
	FlickCharm *fc;
};

#endif
