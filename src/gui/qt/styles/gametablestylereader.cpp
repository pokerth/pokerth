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

GameTableStyleReader::GameTableStyleReader(ConfigFile *c, QWidget *w) : myConfig(c), myW(w), fallBack(0), loadedSuccessfull(0)
{

	//set fonts and font sizes
#ifdef _WIN32
	font1String = "font-family: \"Arial\";";
	font2String = "font-family: \"Nimbus Sans L\";";
	cashFontSize = "11";
	setLabelFontSize = "11";
	playerNameLabelFontSize = "11";
	smallBoardFontSize = "13";
	bigBoardFontSize = "18";
	humanPlayerButtonFontSize = "13";
	betValueFontSize = "11";
	
	tabBarPaddingTop = "2";
	tabBarPaddingSide = "10";
#else 
	#ifdef __APPLE__	
		font1String = "font-family: \"Lucida Grande\";";
		font2String = "font-family: \"Lucida Grande\";";

		tabBarPaddingTop = "1";
		tabBarPaddingSide = "9";
	#else 
		font1String = "font-family: \"Nimbus Sans L\";";
		font2String = "font-family: \"Bitstream Vera Sans\";";

		tabBarPaddingTop = "0";
		tabBarPaddingSide = "9";
	#endif
	textBrowserFontsize= "10";
	cashFontSize = "10";
	setLabelFontSize = "10";
	playerNameLabelFontSize = "11";
	smallBoardFontSize = "13";
	bigBoardFontSize = "17";
	humanPlayerButtonFontSize = "12";
	betValueFontSize = "10";
#endif

}


GameTableStyleReader::~GameTableStyleReader()
{
}

