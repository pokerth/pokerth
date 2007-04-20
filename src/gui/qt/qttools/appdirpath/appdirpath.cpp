/***************************************************************************
 *   Copyright (C) 2006 by Felix Hammer   *
 *   f.hammer@web.de   *
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
#include "appdirpath.h"

#include <QtCore>

#include <cstdlib>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#endif

using namespace std;

AppDirPath::AppDirPath()
{
#ifdef _WIN32
	// Pfad und Dateinamen setzen
	const char *appDataPath = getenv("AppData");
	if (appDataPath && appDataPath[0] != 0) {
		configFileName = appDataPath; 
	}
	else {
		const int MaxPathSize = 1024;
		char curDir[MaxPathSize + 1];
		curDir[0] = 0;
		_getcwd(curDir, MaxPathSize);
		curDir[MaxPathSize] = 0;
		configFileName = curDir;
		// Testen ob das Verzeichnis beschreibbar ist
		ofstream tmpFile;
		const char *tmpFileName = "pokerth_test.tmp";
		tmpFile.open((configFileName + "\\" + tmpFileName).c_str());
		if (tmpFile) {
			// Erfolgreich, Verzeichnis beschreibbar.
			// Datei wieder loeschen.
			tmpFile.close();
			remove((configFileName + "\\" + tmpFileName).c_str());
		}
		else {
			// Fehlgeschlagen, Verzeichnis nicht beschreibbar
			curDir[0] = 0;
			GetTempPathA(MaxPathSize, curDir);
			curDir[MaxPathSize] = 0;
			configFileName = curDir;
		}
	}
#else
	const char *homePath = getenv("HOME");
	configFileName = homePath;
#endif
	//UTF8 !!!!
	QString tmpString = QString::fromStdString(configFileName);
	configFileName = tmpString.toUtf8().constData();
	//UTF8 !!!!
}

AppDirPath::~AppDirPath()
{
}

