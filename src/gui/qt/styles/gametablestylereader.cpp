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

GameTableStyleReader::GameTableStyleReader(ConfigFile *c, gameTableImpl *w) : myConfig(c), myW(w), fallBack(0)
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

	string tinyFileName;

	//if style file failed --> default style fallback
	if(QFile(file).exists()) { 
		currentFileName = QFile(file).fileName();
		tinyFileName = currentFileName.toUtf8().constData();		 
	}
	else { 
		currentFileName = QFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default/defaulttablestyle.xml").fileName(); 
		tinyFileName = currentFileName.toUtf8().constData(); 
		fallBack = 1;
	}
	QFileInfo info(currentFileName);
	currentDir = info.absolutePath()+"/";

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

// 				INFOS
				if(itemsList->ValueStr() == "StyleDescription") { StyleDescription = QString::fromUtf8(tempString1.c_str()); }
				else if(itemsList->ValueStr() == "StyleMaintainerEMail") { StyleMaintainerEMail = QString::fromUtf8(tempString1.c_str()); }
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
				else if (itemsList->ValueStr() == "ActionAllIn") { ActionAllIn = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionRaise") { ActionRaise = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionBet") { ActionBet = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionCall") { ActionCall = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionCheck") { ActionCheck = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionFold") { ActionFold = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ActionWinner") { ActionWinner = currentDir+QString::fromUtf8(tempString1.c_str()); }
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

// 				std::cout << "ÖLP" << itemsList->ValueStr() <<"§4s"<< itemsList->ValueStr()<<"§89"<< endl;
			}
		}
		wrongItems.clear();

// 		INFOS
		if(StyleDescription == "") { wrongItems << "StyleDescription"; }
// 		WINDOWS SETTINGS
		if(IfFixedWindowSize == "") { wrongItems << "IfFixedWindowSize"; }
		if(FixedWindowWidth == "") { wrongItems << "FixedWindowWidth"; }
		if(FixedWindowHeight == "") { wrongItems << "FixedWindowHeight"; }
		if(MinimumWindowWidth == "") { wrongItems << "MinimumWindowWidth"; }
		if(MinimumWindowHeight == "") { wrongItems << "MinimumWindowHeight"; }
		if(MaximumWindowWidth == "") { wrongItems << "MaximumWindowWidth"; }
		if(MaximumWindowHeight == "") { wrongItems << "MaximumWindowHeight"; }
// 		PICS
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
		if(RadioButtonPressed == "") { wrongItems << "RadioButtonPressed"; }
		if(RadioButtonChecked == "") { wrongItems << "RadioButtonChecked"; }
		if(RadioButtonCheckedHover == "") { wrongItems << "RadioButtonCheckedHover"; }
		if(RadioButtonUnchecked == "") { wrongItems << "RadioButtonUnchecked"; }
		if(RadioButtonUncheckedHover == "") { wrongItems << "RadioButtonUncheckedHover"; }
		if(PlayerTopSeatActive == "") { wrongItems << "PlayerTopSeatActive"; }
		if(PlayerTopSeatInactive == "") { wrongItems << "PlayerTopSeatInactive"; }
		if(PlayerBottomSeatActive == "") { wrongItems << "PlayerBottomSeatActive"; }
		if(PlayerBottomSeatInactive == "") { wrongItems << "PlayerBottomSeatInactive"; }
		if(Table == "") { wrongItems << "Table"; }
		if(HandRanking == "") { wrongItems << "HandRanking"; }
		if(ToolBoxBackground == "") { wrongItems << "ToolBoxBackground"; }
