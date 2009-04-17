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
#include <qttoolsinterface.h>
#include <core/loghelper.h>
#include "tinyxml.h"

#define MODUS 0711

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#endif

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>

using namespace std;


ConfigFile::ConfigFile(char *argv0, bool readonly) : noWriteAccess(readonly)
{

	myArgv0 = argv0;

	myQtToolsInterface = CreateQtToolsWrapper();

	// !!!! Revisionsnummer der Configdefaults !!!!!
	configRev = 66;

	//standard defaults
	logOnOffDefault = "1";

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

	ostringstream tempIntToString;
	tempIntToString << configRev;
	configList.push_back(ConfigInfo("ConfigRevision", CONFIG_TYPE_INT, tempIntToString.str()));
	configList.push_back(ConfigInfo("AppDataDir", CONFIG_TYPE_STRING, myQtToolsInterface->getDataPathStdString(myArgv0)));
	configList.push_back(ConfigInfo("Language", CONFIG_TYPE_INT, myQtToolsInterface->getDefaultLanguage()));
	/*configList.push_back(ConfigInfo("InternetServerAddressIRCChannelUpdateDone", CONFIG_TYPE_INT, "1"));*/ //HACK
	configList.push_back(ConfigInfo("ShowLeftToolBox", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowRightToolBox", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowFadeOutCardsAnimation", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowFlipCardsAnimation", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowBlindButtons", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowCardsChanceMonitor", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("DontTranslateInternationalPokerStringsFromStyle", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AntiPeekMode", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AlternateFKeysUserActionMode", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("EnableBetInputFocusSwitch", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("FlipsideTux", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("FlipsideOwn", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("FlipsideOwnFile", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("GameTableStylesList", CONFIG_TYPE_STRING_LIST, "GameTableStyles"));
	configList.push_back(ConfigInfo("CurrentGameTableStyle", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("CardDeckStylesList", CONFIG_TYPE_STRING_LIST, "CardDeckStyles"));
	configList.push_back(ConfigInfo("CurrentCardDeckStyle", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("PlaySoundEffects", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("SoundVolume", CONFIG_TYPE_INT, "8"));
	configList.push_back(ConfigInfo("PlayGameActions", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("PlayLobbyChatNotification", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("PlayNetworkGameNotification", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("NumberOfPlayers", CONFIG_TYPE_INT, "10"));
	configList.push_back(ConfigInfo("StartCash", CONFIG_TYPE_INT, "5000"));
	configList.push_back(ConfigInfo("FirstSmallBlind", CONFIG_TYPE_INT, "10"));
	configList.push_back(ConfigInfo("RaiseBlindsAtHands", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("RaiseBlindsAtMinutes", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("RaiseSmallBlindEveryHands", CONFIG_TYPE_INT, "8"));
	configList.push_back(ConfigInfo("RaiseSmallBlindEveryMinutes", CONFIG_TYPE_INT, "5"));
	configList.push_back(ConfigInfo("AlwaysDoubleBlinds", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ManualBlindsOrder", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ManualBlindsList", CONFIG_TYPE_INT_LIST, "Blind"));
	configList.push_back(ConfigInfo("AfterMBAlwaysDoubleBlinds", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("AfterMBAlwaysRaiseAbout", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AfterMBAlwaysRaiseValue", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AfterMBStayAtLastBlind", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("GameSpeed", CONFIG_TYPE_INT, "4"));
	configList.push_back(ConfigInfo("PauseBetweenHands", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ShowGameSettingsDialogOnNewGame", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("NetNumberOfPlayers", CONFIG_TYPE_INT, "10"));
	configList.push_back(ConfigInfo("NetStartCash", CONFIG_TYPE_INT, "3000"));
	configList.push_back(ConfigInfo("NetFirstSmallBlind", CONFIG_TYPE_INT, "10"));
	configList.push_back(ConfigInfo("NetRaiseBlindsAtHands", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("NetRaiseBlindsAtMinutes", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("NetRaiseSmallBlindEveryHands", CONFIG_TYPE_INT, "8"));
	configList.push_back(ConfigInfo("NetRaiseSmallBlindEveryMinutes", CONFIG_TYPE_INT, "5"));
	configList.push_back(ConfigInfo("NetAlwaysDoubleBlinds", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("NetManualBlindsOrder", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("NetManualBlindsList", CONFIG_TYPE_INT_LIST, "NetBlind"));
	configList.push_back(ConfigInfo("NetAfterMBAlwaysDoubleBlinds", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("NetAfterMBAlwaysRaiseAbout", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("NetAfterMBAlwaysRaiseValue", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("NetAfterMBStayAtLastBlind", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("NetGameSpeed", CONFIG_TYPE_INT, "4"));
	configList.push_back(ConfigInfo("NetTimeOutPlayerAction", CONFIG_TYPE_INT, "20"));
	configList.push_back(ConfigInfo("ServerPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ServerUseIpv6", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerUseSctp", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerPort", CONFIG_TYPE_INT, "7234"));
	configList.push_back(ConfigInfo("ServerUsePutAvatars", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerPutAvatarsAddress", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ServerPutAvatarsUser", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ServerPutAvatarsPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("InternetServerConfigMode", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("InternetServerListAddress", CONFIG_TYPE_STRING, "pokerth.net/serverlist.xml.z"));
	configList.push_back(ConfigInfo("InternetServerAddress", CONFIG_TYPE_STRING, "pokerth.6dns.org"));
	configList.push_back(ConfigInfo("InternetServerPort", CONFIG_TYPE_INT, "7234"));
	configList.push_back(ConfigInfo("InternetServerPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("InternetServerUseIpv6", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("InternetServerUseSctp", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("UseAvatarServer", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AvatarServerAddress", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("UseInternetGamePassword", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("InternetGamePassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("UseIRCLobbyChat", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("IRCServerAddress", CONFIG_TYPE_STRING, "chat.freenode.net"));
	configList.push_back(ConfigInfo("IRCServerPort", CONFIG_TYPE_INT, "6667"));
	configList.push_back(ConfigInfo("IRCChannel", CONFIG_TYPE_STRING, "#pokerth-lobby"));
	configList.push_back(ConfigInfo("IRCChannelPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("IRCServerUseIpv6", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("UseAdminIRC", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AdminIRCServerAddress", CONFIG_TYPE_STRING, "chat.freenode.net"));
	configList.push_back(ConfigInfo("AdminIRCServerPort", CONFIG_TYPE_INT, "6667"));
	configList.push_back(ConfigInfo("AdminIRCChannel", CONFIG_TYPE_STRING, "#test"));
	configList.push_back(ConfigInfo("AdminIRCChannelPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("AdminIRCServerUseIpv6", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AdminIRCServerNick", CONFIG_TYPE_INT, "PokerTH"));
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
	configList.push_back(ConfigInfo("Opponent7Name", CONFIG_TYPE_STRING, "Player 7"));
	configList.push_back(ConfigInfo("Opponent7Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent8Name", CONFIG_TYPE_STRING, "Player 8"));
	configList.push_back(ConfigInfo("Opponent8Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent9Name", CONFIG_TYPE_STRING, "Player 9"));
	configList.push_back(ConfigInfo("Opponent9Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("LogOnOff", CONFIG_TYPE_INT, logOnOffDefault));
	configList.push_back(ConfigInfo("LogDir", CONFIG_TYPE_STRING, logDir));
	configList.push_back(ConfigInfo("LogStoreDuration", CONFIG_TYPE_INT, "2"));
	configList.push_back(ConfigInfo("LogInterval", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("UserDataDir", CONFIG_TYPE_STRING, dataDir));
	configList.push_back(ConfigInfo("CacheDir", CONFIG_TYPE_STRING, cacheDir));
	configList.push_back(ConfigInfo("CLA_NoWriteAccess", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("DisableBackToLobbyWarning", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("DlgGameLobbyGameListSortingSection", CONFIG_TYPE_INT, "2"));
	configList.push_back(ConfigInfo("DlgGameLobbyGameListSortingOrder", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("GameTableFullScreenSave", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("GameTableHeightSave", CONFIG_TYPE_INT, "600"));
	configList.push_back(ConfigInfo("GameTableWidthSave", CONFIG_TYPE_INT, "1024"));

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

			if (tempRevision < configRev || tempAppDataPath != myQtToolsInterface->getDataPathStdString(myArgv0) ) { /*löschen()*/
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

	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	string tempString1("");
	string tempString2("");

	TiXmlDocument doc(configFileName);

	if(doc.LoadFile()) {
	TiXmlHandle docHandle( &doc );

		for (i=0; i<configBufferList.size(); i++) {

			TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( configList[i].name ).ToElement();

			if ( conf ) {

				const char *tmpStr1 = conf->Attribute("value");
				if (tmpStr1) tempString1 = tmpStr1;
				configBufferList[i].defaultValue = tempString1;

				const char *tmpStr2 = conf->Attribute("type");
				if (tmpStr2) {
					tempString2 = tmpStr2;
					if(tempString2 == "list") {

						list<string> tempStringList2;

						TiXmlElement* confList = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( configList[i].name ).FirstChild().ToElement();

						for( ; confList; confList=confList->NextSiblingElement()) {
							tempStringList2.push_back(confList->Attribute("value"));
						}

						configBufferList[i].defaultListValue = tempStringList2;
					}
				}


			}
			else { LOG_ERROR("Could not find the root element in the config file!"); }

// 			cout << configBufferList[i].name << " " << configBufferList[i].defaultValue << endl;
		}
	}
}

void ConfigFile::writeBuffer() const {

	boost::recursive_mutex::scoped_lock lock(m_configMutex);

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

			if(configBufferList[i].type == CONFIG_TYPE_INT_LIST || configBufferList[i].type == CONFIG_TYPE_STRING_LIST) {

				tmpElement->SetAttribute("type", "list");
				list<string> tempList = configBufferList[i].defaultListValue;
				list<string>::iterator it;
				for(it = tempList.begin(); it != tempList.end(); it++) {

					TiXmlElement *tmpSubElement = new TiXmlElement(configBufferList[i].defaultValue);
					tmpElement->LinkEndChild( tmpSubElement );
					tmpSubElement->SetAttribute("value", *it);
				}

			}
		}

		doc.SaveFile( configFileName );
	}
}

void ConfigFile::updateConfig(ConfigState myConfigState) {

	boost::recursive_mutex::scoped_lock lock(m_configMutex);

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

			if(configList[i].type == CONFIG_TYPE_INT_LIST || configBufferList[i].type == CONFIG_TYPE_STRING_LIST) {

				tmpElement->SetAttribute("type", "list");
				list<string> tempList = configList[i].defaultListValue;
				list<string>::iterator it;
				for(it = tempList.begin(); it != tempList.end(); it++) {

					TiXmlElement *tmpSubElement = new TiXmlElement(configList[i].defaultValue);
					tmpElement->LinkEndChild( tmpSubElement );
					tmpSubElement->SetAttribute("value", *it);
				}

			}
		}

		doc.SaveFile( configFileName );
	}


	if(myConfigState == OLD) {

		TiXmlDocument oldDoc(configFileName);

		//load the old one
		if(oldDoc.LoadFile()) {

			string tempString1("");
			string tempString2("");

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
			confElement1->SetAttribute("value", myQtToolsInterface->stringToUtf8(myQtToolsInterface->getDataPathStdString(myArgv0)));

			TiXmlHandle oldDocHandle( &oldDoc );

			for (i=0; i<configList.size(); i++) {

				TiXmlElement* oldConf = oldDocHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( configList[i].name ).ToElement();

// 				//HACK remove after 0.6
// 				TiXmlElement* ISAIRCChannelUpdateDone = oldDocHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( "InternetServerAddressIRCChannelUpdateDone" ).ToElement();

				if ( oldConf /*&& (ISAIRCChannelUpdateDone || !(configList[i].name == "IRCChannel" || (configList[i].name == "InternetServerAddress" )))*/ /*HACK remove after 0.6*/ ) {
				// if element is already there --> take over the saved values

					if(configList[i].name != "ConfigRevision" && configList[i].name != "AppDataDir") {
							// dont update ConfigRevision and AppDataDir becaus it was already set ^

						TiXmlElement *tmpElement = new TiXmlElement(configList[i].name);
						config->LinkEndChild( tmpElement );

						const char *tmpStr1 = oldConf->Attribute("value");
						if (tmpStr1) tempString1 = tmpStr1;
						tmpElement->SetAttribute("value", tempString1);

						//for lists copy elements
						const char *tmpStr2 = oldConf->Attribute("type");
						if (tmpStr2) {
							tempString2 = tmpStr2;
							if(tempString2 == "list") {

								list<string> tempStringList2;

								TiXmlElement* oldConfList = oldDocHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( configList[i].name ).FirstChild().ToElement();

								for( ; oldConfList; oldConfList=oldConfList->NextSiblingElement()) {
									tempStringList2.push_back(oldConfList->Attribute("value"));
									}

								tmpElement->SetAttribute("type", "list");
								list<string> tempList = tempStringList2;
								list<string>::iterator it;
								for(it = tempList.begin(); it != tempList.end(); it++) {

									TiXmlElement *tmpSubElement = new TiXmlElement(tempString1);
									tmpElement->LinkEndChild( tmpSubElement );
									tmpSubElement->SetAttribute("value", *it);
								}
							}
						}
					}
				}
				else {
					// if element is not there --> set it with defaultValue
					TiXmlElement *tmpElement = new TiXmlElement(configList[i].name);
					config->LinkEndChild( tmpElement );
					tmpElement->SetAttribute("value", myQtToolsInterface->stringToUtf8(configList[i].defaultValue));

					if(configList[i].type == CONFIG_TYPE_INT_LIST || configBufferList[i].type == CONFIG_TYPE_STRING_LIST) {

						tmpElement->SetAttribute("type", "list");
						list<string> tempList = configList[i].defaultListValue;
						list<string>::iterator it;
						for(it = tempList.begin(); it != tempList.end(); it++) {

							TiXmlElement *tmpSubElement = new TiXmlElement(configList[i].defaultValue);
							tmpElement->LinkEndChild( tmpSubElement );
							tmpSubElement->SetAttribute("value", *it);
						}
					}
				}
			}
			newDoc.SaveFile( configFileName );
		}
		else { LOG_ERROR("Cannot update config file: Unable to load configuration."); }


	}
}

string ConfigFile::readConfigString(string varName) const
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	string tempString("");

	for (i=0; i<configBufferList.size(); i++) {

		if (configBufferList[i].name == varName) {
			tempString = configBufferList[i].defaultValue;
		}
	}

	return tempString;
 }

int ConfigFile::readConfigInt(string varName) const
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	string tempString("");
	int tempInt=0;

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

list<int> ConfigFile::readConfigIntList(string varName) const
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	list<string> tempStringList;
	list<int> tempIntList;

	for (i=0; i<configBufferList.size(); i++) {

		if (configBufferList[i].name == varName) {
			tempStringList = configBufferList[i].defaultListValue;
		}
	}

	istringstream isst;
	string tempString;
	int tempInt;
	list<string>::iterator it;
	for(it = tempStringList.begin(); it != tempStringList.end(); it++) {

		isst.str(*it);
		isst >> tempInt;
		tempIntList.push_back(tempInt);
		isst.str("");
		isst.clear();
	}

	return tempIntList;
}

list<string> ConfigFile::readConfigStringList(string varName) const
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	list<string> tempStringList;

	for (i=0; i<configBufferList.size(); i++) {

		if (configBufferList[i].name == varName) {
			tempStringList = configBufferList[i].defaultListValue;
		}
	}

	return tempStringList;
}

void ConfigFile::writeConfigInt(string varName, int varCont)
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

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

void ConfigFile::writeConfigIntList(string varName, list<int> varCont)
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	ostringstream intToString;
	list<string> stringList;

	for (i=0; i<configBufferList.size(); i++) {

		if (configBufferList[i].name == varName) {
			string tempString;
			list<int>::iterator it;
			for(it = varCont.begin(); it != varCont.end(); it++) {

				intToString << (*it);
				stringList.push_back(intToString.str());
				intToString.str("");
				intToString.clear();
			}

			configBufferList[i].defaultListValue = stringList;
		}
	}
}

void ConfigFile::writeConfigString(string varName, string varCont)
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	for (i=0; i<configBufferList.size(); i++) {
		if (configBufferList[i].name == varName) { configBufferList[i].defaultValue = varCont; }
	}

}

void ConfigFile::writeConfigStringList(string varName, list<string> varCont)
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	for (i=0; i<configBufferList.size(); i++) {

		if (configBufferList[i].name == varName) { configBufferList[i].defaultListValue = varCont; }
	}
}
