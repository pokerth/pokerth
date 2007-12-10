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
#include "stylesheetreader.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>


using namespace std;

StyleSheetReader::StyleSheetReader(std::string f) : styleSheetFileName(f)
{
	fillBuffer();
}


StyleSheetReader::~StyleSheetReader()
{
}

void StyleSheetReader::fillBuffer() {

	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	string tempString1("");
	
	TiXmlDocument doc(styleSheetFileName); 
		
	if(doc.LoadFile()) {
		TiXmlHandle docHandle( &doc );	
	
		TiXmlElement* colorList = docHandle.FirstChild( "PokerTH" ).FirstChild( "Table" ).FirstChild( "Colors" ).FirstChild().ToElement();
							
		for( ; colorList; colorList=colorList->NextSiblingElement()) {
			const char *tmpStr1 = colorList->Attribute("value");
			if (tmpStr1) {
				tempString1 = tmpStr1;
				colorsBufferList.push_back(Colors(colorList->ValueStr(), tempString1));
// 				cout << colorList->ValueStr() << " : " << tempString1 << endl;
			}
		}
	}
}


string StyleSheetReader::readColorString(string varName) const
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	string tempString("");

	for (i=0; i<colorsBufferList.size(); i++) {	

		if (colorsBufferList[i].name == varName) {
			tempString = colorsBufferList[i].value;	
		}
	}

	return tempString;
 }