// 		COLORS
		if(FKeyIndicatorColor == "") { wrongItems << "FKeyIndicatorColor"; }
		if(ChanceLabelPossibleColor == "") { wrongItems << "ChanceLabelPossibleColor"; }
		if(ChanceLabelImpossibleColor == "") { wrongItems << "ChanceLabelImpossibleColor"; }
		if(ChatLogTextColor == "") { wrongItems << "ChatLogTextColor"; }
		if(ChatTextNickNotifyColor == "") { wrongItems << "ChatTextNickNotifyColor"; }
		if(ChatLogBgColor == "") { wrongItems << "ChatLogBgColor"; }
		if(ChatLogScrollBarBorderColor == "") { wrongItems << "ChatLogScrollBarBorderColor"; }
		if(ChatLogScrollBarBgColor == "") { wrongItems << "ChatLogScrollBarBgColor"; }
		if(ChatLogScrollBarHandleBorderColor == "") { wrongItems << "ChatLogScrollBarHandleBorderColor"; }
		if(ChatLogScrollBarHandleBgColor == "") { wrongItems << "ChatLogScrollBarHandleBgColor"; }
		if(ChatLogScrollBarArrowBorderColor == "") { wrongItems << "ChatLogScrollBarArrowBorderColor"; }
		if(ChatLogScrollBarArrowBgColor == "") { wrongItems << "ChatLogScrollBarArrowBgColor"; }
		if(LogWinnerMainPotColor == "") { wrongItems << "LogWinnerMainPotColor"; }
		if(LogWinnerSidePotColor == "") { wrongItems << "LogWinnerSidePotColor"; }
		if(LogPlayerSitsOutColor == "") { wrongItems << "LogPlayerSitsOutColor"; }
		if(LogNewGameAdminColor == "") { wrongItems << "LogNewGameAdminColor"; }
		if(TabWidgetBorderColor == "") { wrongItems << "TabWidgetBorderColor"; }
		if(TabWidgetBgColor == "") { wrongItems << "TabWidgetBgColor"; }
		if(TabWidgetTextColor == "") { wrongItems << "TabWidgetTextColor"; }
		if(MenuBgColor == "") { wrongItems << "MenuBgColor"; }
		if(MenuTextColor == "") { wrongItems << "MenuTextColor"; }
		if(BreakLobbyButtonBgColor == "") { wrongItems << "BreakLobbyButtonBgColor"; }
		if(BreakLobbyButtonTextColor == "") { wrongItems << "BreakLobbyButtonTextColor"; }
		if(BreakLobbyButtonBgDisabledColor == "") { wrongItems << "BreakLobbyButtonBgDisabledColor"; }
		if(BreakLobbyButtonTextDisabledColor == "") { wrongItems << "BreakLobbyButtonTextDisabledColor"; }
		if(BreakLobbyButtonBgBlinkColor == "") { wrongItems << "BreakLobbyButtonBgBlinkColor"; }
		if(BreakLobbyButtonTextBlinkColor == "") { wrongItems << "BreakLobbyButtonTextBlinkColor"; }
		if(PlayerCashTextColor == "") { wrongItems << "PlayerCashTextColor"; }
		if(PlayerBetTextColor == "") { wrongItems << "PlayerBetTextColor"; }
		if(PlayerNickTextColor == "") { wrongItems << "PlayerNickTextColor"; }
		if(BoardBigTextColor == "") { wrongItems << "BoardBigTextColor"; }
		if(BoardSmallTextColor == "") { wrongItems << "BoardSmallTextColor"; }
		if(SpeedTextColor == "") { wrongItems << "SpeedTextColor"; }
		if(VoteButtonBgColor == "") { wrongItems << "VoteButtonBgColor"; }
		if(VoteButtonTextColor == "") { wrongItems << "VoteButtonTextColor"; }
		if(BetInputTextColor == "") { wrongItems << "BetInputTextColor"; }
		if(BetInputBgColor == "") { wrongItems << "BetInputBgColor"; }
		if(BetInputDisabledTextColor == "") { wrongItems << "BetInputDisabledTextColor"; }
		if(BetInputDisabledBgColor == "") { wrongItems << "BetInputDisabledBgColor"; }
		if(FoldButtonTextColor == "") { wrongItems << "FoldButtonTextColor"; }
		if(FoldButtonCheckableTextColor == "") { wrongItems << "FoldButtonCheckableTextColor"; }
		if(CheckCallButtonTextColor == "") { wrongItems << "CheckCallButtonTextColor"; }
		if(CheckCallButtonCheckableTextColor == "") { wrongItems << "CheckCallButtonCheckableTextColor"; }
		if(BetRaiseButtonTextColor == "") { wrongItems << "BetRaiseButtonTextColor"; }
		if(BetRaiseButtonCheckableTextColor == "") { wrongItems << "BetRaiseButtonCheckableTextColor"; }
		if(AllInButtonTextColor == "") { wrongItems << "AllInButtonTextColor"; }
		if(AllInButtonCheckableTextColor == "") { wrongItems << "AllInButtonCheckableTextColor"; }
		if(BetSpeedSliderGrooveBgColor == "") { wrongItems << "BetSpeedSliderGrooveBgColor"; }
		if(BetSpeedSliderGrooveBorderColor == "") { wrongItems << "BetSpeedSliderGrooveBorderColor"; }
		if(BetSpeedSliderHandleBgColor == "") { wrongItems << "BetSpeedSliderHandleBgColor"; }
		if(BetSpeedSliderHandleBorderColor == "") { wrongItems << "BetSpeedSliderHandleBorderColor"; }
