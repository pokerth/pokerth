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
#include "gametablestylereader.h"
#include <cstdlib>

using namespace std;

GameTableStyleReader::GameTableStyleReader(ConfigFile *c, QWidget *w)
	: myConfig(c), myW(w), fallBack(0), loadedSuccessfull(0), myState(GT_STYLE_UNDEFINED)
{

	//set fonts and font sizes
#ifdef _WIN32
	font1String = "font-family: \"Arial\";";
	font2String = "font-family: \"DejaVu Sans\";";
	cashFontSize = "11";
	setLabelFontSize = "11";
	playerNameLabelFontSize = "11";
	smallBoardFontSize = "13";
	bigBoardFontSize = "18";
	humanPlayerButtonFontSize = "14";
	betValueFontSize = "11";
	tabBarPaddingTop = "2";
	tabBarPaddingSide = "10";
#elif __APPLE__
	font1String = "font-family: \"Lucida Grande\";";
	font2String = "font-family: \"Lucida Grande\";";
	tabBarPaddingTop = "1";
	tabBarPaddingSide = "9";
	textBrowserFontsize= "10";
	cashFontSize = "10";
	setLabelFontSize = "10";
	playerNameLabelFontSize = "11";
	smallBoardFontSize = "13";
	bigBoardFontSize = "17";
	humanPlayerButtonFontSize = "12";
	betValueFontSize = "10";
#elif MAEMO
	font1String = "font-family: \"Nimbus Sans L\";";
	font2String = "font-family: \"DejaVu Sans\";";
	tabBarPaddingTop = "0";
	tabBarPaddingSide = "9";
	textBrowserFontsize= "10";
	cashFontSize = "12";
	setLabelFontSize = "13";
	playerNameLabelFontSize = "11";
	smallBoardFontSize = "13";
	bigBoardFontSize = "17";
	humanPlayerButtonFontSize = "12";
	betValueFontSize = "10";
#elif ANDROID
	font1String = "font-family: \"Nimbus Sans L\";";
	font2String = "font-family: \"DejaVu Sans\";";
	tabBarPaddingTop = "0";
	tabBarPaddingSide = "9";
	textBrowserFontsize= "10";
	cashFontSize = "12";
	setLabelFontSize = "13";
	playerNameLabelFontSize = "11";
	smallBoardFontSize = "13";
	bigBoardFontSize = "17";
	humanPlayerButtonFontSize = "12";
	betValueFontSize = "15";
#else //Linux
	font1String = "font-family: \"Nimbus Sans L\";";
	font2String = "font-family: \"DejaVu Sans\";";
	tabBarPaddingTop = "0";
	tabBarPaddingSide = "9";
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

void GameTableStyleReader::readStyleFile(QString file)
{
#ifdef ANDROID
	//on Android we currently just use the defaul style packed with the binary via qrc
	currentFileName = ":/android/android-data/gfx/gui/table/default_800x480/android_tablestyle_800x480.xml";
	currentDir = ":/android/android-data/gfx/gui/table/default_800x480/";
#else
	//if style file failed --> default style fallback
	if(QFile(file).exists()) {
		currentFileName = QFile(file).fileName();
	} else {
		currentFileName = QFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default/defaulttablestyle.xml").fileName();
		fallBack = 1;
	}

	QFileInfo info(currentFileName);
	currentDir = info.absolutePath()+"/";
#endif

	QFile myFile(currentFileName);
	myFile.open(QIODevice::ReadOnly);
	fileContent = myFile.readAll();

	//start reading the file and fill vars
	string tempString1("");
	TiXmlDocument doc;
	doc.Parse(fileContent.constData());

	if(doc.RootElement()) {
		TiXmlHandle docHandle( &doc );
		TiXmlElement *CardDeckElement = docHandle.FirstChild( "PokerTH" ).FirstChild( "CardDeck" ).ToElement();
		if(CardDeckElement) {
			MyMessageBox::warning(myW, tr("Game Table Style Error"),
								  tr("A card deck style was selected instead of a game table style.\nPlease select a game table style and try again!"),
								  QMessageBox::Ok);
		} else {
			//in case of rereading clear old variables:
			StyleDescription.clear();
			StyleMaintainerName.clear();
			StyleMaintainerEMail.clear();
			StyleCreateDate.clear();
			PokerTHStyleFileVersion.clear();
			IfFixedWindowSize.clear();
			FixedWindowWidth.clear();
			FixedWindowHeight.clear();
			MinimumWindowWidth.clear();
			MinimumWindowHeight.clear();
			MaximumWindowWidth.clear();
			MaximumWindowHeight.clear();
			Preview.clear();
			ActionAllInI18NPic.clear();
			ActionRaiseI18NPic.clear();
			ActionBetI18NPic.clear();
			ActionCallI18NPic.clear();
			ActionCheckI18NPic.clear();
			ActionFoldI18NPic.clear();
			ActionWinnerI18NPic.clear();
			BigBlindPuck.clear();
			SmallBlindPuck.clear();
			DealerPuck.clear();
			DefaultAvatar.clear();
			CardHolderFlop.clear();
			CardHolderTurn.clear();
			CardHolderRiver.clear();
			FoldButtonDefault.clear();
			FoldButtonHover.clear();
			FoldButtonChecked.clear();
			FoldButtonCheckedHover.clear();
			CheckCallButtonDefault.clear();
			CheckCallButtonHover.clear();
			CheckCallButtonChecked.clear();
			CheckCallButtonCheckedHover.clear();
			BetRaiseButtonDefault.clear();
			BetRaiseButtonHover.clear();
			BetRaiseButtonChecked.clear();
			BetRaiseButtonCheckedHover.clear();
			AllInButtonDefault.clear();
			AllInButtonHover.clear();
			AllInButtonChecked.clear();
			AllInButtonCheckedHover.clear();
			RadioButtonPressed.clear();
			RadioButtonChecked.clear();
			RadioButtonCheckedHover.clear();
			RadioButtonUnchecked.clear();
			RadioButtonUncheckedHover.clear();
			PlayerTopSeatInactive.clear();
			PlayerTopSeatActive.clear();
			PlayerBottomSeatInactive.clear();
			PlayerBottomSeatActive.clear();
			Table.clear();
			HandRanking.clear();
			ToolBoxBackground.clear();
			ShowMyCardsButtonDefault.clear();
			ShowMyCardsButtonHover.clear();
			ActionAllInI18NString.clear();
			ActionRaiseI18NString.clear();
			ActionBetI18NString.clear();
			ActionCallI18NString.clear();
			ActionCheckI18NString.clear();
			ActionFoldI18NString.clear();
			PotI18NString.clear();
			TotalI18NString.clear();
			BetsI18NString.clear();
			GameI18NString.clear();
			HandI18NString.clear();
			PreflopI18NString.clear();
			FlopI18NString.clear();
			TurnI18NString.clear();
			RiverI18NString.clear();
			FKeyIndicatorColor.clear();
			ChanceLabelPossibleColor.clear();
			ChanceLabelImpossibleColor.clear();
			ChatTextNickNotifyColor.clear();
			ChatLogTextColor.clear();
			ChatLogBgColor.clear();
			ChatLogScrollBarBorderColor.clear();
			ChatLogScrollBarBgColor.clear();
			ChatLogScrollBarHandleBorderColor.clear();
			ChatLogScrollBarHandleBgColor.clear();
			ChatLogScrollBarArrowBorderColor.clear();
			ChatLogScrollBarArrowBgColor.clear();
			LogWinnerMainPotColor.clear();
			LogWinnerSidePotColor.clear();
			LogPlayerSitsOutColor.clear();
			LogNewGameAdminColor.clear();
			TabWidgetBorderColor.clear();
			TabWidgetBgColor.clear();
			TabWidgetTextColor.clear();
			MenuBgColor.clear();
			MenuTextColor.clear();
			BreakLobbyButtonBgColor.clear();
			BreakLobbyButtonTextColor.clear();
			BreakLobbyButtonBgDisabledColor.clear();
			BreakLobbyButtonTextDisabledColor.clear();
			BreakLobbyButtonBgBlinkColor.clear();
			BreakLobbyButtonTextBlinkColor.clear();
			PlayerCashTextColor.clear();
			PlayerBetTextColor.clear();
			PlayerNickTextColor.clear();
			BoardBigTextColor.clear();
			BoardSmallTextColor.clear();
			SpeedTextColor.clear();
			VoteButtonBgColor.clear();
			VoteButtonTextColor.clear();
			BetInputTextColor.clear();
			BetInputBgColor.clear();
			BetInputDisabledTextColor.clear();
			BetInputDisabledBgColor.clear();
			FoldButtonTextColor.clear();
			FoldButtonCheckableTextColor.clear();
			CheckCallButtonTextColor.clear();
			CheckCallButtonCheckableTextColor.clear();
			BetRaiseButtonTextColor.clear();
			BetRaiseButtonCheckableTextColor.clear();
			AllInButtonTextColor.clear();
			AllInButtonCheckableTextColor.clear();
			BetSpeedSliderGrooveBgColor.clear();
			BetSpeedSliderGrooveBorderColor.clear();
			BetSpeedSliderHandleBgColor.clear();
			BetSpeedSliderHandleBorderColor.clear();
			ShowMyCardsButtonTextColor.clear();
			RatingStarsColor.clear();
			PlayerInfoHintTextColor.clear();
			ChatLogTextSize.clear();

			//now reading!
			TiXmlElement* itemsList = docHandle.FirstChild( "PokerTH" ).FirstChild( "TableStyle" ).FirstChild().ToElement();
			for( ; itemsList; itemsList=itemsList->NextSiblingElement()) {
				const char *tmpStr1 = itemsList->Attribute("value");
				if (tmpStr1) {
					tempString1 = tmpStr1;

					// 				INFOS
					if(itemsList->ValueStr() == "StyleDescription") {
						StyleDescription = QString::fromUtf8(tempString1.c_str());
					} else if(itemsList->ValueStr() == "StyleMaintainerName") {
						StyleMaintainerName = QString::fromUtf8(tempString1.c_str());
					} else if(itemsList->ValueStr() == "StyleMaintainerEMail") {
						StyleMaintainerEMail = QString::fromUtf8(tempString1.c_str());
					} else if(itemsList->ValueStr() == "StyleCreateDate") {
						StyleCreateDate = QString::fromUtf8(tempString1.c_str());
					} else if(itemsList->ValueStr() == "PokerTHStyleFileVersion") {
						PokerTHStyleFileVersion = QString::fromUtf8(tempString1.c_str());
					}
					// 				WINDOWS SETTINGS
					else if (itemsList->ValueStr() == "IfFixedWindowSize") {
						IfFixedWindowSize = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "FixedWindowWidth") {
						FixedWindowWidth = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "FixedWindowHeight") {
						FixedWindowHeight = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "MinimumWindowWidth") {
						MinimumWindowWidth = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "MinimumWindowHeight") {
						MinimumWindowHeight = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "MaximumWindowWidth") {
						MaximumWindowWidth = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "MaximumWindowHeight") {
						MaximumWindowHeight = QString::fromUtf8(tempString1.c_str());
					}
					// 				PICS
					else if (itemsList->ValueStr() == "Preview") {
						Preview = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ActionAllInI18NPic") {
						ActionAllInI18NPic = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ActionRaiseI18NPic") {
						ActionRaiseI18NPic = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ActionBetI18NPic") {
						ActionBetI18NPic = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ActionCallI18NPic") {
						ActionCallI18NPic = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ActionCheckI18NPic") {
						ActionCheckI18NPic = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ActionFoldI18NPic") {
						ActionFoldI18NPic = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ActionWinnerI18NPic") {
						ActionWinnerI18NPic = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BigBlindPuck") {
						BigBlindPuck = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "SmallBlindPuck") {
						SmallBlindPuck = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "DealerPuck") {
						DealerPuck = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "DefaultAvatar") {
						DefaultAvatar = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "CardHolderFlop") {
						CardHolderFlop = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "CardHolderTurn") {
						CardHolderTurn = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "CardHolderRiver") {
						CardHolderRiver = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "FoldButtonDefault") {
						FoldButtonDefault = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "FoldButtonHover") {
						FoldButtonHover = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "FoldButtonChecked") {
						FoldButtonChecked = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "FoldButtonCheckedHover") {
						FoldButtonCheckedHover = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "CheckCallButtonDefault") {
						CheckCallButtonDefault = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "CheckCallButtonHover") {
						CheckCallButtonHover = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "CheckCallButtonChecked") {
						CheckCallButtonChecked = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "CheckCallButtonCheckedHover") {
						CheckCallButtonCheckedHover = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetRaiseButtonDefault") {
						BetRaiseButtonDefault = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetRaiseButtonHover") {
						BetRaiseButtonHover = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetRaiseButtonChecked") {
						BetRaiseButtonChecked = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetRaiseButtonCheckedHover") {
						BetRaiseButtonCheckedHover = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "AllInButtonDefault") {
						AllInButtonDefault = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "AllInButtonHover") {
						AllInButtonHover = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "AllInButtonChecked") {
						AllInButtonChecked = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "AllInButtonCheckedHover") {
						AllInButtonCheckedHover = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "RadioButtonPressed") {
						RadioButtonPressed = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "RadioButtonChecked") {
						RadioButtonChecked = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "RadioButtonCheckedHover") {
						RadioButtonCheckedHover = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "RadioButtonUnchecked") {
						RadioButtonUnchecked = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "RadioButtonUncheckedHover") {
						RadioButtonUncheckedHover = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "PlayerTopSeatInactive") {
						PlayerTopSeatInactive = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "PlayerTopSeatActive") {
						PlayerTopSeatActive = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "PlayerBottomSeatInactive") {
						PlayerBottomSeatInactive = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "PlayerBottomSeatActive") {
						PlayerBottomSeatActive = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "Table") {
						Table = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "HandRanking") {
						HandRanking = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ToolBoxBackground") {
						ToolBoxBackground = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ShowMyCardsButtonDefault") {
						ShowMyCardsButtonDefault = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ShowMyCardsButtonHover") {
						ShowMyCardsButtonHover = currentDir+QString::fromUtf8(tempString1.c_str());
					}

					//I18N ACTION STRINGS
					else if (itemsList->ValueStr() == "ActionAllInI18NString") {
						ActionAllInI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ActionRaiseI18NString") {
						ActionRaiseI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ActionBetI18NString") {
						ActionBetI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ActionCallI18NString") {
						ActionCallI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ActionCheckI18NString") {
						ActionCheckI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ActionFoldI18NString") {
						ActionFoldI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "PotI18NString") {
						PotI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "TotalI18NString") {
						TotalI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetsI18NString") {
						BetsI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "GameI18NString") {
						GameI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "HandI18NString") {
						HandI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "PreflopI18NString") {
						PreflopI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "FlopI18NString") {
						FlopI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "TurnI18NString") {
						TurnI18NString = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "RiverI18NString") {
						RiverI18NString = QString::fromUtf8(tempString1.c_str());
					}

					// 				COLORS
					if (itemsList->ValueStr() == "FKeyIndicatorColor") {
						FKeyIndicatorColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ChanceLabelPossibleColor") {
						ChanceLabelPossibleColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ChanceLabelImpossibleColor") {
						ChanceLabelImpossibleColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ChatTextNickNotifyColor") {
						ChatTextNickNotifyColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ChatLogTextColor") {
						ChatLogTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ChatLogBgColor") {
						ChatLogBgColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ChatLogScrollBarBorderColor") {
						ChatLogScrollBarBorderColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ChatLogScrollBarBgColor") {
						ChatLogScrollBarBgColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ChatLogScrollBarHandleBorderColor") {
						ChatLogScrollBarHandleBorderColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ChatLogScrollBarHandleBgColor") {
						ChatLogScrollBarHandleBgColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ChatLogScrollBarArrowBorderColor") {
						ChatLogScrollBarArrowBorderColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ChatLogScrollBarArrowBgColor") {
						ChatLogScrollBarArrowBgColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "LogWinnerMainPotColor") {
						LogWinnerMainPotColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "LogWinnerSidePotColor") {
						LogWinnerSidePotColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "LogPlayerSitsOutColor") {
						LogPlayerSitsOutColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "LogNewGameAdminColor") {
						LogNewGameAdminColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "TabWidgetBorderColor") {
						TabWidgetBorderColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "TabWidgetBgColor") {
						TabWidgetBgColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "TabWidgetTextColor") {
						TabWidgetTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "MenuBgColor") {
						MenuBgColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "MenuTextColor") {
						MenuTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BreakLobbyButtonBgColor") {
						BreakLobbyButtonBgColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BreakLobbyButtonTextColor") {
						BreakLobbyButtonTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BreakLobbyButtonBgDisabledColor") {
						BreakLobbyButtonBgDisabledColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BreakLobbyButtonTextDisabledColor") {
						BreakLobbyButtonTextDisabledColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BreakLobbyButtonBgBlinkColor") {
						BreakLobbyButtonBgBlinkColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BreakLobbyButtonTextBlinkColor") {
						BreakLobbyButtonTextBlinkColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "PlayerCashTextColor") {
						PlayerCashTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "PlayerBetTextColor") {
						PlayerBetTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "PlayerNickTextColor") {
						PlayerNickTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BoardBigTextColor") {
						BoardBigTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BoardSmallTextColor") {
						BoardSmallTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "SpeedTextColor") {
						SpeedTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "VoteButtonBgColor") {
						VoteButtonBgColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "VoteButtonTextColor") {
						VoteButtonTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetInputTextColor") {
						BetInputTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetInputBgColor") {
						BetInputBgColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetInputDisabledTextColor") {
						BetInputDisabledTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetInputDisabledBgColor") {
						BetInputDisabledBgColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "FoldButtonTextColor") {
						FoldButtonTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "FoldButtonCheckableTextColor") {
						FoldButtonCheckableTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "CheckCallButtonTextColor") {
						CheckCallButtonTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "CheckCallButtonCheckableTextColor") {
						CheckCallButtonCheckableTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetRaiseButtonTextColor") {
						BetRaiseButtonTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetRaiseButtonCheckableTextColor") {
						BetRaiseButtonCheckableTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "AllInButtonTextColor") {
						AllInButtonTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "AllInButtonCheckableTextColor") {
						AllInButtonCheckableTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetSpeedSliderGrooveBgColor") {
						BetSpeedSliderGrooveBgColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetSpeedSliderGrooveBorderColor") {
						BetSpeedSliderGrooveBorderColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetSpeedSliderHandleBgColor") {
						BetSpeedSliderHandleBgColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BetSpeedSliderHandleBorderColor") {
						BetSpeedSliderHandleBorderColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "ShowMyCardsButtonTextColor") {
						ShowMyCardsButtonTextColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "RatingStarsColor") {
						RatingStarsColor = QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "PlayerInfoHintTextColor") {
						PlayerInfoHintTextColor = QString::fromUtf8(tempString1.c_str());
					}
					// 				SIZES
					else if (itemsList->ValueStr() == "ChatLogTextSize") {
						ChatLogTextSize = QString::fromUtf8(tempString1.c_str());
					}
				}
			}

			//check if style items are left and if pictures where not found
			leftItems.clear();
			itemPicsLeft.clear();

			// 		INFOS
			if(StyleDescription == "") {
				leftItems << "StyleDescription";
			}
			if(StyleMaintainerName == "") {
				leftItems << "StyleMaintainerName";
			}
			if(StyleMaintainerEMail == "") {
				leftItems << "StyleMaintainerEMail";
			}
			if(StyleCreateDate == "") {
				leftItems << "StyleCreateDate";
			}
			if(PokerTHStyleFileVersion == "") {
				leftItems << "PokerTHStyleFileVersion";
			}
			// 		WINDOWS SETTINGS
			if(IfFixedWindowSize == "") {
				IfFixedWindowSize = getFallBackFieldContent("IfFixedWindowSize", 0);
				leftItems << "IfFixedWindowSize";
			}
			if(FixedWindowWidth == "") {
				FixedWindowWidth = getFallBackFieldContent("FixedWindowWidth", 0);
				leftItems << "FixedWindowWidth";
			}
			if(FixedWindowHeight == "") {
				FixedWindowHeight = getFallBackFieldContent("FixedWindowHeight", 0);
				leftItems << "FixedWindowHeight";
			}
			if(MinimumWindowWidth == "") {
				MinimumWindowWidth = getFallBackFieldContent("MinimumWindowWidth", 0);
				leftItems << "MinimumWindowWidth";
			}
			if(MinimumWindowHeight == "") {
				MinimumWindowHeight = getFallBackFieldContent("MinimumWindowHeight", 0);
				leftItems << "MinimumWindowHeight";
			}
			if(MaximumWindowWidth == "") {
				MaximumWindowWidth = getFallBackFieldContent("MaximumWindowWidth", 0);
				leftItems << "MaximumWindowWidth";
			}
			if(MaximumWindowHeight == "") {
				MaximumWindowHeight = getFallBackFieldContent("MaximumWindowHeight", 0);
				leftItems << "MaximumWindowHeight";
			}
			// 		P I C S

			// I18N Pics
			if(ActionAllInI18NPic == "") {
				ActionAllInI18NPic = getFallBackFieldContent("ActionAllInI18NPic", 1);
				leftItems << "ActionAllInI18NPic";
			} else if(ActionAllInI18NPic != QString(currentDir+"NULL") && !QFile(ActionAllInI18NPic).exists()) {
				itemPicsLeft << "ActionAllInI18NPic = "+ActionAllInI18NPic;
				ActionAllInI18NPic = getFallBackFieldContent("ActionAllInI18NPic", 1);
			}

			if(ActionRaiseI18NPic == "") {
				ActionRaiseI18NPic = getFallBackFieldContent("ActionRaiseI18NPic", 1);
				leftItems << "ActionRaiseI18NPic";
			} else if(ActionRaiseI18NPic != QString(currentDir+"NULL") && !QFile(ActionRaiseI18NPic).exists()) {
				itemPicsLeft << "ActionRaiseI18NPic = "+ActionRaiseI18NPic;
				ActionRaiseI18NPic = getFallBackFieldContent("ActionRaiseI18NPic", 1);
			}

			if(ActionBetI18NPic == "") {
				ActionBetI18NPic = getFallBackFieldContent("ActionBetI18NPic", 1);
				leftItems << "ActionBetI18NPic";
			} else if(ActionBetI18NPic != QString(currentDir+"NULL") && !QFile(ActionBetI18NPic).exists()) {
				itemPicsLeft << "ActionBetI18NPic = "+ActionBetI18NPic;
				ActionBetI18NPic = getFallBackFieldContent("ActionBetI18NPic", 1);
			}

			if(ActionCallI18NPic == "") {
				ActionCallI18NPic = getFallBackFieldContent("ActionCallI18NPic", 1);
				leftItems << "ActionCallI18NPic";
			} else if(ActionCallI18NPic != QString(currentDir+"NULL") && !QFile(ActionCallI18NPic).exists()) {
				itemPicsLeft << "ActionCallI18NPic = "+ActionCallI18NPic;
				ActionCallI18NPic = getFallBackFieldContent("ActionCallI18NPic", 1);
			}

			if(ActionCheckI18NPic == "") {
				ActionCheckI18NPic = getFallBackFieldContent("ActionCheckI18NPic", 1);
				leftItems << "ActionCheckI18NPic";
			} else if(ActionCheckI18NPic != QString(currentDir+"NULL") && !QFile(ActionCheckI18NPic).exists()) {
				itemPicsLeft << "ActionCheckI18NPic = "+ActionCheckI18NPic;
				ActionCheckI18NPic = getFallBackFieldContent("ActionCheckI18NPic", 1);
			}

			if(ActionFoldI18NPic == "") {
				ActionFoldI18NPic = getFallBackFieldContent("ActionFoldI18NPic", 1);
				leftItems << "ActionFoldI18NPic";
			} else if(ActionFoldI18NPic != QString(currentDir+"NULL") && !QFile(ActionFoldI18NPic).exists()) {
				itemPicsLeft << "ActionFoldI18NPic = "+ActionFoldI18NPic;
				ActionFoldI18NPic = getFallBackFieldContent("ActionFoldI18NPic", 1);
			}

			if(ActionWinnerI18NPic == "") {
				ActionWinnerI18NPic = getFallBackFieldContent(" ActionWinnerI18NPic", 1);
				leftItems << "ActionWinnerI18NPic";
			} else if(ActionWinnerI18NPic != QString(currentDir+"NULL") && !QFile(ActionWinnerI18NPic).exists()) {
				itemPicsLeft << "ActionWinnerI18NPic = "+ActionWinnerI18NPic;
				ActionWinnerI18NPic = getFallBackFieldContent(" ActionWinnerI18NPic", 1);
			}

			// Other Pics
			if(BigBlindPuck == "") {
				BigBlindPuck = getFallBackFieldContent("BigBlindPuck", 1);
				leftItems << "BigBlindPuck";
			} else if(BigBlindPuck != QString(currentDir+"NULL") && !QFile(BigBlindPuck).exists()) {
				itemPicsLeft << "BigBlindPuck = "+BigBlindPuck;
				BigBlindPuck = getFallBackFieldContent("BigBlindPuck", 1);
			}

			if(SmallBlindPuck == "") {
				SmallBlindPuck = getFallBackFieldContent("SmallBlindPuck", 1);
				leftItems << "SmallBlindPuck";
			} else if(SmallBlindPuck != QString(currentDir+"NULL") && !QFile(SmallBlindPuck).exists()) {
				itemPicsLeft << "SmallBlindPuck = "+SmallBlindPuck;
				SmallBlindPuck = getFallBackFieldContent("SmallBlindPuck", 1);
			}

			if(DealerPuck == "") {
				DealerPuck = getFallBackFieldContent("DealerPuck", 1);
				leftItems << "DealerPuck";
			} else if(DealerPuck != QString(currentDir+"NULL") && !QFile(DealerPuck).exists()) {
				itemPicsLeft << "DealerPuck = "+DealerPuck;
				DealerPuck = getFallBackFieldContent("DealerPuck", 1);
			}

			if(DefaultAvatar == "") {
				DefaultAvatar = getFallBackFieldContent("DefaultAvatar", 1);
				leftItems << "DefaultAvatar";
			} else if(DefaultAvatar != QString(currentDir+"NULL") && !QFile(DefaultAvatar).exists()) {
				itemPicsLeft << "DefaultAvatar = "+DefaultAvatar;
				DefaultAvatar = getFallBackFieldContent("DefaultAvatar", 1);
			}

			if(CardHolderFlop == "") {
				CardHolderFlop = getFallBackFieldContent("CardHolderFlop", 1);
				leftItems << "CardHolderFlop";
			} else if(CardHolderFlop != QString(currentDir+"NULL") && !QFile(CardHolderFlop).exists()) {
				itemPicsLeft << "CardHolderFlop = "+CardHolderFlop;
				CardHolderFlop = getFallBackFieldContent("CardHolderFlop", 1);
			}

			if(CardHolderTurn == "") {
				CardHolderTurn = getFallBackFieldContent("CardHolderTurn", 1);
				leftItems << "CardHolderTurn";
			} else if(CardHolderTurn != QString(currentDir+"NULL") && !QFile(CardHolderTurn).exists()) {
				itemPicsLeft << "CardHolderTurn = "+CardHolderTurn;
				CardHolderTurn = getFallBackFieldContent("CardHolderTurn", 1);
			}

			if(CardHolderRiver == "") {
				CardHolderRiver = getFallBackFieldContent("CardHolderRiver", 1);
				leftItems << "CardHolderRiver";
			} else if(CardHolderRiver != QString(currentDir+"NULL") && !QFile(CardHolderRiver).exists()) {
				itemPicsLeft << "CardHolderRiver = "+CardHolderRiver;
				CardHolderRiver = getFallBackFieldContent("CardHolderRiver", 1);
			}

			if(FoldButtonDefault == "") {
				FoldButtonDefault = getFallBackFieldContent("FoldButtonDefault", 1);
				leftItems << "FoldButtonDefault";
			} else if(FoldButtonDefault != QString(currentDir+"NULL") && !QFile(FoldButtonDefault).exists()) {
				itemPicsLeft << "FoldButtonDefault = "+FoldButtonDefault;
				FoldButtonDefault = getFallBackFieldContent("FoldButtonDefault", 1);
			}

			if(FoldButtonHover == "") {
				FoldButtonHover = getFallBackFieldContent("FoldButtonHover", 1);
				leftItems << "FoldButtonHover";
			} else if(FoldButtonHover != QString(currentDir+"NULL") && !QFile(FoldButtonHover).exists()) {
				itemPicsLeft << "FoldButtonHover = "+FoldButtonHover;
				FoldButtonHover = getFallBackFieldContent("FoldButtonHover", 1);
			}

			if(FoldButtonChecked == "") {
				FoldButtonChecked = getFallBackFieldContent("FoldButtonChecked", 1);
				leftItems << "FoldButtonChecked";
			} else if(FoldButtonChecked != QString(currentDir+"NULL") && !QFile(FoldButtonChecked).exists()) {
				itemPicsLeft << "FoldButtonChecked = "+FoldButtonChecked;
				FoldButtonChecked = getFallBackFieldContent("FoldButtonChecked", 1);
			}

			if(FoldButtonCheckedHover == "") {
				FoldButtonCheckedHover = getFallBackFieldContent("FoldButtonCheckedHover", 1);
				leftItems << "FoldButtonCheckedHover";
			} else if(FoldButtonCheckedHover != QString(currentDir+"NULL") && !QFile(FoldButtonCheckedHover).exists()) {
				itemPicsLeft << "FoldButtonCheckedHover = "+FoldButtonCheckedHover;
				FoldButtonCheckedHover = getFallBackFieldContent("FoldButtonCheckedHover", 1);
			}

			if(CheckCallButtonDefault == "") {
				CheckCallButtonDefault = getFallBackFieldContent("CheckCallButtonDefault", 1);
				leftItems << "CheckCallButtonDefault";
			} else if(CheckCallButtonDefault != QString(currentDir+"NULL") && !QFile(CheckCallButtonDefault).exists()) {
				itemPicsLeft << "CheckCallButtonDefault = "+CheckCallButtonDefault;
				CheckCallButtonDefault = getFallBackFieldContent("CheckCallButtonDefault", 1);
			}

			if(CheckCallButtonHover == "") {
				CheckCallButtonHover = getFallBackFieldContent("CheckCallButtonHover", 1);
				leftItems << "CheckCallButtonHover";
			} else if(CheckCallButtonHover != QString(currentDir+"NULL") && !QFile(CheckCallButtonHover).exists()) {
				itemPicsLeft << "CheckCallButtonHover = "+CheckCallButtonHover;
				CheckCallButtonHover = getFallBackFieldContent("CheckCallButtonHover", 1);
			}

			if(CheckCallButtonChecked == "") {
				CheckCallButtonChecked = getFallBackFieldContent("CheckCallButtonChecked", 1);
				leftItems << "CheckCallButtonChecked";
			} else if(CheckCallButtonChecked != QString(currentDir+"NULL") && !QFile(CheckCallButtonChecked).exists()) {
				itemPicsLeft << "CheckCallButtonChecked = "+CheckCallButtonChecked;
				CheckCallButtonChecked = getFallBackFieldContent("CheckCallButtonChecked", 1);
			}

			if(CheckCallButtonCheckedHover == "") {
				CheckCallButtonCheckedHover = getFallBackFieldContent("CheckCallButtonCheckedHover", 1);
				leftItems << "CheckCallButtonCheckedHover";
			} else if(CheckCallButtonCheckedHover != QString(currentDir+"NULL") && !QFile(CheckCallButtonCheckedHover).exists()) {
				itemPicsLeft << "CheckCallButtonCheckedHover = "+CheckCallButtonCheckedHover;
				CheckCallButtonCheckedHover = getFallBackFieldContent("CheckCallButtonCheckedHover", 1);
			}

			if(BetRaiseButtonDefault == "") {
				BetRaiseButtonDefault = getFallBackFieldContent("BetRaiseButtonDefault", 1);
				leftItems << "BetRaiseButtonDefault";
			} else if(BetRaiseButtonDefault != QString(currentDir+"NULL") && !QFile(BetRaiseButtonDefault).exists()) {
				itemPicsLeft << "BetRaiseButtonDefault = "+BetRaiseButtonDefault;
				BetRaiseButtonDefault = getFallBackFieldContent("BetRaiseButtonDefault", 1);
			}

			if(BetRaiseButtonHover == "") {
				BetRaiseButtonHover = getFallBackFieldContent("BetRaiseButtonHover", 1);
				leftItems << "BetRaiseButtonHover";
			} else if(BetRaiseButtonHover != QString(currentDir+"NULL") && !QFile(BetRaiseButtonHover).exists()) {
				itemPicsLeft << "BetRaiseButtonHover = "+BetRaiseButtonHover;
				BetRaiseButtonHover = getFallBackFieldContent("BetRaiseButtonHover", 1);
			}

			if(BetRaiseButtonChecked == "") {
				BetRaiseButtonChecked = getFallBackFieldContent("BetRaiseButtonChecked", 1);
				leftItems << "BetRaiseButtonChecked";
			} else if(BetRaiseButtonChecked != QString(currentDir+"NULL") && !QFile(BetRaiseButtonChecked).exists()) {
				itemPicsLeft << "BetRaiseButtonChecked = "+BetRaiseButtonChecked;
				BetRaiseButtonChecked = getFallBackFieldContent("BetRaiseButtonChecked", 1);
			}

			if(BetRaiseButtonCheckedHover == "") {
				BetRaiseButtonCheckedHover = getFallBackFieldContent("BetRaiseButtonCheckedHover", 1);
				leftItems << "BetRaiseButtonCheckedHover";
			} else if(BetRaiseButtonCheckedHover != QString(currentDir+"NULL") && !QFile(BetRaiseButtonCheckedHover).exists()) {
				itemPicsLeft << "BetRaiseButtonCheckedHover = "+BetRaiseButtonCheckedHover;
				BetRaiseButtonCheckedHover = getFallBackFieldContent("BetRaiseButtonCheckedHover", 1);
			}

			if(AllInButtonDefault == "") {
				AllInButtonDefault = getFallBackFieldContent("AllInButtonDefault", 1);
				leftItems << "AllInButtonDefault";
			} else if(AllInButtonDefault != QString(currentDir+"NULL") && !QFile(AllInButtonDefault).exists()) {
				itemPicsLeft << "AllInButtonDefault = "+AllInButtonDefault;
				AllInButtonDefault = getFallBackFieldContent("AllInButtonDefault", 1);
			}

			if(AllInButtonHover == "") {
				AllInButtonHover = getFallBackFieldContent("AllInButtonHover", 1);
				leftItems << "AllInButtonHover";
			} else if(AllInButtonHover != QString(currentDir+"NULL") && !QFile(AllInButtonHover).exists()) {
				itemPicsLeft << "AllInButtonHover = "+AllInButtonHover;
				AllInButtonHover = getFallBackFieldContent("AllInButtonHover", 1);
			}

			if(AllInButtonChecked == "") {
				AllInButtonChecked = getFallBackFieldContent("AllInButtonChecked", 1);
				leftItems << "AllInButtonChecked";
			} else if(AllInButtonChecked != QString(currentDir+"NULL") && !QFile(AllInButtonChecked).exists()) {
				itemPicsLeft << "AllInButtonChecked = "+AllInButtonChecked;
				AllInButtonChecked = getFallBackFieldContent("AllInButtonChecked", 1);
			}

			if(AllInButtonCheckedHover == "") {
				AllInButtonCheckedHover = getFallBackFieldContent("AllInButtonCheckedHover", 1);
				leftItems << "AllInButtonCheckedHover";
			} else if(AllInButtonCheckedHover != QString(currentDir+"NULL") && !QFile(AllInButtonCheckedHover).exists()) {
				itemPicsLeft << "AllInButtonCheckedHover = "+AllInButtonCheckedHover;
				AllInButtonCheckedHover = getFallBackFieldContent("AllInButtonCheckedHover", 1);
			}

			if(RadioButtonPressed == "") {
				RadioButtonPressed = getFallBackFieldContent("RadioButtonPressed", 1);
				leftItems << "RadioButtonPressed";
			} else if(RadioButtonPressed != QString(currentDir+"NULL") && !QFile(RadioButtonPressed).exists()) {
				itemPicsLeft << "RadioButtonPressed = "+RadioButtonPressed;
				RadioButtonPressed = getFallBackFieldContent("RadioButtonPressed", 1);
			}

			if(RadioButtonChecked == "") {
				RadioButtonChecked = getFallBackFieldContent("RadioButtonChecked", 1);
				leftItems << "RadioButtonChecked";
			} else if(RadioButtonChecked != QString(currentDir+"NULL") && !QFile(RadioButtonChecked).exists()) {
				itemPicsLeft << "RadioButtonChecked = "+RadioButtonChecked;
				RadioButtonChecked = getFallBackFieldContent("RadioButtonChecked", 1);
			}

			if(RadioButtonCheckedHover == "") {
				RadioButtonCheckedHover = getFallBackFieldContent("RadioButtonCheckedHover", 1);
				leftItems << "RadioButtonCheckedHover";
			} else if(RadioButtonCheckedHover != QString(currentDir+"NULL") && !QFile(RadioButtonCheckedHover).exists()) {
				itemPicsLeft << "RadioButtonCheckedHover = "+RadioButtonCheckedHover;
				RadioButtonCheckedHover = getFallBackFieldContent("RadioButtonCheckedHover", 1);
			}

			if(RadioButtonUnchecked == "") {
				RadioButtonUnchecked = getFallBackFieldContent("RadioButtonUnchecked", 1);
				leftItems << "RadioButtonUnchecked";
			} else if(RadioButtonUnchecked != QString(currentDir+"NULL") && !QFile(RadioButtonUnchecked).exists()) {
				itemPicsLeft << "RadioButtonUnchecked = "+RadioButtonUnchecked;
				RadioButtonUnchecked = getFallBackFieldContent("RadioButtonUnchecked", 1);
			}

			if(RadioButtonUncheckedHover == "") {
				RadioButtonUncheckedHover = getFallBackFieldContent("RadioButtonUncheckedHover", 1);
				leftItems << "RadioButtonUncheckedHover";
			} else if(RadioButtonUncheckedHover != QString(currentDir+"NULL") && !QFile(RadioButtonUncheckedHover).exists()) {
				itemPicsLeft << "RadioButtonUncheckedHover = "+RadioButtonUncheckedHover;
				RadioButtonUncheckedHover = getFallBackFieldContent("RadioButtonUncheckedHover", 1);
			}

			if(PlayerTopSeatActive == "") {
				PlayerTopSeatActive = getFallBackFieldContent("PlayerTopSeatActive", 1);
				leftItems << "PlayerTopSeatActive";
			} else if(PlayerTopSeatActive != QString(currentDir+"NULL") && !QFile(PlayerTopSeatActive).exists()) {
				itemPicsLeft << "PlayerTopSeatActive = "+PlayerTopSeatActive;
				PlayerTopSeatActive = getFallBackFieldContent("PlayerTopSeatActive", 1);
			}

			if(PlayerTopSeatInactive == "") {
				PlayerTopSeatInactive = getFallBackFieldContent("PlayerTopSeatInactive", 1);
				leftItems << "PlayerTopSeatInactive";
			} else if(PlayerTopSeatInactive != QString(currentDir+"NULL") && !QFile(PlayerTopSeatInactive).exists()) {
				itemPicsLeft << "PlayerTopSeatInactive = "+PlayerTopSeatInactive;
				PlayerTopSeatInactive = getFallBackFieldContent("PlayerTopSeatInactive", 1);
			}

			if(PlayerBottomSeatActive == "") {
				PlayerBottomSeatActive = getFallBackFieldContent("PlayerBottomSeatActive", 1);
				leftItems << "PlayerBottomSeatActive";
			} else if(PlayerBottomSeatActive != QString(currentDir+"NULL") && !QFile(PlayerBottomSeatActive).exists()) {
				itemPicsLeft << "PlayerBottomSeatActive = "+PlayerBottomSeatActive;
				PlayerBottomSeatActive = getFallBackFieldContent("PlayerBottomSeatActive", 1);
			}

			if(PlayerBottomSeatInactive == "") {
				PlayerBottomSeatInactive = getFallBackFieldContent("PlayerBottomSeatInactive", 1);
				leftItems << "PlayerBottomSeatInactive";
			} else if(PlayerBottomSeatInactive != QString(currentDir+"NULL") && !QFile(PlayerBottomSeatInactive).exists()) {
				itemPicsLeft << "PlayerBottomSeatInactive = "+PlayerBottomSeatInactive;
				PlayerBottomSeatInactive = getFallBackFieldContent("PlayerBottomSeatInactive", 1);
			}

			if(Table == "") {
				Table = getFallBackFieldContent("Table", 1);
				leftItems << "Table";
			} else if(Table != QString(currentDir+"NULL") && !QFile(Table).exists()) {
				itemPicsLeft << "Table = "+Table;
				Table = getFallBackFieldContent("Table", 1);
			}

			if(HandRanking == "") {
				HandRanking = getFallBackFieldContent("HandRanking", 1);
				leftItems << "HandRanking";
			} else if(HandRanking != QString(currentDir+"NULL") && !QFile(HandRanking).exists()) {
				itemPicsLeft << "HandRanking = "+HandRanking;
				HandRanking = getFallBackFieldContent("HandRanking", 1);
			}

			if(ToolBoxBackground == "") {
				ToolBoxBackground = getFallBackFieldContent("ToolBoxBackground", 1);
				leftItems << "ToolBoxBackground";
			} else if(ToolBoxBackground != QString(currentDir+"NULL") && !QFile(ToolBoxBackground).exists()) {
				itemPicsLeft << "ToolBoxBackground = "+ToolBoxBackground;
				ToolBoxBackground = getFallBackFieldContent("ToolBoxBackground", 1);
			}

			if(ShowMyCardsButtonDefault == "") {
				ShowMyCardsButtonDefault = getFallBackFieldContent("ShowMyCardsButtonDefault", 1);
				leftItems << "ShowMyCardsButtonDefault";
			} else if(ShowMyCardsButtonDefault != QString(currentDir+"NULL") && !QFile(ShowMyCardsButtonDefault).exists()) {
				itemPicsLeft << "ShowMyCardsButtonDefault = "+ShowMyCardsButtonDefault;
				ShowMyCardsButtonDefault = getFallBackFieldContent("ShowMyCardsButtonDefault", 1);
			}

			if(ShowMyCardsButtonHover == "") {
				ShowMyCardsButtonHover = getFallBackFieldContent("ShowMyCardsButtonHover", 1);
				leftItems << "ShowMyCardsButtonHover";
			} else if(ShowMyCardsButtonHover != QString(currentDir+"NULL") && !QFile(ShowMyCardsButtonHover).exists()) {
				itemPicsLeft << "ShowMyCardsButtonHover = "+ShowMyCardsButtonHover;
				ShowMyCardsButtonHover = getFallBackFieldContent("ShowMyCardsButtonHover", 1);
			}


			//I18N ACTION STRINGS
			if(ActionAllInI18NString == "") {
				ActionAllInI18NString = "NULL";
				leftItems << "ActionAllInI18NString";
			}
			if(ActionRaiseI18NString == "") {
				ActionRaiseI18NString = "NULL";
				leftItems << "ActionRaiseI18NString";
			}
			if(ActionBetI18NString == "") {
				ActionBetI18NString = "NULL";
				leftItems << "ActionBetI18NString";
			}
			if(ActionCallI18NString == "") {
				ActionCallI18NString = "NULL";
				leftItems << "ActionCallI18NString";
			}
			if(ActionCheckI18NString == "") {
				ActionCheckI18NString = "NULL";
				leftItems << "ActionCheckI18NString";
			}
			if(ActionFoldI18NString == "") {
				ActionFoldI18NString = "NULL";
				leftItems << "ActionFoldI18NString";
			}
			if(PotI18NString == "") {
				PotI18NString = "NULL";
				leftItems << "PotI18NString";
			}
			if(TotalI18NString == "") {
				TotalI18NString = "NULL";
				leftItems << "TotalI18NString";
			}
			if(BetsI18NString == "") {
				BetsI18NString = "NULL";
				leftItems << "BetsI18NString";
			}
			if(GameI18NString == "") {
				GameI18NString = "NULL";
				leftItems << "GameI18NString";
			}
			if(HandI18NString == "") {
				HandI18NString = "NULL";
				leftItems << "HandI18NString";
			}
			if(PreflopI18NString == "") {
				PreflopI18NString = "NULL";
				leftItems << "PreflopI18NString";
			}
			if(FlopI18NString == "") {
				FlopI18NString = "NULL";
				leftItems << "FlopI18NString";
			}
			if(TurnI18NString == "") {
				TurnI18NString = "NULL";
				leftItems << "TurnI18NString";
			}
			if(RiverI18NString == "") {
				RiverI18NString = "NULL";
				leftItems << "RiverI18NString";
			}

			// 		COLORS
			if(FKeyIndicatorColor == "") {
				FKeyIndicatorColor = getFallBackFieldContent("FKeyIndicatorColor", 0);
				leftItems << "FKeyIndicatorColor";
			}
			if(ChanceLabelPossibleColor == "") {
				ChanceLabelPossibleColor = getFallBackFieldContent("ChanceLabelPossibleColor", 0);
				leftItems << "ChanceLabelPossibleColor";
			}
			if(ChanceLabelImpossibleColor == "") {
				ChanceLabelImpossibleColor = getFallBackFieldContent("ChanceLabelImpossibleColor", 0);
				leftItems << "ChanceLabelImpossibleColor";
			}
			if(ChatLogTextColor == "") {
				ChatLogTextColor = getFallBackFieldContent("ChatLogTextColor", 0);
				leftItems << "ChatLogTextColor";
			}
			if(ChatTextNickNotifyColor == "") {
				ChatTextNickNotifyColor = getFallBackFieldContent("ChatTextNickNotifyColor", 0);
				leftItems << "ChatTextNickNotifyColor";
			}
			if(ChatLogBgColor == "") {
				ChatLogBgColor = getFallBackFieldContent("ChatLogBgColor", 0);
				leftItems << "ChatLogBgColor";
			}
			if(ChatLogScrollBarBorderColor == "") {
				ChatLogScrollBarBorderColor = getFallBackFieldContent("ChatLogScrollBarBorderColor", 0);
				leftItems << "ChatLogScrollBarBorderColor";
			}
			if(ChatLogScrollBarBgColor == "") {
				ChatLogScrollBarBgColor = getFallBackFieldContent("ChatLogScrollBarBgColor", 0);
				leftItems << "ChatLogScrollBarBgColor";
			}
			if(ChatLogScrollBarHandleBorderColor == "") {
				ChatLogScrollBarHandleBorderColor = getFallBackFieldContent("ChatLogScrollBarHandleBorderColor", 0);
				leftItems << "ChatLogScrollBarHandleBorderColor";
			}
			if(ChatLogScrollBarHandleBgColor == "") {
				ChatLogScrollBarHandleBgColor = getFallBackFieldContent("ChatLogScrollBarHandleBgColor", 0);
				leftItems << "ChatLogScrollBarHandleBgColor";
			}
			if(ChatLogScrollBarArrowBorderColor == "") {
				ChatLogScrollBarArrowBorderColor = getFallBackFieldContent("ChatLogScrollBarArrowBorderColor", 0);
				leftItems << "ChatLogScrollBarArrowBorderColor";
			}
			if(ChatLogScrollBarArrowBgColor == "") {
				ChatLogScrollBarArrowBgColor = getFallBackFieldContent("ChatLogScrollBarArrowBgColor", 0);
				leftItems << "ChatLogScrollBarArrowBgColor";
			}
			if(LogWinnerMainPotColor == "") {
				LogWinnerMainPotColor = getFallBackFieldContent("LogWinnerMainPotColor", 0);
				leftItems << "LogWinnerMainPotColor";
			}
			if(LogWinnerSidePotColor == "") {
				LogWinnerSidePotColor = getFallBackFieldContent("LogWinnerSidePotColor", 0);
				leftItems << "LogWinnerSidePotColor";
			}
			if(LogPlayerSitsOutColor == "") {
				LogPlayerSitsOutColor = getFallBackFieldContent("LogPlayerSitsOutColor", 0);
				leftItems << "LogPlayerSitsOutColor";
			}
			if(LogNewGameAdminColor == "") {
				LogNewGameAdminColor = getFallBackFieldContent("LogNewGameAdminColor", 0);
				leftItems << "LogNewGameAdminColor";
			}
			if(TabWidgetBorderColor == "") {
				TabWidgetBorderColor = getFallBackFieldContent("TabWidgetBorderColor", 0);
				leftItems << "TabWidgetBorderColor";
			}
			if(TabWidgetBgColor == "") {
				TabWidgetBgColor = getFallBackFieldContent("TabWidgetBgColor", 0);
				leftItems << "TabWidgetBgColor";
			}
			if(TabWidgetTextColor == "") {
				TabWidgetTextColor = getFallBackFieldContent("TabWidgetTextColor", 0);
				leftItems << "TabWidgetTextColor";
			}
			if(MenuBgColor == "") {
				MenuBgColor = getFallBackFieldContent("MenuBgColor", 0);
				leftItems << "MenuBgColor";
			}
			if(MenuTextColor == "") {
				MenuTextColor = getFallBackFieldContent("MenuTextColor", 0);
				leftItems << "MenuTextColor";
			}
			if(BreakLobbyButtonBgColor == "") {
				BreakLobbyButtonBgColor = getFallBackFieldContent("BreakLobbyButtonBgColor", 0);
				leftItems << "BreakLobbyButtonBgColor";
			}
			if(BreakLobbyButtonTextColor == "") {
				BreakLobbyButtonTextColor = getFallBackFieldContent("BreakLobbyButtonTextColor", 0);
				leftItems << "BreakLobbyButtonTextColor";
			}
			if(BreakLobbyButtonBgDisabledColor == "") {
				BreakLobbyButtonBgDisabledColor = getFallBackFieldContent("BreakLobbyButtonBgDisabledColor", 0);
				leftItems << "BreakLobbyButtonBgDisabledColor";
			}
			if(BreakLobbyButtonTextDisabledColor == "") {
				BreakLobbyButtonTextDisabledColor = getFallBackFieldContent("BreakLobbyButtonTextDisabledColor", 0);
				leftItems << "BreakLobbyButtonTextDisabledColor";
			}
			if(BreakLobbyButtonBgBlinkColor == "") {
				BreakLobbyButtonBgBlinkColor = getFallBackFieldContent("BreakLobbyButtonBgBlinkColor", 0);
				leftItems << "BreakLobbyButtonBgBlinkColor";
			}
			if(BreakLobbyButtonTextBlinkColor == "") {
				BreakLobbyButtonTextBlinkColor = getFallBackFieldContent("BreakLobbyButtonTextBlinkColor", 0);
				leftItems << "BreakLobbyButtonTextBlinkColor";
			}
			if(PlayerCashTextColor == "") {
				PlayerCashTextColor = getFallBackFieldContent("PlayerCashTextColor", 0);
				leftItems << "PlayerCashTextColor";
			}
			if(PlayerBetTextColor == "") {
				PlayerBetTextColor = getFallBackFieldContent("PlayerBetTextColor", 0);
				leftItems << "PlayerBetTextColor";
			}
			if(PlayerNickTextColor == "") {
				PlayerNickTextColor = getFallBackFieldContent("PlayerNickTextColor", 0);
				leftItems << "PlayerNickTextColor";
			}
			if(BoardBigTextColor == "") {
				BoardBigTextColor = getFallBackFieldContent("BoardBigTextColor", 0);
				leftItems << "BoardBigTextColor";
			}
			if(BoardSmallTextColor == "") {
				BoardSmallTextColor = getFallBackFieldContent("BoardSmallTextColor", 0);
				leftItems << "BoardSmallTextColor";
			}
			if(SpeedTextColor == "") {
				SpeedTextColor = getFallBackFieldContent("SpeedTextColor", 0);
				leftItems << "SpeedTextColor";
			}
			if(VoteButtonBgColor == "") {
				VoteButtonBgColor = getFallBackFieldContent("VoteButtonBgColor", 0);
				leftItems << "VoteButtonBgColor";
			}
			if(VoteButtonTextColor == "") {
				VoteButtonTextColor = getFallBackFieldContent("VoteButtonTextColor", 0);
				leftItems << "VoteButtonTextColor";
			}
			if(BetInputTextColor == "") {
				BetInputTextColor = getFallBackFieldContent("BetInputTextColor", 0);
				leftItems << "BetInputTextColor";
			}
			if(BetInputBgColor == "") {
				BetInputBgColor = getFallBackFieldContent("BetInputBgColor", 0);
				leftItems << "BetInputBgColor";
			}
			if(BetInputDisabledTextColor == "") {
				BetInputDisabledTextColor = getFallBackFieldContent("BetInputDisabledTextColor", 0);
				leftItems << "BetInputDisabledTextColor";
			}
			if(BetInputDisabledBgColor == "") {
				BetInputDisabledBgColor = getFallBackFieldContent("BetInputDisabledBgColor", 0);
				leftItems << "BetInputDisabledBgColor";
			}
			if(FoldButtonTextColor == "") {
				FoldButtonTextColor = getFallBackFieldContent("FoldButtonTextColor", 0);
				leftItems << "FoldButtonTextColor";
			}
			if(FoldButtonCheckableTextColor == "") {
				FoldButtonCheckableTextColor = getFallBackFieldContent("FoldButtonCheckableTextColor", 0);
				leftItems << "FoldButtonCheckableTextColor";
			}
			if(CheckCallButtonTextColor == "") {
				CheckCallButtonTextColor = getFallBackFieldContent("CheckCallButtonTextColor", 0);
				leftItems << "CheckCallButtonTextColor";
			}
			if(CheckCallButtonCheckableTextColor == "") {
				CheckCallButtonCheckableTextColor = getFallBackFieldContent("CheckCallButtonCheckableTextColor", 0);
				leftItems << "CheckCallButtonCheckableTextColor";
			}
			if(BetRaiseButtonTextColor == "") {
				BetRaiseButtonTextColor = getFallBackFieldContent("BetRaiseButtonTextColor", 0);
				leftItems << "BetRaiseButtonTextColor";
			}
			if(BetRaiseButtonCheckableTextColor == "") {
				BetRaiseButtonCheckableTextColor = getFallBackFieldContent("BetRaiseButtonCheckableTextColor", 0);
				leftItems << "BetRaiseButtonCheckableTextColor";
			}
			if(AllInButtonTextColor == "") {
				AllInButtonTextColor = getFallBackFieldContent("AllInButtonTextColor", 0);
				leftItems << "AllInButtonTextColor";
			}
			if(AllInButtonCheckableTextColor == "") {
				AllInButtonCheckableTextColor = getFallBackFieldContent("AllInButtonCheckableTextColor", 0);
				leftItems << "AllInButtonCheckableTextColor";
			}
			if(BetSpeedSliderGrooveBgColor == "") {
				BetSpeedSliderGrooveBgColor = getFallBackFieldContent("BetSpeedSliderGrooveBgColor", 0);
				leftItems << "BetSpeedSliderGrooveBgColor";
			}
			if(BetSpeedSliderGrooveBorderColor == "") {
				BetSpeedSliderGrooveBorderColor = getFallBackFieldContent("BetSpeedSliderGrooveBorderColor", 0);
				leftItems << "BetSpeedSliderGrooveBorderColor";
			}
			if(BetSpeedSliderHandleBgColor == "") {
				BetSpeedSliderHandleBgColor = getFallBackFieldContent("BetSpeedSliderHandleBgColor", 0);
				leftItems << "BetSpeedSliderHandleBgColor";
			}
			if(BetSpeedSliderHandleBorderColor == "") {
				BetSpeedSliderHandleBorderColor = getFallBackFieldContent("BetSpeedSliderHandleBorderColor", 0);
				leftItems << "BetSpeedSliderHandleBorderColor";
			}
			if(ShowMyCardsButtonTextColor == "") {
				ShowMyCardsButtonTextColor = getFallBackFieldContent("ShowMyCardsButtonTextColor", 0);
				leftItems << "ShowMyCardsButtonTextColor";
			}
			if(RatingStarsColor == "") {
				RatingStarsColor = getFallBackFieldContent("RatingStarsColor", 0);
				leftItems << "RatingStarsColor";
			}
			if(PlayerInfoHintTextColor == "") {
				PlayerInfoHintTextColor = getFallBackFieldContent("PlayerInfoHintTextColor", 0);
				leftItems << "PlayerInfoHintTextColor";
			}

			// 		SIZE
			if(ChatLogTextSize == "") {
				ChatLogTextSize = getFallBackFieldContent("ChatLogTextSize", 0);
				leftItems << "ChatLogTextSize";
			}

			//set loadedSuccessfull true if everything works
			//            qDebug() << "leftitem is empty: " << leftItems.isEmpty() << "pics left is empty: " << itemPicsLeft.isEmpty() << "stylefileversion is: " << PokerTHStyleFileVersion;
			if(leftItems.isEmpty() && itemPicsLeft.isEmpty() && PokerTHStyleFileVersion != "" && PokerTHStyleFileVersion.toInt() == POKERTH_GT_STYLE_FILE_VERSION) {
				myState = GT_STYLE_OK;
				//                qDebug() << "myState of: " << StyleDescription << "is now: " << myState;
			} else {
				//check for style file version
				if(PokerTHStyleFileVersion != "" && PokerTHStyleFileVersion.toInt() != POKERTH_GT_STYLE_FILE_VERSION) {
					myState = GT_STYLE_OUTDATED;
				} else {
					//if one or more items are left
					if(!leftItems.isEmpty() && myW != 0) myState = GT_STYLE_FIELDS_EMPTY;

					//if one or more pictures where not found
					if(!itemPicsLeft.isEmpty() && myW != 0) myState = GT_STYLE_PICTURES_MISSING;
				}

				//                qDebug() << "myState of: " << StyleDescription << "is now: " << myState;
			}
			loadedSuccessfull = 1;
		}
	} else {
		loadedSuccessfull = 0;
		MyMessageBox::warning(myW, tr("Game Table Style Error"),
							  tr("Cannot load game table style file: %1 \n\nPlease check the style file or choose another style!").arg(currentFileName),
							  QMessageBox::Ok);
	}
}

void GameTableStyleReader::showErrorMessage()
{
	switch (myState) {
	case GT_STYLE_PICTURES_MISSING:
		showItemPicsLeftErrorMessage();
		break;
	case GT_STYLE_FIELDS_EMPTY:
		showLeftItemsErrorMessage();
		break;
	case GT_STYLE_OUTDATED:
		showOutdatedErrorMessage();
		break;
	default:
		;
	}
}

void GameTableStyleReader::showLeftItemsErrorMessage()
{
	QString items = leftItems.join("\n");
	QString EMail;
	if(StyleMaintainerEMail != "NULL") EMail = StyleMaintainerEMail;

	myMessageDialogImpl dialog(myConfig, myW);

	if(dialog.checkIfMesssageWillBeDisplayed(GT_VALUES_MISSING)) {
		dialog.exec(GT_VALUES_MISSING, tr("Selected game table style \"%1\" seems to be incomplete or defective. \n\nThe value(s) of: \n%2 \nis/are missing. \n\nAnyway you can play with this style, because the missing content will be filled up by PokerTH default style. \n\nPlease contact the game table style builder via \"%3\".").arg(StyleDescription).arg(items).arg(EMail), tr("Game Table Style Error - Fields content missing"), QPixmap(":/gfx/emblem-important-64.png"), QDialogButtonBox::Ok, true);
	}

}

void GameTableStyleReader::showItemPicsLeftErrorMessage()
{
	QString pics = itemPicsLeft.join("\n");
	QString EMail;
	if(StyleMaintainerEMail != "NULL") EMail = StyleMaintainerEMail;

	myMessageDialogImpl dialog(myConfig, myW);

	if(dialog.checkIfMesssageWillBeDisplayed(GT_PICS_MISSING)) {
		dialog.exec(GT_PICS_MISSING, tr("One or more pictures from current game table style \"%1\" were not found: \n\n%2 \n\nAnyway you can play with this style, because the missing content will be filled up by PokerTH default style. \n\nPlease contact the game table style builder via \"%3\".").arg(StyleDescription).arg(pics).arg(EMail), tr("Game Table Style Error - Pictures missing"), QPixmap(":/gfx/emblem-important-64.png"), QDialogButtonBox::Ok, true);
	}
}

void GameTableStyleReader::showOutdatedErrorMessage()
{
	QString EMail;
	if(StyleMaintainerEMail != "NULL") EMail = StyleMaintainerEMail;

	myMessageDialogImpl dialog(myConfig, myW);

	if(dialog.checkIfMesssageWillBeDisplayed(GT_OUTDATED)) {
		dialog.exec(GT_OUTDATED, tr("Selected game table style \"%1\" seems to be outdated. \nThe current PokerTH game table style version is \"%2\", but this style has version \"%3\" set. \n\nAnyway you can play with this style, because the missing content will be filled up by PokerTH default style. \n\nPlease contact the game table style builder  via \"%4\".").arg(StyleDescription).arg(POKERTH_GT_STYLE_FILE_VERSION).arg(PokerTHStyleFileVersion).arg(EMail), tr("Game Table Style Error - Outdated"), QPixmap(":/gfx/emblem-important-64.png"), QDialogButtonBox::Ok, true);
	}
}

void GameTableStyleReader::setTableBackground(gameTableImpl *gt)
{
	gt->setStyleSheet("QMainWindow { background-image: url(\""+Table+"\"); background-position: bottom center; background-origin: content;  background-repeat: no-repeat;}");
}

void GameTableStyleReader::setChatLogStyle(QTextBrowser *tb)
{
#ifdef GUI_800x480
	//make the scrollbar touchable for mobile guis
	tb->setStyleSheet("QTextBrowser { "+ font1String +" font-size: "+ChatLogTextSize+"px; color: #"+ChatLogTextColor+"; background-color: #"+ChatLogBgColor+"; border:none; } QScrollBar:vertical { border: 1px solid #"+ChatLogScrollBarBorderColor+"; background: #"+ChatLogScrollBarBgColor+"; width: 60px; margin: 0px -1px 0px 0px; } QScrollBar::handle:vertical { border-radius: 4px; border: 3px solid #"+ChatLogScrollBarHandleBorderColor+"; background: #"+ChatLogScrollBarHandleBgColor+"; min-height: 60px; } QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical, QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical, QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
#else
	tb->setStyleSheet("QTextBrowser { "+ font1String +" font-size: "+ChatLogTextSize+"px; color: #"+ChatLogTextColor+"; background-color: #"+ChatLogBgColor+"; border:none; } QScrollBar:vertical { border: 1px solid #"+ChatLogScrollBarBorderColor+"; background: #"+ChatLogScrollBarBgColor+"; width: 15px; margin: 17px -1px 17px 0px; } QScrollBar::handle:vertical { border-radius: 1px; border: 1px solid #"+ChatLogScrollBarHandleBorderColor+"; background: #"+ChatLogScrollBarHandleBgColor+"; min-height: 20px; } QScrollBar::add-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 2px; border-bottom-left-radius: 2px; border-top-right-radius: 1px; border-top-left-radius: 1px; border: 1px solid #"+ChatLogScrollBarHandleBorderColor+"; background: #"+ChatLogScrollBarHandleBgColor+"; height: 15px; subcontrol-position: bottom; subcontrol-origin: margin; } QScrollBar::sub-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 1px; border-bottom-left-radius: 1px; border-top-right-radius: 2px; border-top-left-radius: 2px; border: 1px solid #"+ChatLogScrollBarHandleBorderColor+"; background: #"+ChatLogScrollBarHandleBgColor+"; height: 15px; subcontrol-position: top; subcontrol-origin: margin; } QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical { border: 1px solid #"+ChatLogScrollBarArrowBorderColor+"; height: 3px; width: 3px; background: #"+ChatLogScrollBarArrowBgColor+"; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
#endif
}

void GameTableStyleReader::setChatLogStyle(QPlainTextEdit* pte)
{
#ifdef GUI_800x480
	//make the scrollbar touchable for mobile guis
	pte->setStyleSheet("QPlainTextEdit { "+ font1String +" font-size: "+ChatLogTextSize+"px; color: #"+ChatLogTextColor+"; background-color: #"+ChatLogBgColor+"; border:none; } QScrollBar:vertical { border: 1px solid #"+ChatLogScrollBarBorderColor+"; background: #"+ChatLogScrollBarBgColor+"; width: 60px; margin: 0px -1px 0px 0px; } QScrollBar::handle:vertical { border-radius: 4px; border: 3px solid #"+ChatLogScrollBarHandleBorderColor+"; background: #"+ChatLogScrollBarHandleBgColor+"; min-height: 60px; } QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical, QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical, QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
#else
	pte->setStyleSheet("QPlainTextEdit { "+ font1String +" font-size: "+ChatLogTextSize+"px; color: #"+ChatLogTextColor+"; background-color: #"+ChatLogBgColor+"; border:none; } QScrollBar:vertical { border: 1px solid #"+ChatLogScrollBarBorderColor+"; background: #"+ChatLogScrollBarBgColor+"; width: 15px; margin: 17px -1px 17px 0px; } QScrollBar::handle:vertical { border-radius: 1px; border: 1px solid #"+ChatLogScrollBarHandleBorderColor+"; background: #"+ChatLogScrollBarHandleBgColor+"; min-height: 20px; } QScrollBar::add-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 2px; border-bottom-left-radius: 2px; border-top-right-radius: 1px; border-top-left-radius: 1px; border: 1px solid #"+ChatLogScrollBarHandleBorderColor+"; background: #"+ChatLogScrollBarHandleBgColor+"; height: 15px; subcontrol-position: bottom; subcontrol-origin: margin; } QScrollBar::sub-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 1px; border-bottom-left-radius: 1px; border-top-right-radius: 2px; border-top-left-radius: 2px; border: 1px solid #"+ChatLogScrollBarHandleBorderColor+"; background: #"+ChatLogScrollBarHandleBgColor+"; height: 15px; subcontrol-position: top; subcontrol-origin: margin; } QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical { border: 1px solid #"+ChatLogScrollBarArrowBorderColor+"; height: 3px; width: 3px; background: #"+ChatLogScrollBarArrowBgColor+"; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
#endif
}


void GameTableStyleReader::setChatInputStyle(QLineEdit *ci)
{
	QString myFontSize;
#ifdef GUI_800x480
	myFontSize="28";
#else
	myFontSize=ChatLogTextSize;
#endif
	ci->setStyleSheet("QLineEdit { "+ font1String +" font-size: "+myFontSize+"px; color: #"+ChatLogTextColor+"; background-color: #"+ChatLogBgColor+"; border-top: 2px solid #"+TabWidgetBorderColor+"; }");
}

void GameTableStyleReader::setCashLabelStyle(QLabel *cl)
{
	cl->setStyleSheet("QLabel { "+ font2String +" font-size: "+cashFontSize+"px; font-weight: bold; color: #"+PlayerCashTextColor+"; }");
}

void GameTableStyleReader::setSpectatorNumberLabelStyle(QLabel *snl)
{
	snl->setStyleSheet("QLabel { "+ font2String +" font-size: 12px; font-weight: bold; color: #"+MenuTextColor+"; }");
	snl->setAlignment(Qt::AlignHCenter);
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
	case 0:
		l->setPixmap(CardHolderFlop);
		break;
	case 1:
		l->setPixmap(CardHolderTurn);
		break;
	case 2:
		l->setPixmap(CardHolderRiver);
		break;
	}
}

void GameTableStyleReader::setMenuBarStyle(QMenuBar *mb)
{
	mb->setAttribute(Qt::WA_TranslucentBackground);
	mb->setStyleSheet("QMenuBar { background-color: #"+MenuBgColor+"; font-size:12px; border-width: 0px;} QMenuBar::item { background: transparent; color: #"+MenuTextColor+"; } QMenuBar::item:selected { background: #"+MenuTextColor+"; color: #"+MenuBgColor+"; } QMenuBar::item:pressed { background: #"+MenuTextColor+"; color: #"+MenuBgColor+"; }");
}

void GameTableStyleReader::setBreakButtonStyle(QPushButton *bb, int state)
{
	switch(state) {
	// 		default
	case 0:
#ifdef GUI_800x480
		bb->setStyleSheet("QPushButton:enabled { padding: 10px; background-color: #"+BreakLobbyButtonBgColor+"; color: #"+BreakLobbyButtonTextColor+"; font-size: 26px} QPushButton:disabled { padding: 10px; background-color: #"+BreakLobbyButtonBgDisabledColor+"; color: #"+BreakLobbyButtonTextDisabledColor+"; font-weight: 900; font-size: 26px}");
#else
		bb->setStyleSheet("QPushButton:enabled { background-color: #"+BreakLobbyButtonBgColor+"; color: #"+BreakLobbyButtonTextColor+";} QPushButton:disabled { background-color: #"+BreakLobbyButtonBgDisabledColor+"; color: #"+BreakLobbyButtonTextDisabledColor+"; font-weight: 900;}");
#endif
		break;
	// 		blink
	case 1:
#ifdef GUI_800x480
		bb->setStyleSheet("QPushButton { padding: 10px; background-color: #"+BreakLobbyButtonBgBlinkColor+"; color: "+BreakLobbyButtonTextBlinkColor+"; font-size: 26px}");
#else
		bb->setStyleSheet("QPushButton { background-color: #"+BreakLobbyButtonBgBlinkColor+"; color: "+BreakLobbyButtonTextBlinkColor+";}");
#endif
		break;
	}
}

void GameTableStyleReader::setSpeedStringStyle(QLabel *l)
{
#ifdef GUI_800x480
	l->setStyleSheet("QLabel { color: #"+SpeedTextColor+"; font-size: 24px}");
#else
	l->setStyleSheet("QLabel { color: #"+SpeedTextColor+";}");
#endif
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
	} else {
		ps->setStyleSheet("QGroupBox { border:none; background-image: url(\""+PlayerTopSeatInactive+"\") }");
	}
}

void GameTableStyleReader::setPlayerSeatActiveStyle(QGroupBox *ps)
{
	// 	check if seat is on top or bottom line
	if(ps->objectName() == "groupBox2" || ps->objectName() == "groupBox1" || ps->objectName() == "groupBox0" || ps->objectName() == "groupBox9" || ps->objectName() == "groupBox8") {
		ps->setStyleSheet("QGroupBox { border:none; background-image: url(\""+PlayerBottomSeatActive+"\") }");
	} else {
		ps->setStyleSheet("QGroupBox { border:none; background-image: url(\""+PlayerTopSeatActive+"\") }");
	}
}

void GameTableStyleReader::setBetValueInputStyle(QSpinBox *bv)
{
	bv->setStyleSheet("QSpinBox { "+ font2String +" font-size: "+betValueFontSize+"px; font-weight: bold; background-color: #"+BetInputBgColor+"; color: #"+BetInputTextColor+"; } QSpinBox:disabled { background-color: #"+BetInputDisabledBgColor+"; color: #"+BetInputDisabledTextColor+" }");
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
	}
	return QString("");
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
		br->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+BetRaiseButtonTextColor+";} QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonChecked+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonHover+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonCheckedHover+"\");}");

		cc->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+CheckCallButtonTextColor+";} QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonChecked+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonHover+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonCheckedHover+"\");}");

		f->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+FoldButtonTextColor+";}  QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonChecked+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonHover+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonCheckedHover+"\");}");

		a->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+AllInButtonTextColor+";}  QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonChecked+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonHover+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonCheckedHover+"\");}");
	}
	break;
	//no hover
	case 1: {
		br->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+BetRaiseButtonTextColor+";} QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonChecked+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonDefault+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonChecked+"\");}");

		cc->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+CheckCallButtonTextColor+";} QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonChecked+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonDefault+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonChecked+"\");}");

		f->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+FoldButtonTextColor+";}  QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonChecked+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonDefault+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonChecked+"\");}");

		a->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+AllInButtonTextColor+";}  QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonChecked+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonDefault+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonChecked+"\");}");
	}
	break;
	//checkable
	case 2: {
		br->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+BetRaiseButtonCheckableTextColor+";} QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonChecked+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonHover+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonCheckedHover+"\");}");

		cc->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+CheckCallButtonCheckableTextColor+";} QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonChecked+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonHover+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+CheckCallButtonCheckedHover+"\");}");

		f->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+FoldButtonCheckableTextColor+";}  QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonChecked+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonHover+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+FoldButtonCheckedHover+"\");}");

		a->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+AllInButtonCheckableTextColor+";}  QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonChecked+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonHover+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+AllInButtonCheckedHover+"\");}");
	}
	break;
	}

}

