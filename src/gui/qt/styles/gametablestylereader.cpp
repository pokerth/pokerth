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
	textBrowserFontsize= "11";
	cashFontSize = "11";
	setLabelFontSize = "11";
	playerNameLabelFontSize = "11";
	smallBoardFontSize = "13";
	bigBoardFontSize = "18";
	humanPlayerButtonFontSize = "13";
	betValueFontSize = "11";
	
	tabBarPaddingTop = "1";
	tabBarPaddingSide = "7";
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

				if(itemsList->ValueStr() == "StyleDescription") { StyleDescription = QString::fromUtf8(tempString1.c_str()); }
				else if(itemsList->ValueStr() == "StyleMaintainerEMail") { StyleMaintainerEMail = QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "Preview") { Preview = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "IfFixedWindowSize") { IfFixedWindowSize = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FixedWindowWidth") { FixedWindowWidth = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "FixedWindowHeight") { FixedWindowHeight = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MinimumWindowWidth") { MinimumWindowWidth = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MinimumWindowHeight") { MinimumWindowHeight = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MaximumWindowWidth") { MaximumWindowWidth = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "MaximumWindowHeight") { MaximumWindowHeight = currentDir+QString::fromUtf8(tempString1.c_str()); }
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
				else if (itemsList->ValueStr() == "PlayerSeatInActive") { PlayerSeatInActive = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "PlayerSeatActive") { PlayerSeatActive = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "Table") { Table = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "HandRanking") { HandRanking = currentDir+QString::fromUtf8(tempString1.c_str()); }
				else if (itemsList->ValueStr() == "ToolBoxBackground") { ToolBoxBackground = currentDir+QString::fromUtf8(tempString1.c_str()); }

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
	else {	qDebug() << "could not load game table style file: " << tinyFileName.c_str(); }
}

void GameTableStyleReader::showErrorMessage(QString style, QStringList failedItems, QString email)
{
	QString items = failedItems.join(", ");

	QMessageBox::warning(myW, tr("Game Table Style Error"),
                                tr("Selected game table style \"%1\" seems to be incomplete or defective. \nThe value of \"%2\" is wrong or left. Please contact the style builder %3.").arg(style).arg(items).arg(email),
                                QMessageBox::Ok);
}

void GameTableStyleReader::setTableBackground(gameTableImpl *gt)
{
	gt->setStyleSheet("QMainWindow { background-image: url("+Table+"); background-position: bottom center; background-origin: content;}");
}

