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
#include <boost/shared_ptr.hpp>
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
	configRev = 18;

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
			myConfigState = NONEXISTING;
			updateConfig(myConfigState); 
		}
		else { 
		//Prüfen ob die Revision stimmt ansonsten löschen und neue anlegen 
			int temp = 0;
			TiXmlHandle docHandle( &doc );		
			TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( "ConfigRevision" ).ToElement();
			if ( conf ) { conf->QueryIntAttribute("value", &temp ); }
			if (temp < configRev) { /*löschen()*/ 
				myConfigState = OLD;
				updateConfig(myConfigState) ;
			}
		}
	}
}


ConfigFile::~ConfigFile()
{
}

void ConfigFile::updateConfig(ConfigState myConfigState) {
	
	size_t i;

	QtToolsInterface *myQtToolsInterface = new QtToolsWrapper;
	
	vector<ConfigInfo> configList;

	ostringstream tempIntToString;
	tempIntToString << configRev;
	configList.push_back(ConfigInfo("ConfigRevision", CONFIG_TYPE_INT, tempIntToString.str()));
	configList.push_back(ConfigInfo("ShowLeftToolBox", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowRightToolBox", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowStatusbarMessages", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowIntro", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowFadeOutCardsAnimation", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowFlipCardsAnimation", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("FlipsideTux", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("FlipsideOwn", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("FlipsideOwnFile", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("NumberOfPlayers", CONFIG_TYPE_INT, "7"));
	configList.push_back(ConfigInfo("StartCash", CONFIG_TYPE_INT, "3000"));
	configList.push_back(ConfigInfo("SmallBlind", CONFIG_TYPE_INT, "10"));
	configList.push_back(ConfigInfo("HandsBeforeRaiseSmallBlind", CONFIG_TYPE_INT, "8"));
	configList.push_back(ConfigInfo("GameSpeed", CONFIG_TYPE_INT, "4"));
	configList.push_back(ConfigInfo("EngineVersion", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("PauseBetweenHands", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ShowGameSettingsDialogOnNewGame", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("NetNumberOfPlayers", CONFIG_TYPE_INT, "7"));
	configList.push_back(ConfigInfo("NetStartCash", CONFIG_TYPE_INT, "3000"));
	configList.push_back(ConfigInfo("NetSmallBlind", CONFIG_TYPE_INT, "10"));
	configList.push_back(ConfigInfo("NetHandsBeforeRaiseSmallBlind", CONFIG_TYPE_INT, "8"));
	configList.push_back(ConfigInfo("NetGameSpeed", CONFIG_TYPE_INT, "4"));
	configList.push_back(ConfigInfo("NetEngineVersion", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ServerUseIpv6", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerPort", CONFIG_TYPE_INT, "7234"));
	configList.push_back(ConfigInfo("MyName", CONFIG_TYPE_STRING, "Human Player"));
	configList.push_back(ConfigInfo("MyAvatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent1Name", CONFIG_TYPE_STRING, "Player 1"));
	configList.push_back(ConfigInfo("Opponent1Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent2Name", CONFIG_TYPE_STRING, "Player 2"));
	configList.push_back(ConfigInfo("Opponent2Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent3Name", CONFIG_TYPE_STRING, "Player 3"));
	configList.push_back(ConfigInfo("Opponent3Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent4Name", CONFIG_TYPE_STRING, "Player 4"));
	configList.push_back(ConfigInfo("Opponent4Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent5Name", CONFIG_TYPE_STRING, "Player 5"));
	configList.push_back(ConfigInfo("Opponent5Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent6Name", CONFIG_TYPE_STRING, "Player 6"));
	configList.push_back(ConfigInfo("Opponent6Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("LogOnOff", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("LogDir", CONFIG_TYPE_STRING, logDir));
	configList.push_back(ConfigInfo("LogStoreDuration", CONFIG_TYPE_INT, "2"));
	configList.push_back(ConfigInfo("LogInterval", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("DataDir", CONFIG_TYPE_STRING, dataDir));


	if(myConfigState == NONEXISTING) {
		
		//Create a new ConfigFile!
		TiXmlDocument doc;  
		TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", ""); 
		doc.LinkEndChild( decl );  
		
		TiXmlElement * root = new TiXmlElement( "PokerTH" );  
		doc.LinkEndChild( root );  		
		
		TiXmlElement * config;
		config = new TiXmlElement( "Configuration" );  
		root->LinkEndChild( config );  

		for (i=0; i<configList.size(); i++) {
			TiXmlElement *tmpElement = new TiXmlElement(configList[i].name);
			config->LinkEndChild( tmpElement );
			tmpElement->SetAttribute("value", myQtToolsInterface->stringToUtf8(configList[i].defaultValue));
		}
			
		doc.SaveFile( configFileName );
	}


	if(myConfigState == OLD) {
	
		TiXmlDocument oldDoc(configFileName); 
		
		//load the old one
		if(oldDoc.LoadFile()) {
			
			string tempString("");
			 
			TiXmlDocument newDoc;
			
			//Create the new one
			TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", ""); 
			newDoc.LinkEndChild( decl );  
				
			TiXmlElement * root = new TiXmlElement( "PokerTH" );  
			newDoc.LinkEndChild( root );  		
			
			TiXmlElement * config;
			config = new TiXmlElement( "Configuration" );  
			root->LinkEndChild( config ); 

			TiXmlElement * confElement0 = new TiXmlElement( "ConfigRevision" ); 
			config->LinkEndChild( confElement0 );
			confElement0->SetAttribute("value", configRev);

			TiXmlHandle docHandle( &oldDoc );	

			// not i=0 because ConfigRevision is already set ^^
			for (i=1; i<configList.size(); i++) {	
				TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( configList[i].name ).ToElement();
				
				if ( conf ) {
					// if element is already there --> take over the saved values
					TiXmlElement *tmpElement = new TiXmlElement(configList[i].name);
					config->LinkEndChild( tmpElement );
				
					const char *tmpStr = conf->Attribute("value");
					if (tmpStr) tempString = tmpStr;
					tmpElement->SetAttribute("value", myQtToolsInterface->stringToUtf8(tempString));
				}	
				else {
					// if element is not there --> set it with defaultValue
					TiXmlElement *tmpElement = new TiXmlElement(configList[i].name);
					config->LinkEndChild( tmpElement );
					tmpElement->SetAttribute("value", myQtToolsInterface->stringToUtf8(configList[i].defaultValue));
				}
			}
			
			newDoc.SaveFile( configFileName );
		}
		else { 	cout << "cannot update config file. did not found it" << endl;	}

		
	}
	
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
