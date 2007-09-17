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

#define MODUS 0711

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#endif

#include <boost/filesystem.hpp>

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

class QtToolsWrapper;


ConfigFile::ConfigFile(int argc, char **argv) : noWriteAccess(0)
{

	myArgv = argv;
	int i;

	myQtToolsInterface = new QtToolsWrapper;

	for (i=1; i<argc; i++) {
		if(strcmp(argv[i], "--nowriteaccess") == 0) { noWriteAccess = 1; }
	}
	// !!!! Revisionsnummer der Configdefaults !!!!!
	configRev = 35;

	//standard defaults
	logOnOffDefault = "1";
	claNoWriteAccess = "0";


	if(!noWriteAccess) {
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
		////define cache-dir
		cacheDir = configFileName;
		cacheDir += "cache\\";
		//create directories on first start of app
		mkdir(configFileName.c_str());
		mkdir(logDir.c_str());
		mkdir(dataDir.c_str());
		mkdir(cacheDir.c_str());
	
		
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
			////define cache-dir
			cacheDir = configFileName;
			cacheDir += "cache/";
			//create directories on first start of app
			mkdir(configFileName.c_str(), MODUS) ;
			mkdir(logDir.c_str(), MODUS);
			mkdir(dataDir.c_str(), MODUS);
			mkdir(cacheDir.c_str(), MODUS);
		}
	
#endif
	}
	else {
	//no writeaccess
		configFileName = "";
		logDir = "";
		dataDir = "";
		logOnOffDefault = "0";
		claNoWriteAccess = "1";
	}

	boost::filesystem::path startPath(argv[0]);

	ostringstream tempIntToString;
	tempIntToString << configRev;
	configList.push_back(ConfigInfo("ConfigRevision", CONFIG_TYPE_INT, tempIntToString.str()));
	configList.push_back(ConfigInfo("AppDataDir", CONFIG_TYPE_STRING, myQtToolsInterface->getDataPathStdString(startPath.remove_leaf().directory_string())));
	configList.push_back(ConfigInfo("Language", CONFIG_TYPE_INT, myQtToolsInterface->getDefaultLanguage()));
	configList.push_back(ConfigInfo("ShowLeftToolBox", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowRightToolBox", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowStatusbarMessages", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowIntro", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowFadeOutCardsAnimation", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowFlipCardsAnimation", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowBlindButtons", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("AntiPeekMode", CONFIG_TYPE_INT, "0"));		
	configList.push_back(ConfigInfo("AlternateFKeysUserActionMode", CONFIG_TYPE_INT, "0"));		
	configList.push_back(ConfigInfo("PlaySoundEffects", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("SoundVolume", CONFIG_TYPE_INT, "8"));
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
	configList.push_back(ConfigInfo("NetTimeOutPlayerAction", CONFIG_TYPE_INT, "20"));
	configList.push_back(ConfigInfo("ServerPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ServerUseIpv6", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerUseSctp", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerPort", CONFIG_TYPE_INT, "7234"));
	configList.push_back(ConfigInfo("InternetServerAddress", CONFIG_TYPE_STRING, "pokerth.dyndns.org"));
	configList.push_back(ConfigInfo("InternetServerPort", CONFIG_TYPE_INT, "7234"));
	configList.push_back(ConfigInfo("InternetServerPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("InternetServerUseIpv6", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("InternetServerUseSctp", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("UseInternetGamePassword", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("InternetGamePassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("IRCServerAddress", CONFIG_TYPE_STRING, "chat.freenode.net"));
	configList.push_back(ConfigInfo("IRCServerPort", CONFIG_TYPE_INT, "6667"));
	configList.push_back(ConfigInfo("IRCChannel", CONFIG_TYPE_STRING, "#pokerth"));
	configList.push_back(ConfigInfo("IRCServerUseIpv6", CONFIG_TYPE_INT, "0"));
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
	configList.push_back(ConfigInfo("LogOnOff", CONFIG_TYPE_INT, logOnOffDefault));
	configList.push_back(ConfigInfo("LogDir", CONFIG_TYPE_STRING, logDir));
	configList.push_back(ConfigInfo("LogStoreDuration", CONFIG_TYPE_INT, "2"));
	configList.push_back(ConfigInfo("LogInterval", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("UserDataDir", CONFIG_TYPE_STRING, dataDir));
	configList.push_back(ConfigInfo("CacheDir", CONFIG_TYPE_STRING, cacheDir));	
	configList.push_back(ConfigInfo("CLA_NoWriteAccess", CONFIG_TYPE_INT, claNoWriteAccess));

	//fill tempList firstTime
	configBufferList = configList;

// 	cout << configTempList[3].name << " " << configTempList[10].defaultValue << endl;

	if(!noWriteAccess) {
		configFileName += "config.xml";
		
		//Prüfen ob Configfile existiert --> sonst anlegen
		TiXmlDocument doc(configFileName); 
		if(!doc.LoadFile()){ 
			myConfigState = NONEXISTING;
			updateConfig(myConfigState); 
		}
		else { 
		//Check if config revision and AppDataDir is ok. Otherwise --> update()
			int tempRevision = 0;
			string tempAppDataPath ("");
	
			TiXmlHandle docHandle( &doc );		
			TiXmlElement* confRevision = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( "ConfigRevision" ).ToElement();
			if ( confRevision ) { confRevision->QueryIntAttribute("value", &tempRevision ); }
			TiXmlElement* confAppDataPath = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( "AppDataDir" ).ToElement();
			if ( confAppDataPath ) { 
				const char *tmpStr = confAppDataPath->Attribute("value");
				if (tmpStr) tempAppDataPath = tmpStr;
			}

			if (tempRevision < configRev || tempAppDataPath != myQtToolsInterface->getDataPathStdString(startPath.remove_leaf().directory_string()) ) { /*löschen()*/ 
				myConfigState = OLD;
				updateConfig(myConfigState) ;
			}
		}
		
	
		fillBuffer();
	}
}


ConfigFile::~ConfigFile()
{
	delete myQtToolsInterface;
	myQtToolsInterface = 0;
	
}


void ConfigFile::fillBuffer() {

	boost::mutex::scoped_lock lock(m_configMutex);

	size_t i;
	string tempString("");

	TiXmlDocument doc(configFileName); 
		
	if(doc.LoadFile()) {
	TiXmlHandle docHandle( &doc );	

		for (i=0; i<configBufferList.size(); i++) {	
	
			TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( configList[i].name ).ToElement();
				
			if ( conf ) {

				const char *tmpStr = conf->Attribute("value");
				if (tmpStr) tempString = tmpStr;
				configBufferList[i].defaultValue = tempString;
			}	
			else {	cout << "Could not find the element to fill the config-buffer with!";	}
				
// 			cout << configBufferList[i].name << " " << configBufferList[i].defaultValue << endl;
		}
	}
}

void ConfigFile::writeBuffer() const {

	boost::mutex::scoped_lock lock(m_configMutex);

	//write buffer to disc if enabled
	if(!noWriteAccess) {
		TiXmlDocument doc;  
		TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", ""); 
		doc.LinkEndChild( decl );  
			
		TiXmlElement * root = new TiXmlElement( "PokerTH" );  
		doc.LinkEndChild( root );  		
			
		TiXmlElement * config;
		config = new TiXmlElement( "Configuration" );  
		root->LinkEndChild( config );  
	
		size_t i;
	
		for (i=0; i<configBufferList.size(); i++) {
			TiXmlElement *tmpElement = new TiXmlElement(configBufferList[i].name);
			config->LinkEndChild( tmpElement );
			tmpElement->SetAttribute("value", configBufferList[i].defaultValue);
		}
				
		doc.SaveFile( configFileName );
	}
}

void ConfigFile::updateConfig(ConfigState myConfigState) {

	boost::mutex::scoped_lock lock(m_configMutex);
	boost::filesystem::path startPath(myArgv[0]);

	size_t i;
	
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

			//change configRev and AppDataPath				
			TiXmlElement * confElement0 = new TiXmlElement( "ConfigRevision" ); 
			config->LinkEndChild( confElement0 );
			confElement0->SetAttribute("value", configRev);

			TiXmlElement * confElement1 = new TiXmlElement( "AppDataDir" ); 
			config->LinkEndChild( confElement1 );
			confElement1->SetAttribute("value", myQtToolsInterface->getDataPathStdString(startPath.remove_leaf().directory_string()));

			TiXmlHandle docHandle( &oldDoc );	

			
			for (i=0; i<configList.size(); i++) {	

				TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( configList[i].name ).ToElement();
				
				if ( conf ) {
					// not for ConfigRevision and AppDataDir becaus it was already set ^
					if(configList[i].name != "ConfigRevision" && configList[i].name != "AppDataDir") {

						// if element is already there --> take over the saved values
						TiXmlElement *tmpElement = new TiXmlElement(configList[i].name);
						config->LinkEndChild( tmpElement );
					
						const char *tmpStr = conf->Attribute("value");
						if (tmpStr) tempString = tmpStr;
						tmpElement->SetAttribute("value", tempString);
					}
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
}

string ConfigFile::readConfigString(string varName) const
{
	boost::mutex::scoped_lock lock(m_configMutex);

	size_t i;
	string tempString("");

// 	TiXmlDocument doc(configFileName); 
// 	if(!doc.LoadFile()) {	cout << "Could Not Load Config-File!!! " << configFileName << "\n"; }
// 	TiXmlHandle docHandle( &doc );		
// 
// 	TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( varName ).ToElement();
// 	if ( conf ) { 
// 		const char *tmpStr = conf->Attribute("value");
// 		if (tmpStr) tempString = tmpStr;
//         } /*else {
// 		//Wenn nicht gefunden eines neues Anlegen
// 		TiXmlElement* config = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).ToElement();	
// 		if ( config ) { 		
// 			TiXmlElement * confElement1 = new TiXmlElement( varName ); 
// 			config->LinkEndChild( confElement1 );
// 			confElement1->SetAttribute("value", defaultValue);
// 			if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << configFileName << "\n"; }
// 
// 			return readConfigString(varName, defaultValue);
// 		}
// 	}*/
	for (i=0; i<configBufferList.size(); i++) {	

		if (configBufferList[i].name == varName) {
			tempString = configBufferList[i].defaultValue;	
		}
	}

	return tempString;
 }

int ConfigFile::readConfigInt(string varName) const
{
	boost::mutex::scoped_lock lock(m_configMutex);

	size_t i;
	string tempString("");
	int tempInt=0;

// 	cout << varName << " : " << tempInt << "\n";
// 	TiXmlDocument doc(configFileName); 
// 	if(!doc.LoadFile()) {	cout << "Could Not Load Config-File!!! " << configFileName << "\n"; }
// 	TiXmlHandle docHandle( &doc );		
// 	
// 	TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( varName ).ToElement();
// 	if ( conf ) {
// // 		cout << varName << " : " << tempInt << "\n";
// 		conf->QueryIntAttribute("value", &tempInt );
// // 		cout << varName << " : " << tempInt << "\n";
//         } /*else {
// // 		Wenn nicht gefunden eines neues Anlegen
// 		TiXmlElement* config = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).ToElement();	
// 		if ( config ) { 		
// 			TiXmlElement * confElement1 = new TiXmlElement( varName ); 
// 			config->LinkEndChild( confElement1 );
// 			confElement1->SetAttribute("value", defaultValue);
// 			if(!doc.SaveFile()) {	cout << "Could Not Save Config-File!!! " << configFileName << "\n"; }
// 
// 			return readConfigInt(varName, defaultValue);
// 		}
// 	}*/

	for (i=0; i<configBufferList.size(); i++) {	

		if (configBufferList[i].name == varName) {
			tempString = configBufferList[i].defaultValue;
		}
	}
	
	istringstream isst;
	isst.str (tempString);
	isst >> tempInt;
	
	return tempInt;
}


void ConfigFile::writeConfigInt(string varName, int varCont)
{
	boost::mutex::scoped_lock lock(m_configMutex);

	size_t i;
	string tempString;
	ostringstream intToString;

	for (i=0; i<configBufferList.size(); i++) {	

		if (configBufferList[i].name == varName) {
			intToString << varCont;
			configBufferList[i].defaultValue = intToString.str();	
		}
	}
}

void ConfigFile::writeConfigString(string varName, string varCont)
{
	boost::mutex::scoped_lock lock(m_configMutex);

	size_t i;
	for (i=0; i<configBufferList.size(); i++) {	
		if (configBufferList[i].name == varName) { configBufferList[i].defaultValue = varCont; }
	}

}
