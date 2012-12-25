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
#include "textfloodcheck.h"
#include <QtCore>

TextFloodCheck::TextFloodCheck()
{
	timer.reset();
	timer.start();

	cleanTimer = new QTimer();

	connect(cleanTimer, SIGNAL(timeout()), this, SLOT(cleanMsgTimesList()));

	cleanTimer->start(4000);

}

TextFloodCheck::~TextFloodCheck()
{
	delete cleanTimer;
}

bool TextFloodCheck::run(unsigned playerId)
{

	QMapIterator<unsigned, TextFloodInfos> it(msgTimesList);
	while (it.hasNext()) {
		it.next();
//		 qDebug() << "counter: " << msgTimesList.count() << " playerid: " << it.key() << " floodlevel: " << it.value().floodLevel << " timestamp: " <<  it.value().timeStamp;
	}

	QMap<unsigned, TextFloodInfos>::const_iterator i = msgTimesList.find(playerId);

	if(i == msgTimesList.end()) {
		TextFloodInfos tmpInfos1;
		tmpInfos1.floodLevel = 0;
		tmpInfos1.timeStamp = timer.elapsed().total_seconds();
		msgTimesList.insert(playerId, tmpInfos1);
		qDebug () << "Add Player: set player floodlevel to " << tmpInfos1.floodLevel;
	} else {
		TextFloodInfos tmpInfos2;
		if(timer.elapsed().total_seconds()-i.value().timeStamp <= 1) {
			if(i.value().floodLevel == textFloodLevelToTrigger) {
				TextFloodInfos tmpInfos3;
				tmpInfos3.floodLevel = i.value().floodLevel-1;
				tmpInfos3.timeStamp = timer.elapsed().total_seconds();
				qDebug () << "Trigger: set player floodlevel to " << tmpInfos3.floodLevel;
				msgTimesList.insert(playerId, tmpInfos3);
				return true;
			} else {
				tmpInfos2.floodLevel = i.value().floodLevel+1;
				qDebug () << "Raise: set player floodlevel to " << tmpInfos2.floodLevel << " from " << i.value().floodLevel;
			}
		} else {
			tmpInfos2.floodLevel = i.value().floodLevel;
			qDebug () << "Keep: player floodlevel is " << tmpInfos2.floodLevel << " from " << i.value().floodLevel;
		}

		tmpInfos2.timeStamp = timer.elapsed().total_seconds();
		msgTimesList.insert(playerId, tmpInfos2);
	}
	return false;
}

void TextFloodCheck::cleanMsgTimesList()
{

	QMapIterator<unsigned, TextFloodInfos> it(msgTimesList);
	while (it.hasNext()) {
		it.next();
		if(timer.elapsed().total_seconds()-it.value().timeStamp > 3) {

			if(it.value().floodLevel == 0) {
				msgTimesList.remove(it.key());
				qDebug () << "Refresh: player removed from List";
			} else {
				TextFloodInfos tmpInfos;
				tmpInfos.floodLevel = it.value().floodLevel-1;
				tmpInfos.timeStamp = it.value().timeStamp;
				qDebug () << "Refresh: player floodlevel to " << tmpInfos.floodLevel;
				msgTimesList.insert(it.key(), tmpInfos);
			}
		}
//		qDebug() << msgTimesList.count() << it.key() << ": " << it.value().floodLevel << it.value().timeStamp << endl;
	}
}

void TextFloodCheck::removeNickFromList(unsigned playerId)
{

//	qDebug() << "id " << playerId << "removed from textfloodcheck list" << endl;
	msgTimesList.remove(playerId);
}
