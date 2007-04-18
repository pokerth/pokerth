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
#include "configfile.h"

#include <cstdlib>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#define MODUS 0711

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#endif

using namespace std;

ConfigFile::ConfigFile(std::string path)
: myPath(path)
{	
}

ConfigFile::~ConfigFile()
{
}

string ConfigFile::readConfigString(string varName)
{
  	string tempString("");

	TiXmlDocument doc(myPath.c_str()); 
	if(!doc.LoadFile()) {	cout << "Could Not Load Config-File!!! " << myPath << "\n"; }
	TiXmlHandle docHandle( &doc );		

	TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( varName ).ToElement();
	if ( conf ) { 
		const char *tmpStr = conf->Attribute("value");
		if (tmpStr) tempString = tmpStr;
        } /*else {
		//Wenn nicht gefunden eines neues Anlegen
		TiXmlElement* config = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).ToElement();	
		if ( config ) { 		
			TiXmlElement * confElement1 = new TiXmlElement( varName ); 
			config->LinkEndChild( confElement1 );
			confElement1->SetAttribute("value", defaultValue);
			if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << myPath << "\n"; }

			return readConfigString(varName, defaultValue);
		}
	}*/

	return tempString;
 }

int ConfigFile::readConfigInt(string varName)
{
  	int tempInt=0;
// 	cout << varName << " : " << tempInt << "\n";
	TiXmlDocument doc(myPath.c_str()); 
	if(!doc.LoadFile()) {	cout << "Could Not Load Config-File!!! " << myPath << "\n"; }
	TiXmlHandle docHandle( &doc );		
	
	TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( varName ).ToElement();
	if ( conf ) {
// 		cout << varName << " : " << tempInt << "\n";
		conf->QueryIntAttribute("value", &tempInt );
// 		cout << varName << " : " << tempInt << "\n";
        } /*else {
// 		Wenn nicht gefunden eines neues Anlegen
		TiXmlElement* config = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).ToElement();	
		if ( config ) { 		
			TiXmlElement * confElement1 = new TiXmlElement( varName ); 
			config->LinkEndChild( confElement1 );
			confElement1->SetAttribute("value", defaultValue);
			if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << myPath << "\n"; }

			return readConfigInt(varName, defaultValue);
		}
	}*/

	return tempInt;
}


void ConfigFile::writeConfigInt(string varName, int varCont)
 {	

	TiXmlDocument doc(myPath.c_str()); 
	if(!doc.LoadFile()) {	cout << "Could Not Load Config-File!!! " << myPath << "\n"; }
	TiXmlHandle docHandle( &doc );		

	TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( varName ).ToElement();
	if ( conf ) {
		conf->SetAttribute("value", varCont );
		if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << myPath << "\n"; }
        } else {
		//Wenn nicht gefunden eines neues Anlegen
		TiXmlElement* config = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).ToElement();	

		if ( config ) { 		

			TiXmlElement * confElement1 = new TiXmlElement( varName ); 
			config->LinkEndChild( confElement1 );
			confElement1->SetAttribute("value", varCont);

			if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << myPath << "\n"; }
		}
	}
	if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << myPath << "\n"; }
}

void ConfigFile::writeConfigString(string varName, string varCont)
 {
	
	TiXmlDocument doc(myPath.c_str()); 
	if(!doc.LoadFile()) {	cout << "Could Not Load Config-File!!! " << myPath << "\n"; }
	TiXmlHandle docHandle( &doc );		

	TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( varName ).ToElement();
	if ( conf ) {
		conf->SetAttribute("value", varCont );
	} else {
		//Wenn nicht gefunden eines neues Anlegen
		TiXmlElement* config = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).ToElement();	
		if ( config ) { 		
			TiXmlElement * confElement1 = new TiXmlElement( varName ); 
			config->LinkEndChild( confElement1 );
			confElement1->SetAttribute("value", varCont);
			if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << myPath << "\n"; }
		}
	}
	if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << myPath << "\n"; }
	
        
}
