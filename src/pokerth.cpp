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

//wenn die Interfaces fertig sind werden die folgenden beiden includes nicht mehr gebraucht
#include "session.h"
#include "mainwindowimpl.h"

#include "guiwrapper.h"

class GuiWrapper;

int main( int argc, char **argv )
{

	srand( time(0) );

	QApplication a( argc, argv );
	Q_INIT_RESOURCE(deck);
	
	//wenn die Interfaces fertig sind werden die folgenden beiden Objekte nicht mehr gebraucht da sie von GuiWrapper und EngineWrapper erstellt werden.
	
	mainWindowImpl *myW = new mainWindowImpl;
	myW->show();

	GuiInterface *myGuiInterface = new GuiWrapper(myW);


	Session theFirst(myW, myGuiInterface);


	return a.exec();
}
