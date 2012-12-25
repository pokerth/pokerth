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
#include "messagefilter.h"

#include <QtCore>
#include "badwordcheck.h"
#include "textfloodcheck.h"
#include "cleanerconfig.h"
#include "capsfloodcheck.h"
#include "letterrepeatingcheck.h"
#include "urlcheck.h"

enum ActionType {
	NOTHING,
	WARN,
	KICK,
	KICKBAN,
	MUTE
};

enum OffenceType {
	NONE,
	BAD_WORD,
	TEXT_FLOOD_LINES,
	CAPS_FLOOD,
	LETTER_REPEATING,
	URL
};

MessageFilter::MessageFilter(CleanerConfig *c): config(c)
{
	myBadWordCheck = new BadWordCheck;
	myTextFloodCheck = new TextFloodCheck;
	myCapsFloodCheck = new CapsFloodCheck;
	myLetterRepeatingCheck = new LetterRepeatingCheck;
	myUrlCheck = new UrlCheck;

	cleanTimer = new QTimer();
	connect(cleanTimer, SIGNAL(timeout()), this, SLOT(cleanKickCounterList()));
	cleanTimer->start(30000);
}

MessageFilter::~MessageFilter()
{

	delete myBadWordCheck;
	delete myTextFloodCheck;
	delete myCapsFloodCheck;
	delete myLetterRepeatingCheck;
	delete myUrlCheck;
	delete cleanTimer;
}

QStringList MessageFilter::check(unsigned gameId, unsigned playerId, QString nick, QString msg)
{
	QStringList returnList;
	QString returnMessage;
	QString returnAction;

	OffenceType offence = NONE;

	if(myBadWordCheck->run(msg)) offence = BAD_WORD;
	if(myCapsFloodCheck->run(msg)) offence = CAPS_FLOOD;
	if(myLetterRepeatingCheck->run(msg)) offence = LETTER_REPEATING;
	if(myUrlCheck->run(msg)) offence = URL;
	if(myTextFloodCheck->run(playerId)) offence = TEXT_FLOOD_LINES;

	if(offence) {

		ActionType action = NOTHING;
		QMap<unsigned, ClientWarnInfos>::const_iterator i = myClientWarnLevelList.find(playerId);

		if(i == myClientWarnLevelList.end()) {
			ClientWarnInfos tmpInfos;
			tmpInfos.warnLevel = 1;
			tmpInfos.lastWarnType = offence;
			tmpInfos.nick = nick;
			myClientWarnLevelList.insert(playerId, tmpInfos);
			action = WARN;
		} else {
			if(i.value().warnLevel == warnLevelToKick || i.value().lastWarnType == offence) {
				if(gameId) {
					//check for ingame to do not kick but mute
					action = MUTE;
					//remove playerId from all lists and as LAST from myClientWarnLevelList
					myTextFloodCheck->removeNickFromList(i.key());
					myClientWarnLevelList.remove(i.key());
				} else {
					//				Kick Command
					action = KICK;
					//remove playerId from all lists and as LAST from myClientWarnLevelList
					myTextFloodCheck->removeNickFromList(i.key());
					myClientWarnLevelList.remove(i.key());
					//check if player is already on kickCounterList
					QMap<QString, ClientKickInfos>::const_iterator j = myClientKickCounterList.find(nick);
					if(j == myClientKickCounterList.end()) {
						//if player is NOT on this list put the playerId on it to ban after multiple offence
						ClientKickInfos tmpInfos;
						tmpInfos.kickNumber = 1;
						tmpInfos.lastKickTimestamp = timer.elapsed().total_seconds();
						myClientKickCounterList.insert(nick, tmpInfos);
					} else {
						//pleayer is already on the list: either raise kickNumber or kickban when kickNumerToBan is reached
						if(j.value().kickNumber == kickNumberToBan) {
							action = KICKBAN;
							//remove player from kickCounterList
							myClientKickCounterList.remove(j.key());
						} else {
							ClientKickInfos tmpInfos;
							tmpInfos.kickNumber = j.value().kickNumber+1;
							tmpInfos.lastKickTimestamp = timer.elapsed().total_seconds();
							myClientKickCounterList.insert(nick, tmpInfos);
						}
					}
				}
			} else {
				ClientWarnInfos tmpInfos;
				tmpInfos.warnLevel = i.value().warnLevel+1;
				tmpInfos.lastWarnType = offence;
				tmpInfos.nick = nick;
				myClientWarnLevelList.insert(playerId, tmpInfos);
				action = WARN;
			}
		}

		if(action == WARN) {

			switch(offence) {
			case BAD_WORD: {
				returnMessage = QString ("%1: Warning! No racial, religious, sexually inflammatory or otherwise insulting language\n").arg(nick);
			}
			break;
			case TEXT_FLOOD_LINES: {
				returnMessage = QString ("%1: Warning! You've triggered text flood (lines) protection, slow down your typing!\n").arg(nick);
			}
			break;
			case CAPS_FLOOD: {
				returnMessage = QString ("%1: Warning: You've triggered caps flood protection, release your caps!\n").arg(nick);
			}
			break;
			case LETTER_REPEATING: {
				returnMessage = QString ("%1: Warning: You've triggered letter repeating protection, stop repeating!\n").arg(nick);
			}
			break;
			case URL: {
				returnMessage = QString ("%1: Warning: You've triggered url spam protection, stop posting urls!\n").arg(nick);
			}
			break;
			default:
				;
			}
			returnAction = QString("warn");
		} else if(action == KICK) {
			returnMessage = QString("%1 kicked! Please respect: http://chatrules.pokerth.net\n").arg(nick);
			returnAction = QString("kick");
		} else if(action == KICKBAN) {
			returnMessage = QString("%1 kicked and banned! Please respect: http://chatrules.pokerth.net\n").arg(nick);
			returnAction = QString("kickban");
		} else if(action == MUTE) {
			returnMessage = QString("%1 muted! Please respect: http://chatrules.pokerth.net\n").arg(nick);
			returnAction = QString("mute");
		}
	} else {
		returnAction = QString("");
		returnMessage = QString("");
	}

	returnList << returnAction << returnMessage;
	return returnList;
}