void GameTableStyleReader::setShowMyCardsButtonStyle( MyActionButton *sc)
{
	sc->setMyStyle(this);

	//Show My Cards Button has same look and feel all the time
	sc->setStyleSheet("QPushButton { border:none; background-repeat: no-repeat; background-position: center center; background-image: url(\""+ShowMyCardsButtonDefault+"\"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #"+ShowMyCardsButtonTextColor+";} QPushButton:unchecked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+ShowMyCardsButtonDefault+"\"); } QPushButton:checked { background-repeat: no-repeat; background-position: center center; background-image: url(\""+BetRaiseButtonDefault+"\");} QPushButton:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+ShowMyCardsButtonHover+"\"); } QPushButton:checked:hover { background-repeat: no-repeat; background-position: center center; background-image: url(\""+ShowMyCardsButtonHover+"\");}");

}

void GameTableStyleReader::setToolBoxBackground(QGroupBox* gb)
{
#ifndef ANDROID
	gb->setStyleSheet("QGroupBox { border:none; background-image: url(\""+ToolBoxBackground+"\") }");
#endif
}

void GameTableStyleReader::setTabWidgetStyle(QTabWidget *tw, QTabBar *tb)
{
	tw->setStyleSheet("QTabWidget::pane { border: 2px solid #"+TabWidgetBorderColor+"; border-radius: 2px; background-color: #"+TabWidgetBgColor+"; }  QTabWidget::tab-bar { left: 5px; } ");

	QString bottomPadding("");

#ifdef _WIN32
	bottomPadding = " padding-bottom: 3px;";
#endif

	QString tabTextFontSize;
#ifndef ANDROID
	tabTextFontSize = "font-size: 11px; ";
#endif

#ifndef MAEMO
	tb->setStyleSheet("QTabBar::tab{ "+ font1String + tabTextFontSize +" color: #"+TabWidgetTextColor+"; background-color: #"+TabWidgetBgColor+"; border: 2px solid #"+TabWidgetBorderColor+"; border-bottom-color: #"+TabWidgetBorderColor+"; border-top-left-radius: 4px; border-top-right-radius: 4px; padding-top: "+tabBarPaddingTop+"px;"+bottomPadding+" padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:selected, QTabBar::tab:hover { background-color: #"+TabWidgetBgColor+"; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:selected { border-color: #"+TabWidgetBorderColor+"; border-bottom-color: #"+TabWidgetBgColor+"; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;}  QTabBar::tab:!selected { margin-top: 2px; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:selected { margin-left: -4px; margin-right: -4px; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:first:selected { margin-left: 0; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:last:selected { margin-right: 0; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:only-one { margin: 0; } ");
#endif

}

