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

#include <qttoolswrapper.h>

#include <cstdlib>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#define MODUS 0711

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#endif

class QtToolsWrapper;

using namespace std;

ConfigFile::ConfigFile(bool configFirstStart)
{
	// !!!! Revisionsnummer der Configdefaults !!!!!
	configRev = 17;

	// Pfad und Dateinamen setzen
#ifdef _WIN32
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
	//define app-dir
	configFileName += "\\pokerth\\";
	////define log-dir
	logDir = configFileName;
	logDir += "log-files\\";
	////define data-dir
	dataDir = configFileName;
	dataDir += "data\\";
	//create directories on first start of app
	if (configFirstStart) {
		mkdir(configFileName.c_str());
		mkdir(logDir.c_str());
		mkdir(dataDir.c_str());
	}
	
#else
	//define app-dir
	const char *homePath = getenv("HOME");
	if(homePath) {
		configFileName = homePath;
		configFileName += "/.pokerth/";
		////define log-dir
		logDir = configFileName;
		logDir += "log-files/";
		////define data-dir
		dataDir = configFileName;
		dataDir += "data/";
		//create directories on first start of app
		if (configFirstStart) {
			mkdir(configFileName.c_str(), MODUS) ;
			mkdir(logDir.c_str(), MODUS);
			mkdir(dataDir.c_str(), MODUS);
		}
	}
#endif
	configFileName += "config.xml";
	
	if (configFirstStart) {
		//Prüfen ob Configfile existiert --> sonst anlegen
		TiXmlDocument doc(configFileName); 
		if(!doc.LoadFile()){ 
			configState = nonexisting;
			updateConfig(configState); 
		}
		else { 
		//Prüfen ob die Revision stimmt ansonsten löschen und neue anlegen 
			int temp = 0;
			TiXmlHandle docHandle( &doc );		
			TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( "ConfigRevision" ).ToElement();
			if ( conf ) { conf->QueryIntAttribute("value", &temp ); }
			if (temp < configRev) { /*löschen()*/ 
				configState = old;
				updateConfig(configState) ;
			}
		}
	}
}


ConfigFile::~ConfigFile()
{
}

