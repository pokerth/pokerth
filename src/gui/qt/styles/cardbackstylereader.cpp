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
#include "cardbackstylereader.h"

#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QFileInfo>

CardBackStyleReader::CardBackStyleReader(QObject *parent) : QObject(parent), loadedSuccessfull(0), myState(CB_STYLE_UNDEFINED)
{

}


CardBackStyleReader::~CardBackStyleReader()
{
}

void CardBackStyleReader::readStyleFile(QString file)
{
	StyleDescription = "";
	StyleMaintainerName = "";
	StyleMaintainerEMail = "";
	StyleCreateDate = "";
	PokerTHStyleFileVersion = "";
	Backside = "";
	leftItems.clear();
	loadedSuccessfull = 0;
	myState = CB_STYLE_UNDEFINED;

	//no fallback here: the caller keeps the card deck flipside if this fails
	if(!QFile(file).exists()) {
		return;
	}
	currentFileName = file;

	QFileInfo info(currentFileName);
	currentDir = info.absolutePath()+"/";

	QFile myFile(currentFileName);
	if(!myFile.open(QIODevice::ReadOnly)) {
		return;
	}

	QDomDocument xmlDoc;
	xmlDoc.setContent(myFile.readAll());
	if(xmlDoc.documentElement().isNull()) {
		return;
	}

	QDomElement itemsList = xmlDoc.documentElement().firstChildElement("CardBack");
	if(itemsList.isNull()) {
		//a game table or card deck style was passed in - not a card back style
		return;
	}

	QString backsideFile;
	for(QDomElement n = itemsList.firstChildElement(); !n.isNull(); n = n.nextSiblingElement()) {
		const QString value = n.attribute("value");

		if(n.tagName() == "StyleDescription") {
			StyleDescription = value;
		} else if(n.tagName() == "StyleMaintainerName") {
			StyleMaintainerName = value;
		} else if(n.tagName() == "StyleMaintainerEMail") {
			StyleMaintainerEMail = value;
		} else if(n.tagName() == "StyleCreateDate") {
			StyleCreateDate = value;
		} else if(n.tagName() == "PokerTHStyleFileVersion") {
			PokerTHStyleFileVersion = value;
		} else if(n.tagName() == "Backside") {
			backsideFile = value;
		}
	}

	//check if style items are left
	if(StyleDescription == "") leftItems << "StyleDescription";
	if(StyleMaintainerName == "") leftItems << "StyleMaintainerName";
	if(StyleMaintainerEMail == "") leftItems << "StyleMaintainerEMail";
	if(StyleCreateDate == "") leftItems << "StyleCreateDate";
	if(PokerTHStyleFileVersion == "") leftItems << "PokerTHStyleFileVersion";
	if(backsideFile == "") leftItems << "Backside";

	if(backsideFile != "" && QDir(currentDir).exists(backsideFile)) {
		Backside = currentDir+backsideFile;
	}

	if(PokerTHStyleFileVersion != "" && PokerTHStyleFileVersion.toInt() != POKERTH_CB_STYLE_FILE_VERSION) {
		myState = CB_STYLE_OUTDATED;
	} else if(!leftItems.isEmpty()) {
		myState = CB_STYLE_FIELDS_EMPTY;
	} else if(Backside == "") {
		myState = CB_STYLE_PICTURE_MISSING;
	} else {
		myState = CB_STYLE_OK;
	}

	loadedSuccessfull = 1;
}

QString CardBackStyleReader::getMyStateToolTipInfo() const
{
	switch (myState) {
	case CB_STYLE_OK:
		return QString(tr("Everything OK!"));
	case CB_STYLE_PICTURE_MISSING:
		return QString(tr("The card back picture is missing, please contact style maintainer for this issue."));
	case CB_STYLE_FIELDS_EMPTY:
		return QString(tr("Some style fields are missing, please contact style maintainer for this issue."));
	case CB_STYLE_OUTDATED:
		return QString(tr("This style is outdated, please contact style maintainer for this issue."));
	default:
		return QString("");
	}
}