// 		SIZE
		if(ChatLogTextSize == "") { wrongItems << "ChatLogTextSize"; }
			
		//if one or more items are wrong or left show detailed error message
		if(!wrongItems.isEmpty() && myW != 0) showErrorMessage(StyleDescription, wrongItems, StyleMaintainerEMail);
	}	
	else {	qDebug() << "could not load game table style file: " << tinyFileName.c_str(); }
}

void GameTableStyleReader::showErrorMessage(QString style, QStringList failedItems, QString email)
{
	QString items = failedItems.join(", ");

	QMessageBox::warning(myW, tr("Game Table Style Error"),
                                tr("Selected game table style \"%1\" seems to be incomplete or defective. \nThe value(s) of \"%2\" is/are wrong or left. \n\nPlease contact the game table style builder %3.").arg(style).arg(items).arg(email),
                                QMessageBox::Ok);
}

void GameTableStyleReader::setTableBackground(gameTableImpl *gt)
{
	gt->setStyleSheet("QMainWindow { background-image: url("+Table+"); background-position: bottom center; background-origin: content;  background-repeat: no-repeat;}");
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
		ps->setStyleSheet("QGroupBox { border:none; background-image: url("+PlayerBottomSeatInactive+") }"); 
	}
	else {
		ps->setStyleSheet("QGroupBox { border:none; background-image: url("+PlayerTopSeatInactive+") }"); 
	}
}

void GameTableStyleReader::setPlayerSeatActiveStyle(QGroupBox *ps)
{
// 	check if seat is on top or bottom line
	if(ps->objectName() == "groupBox2" || ps->objectName() == "groupBox1" || ps->objectName() == "groupBox0" || ps->objectName() == "groupBox9" || ps->objectName() == "groupBox8") {
		ps->setStyleSheet("QGroupBox { border:none; background-image: url("+PlayerBottomSeatActive+") }"); 
	}
	else {
		ps->setStyleSheet("QGroupBox { border:none; background-image: url("+PlayerTopSeatActive+") }"); 
	}
}

void GameTableStyleReader::setBetValueInputStyle(QLineEdit *bv)
{
	bv->setStyleSheet("QLineEdit { "+ font2String +" font-size: "+betValueFontSize+"px; font-weight: bold; background-color: #"+BetInputBgColor+"; color: #"+BetInputTextColor+"; } QLineEdit:disabled { background-color: #"+BetInputDisabledBgColor+"; color: #"+BetInputDisabledTextColor+" }");
}