void ConfigFile::updateConfig(CONFIGSTATE myConfigState) {
	
	QtToolsInterface *myQtToolsInterface = new QtToolsWrapper;
	
	if(myConfigState == old) {
		configFileName += "config.xml";
		TiXmlDocument doc(configFileName);
		TiXmlHandle docHandle( &doc );	 
		TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( "ConfigRevision" ).ToElement();
		
	}

	//Anlegen!
	TiXmlDocument doc;  
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", ""); 
	doc.LinkEndChild( decl );  
	
	TiXmlElement * root = new TiXmlElement( "PokerTH" );  
	doc.LinkEndChild( root );  		
	
	TiXmlElement * config;
	config = new TiXmlElement( "Configuration" );  
	root->LinkEndChild( config );  
		
	TiXmlElement * confElement0 = new TiXmlElement( "ConfigRevision" ); 
	config->LinkEndChild( confElement0 );
	confElement0->SetAttribute("value", configRev);
	TiXmlElement * confElement11 = new TiXmlElement( "ShowLeftToolBox" );
	config->LinkEndChild( confElement11 );
	confElement11->SetAttribute("value", 1);
	TiXmlElement * confElement22 = new TiXmlElement( "ShowRightToolBox" );
	config->LinkEndChild( confElement22 );
	confElement22->SetAttribute("value", 1);
	TiXmlElement * confElement43 = new TiXmlElement( "ShowStatusbarMessages" );
	config->LinkEndChild( confElement43 );
      	confElement43->SetAttribute("value", 1);
	TiXmlElement * confElement13 = new TiXmlElement( "ShowIntro" );
	config->LinkEndChild( confElement13 );
      	confElement13->SetAttribute("value", 1);
	TiXmlElement * confElement15 = new TiXmlElement( "ShowFadeOutCardsAnimation" );
	config->LinkEndChild( confElement15 );
      	confElement15->SetAttribute("value", 1);
	TiXmlElement * confElement19 = new TiXmlElement( "ShowFlipCardsAnimation" );
	config->LinkEndChild( confElement19 );
      	confElement19->SetAttribute("value", 1);
	TiXmlElement * confElement16 = new TiXmlElement( "FlipsideTux" );
	config->LinkEndChild( confElement16 );
      	confElement16->SetAttribute("value", 1);
	TiXmlElement * confElement17 = new TiXmlElement( "FlipsideOwn" );
	config->LinkEndChild( confElement17 );
      	confElement17->SetAttribute("value", 0);
	TiXmlElement * confElement18 = new TiXmlElement( "FlipsideOwnFile" );
	config->LinkEndChild( confElement18 );
      	confElement18->SetAttribute("value", "");
	
	TiXmlElement * confElement1 = new TiXmlElement( "NumberOfPlayers" ); 
	config->LinkEndChild( confElement1 );
	confElement1->SetAttribute("value", 7);
        TiXmlElement * confElement2 = new TiXmlElement( "StartCash" );
	config->LinkEndChild( confElement2 );
        confElement2->SetAttribute("value", 3000);
	TiXmlElement * confElement3 = new TiXmlElement( "SmallBlind" );
  	config->LinkEndChild( confElement3 );
        confElement3->SetAttribute("value", 10);
	TiXmlElement * confElement12 = new TiXmlElement( "HandsBeforeRaiseSmallBlind" );
	config->LinkEndChild( confElement12 );
      	confElement12->SetAttribute("value", 8);	
	TiXmlElement * confElement4 = new TiXmlElement( "GameSpeed" );
	config->LinkEndChild( confElement4 );
      	confElement4->SetAttribute("value", 4);
	TiXmlElement * confElement32 = new TiXmlElement( "EngineVersion" );
	config->LinkEndChild( confElement32 );
      	confElement32->SetAttribute("value", 0);
	TiXmlElement * confElement14 = new TiXmlElement( "PauseBetweenHands" );
	config->LinkEndChild( confElement14 );
      	confElement14->SetAttribute("value", 0);		
	TiXmlElement * confElement5 = new TiXmlElement( "ShowGameSettingsDialogOnNewGame" );
	config->LinkEndChild( confElement5 );
      	confElement5->SetAttribute("value", 1);		
	
	TiXmlElement * confElement23 = new TiXmlElement( "NetNumberOfPlayers" ); 
	config->LinkEndChild( confElement23 );
	confElement23->SetAttribute("value", 7);
        TiXmlElement * confElement24 = new TiXmlElement( "NetStartCash" );
	config->LinkEndChild( confElement24 );
        confElement24->SetAttribute("value", 2000);
	TiXmlElement * confElement25 = new TiXmlElement( "NetSmallBlind" );
  	config->LinkEndChild( confElement25 );
        confElement25->SetAttribute("value", 10);
	TiXmlElement * confElement26 = new TiXmlElement( "NetHandsBeforeRaiseSmallBlind" );
	config->LinkEndChild( confElement26 );
      	confElement26->SetAttribute("value", 9);	
	TiXmlElement * confElement27 = new TiXmlElement( "NetGameSpeed" );
	config->LinkEndChild( confElement27 );
      	confElement27->SetAttribute("value", 4);
	TiXmlElement * confElement33 = new TiXmlElement( "NetEngineVersion" );
	config->LinkEndChild( confElement33 );
      	confElement33->SetAttribute("value", 1);
	TiXmlElement * confElement28 = new TiXmlElement( "ServerPassword" );
	config->LinkEndChild( confElement28 );
      	confElement28->SetAttribute("value", "");
	TiXmlElement * confElement29 = new TiXmlElement( "ServerUseIpv6" );
	config->LinkEndChild( confElement29 );
      	confElement29->SetAttribute("value", 0);
	TiXmlElement * confElement30 = new TiXmlElement( "ServerPort" );
	config->LinkEndChild( confElement30 );
      	confElement30->SetAttribute("value", 7234);
	
	TiXmlElement * confElement6 = new TiXmlElement( "MyName" );
	config->LinkEndChild( confElement6 );
      	confElement6->SetAttribute("value", "Human Player");		
	TiXmlElement * confElement34 = new TiXmlElement( "MyAvatar" );
	config->LinkEndChild( confElement34 );
      	confElement34->SetAttribute("value", "");		
	TiXmlElement * confElement7 = new TiXmlElement( "Opponent1Name" );
	config->LinkEndChild( confElement7 );
      	confElement7->SetAttribute("value", "Player 1");
	TiXmlElement * confElement35 = new TiXmlElement( "Opponent1Avatar" );
	config->LinkEndChild( confElement35 );
      	confElement35->SetAttribute("value", "");
	TiXmlElement * confElement8 = new TiXmlElement( "Opponent2Name" );
	config->LinkEndChild( confElement8 );
      	confElement8->SetAttribute("value", "Player 2");
	TiXmlElement * confElement36 = new TiXmlElement( "Opponent2Avatar" );
	config->LinkEndChild( confElement36 );
      	confElement36->SetAttribute("value", "");		
	TiXmlElement * confElement9 = new TiXmlElement( "Opponent3Name" );
	config->LinkEndChild( confElement9 );
      	confElement9->SetAttribute("value", "Player 3");
	TiXmlElement * confElement37 = new TiXmlElement( "Opponent3Avatar" );
	config->LinkEndChild( confElement37 );
      	confElement37->SetAttribute("value", "");
	TiXmlElement * confElement10 = new TiXmlElement( "Opponent4Name" );
	config->LinkEndChild( confElement10 );
      	confElement10->SetAttribute("value", "Player 4");
	TiXmlElement * confElement38 = new TiXmlElement( "Opponent4Avatar" );
	config->LinkEndChild( confElement38 );
      	confElement38->SetAttribute("value", "");
	TiXmlElement * confElement39 = new TiXmlElement( "Opponent5Name" );
	config->LinkEndChild( confElement39 );
      	confElement39->SetAttribute("value", "Player 5");
	TiXmlElement * confElement40 = new TiXmlElement( "Opponent5Avatar" );
	config->LinkEndChild( confElement40 );
      	confElement40->SetAttribute("value", "");		
	TiXmlElement * confElement41 = new TiXmlElement( "Opponent6Name" );
	config->LinkEndChild( confElement41 );
      	confElement41->SetAttribute("value", "Player 6");
	TiXmlElement * confElement42 = new TiXmlElement( "Opponent6Avatar" );
	config->LinkEndChild( confElement42 );
      	confElement42->SetAttribute("value", "");

	TiXmlElement * confElement20 = new TiXmlElement( "LogDir" );
	config->LinkEndChild( confElement20 );
      	confElement20->SetAttribute("value", myQtToolsInterface->stringToUtf8(logDir));
	TiXmlElement * confElement21 = new TiXmlElement( "LogStoreDuration" );
	config->LinkEndChild( confElement21 );
      	confElement21->SetAttribute("value", 2);

	TiXmlElement * confElement31 = new TiXmlElement( "DataDir" );
	config->LinkEndChild( confElement31 );
      	confElement31->SetAttribute("value", myQtToolsInterface->stringToUtf8(dataDir));
		
	doc.SaveFile( configFileName );

	delete myQtToolsInterface;
	myQtToolsInterface = 0;

}

