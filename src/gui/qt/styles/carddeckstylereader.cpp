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
#include "carddeckstylereader.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include "mymessagebox.h"
#include "mymessagedialogimpl.h"
#include "game_defs.h"

using namespace std;

CardDeckStyleReader::CardDeckStyleReader(ConfigFile *c, QWidget *w) : myConfig(c), myW(w), fallBack(0), loadedSuccessfull(0), myState(CD_STYLE_UNDEFINED)
{

}


CardDeckStyleReader::~CardDeckStyleReader()
{
}

void CardDeckStyleReader::readStyleFile(QString file)
{
	BigIndexesActionBottom = "";

#ifdef ANDROID
	//on Android we use just the defaul style packed with the binary via qrc
	currentFileName = ":/android/android-data/gfx/cards/default_800x480/defaultdeckstyle_800x480.xml";
	currentDir = ":/android/android-data/gfx/cards/default_800x480/";
#else
	//if style file failed --> default style fallback
	if(QFile(file).exists()) {
		currentFileName = file;
	} else {
		currentFileName = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default_800x480/defaultdeckstyle_800x480.xml";
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
		TiXmlElement *GameTableElement = docHandle.FirstChild( "PokerTH" ).FirstChild( "TableStyle" ).ToElement();
		if(GameTableElement) {
			MyMessageBox::warning(myW, tr("Card Deck Style Error"),
								  tr("A game table style was selected instead of a card deck style.\nPlease select a card deck style and try again!"),
								  QMessageBox::Ok);
		} else {

			TiXmlElement* itemsList = docHandle.FirstChild( "PokerTH" ).FirstChild( "CardDeck" ).FirstChild().ToElement();
			for( ; itemsList; itemsList=itemsList->NextSiblingElement()) {
				const char *tmpStr1 = itemsList->Attribute("value");
				if (tmpStr1) {
					tempString1 = tmpStr1;

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
					} else if (itemsList->ValueStr() == "Preview") {
						Preview = currentDir+QString::fromUtf8(tempString1.c_str());
					} else if (itemsList->ValueStr() == "BigIndexesActionBottom") {
						BigIndexesActionBottom = QString::fromUtf8(tempString1.c_str());
					}
				}
			}
			//check if style items are left and show warning
			leftItems.clear();

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
			if(BigIndexesActionBottom == "") {
				//ATTENTION: no fallback action here because default position is mostly CENTER
				BigIndexesActionBottom = "0";
				leftItems << "BigIndexesActionBottom";
			}

			//check if all files are there
			cardsLeft.clear();
			int i;
			for(i=0; i<52; i++) {
				QString cardString(QString::number(i)+".png");
				if(!QDir(currentDir).exists(cardString)) {
					cardsLeft << cardString;
				}
			}
			if(!QDir(currentDir).exists("flipside.png")) {
				cardsLeft << "flipside.png";
			}

			// set loadedSuccessfull true if everything works
			if(leftItems.isEmpty() && cardsLeft.isEmpty() && PokerTHStyleFileVersion != "" && PokerTHStyleFileVersion.toInt() == POKERTH_CD_STYLE_FILE_VERSION) {
				myState = CD_STYLE_OK;
			} else {
				//check for style file version
				if(PokerTHStyleFileVersion != "" && PokerTHStyleFileVersion.toInt() != POKERTH_CD_STYLE_FILE_VERSION) {
					myState = CD_STYLE_OUTDATED;
				} else {
					//if one or more items are left
					if(!leftItems.isEmpty() && myW != 0) myState = CD_STYLE_FIELDS_EMPTY;

					//if one or more pictures where not found
					if(!cardsLeft.isEmpty() && myW != 0) myState = CD_STYLE_PICTURES_MISSING;
				}
			}
			loadedSuccessfull = 1;
		}
	} else {
		loadedSuccessfull = 0;
		MyMessageBox::warning(myW, tr("Card Deck Style Error"),
							  tr("Cannot load card deck style file: %1 \n\nPlease check the style file or choose another style!").arg(currentFileName),
							  QMessageBox::Ok);
	}
}


