/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#include "cleanerconfig.h"
#include <QtCore>

#include <tinyxml.h>

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


CleanerConfig::CleanerConfig()
{
	// !!!! Revisionsnummer der Configdefaults !!!!!
	configRev = 10;

	// Pfad und Dateinamen setzen
#ifdef _WIN32
	const char *appDataPath = getenv("AppData");
	if (appDataPath && appDataPath[0] != 0) {
		configFileName = appDataPath;
	} else {
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
		} else {
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
// 	logDir = configFileName;
// 	logDir += "log-files\\";

	//create directories on first start of app
	mkdir(configFileName.c_str());
// 	mkdir(logDir.c_str());


#else
	//define app-dir
	const char *homePath = getenv("HOME");
	if(homePath) {
		configFileName = homePath;
		configFileName += "/.pokerth/";
		////define log-dir
// 		logDir = configFileName;
// 		logDir += "log-files/";
		//create directories on first start of app
		mkdir(configFileName.c_str(), MODUS) ;
// 		mkdir(logDir.c_str(), MODUS);

	}

#endif

	ostringstream tempIntToString;
	tempIntToString << configRev;

	configList.push_back(ConfigInfo("ConfigRevision", CONFIG_TYPE_INT, tempIntToString.str()));
	configList.push_back(ConfigInfo("Language", CONFIG_TYPE_STRING, getDefaultLanguage()));

	configList.push_back(ConfigInfo("HostAddress", CONFIG_TYPE_STRING, "0.0.0.0"));
	configList.push_back(ConfigInfo("DefaultListenPort", CONFIG_TYPE_STRING, "4327"));
	configList.push_back(ConfigInfo("ClientAuthString", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ServerAuthString", CONFIG_TYPE_STRING, ""));

	configList.push_back(ConfigInfo("WarnLevelToKick", CONFIG_TYPE_INT, "2"));
	configList.push_back(ConfigInfo("TextFloodLevelToTrigger", CONFIG_TYPE_INT, "3"));
	configList.push_back(ConfigInfo("CapsFloodCapsNumberToTrigger", CONFIG_TYPE_INT, "10"));
	configList.push_back(ConfigInfo("LetterRepeatingNumberToTrigger", CONFIG_TYPE_INT, "10"));

	configList.push_back(ConfigInfo("KickNumberToBan", CONFIG_TYPE_INT, "2"));
	configList.push_back(ConfigInfo("SecondsToForgetAboutKick", CONFIG_TYPE_INT, "1800"));

	list<string> badWordsList;
	badWordsList.push_back("arsch");
	badWordsList.push_back("asshole");
	badWordsList.push_back("bastard");
	badWordsList.push_back("bitch");
	badWordsList.push_back("cunt");
	badWordsList.push_back(" dick ");
	badWordsList.push_back("drecksau");
	badWordsList.push_back(" fag ");
	badWordsList.push_back("fagget");
	badWordsList.push_back("fotze");
	badWordsList.push_back("fuker");
	badWordsList.push_back("fuck");
	badWordsList.push_back(" fuk ");
	badWordsList.push_back("gay");
	badWordsList.push_back("horny");
	badWordsList.push_back("hure");
	badWordsList.push_back("idiot");
	badWordsList.push_back("mistgeburt");
	badWordsList.push_back("missgeburt");
	badWordsList.push_back("motherfucker");
	badWordsList.push_back("nazi");
	badWordsList.push_back("nigga");
	badWordsList.push_back("nigger");
	badWordsList.push_back("nutte");
	badWordsList.push_back("ommak");
	badWordsList.push_back("penis");
	badWordsList.push_back("pussy");
	badWordsList.push_back("schlampe");
	badWordsList.push_back("schwanz");
	badWordsList.push_back("sex");
	badWordsList.push_back("shit");
	badWordsList.push_back("sieg heil");
	badWordsList.push_back("slut");
	badWordsList.push_back("suck");
	badWordsList.push_back("whore");
	configList.push_back(ConfigInfo("BadWordsList", CONFIG_TYPE_STRING_LIST, "BadWords", badWordsList));

	list<string> badWordsExceptionList;
	badWordsExceptionList.push_back("idiots end");
	badWordsExceptionList.push_back("all-in-idiot");
	badWordsExceptionList.push_back("all-in idiot");
	badWordsExceptionList.push_back("allin idiot");
	badWordsExceptionList.push_back("all in idiot");
	configList.push_back(ConfigInfo("BadWordsException", CONFIG_TYPE_STRING_LIST, "BadWordsException", badWordsExceptionList));


	list<string> urlStringsList;
	urlStringsList.push_back("http://");
	urlStringsList.push_back(".com");
	urlStringsList.push_back(".net");
	urlStringsList.push_back(".org");
	urlStringsList.push_back(".de");
	configList.push_back(ConfigInfo("UrlStringsList", CONFIG_TYPE_STRING_LIST, "UrlStrings", urlStringsList));

	list<string> urlExceptionStringsList;
	urlExceptionStringsList.push_back("http://www.esl.");
	urlExceptionStringsList.push_back("http://www.pokerth.net");
	urlExceptionStringsList.push_back("pokerth.net");
	configList.push_back(ConfigInfo("UrlExceptionStringsList", CONFIG_TYPE_STRING_LIST, "UrlExceptionStrings", urlExceptionStringsList));

	//fill tempList firstTime
	configBufferList = configList;

	configFileName += "cleanerconfig.xml";

	//Prüfen ob Configfile existiert --> sonst anlegen
	TiXmlDocument doc(configFileName);
	if(!doc.LoadFile()) {
		myConfigState = NONEXISTING;
		updateConfig(myConfigState);
	} else {
		//Check if config revision is ok. Otherwise --> update()
		int tempRevision = 0;

		TiXmlHandle docHandle( &doc );
		TiXmlElement* confRevision = docHandle.FirstChild( "PokerTHCleaner" ).FirstChild( "Configuration" ).FirstChild( "ConfigRevision" ).ToElement();
		if ( confRevision ) {
			confRevision->QueryIntAttribute("value", &tempRevision );
		}

		if (tempRevision < configRev ) { /*löschen()*/
			myConfigState = OLD;
			updateConfig(myConfigState) ;
		}
	}


	fillBuffer();
}


CleanerConfig::~CleanerConfig()
{
}


void CleanerConfig::fillBuffer()
{

	string tempString1("");
	string tempString2("");

	TiXmlDocument doc(configFileName);

	if(doc.LoadFile()) {
		TiXmlHandle docHandle( &doc );

		for (size_t i=0; i<configBufferList.size(); i++) {

			TiXmlElement* conf = docHandle.FirstChild( "PokerTHCleaner" ).FirstChild( "Configuration" ).FirstChild( configList[i].name ).ToElement();

			if ( conf ) {

				const char *tmpStr1 = conf->Attribute("value");
				if (tmpStr1) tempString1 = tmpStr1;
				configBufferList[i].defaultValue = tempString1;

				const char *tmpStr2 = conf->Attribute("type");
				if (tmpStr2) {
					tempString2 = tmpStr2;
					if(tempString2 == "list") {

						list<string> tempStringList2;

						TiXmlElement* confList = docHandle.FirstChild( "PokerTHCleaner" ).FirstChild( "Configuration" ).FirstChild( configList[i].name ).FirstChild().ToElement();

						for( ; confList; confList=confList->NextSiblingElement()) {
							tempStringList2.push_back(confList->Attribute("value"));
						}

						configBufferList[i].defaultListValue = tempStringList2;
					}
				}
			} else {
				qDebug("Could not find the root element in the config file!");
			}
		}
	}
}

void CleanerConfig::writeBuffer() const
{

	TiXmlDocument doc;
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", "");
	doc.LinkEndChild( decl );

	TiXmlElement * root = new TiXmlElement( "PokerTHCleaner" );
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
			for(it = tempList.begin(); it != tempList.end(); ++it) {

				TiXmlElement *tmpSubElement = new TiXmlElement(configBufferList[i].defaultValue);
				tmpElement->LinkEndChild( tmpSubElement );
				tmpSubElement->SetAttribute("value", *it);
			}

		}
	}
	doc.SaveFile( configFileName );

}

void CleanerConfig::updateConfig(ConfigState myConfigState)
{

	size_t i;

	if(myConfigState == NONEXISTING) {

		//Create a new ConfigFile!
		TiXmlDocument doc;
		TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", "");
		doc.LinkEndChild( decl );

		TiXmlElement * root = new TiXmlElement( "PokerTHCleaner" );
		doc.LinkEndChild( root );

		TiXmlElement * config;
		config = new TiXmlElement( "Configuration" );
		root->LinkEndChild( config );

		for (i=0; i<configList.size(); i++) {
			TiXmlElement *tmpElement = new TiXmlElement(configList[i].name);
			config->LinkEndChild( tmpElement );
			tmpElement->SetAttribute("value", stringToUtf8(configList[i].defaultValue));

			if(configList[i].type == CONFIG_TYPE_INT_LIST || configList[i].type == CONFIG_TYPE_STRING_LIST) {

				tmpElement->SetAttribute("type", "list");
				list<string> tempList = configList[i].defaultListValue;
				list<string>::iterator it;
				for(it = tempList.begin(); it != tempList.end(); ++it) {

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

			TiXmlElement * root = new TiXmlElement( "PokerTHCleaner" );
			newDoc.LinkEndChild( root );

			TiXmlElement * config;
			config = new TiXmlElement( "Configuration" );
			root->LinkEndChild( config );

			//change configRev and AppDataPath
			TiXmlElement * confElement0 = new TiXmlElement( "ConfigRevision" );
			config->LinkEndChild( confElement0 );
			confElement0->SetAttribute("value", configRev);

			TiXmlHandle oldDocHandle( &oldDoc );

			for (i=0; i<configList.size(); i++) {

				TiXmlElement* oldConf = oldDocHandle.FirstChild( "PokerTHCleaner" ).FirstChild( "Configuration" ).FirstChild( configList[i].name ).ToElement();

				if ( oldConf ) {
					// if element is already there --> take over the saved values

					if(configList[i].name != "ConfigRevision") {
						// dont update ConfigRevision because it was already set ^

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

								TiXmlElement* oldConfList = oldDocHandle.FirstChild( "PokerTHCleaner" ).FirstChild( "Configuration" ).FirstChild( configList[i].name ).FirstChild().ToElement();

								for( ; oldConfList; oldConfList=oldConfList->NextSiblingElement()) {
									tempStringList2.push_back(oldConfList->Attribute("value"));
								}

								tmpElement->SetAttribute("type", "list");
								list<string> tempList = tempStringList2;
								list<string>::iterator it;
								for(it = tempList.begin(); it != tempList.end(); ++it) {

									TiXmlElement *tmpSubElement = new TiXmlElement(tempString1);
									tmpElement->LinkEndChild( tmpSubElement );
									tmpSubElement->SetAttribute("value", *it);
								}
							}
						}
					}
				} else {
					// if element is not there --> set it with defaultValue
					TiXmlElement *tmpElement = new TiXmlElement(configList[i].name);
					config->LinkEndChild( tmpElement );
					tmpElement->SetAttribute("value", stringToUtf8(configList[i].defaultValue));

					if(configList[i].type == CONFIG_TYPE_INT_LIST || configList[i].type == CONFIG_TYPE_STRING_LIST) {

						tmpElement->SetAttribute("type", "list");
						list<string> tempList = configList[i].defaultListValue;
						list<string>::iterator it;
						for(it = tempList.begin(); it != tempList.end(); ++it) {

							TiXmlElement *tmpSubElement = new TiXmlElement(configList[i].defaultValue);
							tmpElement->LinkEndChild( tmpSubElement );
							tmpSubElement->SetAttribute("value", *it);
						}
					}
				}
			}
			newDoc.SaveFile( configFileName );
		} else {
			qDebug("Cannot update config file: Unable to load configuration.");
		}


	}
}

string CleanerConfig::readConfigString(string varName) const
{

	size_t i;
	string tempString("");

	for (i=0; i<configBufferList.size(); i++) {

		if (configBufferList[i].name == varName) {
			tempString = configBufferList[i].defaultValue;
		}
	}

	return tempString;
}

int CleanerConfig::readConfigInt(string varName) const
{

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

list<string> CleanerConfig::readConfigStringList(string varName) const
{

	size_t i;
	list<string> tempStringList;

	for (i=0; i<configBufferList.size(); i++) {

		if (configBufferList[i].name == varName) {
			tempStringList = configBufferList[i].defaultListValue;
		}
	}

	return tempStringList;
}


list<int> CleanerConfig::readConfigIntList(string varName) const
{

	size_t i;
	list<string> tempStringList;
	list<int> tempIntList;

	for (i=0; i<configBufferList.size(); i++) {

		if (configBufferList[i].name == varName) {
			tempStringList = configBufferList[i].defaultListValue;
		}
	}

	istringstream isst;
	int tempInt;
	list<string>::iterator it;
	for(it = tempStringList.begin(); it != tempStringList.end(); ++it) {

		isst.str(*it);
		isst >> tempInt;
		tempIntList.push_back(tempInt);
		isst.str("");
		isst.clear();
	}

	return tempIntList;
}


void CleanerConfig::writeConfigInt(string varName, int varCont)
{

	size_t i;
	ostringstream intToString;

	for (i=0; i<configBufferList.size(); i++) {

		if (configBufferList[i].name == varName) {
			intToString << varCont;
			configBufferList[i].defaultValue = intToString.str();
		}
	}
}


void CleanerConfig::writeConfigIntList(string varName, list<int> varCont)
{

	size_t i;
	ostringstream intToString;
	list<string> stringList;

	for (i=0; i<configBufferList.size(); i++) {

		if (configBufferList[i].name == varName) {
			list<int>::iterator it;
			for(it = varCont.begin(); it != varCont.end(); ++it) {

				intToString << (*it);
				stringList.push_back(intToString.str());
				intToString.str("");
				intToString.clear();
			}

			configBufferList[i].defaultListValue = stringList;
		}
	}
}

void CleanerConfig::writeConfigStringList(string varName, list<string> varCont)
{

	size_t i;

	for (i=0; i<configBufferList.size(); i++) {

		if (configBufferList[i].name == varName) {

			configBufferList[i].defaultListValue = varCont;
		}
	}
}

void CleanerConfig::writeConfigString(string varName, string varCont)
{

	size_t i;
	for (i=0; i<configBufferList.size(); i++) {
		if (configBufferList[i].name == varName) {
			configBufferList[i].defaultValue = varCont;
		}
	}

}
std::string CleanerConfig::stringToUtf8(const std::string &myString)
{

	QString tmpString = QString::fromStdString(myString);
	std::string myUtf8String = tmpString.toUtf8().constData();

	return myUtf8String;
}

std::string CleanerConfig::stringFromUtf8(const std::string &myString)
{
	QString tmpString = QString::fromUtf8(myString.c_str());

	return tmpString.toStdString();
}

std::string CleanerConfig::getDefaultLanguage()
{
	return QLocale::system().name().toStdString();
}