string ConfigFile::readConfigString(string varName)
{
  	string tempString("");

	TiXmlDocument doc(configFileName); 
	if(!doc.LoadFile()) {	cout << "Could Not Load Config-File!!! " << configFileName << "\n"; }
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
			if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << configFileName << "\n"; }

			return readConfigString(varName, defaultValue);
		}
	}*/

	return tempString;
 }

int ConfigFile::readConfigInt(string varName)
{
  	int tempInt=0;
// 	cout << varName << " : " << tempInt << "\n";
	TiXmlDocument doc(configFileName); 
	if(!doc.LoadFile()) {	cout << "Could Not Load Config-File!!! " << configFileName << "\n"; }
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
			if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << configFileName << "\n"; }

			return readConfigInt(varName, defaultValue);
		}
	}*/

	return tempInt;
}


void ConfigFile::writeConfigInt(string varName, int varCont)
 {	

	TiXmlDocument doc(configFileName); 
	if(!doc.LoadFile()) {	cout << "Could Not Load Config-File!!! " << configFileName << "\n"; }
	TiXmlHandle docHandle( &doc );		

	TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( varName ).ToElement();
	if ( conf ) {
		conf->SetAttribute("value", varCont );
		if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << configFileName << "\n"; }
        } else {
		//Wenn nicht gefunden eines neues Anlegen
		TiXmlElement* config = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).ToElement();	

		if ( config ) { 		

			TiXmlElement * confElement1 = new TiXmlElement( varName ); 
			config->LinkEndChild( confElement1 );
			confElement1->SetAttribute("value", varCont);

			if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << configFileName << "\n"; }
		}
	}
	if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << configFileName << "\n"; }
}

void ConfigFile::writeConfigString(string varName, string varCont)
 {
	
	TiXmlDocument doc(configFileName); 
	if(!doc.LoadFile()) {	cout << "Could Not Load Config-File!!! " << configFileName << "\n"; }
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
			if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << configFileName << "\n"; }
		}
	}
	if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << configFileName << "\n"; }
	
        
}