void CardDeckStyleReader::showErrorMessage()
{
	switch (myState) {
	case CD_STYLE_PICTURES_MISSING:
		showCardsLeftErrorMessage();
		break;
	case CD_STYLE_FIELDS_EMPTY:
		showLeftItemsErrorMessage();
		break;
	case CD_STYLE_OUTDATED:
		showOutdatedErrorMessage();
		break;
	default:
		;
	}
}

QString CardDeckStyleReader::getMyStateToolTipInfo()
{
	switch (myState) {
	case CD_STYLE_OK:
		return QString(tr("Everything OK!"));
		break;
	case CD_STYLE_PICTURES_MISSING:
		return QString(tr("Some cards pictures are missing, please contact style maintainer for this issue."));
		break;
	case CD_STYLE_FIELDS_EMPTY:
		return QString(tr("Some style fields are missing, please contact style maintainer for this issue."));
		break;
	case CD_STYLE_OUTDATED:
		return QString(tr("This style is outdated, please contact style maintainer for this issue."));
		break;
	default:
		return QString("");
	}
}


void CardDeckStyleReader::showLeftItemsErrorMessage()
{
	QString items = leftItems.join("\n");
	QString EMail;
	if(StyleMaintainerEMail != "NULL") EMail = StyleMaintainerEMail;

	myMessageDialogImpl dialog(myConfig, myW);

	if(dialog.checkIfMesssageWillBeDisplayed(CD_VALUES_MISSING)) {
		dialog.exec(CD_VALUES_MISSING, tr("Selected card deck style \"%1\" seems to be incomplete or defective. \n\nThe value(s) of: \n%2 \nis/are missing. \n\nAnyway you can play with this deck, because the missing content will be filled up by PokerTH default card deck. \n\nPlease contact the card deck style builder via \"%3\".").arg(StyleDescription).arg(items).arg(EMail), tr("Card Deck Style Error - Fields content missing"), QPixmap(":/gfx/emblem-important-64.png"), QDialogButtonBox::Ok, true);
	}

}

void CardDeckStyleReader::showCardsLeftErrorMessage()
{
	QString pics = cardsLeft.join("\n");
	QString EMail;
	if(StyleMaintainerEMail != "NULL") EMail = StyleMaintainerEMail;

	myMessageDialogImpl dialog(myConfig, myW);

	if(dialog.checkIfMesssageWillBeDisplayed(CD_PICS_MISSING)) {
		dialog.exec(CD_PICS_MISSING, tr("One or more pictures from current card deck style \"%1\" were not found: \n\n%2 \n\nPlease contact the card deck style builder via \"%3\".").arg(StyleDescription).arg(pics).arg(EMail), tr("Card Deck Style Error - Pictures missing"), QPixmap(":/gfx/emblem-important-64.png"), QDialogButtonBox::Ok, true);
	}
}

void CardDeckStyleReader::showOutdatedErrorMessage()
{
	QString EMail;
	if(StyleMaintainerEMail != "NULL") EMail = StyleMaintainerEMail;

	myMessageDialogImpl dialog(myConfig, myW);

	if(dialog.checkIfMesssageWillBeDisplayed(CD_OUTDATED)) {
		dialog.exec(CD_OUTDATED, tr("Selected card deck style \"%1\" seems to be outdated. \nThe current PokerTH card deck style version is \"%2\", but this deck has version \"%3\" set. \n\nAnyway you can play with this deck, because the missing content will be filled up by PokerTH default card deck. \n\nPlease contact the card deck style builder via \"%4\".").arg(StyleDescription).arg(POKERTH_CD_STYLE_FILE_VERSION).arg(PokerTHStyleFileVersion).arg(EMail), tr("Card Deck Style Error - Outdated"), QPixmap(":/gfx/emblem-important-64.png"), QDialogButtonBox::Ok, true);
	}
}