void GameTableStyleReader::setLogStyle(QTextBrowser *tb)
{	
	tb->setStyleSheet("QTextBrowser { "+ font1String +" font-size: "+textBrowserFontsize+"px; color: #F0F0F0; background-color: #1D3B00; border:none; } QScrollBar:vertical { border: 1px solid #104600; background: #135000; width: 15px; margin: 17px -1px 17px 0px; } QScrollBar::handle:vertical { border-radius: 1px; border: 1px solid #1B7200; background: #176400; min-height: 20px; } QScrollBar::add-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 2px; border-bottom-left-radius: 2px; border-top-right-radius: 1px; border-top-left-radius: 1px; border: 1px solid #1E7F00; background: #1A6F00; height: 15px; subcontrol-position: bottom; subcontrol-origin: margin; } QScrollBar::sub-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 1px; border-bottom-left-radius: 1px; border-top-right-radius: 2px; border-top-left-radius: 2px; border: 1px solid #1E7F00; background: #1A6F00; height: 15px; subcontrol-position: top; subcontrol-origin: margin; } QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical { border: 1px solid #208A00; height: 3px; width: 3px; background: #27A800; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
	
}

void GameTableStyleReader::setChatStyle(QTextBrowser *tb)
{
	tb->setStyleSheet("QTextBrowser { "+ font1String +" font-size: "+textBrowserFontsize+"px; color: #F0F0F0; background-color: #1D3B00; border:none; } QScrollBar:vertical { border: 1px solid #104600; background: #135000; width: 15px; margin: 17px -1px 17px 0px; } QScrollBar::handle:vertical { border-radius: 1px; border: 1px solid #1B7200; background: #176400; min-height: 20px; } QScrollBar::add-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 2px; border-bottom-left-radius: 2px; border-top-right-radius: 1px; border-top-left-radius: 1px; border: 1px solid #1E7F00; background: #1A6F00; height: 15px; subcontrol-position: bottom; subcontrol-origin: margin; } QScrollBar::sub-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 1px; border-bottom-left-radius: 1px; border-top-right-radius: 2px; border-top-left-radius: 2px; border: 1px solid #1E7F00; background: #1A6F00; height: 15px; subcontrol-position: top; subcontrol-origin: margin; } QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical { border: 1px solid #208A00; height: 3px; width: 3px; background: #27A800; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
}

void GameTableStyleReader::setChatInputStyle(QLineEdit *ci)
{
	ci->setStyleSheet("QLineEdit { "+ font1String +" font-size: "+textBrowserFontsize+"px; color: #F0F0F0; background-color: #1D3B00; border-top: 2px solid #286400; }");
}

void GameTableStyleReader::setCashLabelStyle(QLabel *cl)
{	
	cl->setStyleSheet("QLabel { "+ font2String +" font-size: "+cashFontSize+"px; font-weight: bold; color: #FFFF00; }");
}

void GameTableStyleReader::setSetLabelStyle(QLabel *sl)
{
	sl->setStyleSheet("QLabel { "+ font2String +" font-size: "+setLabelFontSize+"px; font-weight: bold; color: #FFFF00; }");
}

void GameTableStyleReader::setPlayerNameLabelStyle(QLabel *pnl)
{
	pnl->setStyleSheet("QLabel { "+ font2String +" font-size: "+playerNameLabelFontSize+"px; font-weight: bold; color: #F0F0F0; }");
}

void GameTableStyleReader::setSmallFontBoardStyle(QLabel *l)
{
	l->setStyleSheet("QLabel { "+ font2String +" font-size: "+smallBoardFontSize+"px; font-weight: bold; color: #669900; }");
}

void GameTableStyleReader::setBigFontBoardStyle(QLabel *l)
{
	l->setStyleSheet("QLabel { "+ font2String +" font-size: "+bigBoardFontSize+"px; font-weight: bold; color: #669900; }");
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
	mb->setStyleSheet("QMenuBar { background-color: #145300; } QMenuBar::item { color: #99D500; }");
}

void GameTableStyleReader::setBreakButtonStyle(QPushButton *bb)
{
	bb->setStyleSheet("QPushButton:enabled { background-color: #145300; color: #99D500;} QPushButton:disabled { background-color: #145300; color: #486F3E; font-weight: 900;}");
}

void GameTableStyleReader::setSpeedStringStyle(QLabel *l)
{
	l->setStyleSheet("QLabel { color: #99D500;}");	
}

void GameTableStyleReader::setVoteButtonStyle(QPushButton *b)
{
	b->setStyleSheet("QPushButton:enabled { background-color: #1C7000; color: #99D500;} QPushButton:disabled { background-color: #145300; color: #486F3E; font-weight: 900;}");
}

void GameTableStyleReader::setVoteStringsStyle(QLabel *l)
{
	l->setStyleSheet("QLabel { color: #99D500; font-size: 11px;}");
}

void GameTableStyleReader::setPlayerSeatInActiveStyle(QGroupBox *ps)
{
	ps->setStyleSheet("QGroupBox { border:none; background-image: url("+PlayerSeatInActive+") }"); 
}

void GameTableStyleReader::setPlayerSeatActiveStyle(QGroupBox *ps)
{
	ps->setStyleSheet("QGroupBox { border:none; background-image: url("+PlayerSeatActive+") }"); 
}

void GameTableStyleReader::setBetValueInputStyle(QLineEdit *bv)
{
	bv->setStyleSheet("QLineEdit { "+ font2String +" font-size: "+betValueFontSize+"px; font-weight: bold; background-color: #1D3B00; color: #F0F0F0; } QLineEdit:disabled { background-color: #316300; color: #6d7b5f }");
}

void GameTableStyleReader::setAwayRadioButtonsStyle(QRadioButton *rb)
{
	rb->setStyleSheet("QRadioButton { color: #99D500; } QRadioButton::indicator { color: #99D500; }");
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

void GameTableStyleReader::setButtonsStyle(QPushButton *br, QPushButton *cc, QPushButton *f, QPushButton *a, int state)
{
	switch(state) {
		//default
		case 0: {
			br->setStyleSheet("QPushButton { border:none; background-image: url("+BetRaiseButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;} QPushButton:unchecked { background-image: url("+BetRaiseButtonDefault+"); } QPushButton:checked { background-image: url("+BetRaiseButtonChecked+");} QPushButton:hover { background-image: url("+BetRaiseButtonHover+"); } QPushButton:checked:hover { background-image: url("+BetRaiseButtonCheckedHover+");}"); 
			
			cc->setStyleSheet("QPushButton { border:none; background-image: url("+CheckCallButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;} QPushButton:unchecked { background-image: url("+CheckCallButtonDefault+"); } QPushButton:checked { background-image: url("+CheckCallButtonChecked+");} QPushButton:hover { background-image: url("+CheckCallButtonHover+"); } QPushButton:checked:hover { background-image: url("+CheckCallButtonCheckedHover+");}");

			f->setStyleSheet("QPushButton { border:none; background-image: url("+FoldButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;}  QPushButton:unchecked { background-image: url("+FoldButtonDefault+"); } QPushButton:checked { background-image: url("+FoldButtonChecked+");} QPushButton:hover { background-image: url("+FoldButtonHover+"); } QPushButton:checked:hover { background-image: url("+FoldButtonCheckedHover+");}");

			a->setStyleSheet("QPushButton { border:none; background-image: url("+AllInButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;}  QPushButton:unchecked { background-image: url("+AllInButtonDefault+"); } QPushButton:checked { background-image: url("+AllInButtonChecked+");} QPushButton:hover { background-image: url("+AllInButtonHover+"); } QPushButton:checked:hover { background-image: url("+AllInButtonCheckedHover+");}");
		}
		break;
		//no hover
		case 1: {
			br->setStyleSheet("QPushButton { border:none; background-image: url("+BetRaiseButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;} QPushButton:unchecked { background-image: url("+BetRaiseButtonDefault+"); } QPushButton:checked { background-image: url("+BetRaiseButtonChecked+");} QPushButton:hover { background-image: url("+BetRaiseButtonDefault+"); } QPushButton:checked:hover { background-image: url("+BetRaiseButtonChecked+");}"); 
			
			cc->setStyleSheet("QPushButton { border:none; background-image: url("+CheckCallButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;} QPushButton:unchecked { background-image: url("+CheckCallButtonDefault+"); } QPushButton:checked { background-image: url("+CheckCallButtonChecked+");} QPushButton:hover { background-image: url("+CheckCallButtonDefault+"); } QPushButton:checked:hover { background-image: url("+CheckCallButtonChecked+");}");

			f->setStyleSheet("QPushButton { border:none; background-image: url("+FoldButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;}  QPushButton:unchecked { background-image: url("+FoldButtonDefault+"); } QPushButton:checked { background-image: url("+FoldButtonChecked+");} QPushButton:hover { background-image: url("+FoldButtonDefault+"); } QPushButton:checked:hover { background-image: url("+FoldButtonChecked+");}");

			a->setStyleSheet("QPushButton { border:none; background-image: url("+AllInButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;}  QPushButton:unchecked { background-image: url("+AllInButtonDefault+"); } QPushButton:checked { background-image: url("+AllInButtonChecked+");} QPushButton:hover { background-image: url("+AllInButtonDefault+"); } QPushButton:checked:hover { background-image: url("+AllInButtonChecked+");}");
		}
		break;
		//checkable
		case 2: {
			br->setStyleSheet("QPushButton { border:none; background-image: url("+BetRaiseButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #87FF97;} QPushButton:unchecked { background-image: url("+BetRaiseButtonDefault+"); } QPushButton:checked { background-image: url("+BetRaiseButtonChecked+");} QPushButton:hover { background-image: url("+BetRaiseButtonHover+"); } QPushButton:checked:hover { background-image: url("+BetRaiseButtonCheckedHover+");}"); 
			
			cc->setStyleSheet("QPushButton { border:none; background-image: url("+CheckCallButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #87CDFF;} QPushButton:unchecked { background-image: url("+CheckCallButtonDefault+"); } QPushButton:checked { background-image: url("+CheckCallButtonChecked+");} QPushButton:hover { background-image: url("+CheckCallButtonHover+"); } QPushButton:checked:hover { background-image: url("+CheckCallButtonCheckedHover+");}");

			f->setStyleSheet("QPushButton { border:none; background-image: url("+FoldButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #FF8787;}  QPushButton:unchecked { background-image: url("+FoldButtonDefault+"); } QPushButton:checked { background-image: url("+FoldButtonChecked+");} QPushButton:hover { background-image: url("+FoldButtonHover+"); } QPushButton:checked:hover { background-image: url("+FoldButtonCheckedHover+");}");

			a->setStyleSheet("QPushButton { border:none; background-image: url("+AllInButtonDefault+"); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #ffe187;}  QPushButton:unchecked { background-image: url("+AllInButtonDefault+"); } QPushButton:checked { background-image: url("+AllInButtonChecked+");} QPushButton:hover { background-image: url("+AllInButtonHover+"); } QPushButton:checked:hover { background-image: url("+AllInButtonCheckedHover+");}");
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
	tw->setStyleSheet("QTabWidget::pane { border: 2px solid #286400; border-radius: 2px; background-color: #145300; }  QTabWidget::tab-bar { left: 5px; } ");

	tb->setStyleSheet("QTabBar::tab{ "+ font1String +" font-size: 11px; color: #99D500; background-color: #145300; border: 2px solid #286400; border-bottom-color: #286400; border-top-left-radius: 4px; border-top-right-radius: 4px; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:selected, QTabBar::tab:hover { background-color: #145300; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:selected { border-color: #286400; border-bottom-color: #145300; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;}  QTabBar::tab:!selected { margin-top: 2px; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:selected { margin-left: -4px; margin-right: -4px; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:first:selected { margin-left: 0; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:last:selected { margin-right: 0; padding-top: "+tabBarPaddingTop+"px; padding-left:"+tabBarPaddingSide+"px; padding-right:"+tabBarPaddingSide+"px;} QTabBar::tab:only-one { margin: 0; } QTabBar::tab:disabled { font-size: 11px; font-weight: 900; color: #144D03; background-color: #144D03; border-left-color: #255704; border-right-color: #255704; border-top-color: #255704; }");

}