void GameTableStyleReader::readStyleFile(QString file) {

	//if style file failed --> default style fallback
	if(QFile(file).exists()) { 
		currentFileName = QFile(file).fileName();
	}
	else { 
		currentFileName = QFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default/defaulttablestyle.xml").fileName(); 
		fallBack = 1;
	}

	QFile myFile(currentFileName);
	myFile.open(QIODevice::ReadOnly);
	fileContent = myFile.readAll();	

	QFileInfo info(currentFileName);
	currentDir = info.absolutePath()+"/";

	//start reading the file and fill vars	
	string tempString1("");
	TiXmlDocument doc; 
	doc.Parse(fileContent.constData());

	if(doc.RootElement()) {
		TiXmlHandle docHandle( &doc );	
	
		TiXmlElement* itemsList = docHandle.FirstChild( "PokerTH" ).FirstChild( "TableStyle" ).FirstChild().ToElement();
		for( ; itemsList; itemsList=itemsList->NextSiblingElement()) {
			const char *tmpStr1 = itemsList->Attribute("value");
			if (tmpStr1) {
				tempString1 = tmpStr1;

// 				INFOS
				if(itemsList->ValueStr() == "StyleDescription") { StyleDescription = QString::fromUtf8(tempString1.c_str()); }
				else if(itemsList->ValueStr() == "StyleMaintainerName") { StyleMaintainerName = QString::fromUtf8(tempString1.c_str()); }
				else if(itemsList->ValueStr() == "StyleMaintainerEMail") { StyleMaintainerEMail = QString::fromUtf8(tempString1.c_str()); }
				else if(itemsList->ValueStr() == "StyleCreateDate") { StyleCreateDate = QString::fromUtf8(tempString1.c_str()); }
				else if(itemsList->ValueStr() == "PokerTHStyleFileVersion") { PokerTHStyleFileVersion = QString::fromUtf8(tempString1.c_str()); }
// 				WINDOWS SETTINGS
				else if (itemsList->ValueStr() == "IfFixedWindowSize") { IfFixedWindowSize = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FixedWindowWidth") { FixedWindowWidth = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FixedWindowHeight") { FixedWindowHeight = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MinimumWindowWidth") { MinimumWindowWidth = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MinimumWindowHeight") { MinimumWindowHeight = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MaximumWindowWidth") { MaximumWindowWidth = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MaximumWindowHeight") { MaximumWindowHeight = QString::fromUtf8(tempString1.c_str()); }
// 				PICS
				else if (itemsList->ValueStr() == "Preview") { Preview = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionAllInI18NPic") { ActionAllInI18NPic = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionRaiseI18NPic") { ActionRaiseI18NPic = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionBetI18NPic") { ActionBetI18NPic = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionCallI18NPic") { ActionCallI18NPic = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionCheckI18NPic") { ActionCheckI18NPic = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionFoldI18NPic") { ActionFoldI18NPic = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionWinnerI18NPic") { ActionWinnerI18NPic = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BigBlindPuck") { BigBlindPuck = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "SmallBlindPuck") { SmallBlindPuck = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "DealerPuck") { DealerPuck = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "DefaultAvatar") { DefaultAvatar = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CardHolderFlop") { CardHolderFlop = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CardHolderTurn") { CardHolderTurn = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CardHolderRiver") { CardHolderRiver = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FoldButtonDefault") { FoldButtonDefault = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FoldButtonHover") { FoldButtonHover = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FoldButtonChecked") { FoldButtonChecked = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FoldButtonCheckedHover") { FoldButtonCheckedHover = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CheckCallButtonDefault") { CheckCallButtonDefault = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CheckCallButtonHover") { CheckCallButtonHover = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CheckCallButtonChecked") { CheckCallButtonChecked = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CheckCallButtonCheckedHover") { CheckCallButtonCheckedHover = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetRaiseButtonDefault") { BetRaiseButtonDefault = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetRaiseButtonHover") { BetRaiseButtonHover = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetRaiseButtonChecked") { BetRaiseButtonChecked = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetRaiseButtonCheckedHover") { BetRaiseButtonCheckedHover = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "AllInButtonDefault") { AllInButtonDefault = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "AllInButtonHover") { AllInButtonHover = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "AllInButtonChecked") { AllInButtonChecked = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "AllInButtonCheckedHover") { AllInButtonCheckedHover = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "RadioButtonPressed") { RadioButtonPressed = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "RadioButtonChecked") { RadioButtonChecked = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "RadioButtonCheckedHover") { RadioButtonCheckedHover = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "RadioButtonUnchecked") { RadioButtonUnchecked = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "RadioButtonUncheckedHover") { RadioButtonUncheckedHover = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "PlayerTopSeatInactive") { PlayerTopSeatInactive = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "PlayerTopSeatActive") { PlayerTopSeatActive = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "PlayerBottomSeatInactive") { PlayerBottomSeatInactive = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "PlayerBottomSeatActive") { PlayerBottomSeatActive = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "Table") { Table = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "HandRanking") { HandRanking = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ToolBoxBackground") { ToolBoxBackground = currentDir+QString::fromUtf8(tempString1.c_str()); }

				//I18N ACTION STRINGS
				else if (itemsList->ValueStr() == "ActionAllInI18NString") { ActionAllInI18NString = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionRaiseI18NString") { ActionRaiseI18NString = QString::fromUtf8(tempString1.c_str()); }	
				else if (itemsList->ValueStr() == "ActionBetI18NString") { ActionBetI18NString = QString::fromUtf8(tempString1.c_str()); }
     				else if (itemsList->ValueStr() == "ActionCallI18NString") { ActionCallI18NString = QString::fromUtf8(tempString1.c_str()); }
     				else if (itemsList->ValueStr() == "ActionCheckI18NString") { ActionCheckI18NString = QString::fromUtf8(tempString1.c_str()); }
     				else if (itemsList->ValueStr() == "ActionFoldI18NString") { ActionFoldI18NString = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "PotI18NString") { PotI18NString = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "TotalI18NString") { TotalI18NString = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetsI18NString") { BetsI18NString = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "GameI18NString") { GameI18NString = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "HandI18NString") { HandI18NString = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "PreflopI18NString") { PreflopI18NString = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FlopI18NString") { FlopI18NString = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "TurnI18NString") { TurnI18NString = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "RiverI18NString") { RiverI18NString = QString::fromUtf8(tempString1.c_str()); }

// 				COLORS
				else if (itemsList->ValueStr() == "FKeyIndicatorColor") { FKeyIndicatorColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ChanceLabelPossibleColor") { ChanceLabelPossibleColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ChanceLabelImpossibleColor") { ChanceLabelImpossibleColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ChatTextNickNotifyColor") { ChatTextNickNotifyColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ChatLogTextColor") { ChatLogTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ChatLogBgColor") { ChatLogBgColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ChatLogScrollBarBorderColor") { ChatLogScrollBarBorderColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ChatLogScrollBarBgColor") { ChatLogScrollBarBgColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ChatLogScrollBarHandleBorderColor") { ChatLogScrollBarHandleBorderColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ChatLogScrollBarHandleBgColor") { ChatLogScrollBarHandleBgColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ChatLogScrollBarArrowBorderColor") { ChatLogScrollBarArrowBorderColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ChatLogScrollBarArrowBgColor") { ChatLogScrollBarArrowBgColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "LogWinnerMainPotColor") { LogWinnerMainPotColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "LogWinnerSidePotColor") { LogWinnerSidePotColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "LogPlayerSitsOutColor") { LogPlayerSitsOutColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "LogNewGameAdminColor") { LogNewGameAdminColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "TabWidgetBorderColor") { TabWidgetBorderColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "TabWidgetBgColor") { TabWidgetBgColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "TabWidgetTextColor") { TabWidgetTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MenuBgColor") { MenuBgColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MenuTextColor") { MenuTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BreakLobbyButtonBgColor") { BreakLobbyButtonBgColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BreakLobbyButtonTextColor") { BreakLobbyButtonTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BreakLobbyButtonBgDisabledColor") { BreakLobbyButtonBgDisabledColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BreakLobbyButtonTextDisabledColor") { BreakLobbyButtonTextDisabledColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BreakLobbyButtonBgBlinkColor") { BreakLobbyButtonBgBlinkColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BreakLobbyButtonTextBlinkColor") { BreakLobbyButtonTextBlinkColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "PlayerCashTextColor") { PlayerCashTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "PlayerBetTextColor") { PlayerBetTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "PlayerNickTextColor") { PlayerNickTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BoardBigTextColor") { BoardBigTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BoardSmallTextColor") { BoardSmallTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "SpeedTextColor") { SpeedTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "VoteButtonBgColor") { VoteButtonBgColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "VoteButtonTextColor") { VoteButtonTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetInputTextColor") { BetInputTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetInputBgColor") { BetInputBgColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetInputDisabledTextColor") { BetInputDisabledTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetInputDisabledBgColor") { BetInputDisabledBgColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FoldButtonTextColor") { FoldButtonTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FoldButtonCheckableTextColor") { FoldButtonCheckableTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CheckCallButtonTextColor") { CheckCallButtonTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "CheckCallButtonCheckableTextColor") { CheckCallButtonCheckableTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetRaiseButtonTextColor") { BetRaiseButtonTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetRaiseButtonCheckableTextColor") { BetRaiseButtonCheckableTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "AllInButtonTextColor") { AllInButtonTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "AllInButtonCheckableTextColor") { AllInButtonCheckableTextColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetSpeedSliderGrooveBgColor") { BetSpeedSliderGrooveBgColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetSpeedSliderGrooveBorderColor") { BetSpeedSliderGrooveBorderColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetSpeedSliderHandleBgColor") { BetSpeedSliderHandleBgColor = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "BetSpeedSliderHandleBorderColor") { BetSpeedSliderHandleBorderColor = QString::fromUtf8(tempString1.c_str()); }
// 				SIZES
				else if (itemsList->ValueStr() == "ChatLogTextSize") { ChatLogTextSize = QString::fromUtf8(tempString1.c_str()); }
			}
		}

		//check if style items are left and if pictures where not found and show warning
		leftItems.clear();
		itemPicsLeft.clear();

// 		INFOS
		if(StyleDescription == "") { leftItems << "StyleDescription"; }
		if(StyleMaintainerName == "") { leftItems << "StyleMaintainerName"; }
		if(StyleMaintainerEMail == "") { leftItems << "StyleMaintainerEMail"; }
		if(StyleCreateDate == "") { leftItems << "StyleCreateDate"; }
		if(PokerTHStyleFileVersion == "") { leftItems << "PokerTHStyleFileVersion"; }
// 		WINDOWS SETTINGS
		if(IfFixedWindowSize == "") { leftItems << "IfFixedWindowSize"; }
		if(FixedWindowWidth == "") { leftItems << "FixedWindowWidth"; }
		if(FixedWindowHeight == "") { leftItems << "FixedWindowHeight"; }
		if(MinimumWindowWidth == "") { leftItems << "MinimumWindowWidth"; }
		if(MinimumWindowHeight == "") { leftItems << "MinimumWindowHeight"; }
		if(MaximumWindowWidth == "") { leftItems << "MaximumWindowWidth"; }
		if(MaximumWindowHeight == "") { leftItems << "MaximumWindowHeight"; }
// 		PICS
		if(ActionAllInI18NPic == "") { leftItems << "ActionAllInI18NPic"; }
		else if(ActionAllInI18NPic != QString(currentDir+"NULL") && !QFile(ActionAllInI18NPic).exists()) { itemPicsLeft << "ActionAllInI18NPic = "+ActionAllInI18NPic; }

		if(ActionRaiseI18NPic == "") { leftItems << "ActionRaiseI18NPic"; }
		else if(ActionRaiseI18NPic != QString(currentDir+"NULL") && !QFile(ActionRaiseI18NPic).exists()) { itemPicsLeft << "ActionRaiseI18NPic = "+ActionRaiseI18NPic; }

		if(ActionBetI18NPic == "") { leftItems << "ActionBetI18NPic"; }
		else if(ActionBetI18NPic != QString(currentDir+"NULL") && !QFile(ActionBetI18NPic).exists()) { itemPicsLeft << "ActionBetI18NPic = "+ActionBetI18NPic; }

		if(ActionCallI18NPic == "") { leftItems << "ActionCallI18NPic"; }
		else if(ActionCallI18NPic != QString(currentDir+"NULL") && !QFile(ActionCallI18NPic).exists()) { itemPicsLeft << "ActionCallI18NPic = "+ActionCallI18NPic; }

		if(ActionCheckI18NPic == "") { leftItems << "ActionCheckI18NPic"; }
		else if(ActionCheckI18NPic != QString(currentDir+"NULL") && !QFile(ActionCheckI18NPic).exists()) { itemPicsLeft << "ActionCheckI18NPic = "+ActionCheckI18NPic; }

		if(ActionFoldI18NPic == "") { leftItems << "ActionFoldI18NPic"; }
		else if(ActionFoldI18NPic != QString(currentDir+"NULL") && !QFile(ActionFoldI18NPic).exists()) { itemPicsLeft << "ActionFoldI18NPic = "+ActionFoldI18NPic; }

		if(ActionWinnerI18NPic == "") { leftItems << "ActionWinnerI18NPic"; }
		else if(ActionWinnerI18NPic != QString(currentDir+"NULL") && !QFile(ActionWinnerI18NPic).exists()) { itemPicsLeft << "ActionWinnerI18NPic = "+ActionWinnerI18NPic; }

		if(BigBlindPuck == "") { leftItems << "BigBlindPuck"; }
		else if(BigBlindPuck != QString(currentDir+"NULL") && !QFile(BigBlindPuck).exists()) { itemPicsLeft << "BigBlindPuck = "+BigBlindPuck; }

		if(SmallBlindPuck == "") { leftItems << "SmallBlindPuck"; }
		else if(SmallBlindPuck != QString(currentDir+"NULL") && !QFile(SmallBlindPuck).exists()) { itemPicsLeft << "SmallBlindPuck = "+SmallBlindPuck; }

		if(DealerPuck == "") { leftItems << "DealerPuck"; }
		else if(DealerPuck != QString(currentDir+"NULL") && !QFile(DealerPuck).exists()) { itemPicsLeft << "DealerPuck = "+DealerPuck; }

		if(DefaultAvatar == "") { leftItems << "DefaultAvatar"; }
		else if(DefaultAvatar != QString(currentDir+"NULL") && !QFile(DefaultAvatar).exists()) { itemPicsLeft << "DefaultAvatar = "+DefaultAvatar; }

		if(CardHolderFlop == "") { leftItems << "CardHolderFlop"; }
		else if(CardHolderFlop != QString(currentDir+"NULL") && !QFile(CardHolderFlop).exists()) { itemPicsLeft << "CardHolderFlop = "+CardHolderFlop; }

		if(CardHolderTurn == "") { leftItems << "CardHolderTurn"; }
		else if(CardHolderTurn != QString(currentDir+"NULL") && !QFile(CardHolderTurn).exists()) { itemPicsLeft << "CardHolderTurn = "+CardHolderTurn; }

		if(CardHolderRiver == "") { leftItems << "CardHolderRiver"; }
		else if(CardHolderRiver != QString(currentDir+"NULL") && !QFile(CardHolderRiver).exists()) { itemPicsLeft << "CardHolderRiver = "+CardHolderRiver; }

		if(FoldButtonDefault == "") { leftItems << "FoldButtonDefault"; }
		else if(FoldButtonDefault != QString(currentDir+"NULL") && !QFile(FoldButtonDefault).exists()) { itemPicsLeft << "FoldButtonDefault = "+FoldButtonDefault; }

		if(FoldButtonHover == "") { leftItems << "FoldButtonHover"; }
		else if(FoldButtonHover != QString(currentDir+"NULL") && !QFile(FoldButtonHover).exists()) { itemPicsLeft << "FoldButtonHover = "+FoldButtonHover; }

		if(FoldButtonChecked == "") { leftItems << "FoldButtonChecked"; }
		else if(FoldButtonChecked != QString(currentDir+"NULL") && !QFile(FoldButtonChecked).exists()) { itemPicsLeft << "FoldButtonChecked = "+FoldButtonChecked; }

		if(FoldButtonCheckedHover == "") { leftItems << "FoldButtonCheckedHover"; }
		else if(FoldButtonCheckedHover != QString(currentDir+"NULL") && !QFile(FoldButtonCheckedHover).exists()) { itemPicsLeft << "FoldButtonCheckedHover = "+FoldButtonCheckedHover; }

		if(CheckCallButtonDefault == "") { leftItems << "CheckCallButtonDefault"; }
		else if(CheckCallButtonDefault != QString(currentDir+"NULL") && !QFile(CheckCallButtonDefault).exists()) { itemPicsLeft << "CheckCallButtonDefault = "+CheckCallButtonDefault; }

		if(CheckCallButtonHover == "") { leftItems << "CheckCallButtonHover"; }
		else if(CheckCallButtonHover != QString(currentDir+"NULL") && !QFile(CheckCallButtonHover).exists()) { itemPicsLeft << "CheckCallButtonHover = "+CheckCallButtonHover; }

		if(CheckCallButtonChecked == "") { leftItems << "CheckCallButtonChecked"; }
		else if(CheckCallButtonChecked != QString(currentDir+"NULL") && !QFile(CheckCallButtonChecked).exists()) { itemPicsLeft << "CheckCallButtonChecked = "+CheckCallButtonChecked; }

		if(CheckCallButtonCheckedHover == "") { leftItems << "CheckCallButtonCheckedHover"; }
		else if(CheckCallButtonCheckedHover != QString(currentDir+"NULL") && !QFile(CheckCallButtonCheckedHover).exists()) { itemPicsLeft << "CheckCallButtonCheckedHover = "+CheckCallButtonCheckedHover; }

		if(BetRaiseButtonDefault == "") { leftItems << "BetRaiseButtonDefault"; }
		else if(BetRaiseButtonDefault != QString(currentDir+"NULL") && !QFile(BetRaiseButtonDefault).exists()) { itemPicsLeft << "BetRaiseButtonDefault = "+BetRaiseButtonDefault; }

		if(BetRaiseButtonHover == "") { leftItems << "BetRaiseButtonHover"; }
		else if(BetRaiseButtonHover != QString(currentDir+"NULL") && !QFile(BetRaiseButtonHover).exists()) { itemPicsLeft << "BetRaiseButtonHover = "+BetRaiseButtonHover; }

		if(BetRaiseButtonChecked == "") { leftItems << "BetRaiseButtonChecked"; }
		else if(BetRaiseButtonChecked != QString(currentDir+"NULL") && !QFile(BetRaiseButtonChecked).exists()) { itemPicsLeft << "BetRaiseButtonChecked = "+BetRaiseButtonChecked; }

		if(BetRaiseButtonCheckedHover == "") { leftItems << "BetRaiseButtonCheckedHover"; }
		else if(BetRaiseButtonCheckedHover != QString(currentDir+"NULL") && !QFile(BetRaiseButtonCheckedHover).exists()) { itemPicsLeft << "BetRaiseButtonCheckedHover = "+BetRaiseButtonCheckedHover; }

		if(AllInButtonDefault == "") { leftItems << "AllInButtonDefault"; }
		else if(AllInButtonDefault != QString(currentDir+"NULL") && !QFile(AllInButtonDefault).exists()) { itemPicsLeft << "AllInButtonDefault = "+AllInButtonDefault; }

		if(AllInButtonHover == "") { leftItems << "AllInButtonHover"; }
		else if(AllInButtonHover != QString(currentDir+"NULL") && !QFile(AllInButtonHover).exists()) { itemPicsLeft << "AllInButtonHover = "+AllInButtonHover; }

		if(AllInButtonChecked == "") { leftItems << "AllInButtonChecked"; }
		else if(AllInButtonChecked != QString(currentDir+"NULL") && !QFile(AllInButtonChecked).exists()) { itemPicsLeft << "AllInButtonChecked = "+AllInButtonChecked; }

		if(AllInButtonCheckedHover == "") { leftItems << "AllInButtonCheckedHover"; }
		else if(AllInButtonCheckedHover != QString(currentDir+"NULL") && !QFile(AllInButtonCheckedHover).exists()) { itemPicsLeft << "AllInButtonCheckedHover = "+AllInButtonCheckedHover; }

		if(RadioButtonPressed == "") { leftItems << "RadioButtonPressed"; }
		else if(RadioButtonPressed != QString(currentDir+"NULL") && !QFile(RadioButtonPressed).exists()) { itemPicsLeft << "RadioButtonPressed = "+RadioButtonPressed; }

		if(RadioButtonChecked == "") { leftItems << "RadioButtonChecked"; }
		else if(RadioButtonChecked != QString(currentDir+"NULL") && !QFile(RadioButtonChecked).exists()) { itemPicsLeft << "RadioButtonChecked = "+RadioButtonChecked; }

		if(RadioButtonCheckedHover == "") { leftItems << "RadioButtonCheckedHover"; }
		else if(RadioButtonCheckedHover != QString(currentDir+"NULL") && !QFile(RadioButtonCheckedHover).exists()) { itemPicsLeft << "RadioButtonCheckedHover = "+RadioButtonCheckedHover; }

		if(RadioButtonUnchecked == "") { leftItems << "RadioButtonUnchecked"; }
		else if(RadioButtonUnchecked != QString(currentDir+"NULL") && !QFile(RadioButtonUnchecked).exists()) { itemPicsLeft << "RadioButtonUnchecked = "+RadioButtonUnchecked; }

		if(RadioButtonUncheckedHover == "") { leftItems << "RadioButtonUncheckedHover"; }
		else if(RadioButtonUncheckedHover != QString(currentDir+"NULL") && !QFile(RadioButtonUncheckedHover).exists()) { itemPicsLeft << "RadioButtonUncheckedHover = "+RadioButtonUncheckedHover; }

		if(PlayerTopSeatActive == "") { leftItems << "PlayerTopSeatActive"; }
		else if(PlayerTopSeatActive != QString(currentDir+"NULL") && !QFile(PlayerTopSeatActive).exists()) { itemPicsLeft << "PlayerTopSeatActive = "+PlayerTopSeatActive; }

		if(PlayerTopSeatInactive == "") { leftItems << "PlayerTopSeatInactive"; }
		else if(PlayerTopSeatInactive != QString(currentDir+"NULL") && !QFile(PlayerTopSeatInactive).exists()) { itemPicsLeft << "PlayerTopSeatInactive = "+PlayerTopSeatInactive; }

		if(PlayerBottomSeatActive == "") { leftItems << "PlayerBottomSeatActive"; }
		else if(PlayerBottomSeatActive != QString(currentDir+"NULL") && !QFile(PlayerBottomSeatActive).exists()) { itemPicsLeft << "PlayerBottomSeatActive = "+PlayerBottomSeatActive; }

		if(PlayerBottomSeatInactive == "") { leftItems << "PlayerBottomSeatInactive"; }
		else if(PlayerBottomSeatInactive != QString(currentDir+"NULL") && !QFile(PlayerBottomSeatInactive).exists()) { itemPicsLeft << "PlayerBottomSeatInactive = "+PlayerBottomSeatInactive; }

		if(Table == "") { leftItems << "Table"; }
		else if(Table != QString(currentDir+"NULL") && !QFile(Table).exists()) { itemPicsLeft << "Table = "+Table; }

		if(HandRanking == "") { leftItems << "HandRanking"; }
		else if(HandRanking != QString(currentDir+"NULL") && !QFile(HandRanking).exists()) { itemPicsLeft << "HandRanking = "+HandRanking; }

		if(ToolBoxBackground == "") { leftItems << "ToolBoxBackground"; }
		else if(ToolBoxBackground != QString(currentDir+"NULL") && !QFile(ToolBoxBackground).exists()) { itemPicsLeft << "ToolBoxBackground = "+ToolBoxBackground; }

		//I18N ACTION STRINGS
		if(ActionAllInI18NString == "") { leftItems << "ActionAllInI18NString"; }
		if(ActionRaiseI18NString == "") { leftItems << "ActionRaiseI18NString"; }
     		if(ActionBetI18NString == "") { leftItems << "ActionBetI18NString"; }
     		if(ActionCallI18NString == "") { leftItems << "ActionCallI18NString"; }
		if(ActionCheckI18NString == "") { leftItems << "ActionCheckI18NString"; }
		if(ActionFoldI18NString == "") { leftItems << "ActionFoldI18NString"; }
		if(PotI18NString == "") { leftItems << "PotI18NString"; }
		if(TotalI18NString == "") { leftItems << "TotalI18NString"; }
		if(BetsI18NString == "") { leftItems << "BetsI18NString"; }		
		if(GameI18NString == "") { leftItems << "GameI18NString"; }
		if(HandI18NString == "") { leftItems << "HandI18NString"; }
		if(PreflopI18NString == "") { leftItems << "PreflopI18NString"; }
		if(FlopI18NString == "") { leftItems << "FlopI18NString"; }
		if(TurnI18NString == "") { leftItems << "TurnI18NString"; }
		if(RiverI18NString == "") { leftItems << "RiverI18NString"; }

// 		COLORS
		if(FKeyIndicatorColor == "") { leftItems << "FKeyIndicatorColor"; }
		if(ChanceLabelPossibleColor == "") { leftItems << "ChanceLabelPossibleColor"; }
		if(ChanceLabelImpossibleColor == "") { leftItems << "ChanceLabelImpossibleColor"; }
		if(ChatLogTextColor == "") { leftItems << "ChatLogTextColor"; }
		if(ChatTextNickNotifyColor == "") { leftItems << "ChatTextNickNotifyColor"; }
		if(ChatLogBgColor == "") { leftItems << "ChatLogBgColor"; }
		if(ChatLogScrollBarBorderColor == "") { leftItems << "ChatLogScrollBarBorderColor"; }
		if(ChatLogScrollBarBgColor == "") { leftItems << "ChatLogScrollBarBgColor"; }
		if(ChatLogScrollBarHandleBorderColor == "") { leftItems << "ChatLogScrollBarHandleBorderColor"; }
		if(ChatLogScrollBarHandleBgColor == "") { leftItems << "ChatLogScrollBarHandleBgColor"; }
		if(ChatLogScrollBarArrowBorderColor == "") { leftItems << "ChatLogScrollBarArrowBorderColor"; }
		if(ChatLogScrollBarArrowBgColor == "") { leftItems << "ChatLogScrollBarArrowBgColor"; }
		if(LogWinnerMainPotColor == "") { leftItems << "LogWinnerMainPotColor"; }
		if(LogWinnerSidePotColor == "") { leftItems << "LogWinnerSidePotColor"; }
		if(LogPlayerSitsOutColor == "") { leftItems << "LogPlayerSitsOutColor"; }
		if(LogNewGameAdminColor == "") { leftItems << "LogNewGameAdminColor"; }
		if(TabWidgetBorderColor == "") { leftItems << "TabWidgetBorderColor"; }
		if(TabWidgetBgColor == "") { leftItems << "TabWidgetBgColor"; }
		if(TabWidgetTextColor == "") { leftItems << "TabWidgetTextColor"; }
		if(MenuBgColor == "") { leftItems << "MenuBgColor"; }
		if(MenuTextColor == "") { leftItems << "MenuTextColor"; }
		if(BreakLobbyButtonBgColor == "") { leftItems << "BreakLobbyButtonBgColor"; }
		if(BreakLobbyButtonTextColor == "") { leftItems << "BreakLobbyButtonTextColor"; }
		if(BreakLobbyButtonBgDisabledColor == "") { leftItems << "BreakLobbyButtonBgDisabledColor"; }
		if(BreakLobbyButtonTextDisabledColor == "") { leftItems << "BreakLobbyButtonTextDisabledColor"; }
		if(BreakLobbyButtonBgBlinkColor == "") { leftItems << "BreakLobbyButtonBgBlinkColor"; }
		if(BreakLobbyButtonTextBlinkColor == "") { leftItems << "BreakLobbyButtonTextBlinkColor"; }
		if(PlayerCashTextColor == "") { leftItems << "PlayerCashTextColor"; }
		if(PlayerBetTextColor == "") { leftItems << "PlayerBetTextColor"; }
		if(PlayerNickTextColor == "") { leftItems << "PlayerNickTextColor"; }
		if(BoardBigTextColor == "") { leftItems << "BoardBigTextColor"; }
		if(BoardSmallTextColor == "") { leftItems << "BoardSmallTextColor"; }
		if(SpeedTextColor == "") { leftItems << "SpeedTextColor"; }
		if(VoteButtonBgColor == "") { leftItems << "VoteButtonBgColor"; }
		if(VoteButtonTextColor == "") { leftItems << "VoteButtonTextColor"; }
		if(BetInputTextColor == "") { leftItems << "BetInputTextColor"; }
		if(BetInputBgColor == "") { leftItems << "BetInputBgColor"; }
		if(BetInputDisabledTextColor == "") { leftItems << "BetInputDisabledTextColor"; }
		if(BetInputDisabledBgColor == "") { leftItems << "BetInputDisabledBgColor"; }
		if(FoldButtonTextColor == "") { leftItems << "FoldButtonTextColor"; }
		if(FoldButtonCheckableTextColor == "") { leftItems << "FoldButtonCheckableTextColor"; }
		if(CheckCallButtonTextColor == "") { leftItems << "CheckCallButtonTextColor"; }
		if(CheckCallButtonCheckableTextColor == "") { leftItems << "CheckCallButtonCheckableTextColor"; }
		if(BetRaiseButtonTextColor == "") { leftItems << "BetRaiseButtonTextColor"; }
		if(BetRaiseButtonCheckableTextColor == "") { leftItems << "BetRaiseButtonCheckableTextColor"; }
		if(AllInButtonTextColor == "") { leftItems << "AllInButtonTextColor"; }
		if(AllInButtonCheckableTextColor == "") { leftItems << "AllInButtonCheckableTextColor"; }
		if(BetSpeedSliderGrooveBgColor == "") { leftItems << "BetSpeedSliderGrooveBgColor"; }
		if(BetSpeedSliderGrooveBorderColor == "") { leftItems << "BetSpeedSliderGrooveBorderColor"; }
		if(BetSpeedSliderHandleBgColor == "") { leftItems << "BetSpeedSliderHandleBgColor"; }
		if(BetSpeedSliderHandleBorderColor == "") { leftItems << "BetSpeedSliderHandleBorderColor"; }
// 		SIZE
		if(ChatLogTextSize == "") { leftItems << "ChatLogTextSize"; }
			
                //set loadedSuccessfull TRUE if everything works
                if(leftItems.isEmpty() && itemPicsLeft.isEmpty() && PokerTHStyleFileVersion != "" && PokerTHStyleFileVersion.toInt() == POKERTH_GT_STYLE_FILE_VERSION)
                        loadedSuccessfull = 1;
                else
                        loadedSuccessfull = 0;

		//if one or more items are left show detailed error message
		if(!leftItems.isEmpty() && myW != 0) showLeftItemsErrorMessage(StyleDescription, leftItems, StyleMaintainerEMail);
		else {
			//if one or more pictures where not found show detailed error message
			if(!itemPicsLeft.isEmpty() && myW != 0) showItemPicsLeftErrorMessage(StyleDescription, itemPicsLeft, StyleMaintainerEMail);
			//check for style file version
                        if(PokerTHStyleFileVersion != "" && PokerTHStyleFileVersion.toInt() != POKERTH_GT_STYLE_FILE_VERSION) {
                                QString EMail;
                                if(StyleMaintainerEMail != "NULL") EMail = StyleMaintainerEMail;
				QMessageBox::warning(myW, tr("Game Table Style Error"),
                                        tr("Selected game table style \"%1\" seems to be outdated. \n The current PokerTH game table style version is \"%2\", but this style has version \"%3\" set. \n\nPlease contact the game table style builder %4.").arg(StyleDescription).arg(POKERTH_GT_STYLE_FILE_VERSION).arg(PokerTHStyleFileVersion).arg(EMail),
					QMessageBox::Ok);
			}
		}
	}	
        else {
            loadedSuccessfull = 0;
            QMessageBox::warning(myW, tr("Game Table Style Error"),
                                        tr("Cannot load game table style file: %1 \n\nPlease check the style file or choose another style!").arg(currentFileName),
                                        QMessageBox::Ok);
        }
}

void GameTableStyleReader::showLeftItemsErrorMessage(QString style, QStringList failedItems, QString email)
{
	QString items = failedItems.join(", ");
        QString EMail;
        if(email != "NULL") EMail = email;

	QMessageBox::warning(myW, tr("Game Table Style Error"),
                                tr("Selected game table style \"%1\" seems to be incomplete or defective. \n\nThe value(s) of \"%2\" is/are missing. \n\nPlease contact the game table style builder %3.").arg(style).arg(items).arg(EMail),
                                QMessageBox::Ok);
}

void GameTableStyleReader::showItemPicsLeftErrorMessage(QString style, QStringList picsLeft, QString email)
{
        QString pics = picsLeft.join("\n");
        QString EMail;
        if(email != "NULL") EMail = email;

	QMessageBox::warning(myW, tr("Game Table Style Error"),
                                tr("One or more pictures from current game table style \"%1\" where not found: \n\n\"%2\" \n\nPlease contact the game table style builder %3.").arg(style).arg(pics).arg(EMail),
                                QMessageBox::Ok);
}

void GameTableStyleReader::setTableBackground(gameTableImpl *gt)
{
	gt->setStyleSheet("QMainWindow { background-image: url(\""+Table+"\"); background-position: bottom center; background-origin: content;  background-repeat: no-repeat;}");
}

void GameTableStyleReader::setChatLogStyle(QTextBrowser *tb)
{	
	tb->setStyleSheet("QTextBrowser { "+ font1String +" font-size: "+ChatLogTextSize+"px; color: #"+ChatLogTextColor+"; background-color: #"+ChatLogBgColor+"; border:none; } QScrollBar:vertical { border: 1px solid #"+ChatLogScrollBarBorderColor+"; background: #"+ChatLogScrollBarBgColor+"; width: 15px; margin: 17px -1px 17px 0px; } QScrollBar::handle:vertical { border-radius: 1px; border: 1px solid #"+ChatLogScrollBarHandleBorderColor+"; background: #"+ChatLogScrollBarHandleBgColor+"; min-height: 20px; } QScrollBar::add-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 2px; border-bottom-left-radius: 2px; border-top-right-radius: 1px; border-top-left-radius: 1px; border: 1px solid #"+ChatLogScrollBarHandleBorderColor+"; background: #"+ChatLogScrollBarHandleBgColor+"; height: 15px; subcontrol-position: bottom; subcontrol-origin: margin; } QScrollBar::sub-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 1px; border-bottom-left-radius: 1px; border-top-right-radius: 2px; border-top-left-radius: 2px; border: 1px solid #"+ChatLogScrollBarHandleBorderColor+"; background: #"+ChatLogScrollBarHandleBgColor+"; height: 15px; subcontrol-position: top; subcontrol-origin: margin; } QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical { border: 1px solid #"+ChatLogScrollBarArrowBorderColor+"; height: 3px; width: 3px; background: #"+ChatLogScrollBarArrowBgColor+"; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
	
}

void GameTableStyleReader::setChatInputStyle(QLineEdit *ci)
{
	ci->setStyleSheet("QLineEdit { "+ font1String +" font-size: "+ChatLogTextSize+"px; color: #"+ChatLogTextColor+"; background-color: #"+ChatLogBgColor+"; border-top: 2px solid #"+TabWidgetBorderColor+"; }");
}

void GameTableStyleReader::setCashLabelStyle(QLabel *cl)
{	
	cl->setStyleSheet("QLabel { "+ font2String +" font-size: "+cashFontSize+"px; font-weight: bold; color: #"+PlayerCashTextColor+"; }");
}

void GameTableStyleReader::setSetLabelStyle(QLabel *sl)
{
	sl->setStyleSheet("QLabel { "+ font2String +" font-size: "+setLabelFontSize+"px; font-weight: bold; color: #"+PlayerBetTextColor+"; }");
}

void GameTableStyleReader::setPlayerNameLabelStyle(QLabel *pnl)
{
	pnl->setStyleSheet("QLabel { "+ font2String +" font-size: "+playerNameLabelFontSize+"px; font-weight: bold; color: #"+PlayerNickTextColor+"; }");
}

void GameTableStyleReader::setSmallFontBoardStyle(QLabel *l)
{
	l->setStyleSheet("QLabel { "+ font2String +" font-size: "+smallBoardFontSize+"px; font-weight: bold; color: #"+BoardSmallTextColor+"; }");
}

void GameTableStyleReader::setBigFontBoardStyle(QLabel *l)
{
	l->setStyleSheet("QLabel { "+ font2String +" font-size: "+bigBoardFontSize+"px; font-weight: bold; color: #"+BoardBigTextColor+"; }");
}

void GameTableStyleReader::setCardHolderStyle(QLabel *l, int bero)
{
	switch(bero) {
		case 0: l->setPixmap(CardHolderFlop);
		break;
		case 1: l->setPixmap(CardHolderTurn);
		break;
		case 2: l->setPixmap(CardHolderRiver);
		break;
	}
}

void GameTableStyleReader::setMenuBarStyle(QMenuBar *mb)
{
        mb->setStyleSheet("QMenuBar { background-color: #"+MenuBgColor+"; font-size:12px; } QMenuBar::item { color: #"+MenuTextColor+"; }");
}

void GameTableStyleReader::setBreakButtonStyle(QPushButton *bb, int state)
{
	switch(state) {
// 		default
		case 0: bb->setStyleSheet("QPushButton:enabled { background-color: #"+BreakLobbyButtonBgColor+"; color: #"+BreakLobbyButtonTextColor+";} QPushButton:disabled { background-color: #"+BreakLobbyButtonBgDisabledColor+"; color: #"+BreakLobbyButtonTextDisabledColor+"; font-weight: 900;}");
		break;
// 		blink
		case 1: bb->setStyleSheet("QPushButton { background-color: #"+BreakLobbyButtonBgBlinkColor+"; color: "+BreakLobbyButtonTextBlinkColor+";}");

		break;
	}
}

void GameTableStyleReader::setSpeedStringStyle(QLabel *l)
{
	l->setStyleSheet("QLabel { color: #"+SpeedTextColor+";}");	
}

void GameTableStyleReader::setVoteButtonStyle(QPushButton *b)
{
	b->setStyleSheet("QPushButton:enabled { background-color: #"+VoteButtonBgColor+"; color: #"+VoteButtonTextColor+";} ");
}

void GameTableStyleReader::setVoteStringsStyle(QLabel *l)
{
	l->setStyleSheet("QLabel { color: #"+TabWidgetTextColor+"; font-size: 11px;}");
}

void GameTableStyleReader::setPlayerSeatInactiveStyle(QGroupBox *ps)
{
	// 	check if seat is on top or bottom line
	if(ps->objectName() == "groupBox2" || ps->objectName() == "groupBox1" || ps->objectName() == "groupBox0" || ps->objectName() == "groupBox9" || ps->objectName() == "groupBox8") {
		ps->setStyleSheet("QGroupBox { border:none; background-image: url(\""+PlayerBottomSeatInactive+"\") }"); 
	}
	else {
		ps->setStyleSheet("QGroupBox { border:none; background-image: url(\""+PlayerTopSeatInactive+"\") }"); 
	}
}

void GameTableStyleReader::setPlayerSeatActiveStyle(QGroupBox *ps)
{
// 	check if seat is on top or bottom line
	if(ps->objectName() == "groupBox2" || ps->objectName() == "groupBox1" || ps->objectName() == "groupBox0" || ps->objectName() == "groupBox9" || ps->objectName() == "groupBox8") {
		ps->setStyleSheet("QGroupBox { border:none; background-image: url(\""+PlayerBottomSeatActive+"\") }"); 
	}
	else {
		ps->setStyleSheet("QGroupBox { border:none; background-image: url(\""+PlayerTopSeatActive+"\") }"); 
	}
}

void GameTableStyleReader::setBetValueInputStyle(QLineEdit *bv)
{
	bv->setStyleSheet("QLineEdit { "+ font2String +" font-size: "+betValueFontSize+"px; font-weight: bold; background-color: #"+BetInputBgColor+"; color: #"+BetInputTextColor+"; } QLineEdit:disabled { background-color: #"+BetInputDisabledBgColor+"; color: #"+BetInputDisabledTextColor+" }");
}

void GameTableStyleReader::setAwayRadioButtonsStyle(QRadioButton *rb)
{
	rb->setStyleSheet("QRadioButton { color: #"+TabWidgetTextColor+"; } QRadioButton::indicator { width: 13px; height: 13px;} QRadioButton::indicator::checked { image: url(\""+RadioButtonChecked+"\");}  QRadioButton::indicator::unchecked { image: url(\""+RadioButtonUnchecked+"\");} QRadioButton::indicator:unchecked:hover { image: url(\""+RadioButtonUncheckedHover+"\");} QRadioButton::indicator:unchecked:pressed { image: url(\""+RadioButtonPressed+"\");} QRadioButton::indicator::checked { image: url(\""+RadioButtonChecked+"\");} QRadioButton::indicator:checked:hover { image: url(\""+RadioButtonCheckedHover+"\");} QRadioButton::indicator:checked:pressed { image: url(\""+RadioButtonPressed+"\");}");

}

QString GameTableStyleReader::getActionPic(int action)
{
	QString myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());

// 	1 = fold, 2 = check, 3 = call, 4 = bet, 5 = raise, 6 = allin, 7 = winner
	switch(action) {
		case 1: { 
			if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || ActionFoldI18NPic.endsWith("NULL"))
				return myAppDataPath+"gfx/gui/misc/actionpics/action_fold.png";
			else 
				return ActionFoldI18NPic;
		}
		break;
		case 2: { 
			if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || ActionCheckI18NPic.endsWith("NULL"))
				return myAppDataPath+"gfx/gui/misc/actionpics/action_check.png";
			else 
				return ActionCheckI18NPic; 
		}
		break;
		case 3: { 
			if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || ActionCallI18NPic.endsWith("NULL"))
				return myAppDataPath+"gfx/gui/misc/actionpics/action_call.png";
			else 
				 return ActionCallI18NPic; 
		}
		break;
		case 4: {
			if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || ActionBetI18NPic.endsWith("NULL"))
				return myAppDataPath+"gfx/gui/misc/actionpics/action_bet.png";
			else 
				return ActionBetI18NPic; 
		}
		break;
		case 5: { 
			if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || ActionRaiseI18NPic.endsWith("NULL"))
				return myAppDataPath+"gfx/gui/misc/actionpics/action_raise.png";
			else 
				return ActionRaiseI18NPic; 
		}
		break;
		case 6: { 
			if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || ActionAllInI18NPic.endsWith("NULL"))
				return myAppDataPath+"gfx/gui/misc/actionpics/action_allin.png";
			else 
				return ActionAllInI18NPic; 
		}
		break;
		case 7: { 
			if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || ActionWinnerI18NPic.endsWith("NULL"))
				return myAppDataPath+"gfx/gui/misc/actionpics/action_winner.png";
			else 
				return ActionWinnerI18NPic; 
		}
		break;
		default: return QString("");
	}
}