void GameTableStyleReader::setWindowsGeometry(gameTableImpl *gt)
{
	if(IfFixedWindowSize.toInt()) {
#ifndef GUI_800x480
		QDesktopWidget dw;
		int availableWidth = dw.screenGeometry().width();
		int availableHeight = dw.screenGeometry().height();
		if(availableWidth == FixedWindowWidth.toInt() && availableHeight == FixedWindowHeight.toInt()) {
			gt->actionFullScreen->setEnabled(true);
		} else {
			gt->actionFullScreen->setDisabled(true);
			if(gt->isFullScreen()) {
				gt->showNormal();
				gt->move(50,50);
			}
		}
		gt->setMinimumSize(FixedWindowWidth.toInt(), FixedWindowHeight.toInt());
		gt->setMaximumSize(FixedWindowWidth.toInt(), FixedWindowHeight.toInt());
		gt->resize(FixedWindowWidth.toInt(), FixedWindowHeight.toInt());
#endif
	} else {
#ifndef GUI_800x480
		QDesktopWidget dw;
		int availableWidth = dw.screenGeometry().width();
		int availableHeight = dw.screenGeometry().height();
		if(availableWidth <= MaximumWindowWidth.toInt() && availableHeight <= MaximumWindowHeight.toInt()) {
			gt->actionFullScreen->setEnabled(true);
		} else {
			gt->actionFullScreen->setDisabled(true);
			if(gt->isFullScreen()) {
				gt->showNormal();
				gt->move(50,50);
			}
		}
		gt->setMinimumSize(MinimumWindowWidth.toInt(), MinimumWindowHeight.toInt());
		gt->setMaximumSize(MaximumWindowWidth.toInt(), MaximumWindowHeight.toInt());
#endif
	}

}

