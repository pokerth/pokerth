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
#include <net/socket_startup.h>

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

int main( int argc, char **argv )
{
	//ENABLE_LEAK_CHECK();

	//_CrtSetBreakAlloc(49937);

	socket_startup();
	
	/////// can be removed for non-qt-guis ////////////
	QApplication a( argc, argv );
#ifdef __APPLE__
	a.setStyle(new QMacStyle);
#else
	a.setStyle(new QPlastiqueStyle);
#endif
	Q_INIT_RESOURCE(resources);

	QString locale = QLocale::system().name();

   	QTranslator translator;
     	translator.load(QString(":/translations/resources/translations/pokerth_fr") /*+ locale*/);
    	a.installTranslator(&translator);
	///////////////////////////////////////////////////	

	GuiInterface *myGuiInterface = new GuiWrapper();
	Session theFirst(myGuiInterface);
	myGuiInterface->setSession(&theFirst);

	int retVal = a.exec();

	socket_cleanup();
	return retVal;
}
