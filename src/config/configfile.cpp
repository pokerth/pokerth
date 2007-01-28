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

#ifdef _WIN32
#include <direct.h>
#endif

using namespace std;

ConfigFile::ConfigFile()
{
	// !!!! Revisionsnummer der Configdefaults !!!!!
	configRev = 2;

	// Pfad und Dateinamen setzen
#ifdef _WIN32
	const char *appDataPath = getenv("AppData");
	if (appDataPath)
	{
		configFileName = appDataPath;
		configFileName += "\\pokerth\\";
		mkdir(configFileName.c_str());
	}
#else
	// hier Linux/Mac Code zur Basispfadbestimmung, z.B.
	string homePath = getenv("HOME");
	configFileName = homePath+"/.pokerth/";
	// wenn nicht existiert, erzeugen!
#endif
	configFileName += "config.xml";

	//Prüfen ob Configfile existiert --> sonst anlegen
	TiXmlDocument doc(configFileName); 
	if(!doc.LoadFile()){ createDefaultConfig(); }
	else { 
	//Prüfen ob die Revision stimmt ansonsten löschen und neue anlegen 
		int temp = 0;
		TiXmlHandle docHandle( &doc );		
		TiXmlElement* conf = docHandle.FirstChild( "PokerTH" ).FirstChild( "Configuration" ).FirstChild( "ConfigRevision" ).ToElement();
		if ( conf ) { conf->QueryIntAttribute("value", &temp ); }
		if (temp < configRev) { /*löschen()*/ createDefaultConfig() ;}
	}

}


ConfigFile::~ConfigFile()
{
}

void ConfigFile::createDefaultConfig() {

		//Anlegen!
		TiXmlDocument doc;  
		TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", ""); 
		doc.LinkEndChild( decl );  
	
		TiXmlElement * root = new TiXmlElement( "PokerTH" );  
		doc.LinkEndChild( root );  		
// 		
		TiXmlElement * config;
       		config = new TiXmlElement( "Configuration" );  
		root->LinkEndChild( config );  
		
		TiXmlElement * confElement0 = new TiXmlElement( "ConfigRevision" ); 
		config->LinkEndChild( confElement0 );
		confElement0->SetAttribute("value", configRev);
		TiXmlElement * confElement1 = new TiXmlElement( "NumberOfPlayers" ); 
		config->LinkEndChild( confElement1 );
		confElement1->SetAttribute("value", 5);
                TiXmlElement * confElement2 = new TiXmlElement( "StartCash" );
	        config->LinkEndChild( confElement2 );
                confElement2->SetAttribute("value", 2000);
		TiXmlElement * confElement3 = new TiXmlElement( "SmallBlind" );
  		config->LinkEndChild( confElement3 );
        	confElement3->SetAttribute("value", 10);
		TiXmlElement * confElement12 = new TiXmlElement( "HandsBeforeRaiseSmallBlind" );
		config->LinkEndChild( confElement12 );
      		confElement12->SetAttribute("value", 9);	
		TiXmlElement * confElement4 = new TiXmlElement( "GameSpeed" );
		config->LinkEndChild( confElement4 );
      		confElement4->SetAttribute("value", 4);
		TiXmlElement * confElement14 = new TiXmlElement( "PauseBetweenHands" );
		config->LinkEndChild( confElement14 );
      		confElement14->SetAttribute("value", 0);		
		TiXmlElement * confElement5 = new TiXmlElement( "ShowGameSettingsDialogOnNewGame" );
		config->LinkEndChild( confElement5 );
      		confElement5->SetAttribute("value", 1);		
		TiXmlElement * confElement6 = new TiXmlElement( "MyName" );
		config->LinkEndChild( confElement6 );
      		confElement6->SetAttribute("value", "Human Player");		
		TiXmlElement * confElement7 = new TiXmlElement( "Opponent1Name" );
		config->LinkEndChild( confElement7 );
      		confElement7->SetAttribute("value", "Player 1");
		TiXmlElement * confElement8 = new TiXmlElement( "Opponent2Name" );
		config->LinkEndChild( confElement8 );
      		confElement8->SetAttribute("value", "Player 2");
		TiXmlElement * confElement9 = new TiXmlElement( "Opponent3Name" );
		config->LinkEndChild( confElement9 );
      		confElement9->SetAttribute("value", "Player 3");
		TiXmlElement * confElement10 = new TiXmlElement( "Opponent4Name" );
		config->LinkEndChild( confElement10 );
      		confElement10->SetAttribute("value", "Player 4");
		TiXmlElement * confElement11 = new TiXmlElement( "ShowToolBox" );
		config->LinkEndChild( confElement11 );
      		confElement11->SetAttribute("value", 1);
		TiXmlElement * confElement13 = new TiXmlElement( "ShowIntro" );
		config->LinkEndChild( confElement13 );
      		confElement13->SetAttribute("value", 1);
		TiXmlElement * confElement15 = new TiXmlElement( "ShowFadeOutCardsAnimation" );
		config->LinkEndChild( confElement15 );
      		confElement15->SetAttribute("value", 1);

		doc.SaveFile( configFileName );

}

string ConfigFile::readConfigString(string varName, string defaultValue)
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

int ConfigFile::readConfigInt(string varName, int defaultValue)
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