void GameTableStyleReader::setButtonsStyle(MyActionButton *br, MyActionButton *cc, MyActionButton *f, MyActionButton *a, int state)
{
	br->setMyStyle(this);
	cc->setMyStyle(this);
	f->setMyStyle(this);
	a->setMyStyle(this);

	switch(state) {
		//default
		case 0: {
			br->setStyleSheet("QPushButton { border:none; background-image: url(\""+BetRaiseButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+BetRaiseButtonTextColor+";} QPushButton:unchecked { background-image: url(\""+BetRaiseButtonDefault+"\"); } QPushButton:checked { background-image: url(\""+BetRaiseButtonChecked+"\");} QPushButton:hover { background-image: url(\""+BetRaiseButtonHover+"\"); } QPushButton:checked:hover { background-image: url(\""+BetRaiseButtonCheckedHover+"\");}"); 
			
			cc->setStyleSheet("QPushButton { border:none; background-image: url(\""+CheckCallButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+CheckCallButtonTextColor+";} QPushButton:unchecked { background-image: url(\""+CheckCallButtonDefault+"\"); } QPushButton:checked { background-image: url(\""+CheckCallButtonChecked+"\");} QPushButton:hover { background-image: url(\""+CheckCallButtonHover+"\"); } QPushButton:checked:hover { background-image: url(\""+CheckCallButtonCheckedHover+"\");}");

			f->setStyleSheet("QPushButton { border:none; background-image: url(\""+FoldButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+FoldButtonTextColor+";}  QPushButton:unchecked { background-image: url(\""+FoldButtonDefault+"\"); } QPushButton:checked { background-image: url(\""+FoldButtonChecked+"\");} QPushButton:hover { background-image: url(\""+FoldButtonHover+"\"); } QPushButton:checked:hover { background-image: url(\""+FoldButtonCheckedHover+"\");}");

			a->setStyleSheet("QPushButton { border:none; background-image: url(\""+AllInButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+AllInButtonTextColor+";}  QPushButton:unchecked { background-image: url(\""+AllInButtonDefault+"\"); } QPushButton:checked { background-image: url(\""+AllInButtonChecked+"\");} QPushButton:hover { background-image: url(\""+AllInButtonHover+"\"); } QPushButton:checked:hover { background-image: url(\""+AllInButtonCheckedHover+"\");}");
		}
		break;
		//no hover
		case 1: {
			br->setStyleSheet("QPushButton { border:none; background-image: url(\""+BetRaiseButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+BetRaiseButtonTextColor+";} QPushButton:unchecked { background-image: url(\""+BetRaiseButtonDefault+"\"); } QPushButton:checked { background-image: url(\""+BetRaiseButtonChecked+"\");} QPushButton:hover { background-image: url(\""+BetRaiseButtonDefault+"\"); } QPushButton:checked:hover { background-image: url(\""+BetRaiseButtonChecked+"\");}"); 
			
			cc->setStyleSheet("QPushButton { border:none; background-image: url(\""+CheckCallButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+CheckCallButtonTextColor+";} QPushButton:unchecked { background-image: url(\""+CheckCallButtonDefault+"\"); } QPushButton:checked { background-image: url(\""+CheckCallButtonChecked+"\");} QPushButton:hover { background-image: url(\""+CheckCallButtonDefault+"\"); } QPushButton:checked:hover { background-image: url(\""+CheckCallButtonChecked+"\");}");

			f->setStyleSheet("QPushButton { border:none; background-image: url(\""+FoldButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+FoldButtonTextColor+";}  QPushButton:unchecked { background-image: url(\""+FoldButtonDefault+"\"); } QPushButton:checked { background-image: url(\""+FoldButtonChecked+"\");} QPushButton:hover { background-image: url(\""+FoldButtonDefault+"\"); } QPushButton:checked:hover { background-image: url(\""+FoldButtonChecked+"\");}");

			a->setStyleSheet("QPushButton { border:none; background-image: url(\""+AllInButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+AllInButtonTextColor+";}  QPushButton:unchecked { background-image: url(\""+AllInButtonDefault+"\"); } QPushButton:checked { background-image: url(\""+AllInButtonChecked+"\");} QPushButton:hover { background-image: url(\""+AllInButtonDefault+"\"); } QPushButton:checked:hover { background-image: url(\""+AllInButtonChecked+"\");}");
		}
		break;
		//checkable
		case 2: {
			br->setStyleSheet("QPushButton { border:none; background-image: url(\""+BetRaiseButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+BetRaiseButtonCheckableTextColor+";} QPushButton:unchecked { background-image: url(\""+BetRaiseButtonDefault+"\"); } QPushButton:checked { background-image: url(\""+BetRaiseButtonChecked+"\");} QPushButton:hover { background-image: url(\""+BetRaiseButtonHover+"\"); } QPushButton:checked:hover { background-image: url(\""+BetRaiseButtonCheckedHover+"\");}"); 
			
			cc->setStyleSheet("QPushButton { border:none; background-image: url(\""+CheckCallButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+CheckCallButtonCheckableTextColor+";} QPushButton:unchecked { background-image: url(\""+CheckCallButtonDefault+"\"); } QPushButton:checked { background-image: url(\""+CheckCallButtonChecked+"\");} QPushButton:hover { background-image: url(\""+CheckCallButtonHover+"\"); } QPushButton:checked:hover { background-image: url(\""+CheckCallButtonCheckedHover+"\");}");

			f->setStyleSheet("QPushButton { border:none; background-image: url(\""+FoldButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+FoldButtonCheckableTextColor+";}  QPushButton:unchecked { background-image: url(\""+FoldButtonDefault+"\"); } QPushButton:checked { background-image: url(\""+FoldButtonChecked+"\");} QPushButton:hover { background-image: url(\""+FoldButtonHover+"\"); } QPushButton:checked:hover { background-image: url(\""+FoldButtonCheckedHover+"\");}");

			a->setStyleSheet("QPushButton { border:none; background-image: url(\""+AllInButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+AllInButtonCheckableTextColor+";}  QPushButton:unchecked { background-image: url(\""+AllInButtonDefault+"\"); } QPushButton:checked { background-image: url(\""+AllInButtonChecked+"\");} QPushButton:hover { background-image: url(\""+AllInButtonHover+"\"); } QPushButton:checked:hover { background-image: url(\""+AllInButtonCheckedHover+"\");}");
		}
		break;
	}
}

