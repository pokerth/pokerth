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

#include <iostream>

/////// can be removed for non-qt-guis ////////////
#include <qapplication.h>

#ifdef __APPLE__
	#include <QMacStyle>
#endif
///////////////////////////////////////////////////

#include "session.h"
#include "guiwrapper.h"
#include "configfile.h"
#include <net/socket_startup.h>

#include <QtGui>
#include <QtCore>

#include "qthelper.h"

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

using namespace std;

class GuiWrapper;
class Game;

int main( int argc, char **argv )
{
	//create defaultconfig
	ConfigFile *myConfig = new ConfigFile(argc, argv);
	QtHelper *myQtHelper = new QtHelper;
	//ENABLE_LEAK_CHECK();

	//_CrtSetBreakAlloc(49937);

	socket_startup();
	
	/////// can be removed for non-qt-guis ////////////
	QApplication a( argc, argv );

	// set PlastiqueStyle even for mac-version to prevent artefacts on styled widgets
	a.setStyle(new QPlastiqueStyle);

	//set QApplication default font	
#ifdef _WIN32
	QString font1String("font-family: \"Arial\";");

#else 
	QFontDatabase::addApplicationFont (myQtHelper->getDataPath() +"fonts/n019003l.pfb");
	QFontDatabase::addApplicationFont (myQtHelper->getDataPath() +"fonts/VeraBd.ttf");

	QString font1String("font-family: \"Nimbus Sans L\";");
#endif
	a.setStyleSheet("QApplication { " + font1String + " font-size: 12px; }");

	
	QTranslator qtTranslator;
        qtTranslator.load(QString(myQtHelper->getDataPath() +"translations/qt_") + QString::fromStdString(myConfig->readConfigString("Language")));
        a.installTranslator(&qtTranslator);

	QTranslator translator;
	translator.load(QString(myQtHelper->getDataPath() +"translations/pokerth_") + QString::fromStdString(myConfig->readConfigString("Language")));
	a.installTranslator(&translator);
	

	qRegisterMetaType<unsigned>("unsigned");
	qRegisterMetaType<boost::shared_ptr<Game> >("boost::shared_ptr<Game>");
	///////////////////////////////////////////////////	


	boost::shared_ptr<GuiInterface> myGuiInterface(new GuiWrapper(myConfig));
	{
		boost::shared_ptr<Session> session(new Session(myGuiInterface.get(), myConfig));
		myGuiInterface->setSession(session);
	}

	int retVal = a.exec();

	socket_cleanup();
	return retVal;
}
