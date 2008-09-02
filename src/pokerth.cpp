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

#include <qapplication.h>

#ifdef __APPLE__
	#include <QMacStyle>
#endif

#include "session.h"
#include "startwindowimpl.h"
#include "configfile.h"
#include "startsplash.h"
#include "game_defs.h"
#include <net/socket_startup.h>

#include <curl/curl.h>
#include <QtGui>
#include <QtCore>

#include <iostream>
#include <cstdlib>
#include <ctime>

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

//Uncomment this for RELEASE
// #include <QtPlugin>
// Q_IMPORT_PLUGIN(qjpeg)
// Q_IMPORT_PLUGIN(qgif)


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
	QApplication a( argc, argv );

	//create defaultconfig
	ConfigFile *myConfig = new ConfigFile(argv[0], false);

	// set PlastiqueStyle even for mac-version to prevent artefacts on styled widgets
	a.setStyle(new QPlastiqueStyle);

	QString	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());
	//set QApplication default font	

	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/n019003l.pfb");
	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/VeraBd.ttf");	
	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/c059013l.pfb");

#ifdef _WIN32
	QString font1String("font-family: \"Arial\";");
	a.setStyleSheet("QApplication, QWidget, QDialog { " + font1String + " font-size: 12px; }");
#else 
// 	#ifdef __APPLE__
// 		QString font1String("font-family: \"Lucida Grande\";");
// 	#else 
	QString font1String("font-family: \"Nimbus Sans L\";");
// 	#endif
	a.setStyleSheet("QApplication, QWidget, QDialog { " + font1String + " font-size: 12px; }");
#endif	

	QPixmap *pixmap = new QPixmap(myAppDataPath + "gfx/gui/misc/welcomepokerth.png");
     	StartSplash splash(*pixmap);
     	splash.show();
	splash.showMessage(QString("Version %1").arg(POKERTH_BETA_RELEASE_STRING), 0x0042, QColor(153,213,0));
	
	//Set translations
	QTranslator qtTranslator;
        qtTranslator.load(QString(myAppDataPath +"translations/qt_") + QString::fromStdString(myConfig->readConfigString("Language")));
        a.installTranslator(&qtTranslator);

	QTranslator translator;
	translator.load(QString(myAppDataPath +"translations/pokerth_") + QString::fromStdString(myConfig->readConfigString("Language")));
	a.installTranslator(&translator);
	

#ifdef __APPLE__
	QDir dir(QApplication::applicationDirPath());
	dir.cdUp();
	dir.cd("plugins");
	QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif

	qRegisterMetaType<unsigned>("unsigned");
	qRegisterMetaType<boost::shared_ptr<Game> >("boost::shared_ptr<Game>");
	qRegisterMetaType<ServerStats>("ServerStats");
	///////////////////////////////////////////////////	


	boost::shared_ptr<startWindowImpl>(new startWindowImpl(myConfig));
// 	boost::shared_ptr<GuiInterface> myGuiInterface(new GuiWrapper(myConfig));
// 	{
// 		boost::shared_ptr<Session> session(new Session(myGuiInterface.get(), myConfig));
// 		session->init(); // TODO handle error
// 		myGuiInterface->setSession(session);
// 	}
	
	int retVal = a.exec();
	
	curl_global_cleanup();
	socket_cleanup();
	return retVal;
	
}
