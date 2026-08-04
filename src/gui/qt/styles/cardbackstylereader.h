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
#ifndef CARDBACKSTYLEREADER_H
#define CARDBACKSTYLEREADER_H

#include <QtCore>

#define POKERTH_CB_STYLE_FILE_VERSION	1

enum CbStyleState {
	CB_STYLE_OK = 0,
	CB_STYLE_OUTDATED,
	CB_STYLE_FIELDS_EMPTY,
	CB_STYLE_PICTURE_MISSING,
	CB_STYLE_UNDEFINED
};

/** Reader for a stand alone card back style (Kartenrückseite).
 *
 * A card back style consists of exactly one picture plus its meta data, so
 * unlike GameTableStyleReader/CardDeckStyleReader there is neither a fallback
 * style nor an error dialog here: if a style cannot be loaded, the caller
 * simply keeps using the flipside.png of the selected card deck. Problems are
 * reported passively via getState()/getMyStateToolTipInfo() in the style list.
 * For the same reason it needs neither ConfigFile nor a parent widget.
 */
class CardBackStyleReader : public QObject
{
	Q_OBJECT
public:
	explicit CardBackStyleReader(QObject *parent = nullptr);
	~CardBackStyleReader();

	void readStyleFile(QString);

	QString getStyleDescription() const
	{
		return StyleDescription;
	}
	QString getStyleMaintainerName() const
	{
		return StyleMaintainerName;
	}
	QString getStyleMaintainerEMail() const
	{
		return StyleMaintainerEMail;
	}
	QString getStyleCreateDate() const
	{
		return StyleCreateDate;
	}

	QString getCurrentFileName() const
	{
		return currentFileName;
	}
	QString getCurrentDir() const
	{
		return currentDir;
	}

	/** Absolute path of the card back picture (empty if the style is broken). */
	QString getBackside() const
	{
		return Backside;
	}

	bool getLoadedSuccessfull() const
	{
		return loadedSuccessfull;
	}

	CbStyleState getState() const
	{
		return myState;
	}

	QString getMyStateToolTipInfo() const;

private:

	QString StyleDescription;
	QString StyleMaintainerName;
	QString StyleMaintainerEMail;
	QString StyleCreateDate;
	QString PokerTHStyleFileVersion;
	QString Backside;

	QString currentFileName;
	QString currentDir;

	QStringList leftItems;

	bool loadedSuccessfull;

	CbStyleState myState;
};

#endif