void GameTableStyleReader::setSliderStyle(QSlider *s)
{

	QString height("");

#ifdef GUI_800x480
	height = " height: 10px;";
#else
	height = " height: 3px;";
#endif

#if QT_VERSION >= 0x040700
#ifdef GUI_800x480
	s->setStyleSheet("QSlider::groove:horizontal { border: 2px solid #"+BetSpeedSliderGrooveBorderColor+";"+height+" background: #"+BetSpeedSliderGrooveBgColor+"; margin: 2px 0; border-radius: 2px; } QSlider::handle:horizontal { background: #"+BetSpeedSliderHandleBgColor+"; border: 2px solid #"+BetSpeedSliderHandleBorderColor+"; width: 58px; margin: -24px 0; border-radius: 4px;}");
#else
	s->setStyleSheet("QSlider::groove:horizontal { border: 1px solid #"+BetSpeedSliderGrooveBorderColor+";"+height+" background: #"+BetSpeedSliderGrooveBgColor+"; margin: 4px 0; border-radius: 2px; } QSlider::handle:horizontal { background: #"+BetSpeedSliderHandleBgColor+"; border: 1px solid #"+BetSpeedSliderHandleBorderColor+"; width: 12px; margin: -7px 0; border-radius: 4px;}");
#endif
#else
	s->setStyleSheet("QSlider::groove:horizontal { border: 1px solid #"+BetSpeedSliderGrooveBorderColor+";"+height+" background: #"+BetSpeedSliderGrooveBgColor+"; margin: 4px 0; border-radius: 2px; } QSlider::handle:horizontal { background: #"+BetSpeedSliderHandleBgColor+"; border: 1px solid #"+BetSpeedSliderHandleBorderColor+"; width: 12px; margin: -2px 0; border-radius: 4px;}");
#endif

}

