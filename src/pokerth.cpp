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
#include <boost/asio.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <qapplication.h>

#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif
#include <QtGui>
#include <QtCore>

#ifdef __APPLE__
#if QT_VERSION < 0x050000
#include <QMacStyle>
#endif
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

#ifdef __APPLE__
	// The following needs to be done before the application is created, otherwise loading platforms plugin fails.
	QDir dir(argv[0]);
	dir.cdUp();
	dir.cdUp();
	dir.cd("plugins");
	QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif

	/////// can be removed for non-qt-guis ////////////
#ifdef ANDROID
	QApplication a(argc, argv);
	a.setApplicationName("PokerTH");
#else
	SharedTools::QtSingleApplication a( "PokerTH", argc, argv );
	if (a.sendMessage("Wake up!")) {
		return 0;
	}
#endif

	//create defaultconfig
	ConfigFile *myConfig = new ConfigFile(argv[0], false);
	Log *myLog = new Log(myConfig);

	// set PlastiqueStyle even for mac-version to prevent artefacts on styled widgets
#if QT_VERSION < 0x050000
	a.setStyle(new QPlastiqueStyle);
#endif

	QString	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());
	//set QApplication default font

	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/n019003l.pfb");
	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/DejaVuSans-Bold.ttf");

#ifdef _WIN32
	QString font1String("QApplication, QWidget, QDialog { font-size: 12px; }");
#elif __APPLE__
	//            QString font1String("font-family: \"Lucida Grande\";");
	QString font1String("QApplication, QWidget, QDialog { font-size: 11px; }");
#elif ANDROID
	QString font1String("QApplication, QWidget, QDialog { font-family: \"Nimbus Sans L\"; font-size: 26px; }");
	QPalette p = a.palette();
	p.setColor(QPalette::Button, QColor::fromRgb(80,80,80));
	p.setColor(QPalette::Base, QColor::fromRgb(80,80,80));
	p.setColor(QPalette::Window, QColor::fromRgb(50,50,50));
	p.setColor(QPalette::ButtonText, QColor::fromRgb(255,255,255));
	p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor::fromRgb(130,130,130));
	p.setColor(QPalette::WindowText, QColor::fromRgb(255,255,255));
	p.setColor(QPalette::Disabled, QPalette::WindowText, QColor::fromRgb(100,100,100));
	p.setColor(QPalette::Text, QColor::fromRgb(255,255,255));
	p.setColor(QPalette::Disabled, QPalette::Text, QColor::fromRgb(100,100,100));
	p.setColor(QPalette::Link, QColor::fromRgb(192,192,255));
	p.setColor(QPalette::LinkVisited, QColor::fromRgb(192,192,255));
	a.setPalette(p);
#elif MAEMO
	QString font1String("QApplication, QWidget, QDialog { font-family: \"Nimbus Sans L\"; font-size: 22px; }");
	QPalette p = a.palette();
	p.setColor(QPalette::Button, QColor::fromRgb(80,80,80));
	p.setColor(QPalette::Base, QColor::fromRgb(80,80,80));
	p.setColor(QPalette::Window, QColor::fromRgb(50,50,50));
	p.setColor(QPalette::ButtonText, QColor::fromRgb(255,255,255));
	p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor::fromRgb(100,100,100));
	p.setColor(QPalette::WindowText, QColor::fromRgb(255,255,255));
	p.setColor(QPalette::Disabled, QPalette::WindowText, QColor::fromRgb(100,100,100));
	p.setColor(QPalette::Text, QColor::fromRgb(255,255,255));
	p.setColor(QPalette::Disabled, QPalette::Text, QColor::fromRgb(100,100,100));
	p.setColor(QPalette::Link, QColor::fromRgb(192,192,255));
	p.setColor(QPalette::LinkVisited, QColor::fromRgb(192,192,255));
	a.setPalette(p);
#else
	QString font1String("QApplication, QWidget, QDialog { font-family: \"Nimbus Sans L\"; font-size: 12px; }");