void GameTableStyleReader::setToolBoxBackground(QGroupBox* gb)
{
	gb->setStyleSheet("QGroupBox { border:none; background-image: url(\""+ToolBoxBackground+"\") }");
}

void GameTableStyleReader::setTabWidgetStyle(QTabWidget *tw, QTabBar *tb)
{
	tw->setStyleSheet("QTabWidget::pane { border: 2px solid #"+TabWidgetBorderColor+"; border-radius: 2px; background-color: #"+TabWidgetBgColor+"; }  QTabWidget::tab-bar { left: 5px; } ");

	QString bottomPadding("");

#ifdef _WIN32
	bottomPadding = " padding-bottom: 3px;";
#endif

	tb->setStyleSheet("QTabBar::tab{ "+ font1String +" font-size: 11px; color: #"+TabWidgetTextColor+"; background-color: #"+TabWidgetBgColor+"; border: 2px solid #"+TabWidgetBorderColor+"; border-bottom-color: #"+TabWidgetBorderColor+"; border-top-left-radius: 4px; border-top-right-radius: 4px; padding-top: "+tabBarPaddingTop+"px;"+bottomPadding+" padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:selected, QTabBar::tab:hover { background-color: #"+TabWidgetBgColor+"; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:selected { border-color: #"+TabWidgetBorderColor+"; border-bottom-color: #"+TabWidgetBgColor+"; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;}  QTabBar::tab:!selected { margin-top: 2px; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:selected { margin-left: -4px; margin-right: -4px; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:first:selected { margin-left: 0; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:last:selected { margin-right: 0; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:only-one { margin: 0; } ");

}