QString GameTableStyleReader::getFallBackFieldContent(QString field, int type)
{
	QFile myFile(QFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default/defaulttablestyle.xml").fileName());
	myFile.open(QIODevice::ReadOnly);
	QByteArray thisContent = myFile.readAll();

	//start reading the file and fill vars
	string tempString1("");
	TiXmlDocument doc;
	doc.Parse(thisContent.constData());

	if(doc.RootElement()) {
		TiXmlHandle docHandle( &doc );
		TiXmlElement* itemsList = docHandle.FirstChild( "PokerTH" ).FirstChild( "TableStyle" ).FirstChild().ToElement();
		for( ; itemsList; itemsList=itemsList->NextSiblingElement()) {
			const char *tmpStr1 = itemsList->Attribute("value");
			if (tmpStr1) {
				tempString1 = tmpStr1;
				if(itemsList->ValueStr() == field.toStdString()) {
					if(type == 1) return QString(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default/"+QString::fromUtf8(tempString1.c_str()));
					else return QString::fromUtf8(tempString1.c_str());
				}
			}
		}
	}
	return QString("");
}

QString GameTableStyleReader::getMyStateToolTipInfo()
{
	switch (myState) {
	case GT_STYLE_OK:
		return QString(tr("Everything OK!"));
		break;
	case GT_STYLE_PICTURES_MISSING:
		return QString(tr("Some pictures are missing, please contact style maintainer for this issue."));
		break;
	case GT_STYLE_FIELDS_EMPTY:
		return QString(tr("Some style fields are missing, please contact style maintainer for this issue."));
		break;
	case GT_STYLE_OUTDATED:
		return QString(tr("This style is outdated, please contact style maintainer for this issue."));
		break;
	default:
		return QString("");
	}
}