#endif
	a.setStyleSheet(font1String + " QDialogButtonBox, QMessageBox { dialogbuttonbox-buttons-have-icons: 1; dialog-ok-icon: url(:/gfx/dialog_ok_apply.png); dialog-cancel-icon: url(:/gfx/dialog_close.png); dialog-close-icon: url(:/gfx/dialog_close.png); dialog-yes-icon: url(:/gfx/dialog_ok_apply.png); dialog-no-icon: url(:/gfx/dialog_close.png) }");

#ifdef ANDROID
	//check if custom background pictures for the resolution are there. Otherwise create them!
	QString UserDataDir = QString::fromUtf8(myConfig->readConfigString("UserDataDir").c_str());
	QDesktopWidget dw;
	int screenWidth = dw.screenGeometry().width();
	int screenHeight = dw.screenGeometry().height();
	QString customStartWindowBgFileString(UserDataDir+"/startwindowbg10_"+QString::number(screenWidth)+"x"+QString::number(screenHeight)+".png");
	QString customWelcomePokerTHFileString(UserDataDir+"/welcomepokerth10_"+QString::number(screenWidth)+"x"+QString::number(screenHeight)+".png");
	QFile customStartWindowBgFile(customStartWindowBgFileString);
	QFile customWelcomePokerTHFile(customWelcomePokerTHFileString);

	QSplashScreen preSplashFirstRun;
	if(!customStartWindowBgFile.exists()) {

		//load preSplashPix to show that PokerTH is already running during first time pics calculation
		QPixmap prePixBase(":/gfx/logoChip3D.png");
		QPixmap prePix(300, 200);
		prePix.fill(Qt::transparent); // force alpha channel
		{
			QPainter painter(&prePix);
			painter.drawPixmap(0, 0, prePixBase);
			painter.setPen(Qt::white);
			painter.drawText(10, 160, "loading ...");
		}
		preSplashFirstRun.setPixmap(prePix);
		preSplashFirstRun.show();

		QPixmap pix(":/android/android-data/gfx/gui/misc/startwindowbg10_mobile.png");
		pix = pix.scaled(screenWidth, screenHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
		pix.save(customStartWindowBgFileString);
	}

	if(!customWelcomePokerTHFile.exists()) {
		QPixmap base(customStartWindowBgFileString);
		//scale overlay "have a lot of fun" at first
		QPixmap overlay(":/android/android-data/gfx/gui/misc/welcomepokerth10_mobile.png");
		overlay = overlay.scaled(screenWidth, screenHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
		QPixmap result(base.width(), base.height());
		result.fill(Qt::transparent); // force alpha channel
		{
			QPainter painter(&result);
			painter.drawPixmap(0, 0, base);
			painter.drawPixmap(0, 0, overlay);
		}
		result.save(customWelcomePokerTHFileString);
		preSplashFirstRun.hide();
	}

	QPixmap pixmap;
	if(customWelcomePokerTHFile.exists()) {
		pixmap.load(QFileInfo(customWelcomePokerTHFile).absoluteFilePath());
	} else {
		//if custom welcome pic could not be saved locally we need to scale it on the fly
		pixmap.load(":/android/android-data/gfx/gui/misc/welcomepokerth10_mobile.png");
		pixmap = pixmap.scaled(screenWidth, screenHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
	}

#else
	QPixmap pixmap(myAppDataPath + "gfx/gui/misc/welcomepokerth10_desktop.png");
#endif
	StartSplash splash(pixmap);
	if(!myConfig->readConfigInt("DisableSplashScreenOnStartup")) {
		splash.show();
		splash.showMessage(QString("Version %1").arg(POKERTH_BETA_RELEASE_STRING), 0x0042, QColor(255,255,255));
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
#ifdef ANDROID
	mainWin.show();
#else
	a.setActivationWindow(&mainWin, true);
#endif
	int retVal = a.exec();
	curl_global_cleanup();
	socket_cleanup();
	return retVal;
}
