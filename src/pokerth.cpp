/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/
#include <boost/asio.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <qapplication.h>
#include <QtGui>
#include <QtCore>

#ifdef __APPLE__
#include <QMacStyle>
#endif

#include <curl/curl.h>

#include "session.h"
#include "startwindowimpl.h"
#include "configfile.h"
#include "log.h"
#include "startsplash.h"
#include "game_defs.h"
#include <net/socket_startup.h>
#include <third_party/qtsingleapplication/qtsingleapplication.h>

#ifdef _MSC_VER
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define ENABLE_LEAK_CHECK() \
			{ \
				int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); \
				tmpFlag |= _CRTDBG_LEAK_CHECK_DF; \
				_CrtSetDbgFlag(tmpFlag); \
			}
#endif
#endif

#ifndef ENABLE_LEAK_CHECK
#define ENABLE_LEAK_CHECK()
#endif

using namespace std;

class startWindowImpl;
class Game;

int main( int argc, char **argv )
{

	//ENABLE_LEAK_CHECK();

	//_CrtSetBreakAlloc(49937);
	socket_startup();
	curl_global_init(CURL_GLOBAL_NOTHING);

	/////// can be removed for non-qt-guis ////////////
	QtSingleApplication a( argc, argv );

    if (a.sendMessage("Wake up!")) {
		return 0;
	}

#ifdef __APPLE__
	// The following needs to be done directly after the application is created.
	QDir dir(QApplication::applicationDirPath());
	dir.cdUp();
	dir.cd("plugins");
	QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif

	//create defaultconfig
	ConfigFile *myConfig = new ConfigFile(argv[0], false);
	Log *myLog = new Log(myConfig);

        // set PlastiqueStyle even for mac-version to prevent artefacts on styled widgets
        a.setStyle(new QPlastiqueStyle);

        QString	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());
        //set QApplication default font

	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/n019003l.pfb");
	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/VeraBd.ttf");
	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/c059013l.pfb");
	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/DejaVuSans-Bold.ttf");

#ifdef _WIN32
	QString font1String("QApplication, QWidget, QDialog { font-size: 12px; }");
#elif __APPLE__
//            QString font1String("font-family: \"Lucida Grande\";");
	QString font1String("QApplication, QWidget, QDialog { font-size: 11px; }");
#elif ANDROID
        QString font1String("QApplication, QWidget, QDialog { font-family: \"Nimbus Sans L\"; font-size: 26px; }");
#else
        QString font1String("QApplication, QWidget, QDialog { font-family: \"Nimbus Sans L\"; font-size: 12px; }");
#endif
	a.setStyleSheet(font1String + " QDialogButtonBox, QMessageBox { dialogbuttonbox-buttons-have-icons: 1; dialog-ok-icon: url(:/gfx/dialog_ok_apply.png); dialog-cancel-icon: url(:/gfx/dialog_close.png); dialog-close-icon: url(:/gfx/dialog_close.png); dialog-yes-icon: url(:/gfx/dialog_ok_apply.png); dialog-no-icon: url(:/gfx/dialog_close.png) }");

        QPixmap pixmap(myAppDataPath + "gfx/gui/misc/welcomepokerth.png");
#ifdef ANDROID
        QDesktopWidget dw;
        pixmap = pixmap.scaled(dw.screenGeometry().width(), dw.screenGeometry().height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
#endif
        StartSplash splash(pixmap);
        if(!myConfig->readConfigInt("DisableSplashScreenOnStartup")) {
                splash.show();
		splash.showMessage(QString("Version %1").arg(POKERTH_BETA_RELEASE_STRING), 0x0042, QColor(240,240,240));
        }

	//Set translations
	QTranslator qtTranslator;
	qtTranslator.load(QString(myAppDataPath +"translations/qt_") + QString::fromStdString(myConfig->readConfigString("Language")));
	a.installTranslator(&qtTranslator);

	QTranslator translator;
	translator.load(QString(myAppDataPath +"translations/pokerth_") + QString::fromStdString(myConfig->readConfigString("Language")));
	a.installTranslator(&translator);

	qRegisterMetaType<unsigned>("unsigned");
	qRegisterMetaType<boost::shared_ptr<Game> >("boost::shared_ptr<Game>");
	qRegisterMetaType<ServerStats>("ServerStats");
	qRegisterMetaType<DenyGameInvitationReason>("DenyGameInvitationReason");
	///////////////////////////////////////////////////

    startWindowImpl mainWin(myConfig,myLog);
    a.setActivationWindow(&mainWin, true);

	int retVal = a.exec();

	curl_global_cleanup();
	socket_cleanup();


	return retVal;

}
