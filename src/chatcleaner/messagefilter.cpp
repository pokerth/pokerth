#include "messagefilter.h"

#include <QtCore>
#include "badwordcheck.h"
#include "textfloodcheck.h"
#include "cleanerconfig.h"

enum ActionType {
	NOTHING,
	WARN,
	KICK };

enum OffenceType {
	NONE,
	BAD_WORD,
	TEXT_FLOOD_LINES };

MessageFilter::MessageFilter(CleanerConfig *c): config(c)
{
	myBadWordCheck = new BadWordCheck;
	myTextFloodCheck = new TextFloodCheck;
}

QString MessageFilter::check(unsigned playerId, QString nick, QString msg) 
{
	QString returnMessage;
	
	OffenceType offence = NONE;
	ActionType action = NOTHING;
	
	if(myBadWordCheck->run(msg)) offence = BAD_WORD;
	if(myTextFloodCheck->run(playerId)) offence = TEXT_FLOOD_LINES;
	
	if(offence){
	
		QMap<unsigned, ClientWarnInfos>::const_iterator i = myClientWarnLevelList.find(playerId);
		
		if(i == myClientWarnLevelList.end()) {
			myClientWarnInfos.warnLevel = 1;
			myClientWarnInfos.lastWarnType = offence;
			myClientWarnInfos.nick = nick;
			myClientWarnLevelList.insert(playerId, myClientWarnInfos);
			action = WARN;
		}
		else {
			if(i.value().warnLevel == warnLevelToKick) {	
//				Kick Command
				action = KICK;
				myClientWarnLevelList.remove(i.key());
			}
			else {
				myClientWarnInfos.warnLevel++;
				myClientWarnInfos.lastWarnType = offence;
				myClientWarnInfos.nick = nick;
				myClientWarnLevelList.insert(playerId, myClientWarnInfos);
				action = WARN;
			}
		}
		
		if(action == KICK) {
			returnMessage = QString("Kick: %1\n").arg(nick);
		}
		if(action == WARN) {
			
			switch(offence) {
				case BAD_WORD: {
					returnMessage = QString ("<PokerTHCleaner> %1: Warning! No racial, religious, or sexually inflammatory language!\n").arg(nick);
				}
				break;
				case TEXT_FLOOD_LINES: {
					returnMessage = QString ("<PokerTHCleaner> %1: Warning! You've triggered text flood (lines) protection, slow down your typing!\n").arg(nick);
				}
				break;
				default:;
			}
		}
	}
	else {
		returnMessage = QString("");
	}
	return returnMessage;
}

void MessageFilter::refreshConfig() {

	//	global settings
	warnLevelToKick = config->readConfigInt("WarnLevelToKick");
	
	// special check settings
	//Bad Words
	std::list<std::string> badWordsList = config->readConfigStringList("BadWordsList");
	std::list<std::string>::iterator it1;
	QStringList bwList;
	for(it1= badWordsList.begin(); it1 != badWordsList.end(); it1++) {
		bwList << QString::fromUtf8(it1->c_str());
	}
	myBadWordCheck->setBadWords(bwList);
	
	myTextFloodCheck->setTextFloodLevelToTrigger(config->readConfigInt("TextFloodLevelToTrigger"));
}
