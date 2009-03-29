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
#include "gametablestylereader.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>


using namespace std;

GameTableStyleReader::GameTableStyleReader(ConfigFile *c, gameTableImpl *w) : myConfig(c), myW(w)
{
	
}


GameTableStyleReader::~GameTableStyleReader()
{
}

void GameTableStyleReader::readStyleFile(QString file) {

	string tinyFileName;

	QMessageBox::warning(myW, tr("FILE"),
                                file,
                                QMessageBox::Ok);

	//if style file failed --> default style fallback
	if(QFile(file).exists()) { 
		currentFileName = QFile(file).fileName();
		tinyFileName = currentFileName.toStdString();
		 
		QMessageBox::warning(myW, tr("WORKS"),
                                currentFileName,
                                QMessageBox::Ok);
	}
	else { 
		currentFileName = QFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/table/default/defaulttablestyle.xml").fileName(); 
		tinyFileName = currentFileName.toStdString(); 
		QMessageBox::warning(myW, tr("FALLBACK"),
                                currentFileName,
                                QMessageBox::Ok);

	}
	QFileInfo info(currentFileName);
	currentFileDir = info.absolutePath();

	//start reading the file and fill vars	
	string tempString1("");
	TiXmlDocument doc(tinyFileName); 
		
	if(doc.LoadFile()) {
		TiXmlHandle docHandle( &doc );	
	
		TiXmlElement* itemsList = docHandle.FirstChild( "PokerTH" ).FirstChild( "TableStyle" ).FirstChild().ToElement();
		for( ; itemsList; itemsList=itemsList->NextSiblingElement()) {
			const char *tmpStr1 = itemsList->Attribute("value");
			if (tmpStr1) {
				tempString1 = tmpStr1;

				if(itemsList->ValueStr() == "StyleDescription") { StyleDescription = QString::fromUtf8(tempString1.c_str()); }
				else if(itemsList->ValueStr() == "StyleMaintainerEMail") { StyleMaintainerEMail = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "Preview") { Preview = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "IfFixedWindowSize") { IfFixedWindowSize = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FixedWindowWidth") { FixedWindowWidth = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FixedWindowHeight") { FixedWindowHeight = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MinimumWindowWidth") { MinimumWindowWidth = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MinimumWindowHeight") { MinimumWindowHeight = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MaximumWindowWidth") { MaximumWindowWidth = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MaximumWindowHeight") { MaximumWindowHeight = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionAllIn") { ActionAllIn = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionRaise") { ActionRaise = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionBet") { ActionBet = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionCall") { ActionCall = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionCheck") { ActionCheck = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionFold") { ActionFold = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionWinner") { ActionWinner = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BigBlindPuck") { BigBlindPuck = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "SmallBlindPuck") { SmallBlindPuck = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "DealerPuck") { DealerPuck = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "DefaultAvatar") { DefaultAvatar = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CardHolderFlop") { CardHolderFlop = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CardHolderTurn") { CardHolderTurn = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CardHolderRiver") { CardHolderRiver = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FoldButtonDefault") { FoldButtonDefault = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FoldButtonHover") { FoldButtonHover = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FoldButtonChecked") { FoldButtonChecked = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FoldButtonCheckedHover") { FoldButtonCheckedHover = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CheckCallButtonDefault") { CheckCallButtonDefault = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CheckCallButtonHover") { CheckCallButtonHover = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CheckCallButtonChecked") { CheckCallButtonChecked = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CheckCallButtonCheckedHover") { CheckCallButtonCheckedHover = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetRaiseButtonDefault") { BetRaiseButtonDefault = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetRaiseButtonHover") { BetRaiseButtonHover = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetRaiseButtonChecked") { BetRaiseButtonChecked = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetRaiseButtonCheckedHover") { BetRaiseButtonCheckedHover = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "AllInButtonDefault") { AllInButtonDefault = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "AllInButtonHover") { AllInButtonHover = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "AllInButtonChecked") { AllInButtonChecked = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "AllInButtonCheckedHover") { AllInButtonCheckedHover = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "PlayerSeatInActive") { PlayerSeatInActive = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "PlayerSeatActive") { PlayerSeatActive = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "Table") { Table = QString::fromUtf8(tempString1.c_str()); }

// 				std::cout << "ÖLP" << itemsList->ValueStr() <<"§4s"<< itemsList->ValueStr()<<"§89"<< endl;
			}
		}
		wrongItems.clear();

		if(IfFixedWindowSize == "") { wrongItems << "IfFixedWindowSize"; }
		if(FixedWindowWidth == "") { wrongItems << "FixedWindowWidth"; }
		if(FixedWindowHeight == "") { wrongItems << "FixedWindowHeight"; }
		if(MinimumWindowWidth == "") { wrongItems << "MinimumWindowWidth"; }
		if(MinimumWindowHeight == "") { wrongItems << "MinimumWindowHeight"; }
		if(MaximumWindowWidth == "") { wrongItems << "MaximumWindowWidth"; }
		if(MaximumWindowHeight == "") { wrongItems << "MaximumWindowHeight"; }
		if(ActionAllIn == "") { wrongItems << "ActionAllIn"; }
		if(ActionRaise == "") { wrongItems << "ActionRaise"; }
		if(ActionBet == "") { wrongItems << "ActionBet"; }
		if(ActionCall == "") { wrongItems << "ActionCall"; }
		if(ActionCheck == "") { wrongItems << "ActionCheck"; }
		if(ActionFold == "") { wrongItems << "ActionFold"; }
		if(ActionWinner == "") { wrongItems << "ActionWinner"; }
		if(BigBlindPuck == "") { wrongItems << "BigBlindPuck"; }
		if(SmallBlindPuck == "") { wrongItems << "SmallBlindPuck"; }
		if(DealerPuck == "") { wrongItems << "DealerPuck"; }
		if(DefaultAvatar == "") { wrongItems << "DefaultAvatar"; }
		if(CardHolderFlop == "") { wrongItems << "CardHolderFlop"; }
		if(CardHolderTurn == "") { wrongItems << "CardHolderTurn"; }
		if(CardHolderRiver == "") { wrongItems << "CardHolderRiver"; }
		if(FoldButtonDefault == "") { wrongItems << "FoldButtonDefault"; }
		if(FoldButtonHover == "") { wrongItems << "FoldButtonHover"; }
		if(FoldButtonChecked == "") { wrongItems << "FoldButtonChecked"; }
		if(FoldButtonCheckedHover == "") { wrongItems << "FoldButtonCheckedHover"; }
		if(CheckCallButtonDefault == "") { wrongItems << "CheckCallButtonDefault"; }
		if(CheckCallButtonHover == "") { wrongItems << "CheckCallButtonHover"; }
		if(CheckCallButtonChecked == "") { wrongItems << "CheckCallButtonChecked"; }
		if(CheckCallButtonCheckedHover == "") { wrongItems << "CheckCallButtonCheckedHover"; }
		if(BetRaiseButtonDefault == "") { wrongItems << "BetRaiseButtonDefault"; }
		if(BetRaiseButtonHover == "") { wrongItems << "BetRaiseButtonHover"; }
		if(BetRaiseButtonChecked == "") { wrongItems << "BetRaiseButtonChecked"; }
		if(BetRaiseButtonCheckedHover == "") { wrongItems << "BetRaiseButtonCheckedHover"; }
		if(AllInButtonDefault == "") { wrongItems << "AllInButtonDefault"; }
		if(AllInButtonHover == "") { wrongItems << "AllInButtonHover"; }
		if(AllInButtonChecked == "") { wrongItems << "AllInButtonChecked"; }
		if(AllInButtonCheckedHover == "") { wrongItems << "AllInButtonCheckedHover"; }
		if(PlayerSeatActive == "") { wrongItems << "PlayerSeatActive"; }
		if(PlayerSeatInActive == "") { wrongItems << "PlayerSeatInActive"; }
		if(Table == "") { wrongItems << "Table"; }
		
		//if one or more items are wrong or left show detailed error message
		if(!wrongItems.isEmpty() && myW != 0) showErrorMessage(StyleDescription, wrongItems, StyleMaintainerEMail);
	}
}

void GameTableStyleReader::showErrorMessage(QString style, QStringList failedItems, QString email)
{
	QString items = failedItems.join(", ");

	QMessageBox::warning(myW, tr("Game Table Style Error"),
                                tr("Selected game table style \"%1\" seems to be incomplete or defective. \nThe value of \"%2\" is wrong or left. Please contact the style builder %3.").arg(style).arg(items).arg(email),
                                QMessageBox::Ok);
}