void MessageFilter::refreshConfig()
{

	//	global settings
	warnLevelToKick = config->readConfigInt("WarnLevelToKick");
	kickNumberToBan = config->readConfigInt("KickNumberToBan");

	// special check settings
	//Bad Words
	std::list<std::string> badWordsList = config->readConfigStringList("BadWordsList");
	std::list<std::string>::iterator it0;
	QStringList bwList;
	for(it0= badWordsList.begin(); it0 != badWordsList.end(); ++it0) {
		bwList << QString::fromUtf8(it0->c_str());
	}
	myBadWordCheck->setBadWords(bwList);

	std::list<std::string> badWordsExceptionList = config->readConfigStringList("BadWordsException");
	std::list<std::string>::iterator it1;
	QStringList bweList;
	for(it1= badWordsExceptionList.begin(); it1 != badWordsExceptionList.end(); ++it1) {
		bweList << QString::fromUtf8(it1->c_str());
	}
	myBadWordCheck->setBadWordsException(bweList);

	std::list<std::string> urlStringsList = config->readConfigStringList("UrlStringsList");
	std::list<std::string>::iterator it2;
	QStringList urlList;
	for(it2= urlStringsList.begin(); it2 != urlStringsList.end(); ++it2) {
		urlList << QString::fromUtf8(it2->c_str());
	}
	myUrlCheck->setUrlStrings(urlList);

	std::list<std::string> urlExceptionStringsList = config->readConfigStringList("UrlExceptionStringsList");
	std::list<std::string>::iterator it3;
	QStringList urlExceptionList;
	for(it3= urlExceptionStringsList.begin(); it3 != urlExceptionStringsList.end(); ++it3) {
		urlExceptionList << QString::fromUtf8(it3->c_str());
	}
	myUrlCheck->setUrlExceptionStrings(urlExceptionList);

	myTextFloodCheck->setTextFloodLevelToTrigger(config->readConfigInt("TextFloodLevelToTrigger"));
	myCapsFloodCheck->setCapsNumberToTrigger(config->readConfigInt("CapsFloodCapsNumberToTrigger"));
	myLetterRepeatingCheck->setLetterNumberToTrigger(config->readConfigInt("LetterRepeatingNumberToTrigger"));

}

void MessageFilter::cleanKickCounterList()
{
	QMapIterator<QString, ClientKickInfos> it(myClientKickCounterList);
	while (it.hasNext()) {
		it.next();
		if(timer.elapsed().total_seconds()-it.value().lastKickTimestamp > static_cast<unsigned>(config->readConfigInt("SecondsToForgetAboutKick"))) {
//            qDebug() << it.key() << "removed from kick counter list" << endl;
			myClientKickCounterList.remove(it.key());
		}
	}
}