void GameTableStyleReader::setAwayRadioButtonsStyle(QRadioButton *rb)
{
	rb->setStyleSheet("QRadioButton { color: #"+TabWidgetTextColor+"; } QRadioButton::indicator { width: 13px; height: 13px;} QRadioButton::indicator::checked { image: url("+RadioButtonChecked+");}  QRadioButton::indicator::unchecked { image: url("+RadioButtonUnchecked+");} QRadioButton::indicator:unchecked:hover { image: url("+RadioButtonUncheckedHover+");} QRadioButton::indicator:unchecked:pressed { image: url("+RadioButtonPressed+");} QRadioButton::indicator::checked { image: url("+RadioButtonChecked+");} QRadioButton::indicator:checked:hover { image: url("+RadioButtonCheckedHover+");} QRadioButton::indicator:checked:pressed { image: url("+RadioButtonPressed+");}");

}

QString GameTableStyleReader::getActionPic(int action)
{
// 	1 = fold, 2 = check, 3 = call, 4 = bet, 5 = raise, 6 = allin, 7 = winner
	switch(action) {
		case 1: { return ActionFold; }
		break;
		case 2: { return ActionCheck; }
		break;
		case 3: { return ActionCall; }
		break;
		case 4: { return ActionBet; }
		break;
		case 5: { return ActionRaise; }
		break;
		case 6: { return ActionAllIn; }
		break;
		case 7: { return ActionWinner; }
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
			br->setStyleSheet("QPushButton { border:none; background-image: url("+BetRaiseButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+BetRaiseButtonTextColor+";} QPushButton:unchecked { background-image: url("+BetRaiseButtonDefault+"); } QPushButton:checked { background-image: url("+BetRaiseButtonChecked+");} QPushButton:hover { background-image: url("+BetRaiseButtonHover+"); } QPushButton:checked:hover { background-image: url("+BetRaiseButtonCheckedHover+");}"); 
			
			cc->setStyleSheet("QPushButton { border:none; background-image: url("+CheckCallButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+CheckCallButtonTextColor+";} QPushButton:unchecked { background-image: url("+CheckCallButtonDefault+"); } QPushButton:checked { background-image: url("+CheckCallButtonChecked+");} QPushButton:hover { background-image: url("+CheckCallButtonHover+"); } QPushButton:checked:hover { background-image: url("+CheckCallButtonCheckedHover+");}");

			f->setStyleSheet("QPushButton { border:none; background-image: url("+FoldButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+FoldButtonTextColor+";}  QPushButton:unchecked { background-image: url("+FoldButtonDefault+"); } QPushButton:checked { background-image: url("+FoldButtonChecked+");} QPushButton:hover { background-image: url("+FoldButtonHover+"); } QPushButton:checked:hover { background-image: url("+FoldButtonCheckedHover+");}");

			a->setStyleSheet("QPushButton { border:none; background-image: url("+AllInButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+AllInButtonTextColor+";}  QPushButton:unchecked { background-image: url("+AllInButtonDefault+"); } QPushButton:checked { background-image: url("+AllInButtonChecked+");} QPushButton:hover { background-image: url("+AllInButtonHover+"); } QPushButton:checked:hover { background-image: url("+AllInButtonCheckedHover+");}");
		}
		break;
		//no hover
		case 1: {
			br->setStyleSheet("QPushButton { border:none; background-image: url("+BetRaiseButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+BetRaiseButtonTextColor+";} QPushButton:unchecked { background-image: url("+BetRaiseButtonDefault+"); } QPushButton:checked { background-image: url("+BetRaiseButtonChecked+");} QPushButton:hover { background-image: url("+BetRaiseButtonDefault+"); } QPushButton:checked:hover { background-image: url("+BetRaiseButtonChecked+");}"); 
			
			cc->setStyleSheet("QPushButton { border:none; background-image: url("+CheckCallButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+CheckCallButtonTextColor+";} QPushButton:unchecked { background-image: url("+CheckCallButtonDefault+"); } QPushButton:checked { background-image: url("+CheckCallButtonChecked+");} QPushButton:hover { background-image: url("+CheckCallButtonDefault+"); } QPushButton:checked:hover { background-image: url("+CheckCallButtonChecked+");}");

			f->setStyleSheet("QPushButton { border:none; background-image: url("+FoldButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+FoldButtonTextColor+";}  QPushButton:unchecked { background-image: url("+FoldButtonDefault+"); } QPushButton:checked { background-image: url("+FoldButtonChecked+");} QPushButton:hover { background-image: url("+FoldButtonDefault+"); } QPushButton:checked:hover { background-image: url("+FoldButtonChecked+");}");

			a->setStyleSheet("QPushButton { border:none; background-image: url("+AllInButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+AllInButtonTextColor+";}  QPushButton:unchecked { background-image: url("+AllInButtonDefault+"); } QPushButton:checked { background-image: url("+AllInButtonChecked+");} QPushButton:hover { background-image: url("+AllInButtonDefault+"); } QPushButton:checked:hover { background-image: url("+AllInButtonChecked+");}");
		}
		break;
		//checkable
		case 2: {
			br->setStyleSheet("QPushButton { border:none; background-image: url("+BetRaiseButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+BetRaiseButtonCheckableTextColor+";} QPushButton:unchecked { background-image: url("+BetRaiseButtonDefault+"); } QPushButton:checked { background-image: url("+BetRaiseButtonChecked+");} QPushButton:hover { background-image: url("+BetRaiseButtonHover+"); } QPushButton:checked:hover { background-image: url("+BetRaiseButtonCheckedHover+");}"); 
			
			cc->setStyleSheet("QPushButton { border:none; background-image: url("+CheckCallButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+CheckCallButtonCheckableTextColor+";} QPushButton:unchecked { background-image: url("+CheckCallButtonDefault+"); } QPushButton:checked { background-image: url("+CheckCallButtonChecked+");} QPushButton:hover { background-image: url("+CheckCallButtonHover+"); } QPushButton:checked:hover { background-image: url("+CheckCallButtonCheckedHover+");}");

			f->setStyleSheet("QPushButton { border:none; background-image: url("+FoldButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+FoldButtonCheckableTextColor+";}  QPushButton:unchecked { background-image: url("+FoldButtonDefault+"); } QPushButton:checked { background-image: url("+FoldButtonChecked+");} QPushButton:hover { background-image: url("+FoldButtonHover+"); } QPushButton:checked:hover { background-image: url("+FoldButtonCheckedHover+");}");

			a->setStyleSheet("QPushButton { border:none; background-image: url("+AllInButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+AllInButtonCheckableTextColor+";}  QPushButton:unchecked { background-image: url("+AllInButtonDefault+"); } QPushButton:checked { background-image: url("+AllInButtonChecked+");} QPushButton:hover { background-image: url("+AllInButtonHover+"); } QPushButton:checked:hover { background-image: url("+AllInButtonCheckedHover+");}");
		}
		break;
	}
}

void GameTableStyleReader::setToolBoxBackground(QGroupBox* gb)
{
	gb->setStyleSheet("QGroupBox { border:none; background-image: url("+ToolBoxBackground+") }");
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
			if(gt->isFullScreen())
				gt->showNormal();
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
			if(gt->isFullScreen())
				gt->showNormal();
		}
	}

}

void GameTableStyleReader::setSliderStyle(QSlider *s)
{
	s->setStyleSheet("QSlider::groove:horizontal { border: 1px solid #"+BetSpeedSliderGrooveBorderColor+"; height: 3px; background: #"+BetSpeedSliderGrooveBgColor+"; margin: 4px 4px; border-radius: 2px; } QSlider::handle:horizontal { background: #"+BetSpeedSliderHandleBgColor+"; border: 1px solid #"+BetSpeedSliderHandleBorderColor+"; width: 9px; margin: -6px 0; border-radius: 4px;}");

}