void GameTableStyleReader::setWindowsGeometry(gameTableImpl *gt)
{
	if(IfFixedWindowSize.toInt()) {
		gt->setMinimumSize(FixedWindowWidth.toInt(), FixedWindowHeight.toInt());
		gt->setMaximumSize(FixedWindowWidth.toInt(), FixedWindowHeight.toInt());
		gt->resize(FixedWindowWidth.toInt(), FixedWindowHeight.toInt());

		QDesktopWidget dw;
		int availableWidth = dw.screenGeometry().width();
		int availableHeight = dw.screenGeometry().height();
		if(availableWidth == FixedWindowWidth.toInt() && availableHeight == FixedWindowHeight.toInt()) {
			gt->actionFullScreen->setEnabled(TRUE);
		}
		else {
			gt->actionFullScreen->setDisabled(TRUE);
			if(gt->isFullScreen()) {
				gt->showNormal();
				gt->move(50,50);
			}
		}
	}
	else {
		gt->setMinimumSize(MinimumWindowWidth.toInt(), MinimumWindowHeight.toInt());
		gt->setMaximumSize(MaximumWindowWidth.toInt(), MaximumWindowHeight.toInt());
		gt->resize(MinimumWindowWidth.toInt(), MinimumWindowHeight.toInt());

		QDesktopWidget dw;
		int availableWidth = dw.screenGeometry().width();
		int availableHeight = dw.screenGeometry().height();
		if(availableWidth <= MaximumWindowWidth.toInt() && availableHeight <= MaximumWindowHeight.toInt()) {
			gt->actionFullScreen->setEnabled(TRUE);
		}
		else {
			gt->actionFullScreen->setDisabled(TRUE);
			if(gt->isFullScreen()) {
				gt->showNormal();
				gt->move(50,50);
			}
		}
	}

}

void GameTableStyleReader::setSliderStyle(QSlider *s)
{
	s->setStyleSheet("QSlider::groove:horizontal { border: 1px solid #"+BetSpeedSliderGrooveBorderColor+"; height: 3px; background: #"+BetSpeedSliderGrooveBgColor+"; margin: 4px 4px; border-radius: 2px; } QSlider::handle:horizontal { background: #"+BetSpeedSliderHandleBgColor+"; border: 1px solid #"+BetSpeedSliderHandleBorderColor+"; width: 9px; margin: -6px 0; border-radius: 4px;}");

}
