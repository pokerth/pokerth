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

bool TextFloodCheck::run(unsigned playerId) {
	
	QMapIterator<unsigned, TextFloodInfos> it(msgTimesList);
	while (it.hasNext()) {
		 it.next();
//		 qDebug() << msgTimesList.count() << it.key() << ": " << it.value().floodLevel << it.value().timeStamp << endl;
	}
	
	QMap<unsigned, TextFloodInfos>::const_iterator i = msgTimesList.find(playerId);
	
	if(i == msgTimesList.end()) {
		myTextFloodInfos.floodLevel = 0;
		myTextFloodInfos.timeStamp = timer.elapsed().total_seconds();
		msgTimesList.insert(playerId, myTextFloodInfos);
	}
	else {
		if(timer.elapsed().total_seconds()-i.value().timeStamp <= 1) {
			if(i.value().floodLevel == textFloodLevelToTrigger) {
				myTextFloodInfos.floodLevel--;	
//				qDebug () << "set player floodlevel to " << myTextFloodInfos.floodLevel << endl;
				myTextFloodInfos.timeStamp = timer.elapsed().total_seconds();
				msgTimesList.insert(playerId, myTextFloodInfos);
				return true;
			}
			else {
				myTextFloodInfos.floodLevel++;
			}		
		}
		myTextFloodInfos.timeStamp = timer.elapsed().total_seconds();
		msgTimesList.insert(playerId, myTextFloodInfos);
	}
	return false;
}

void TextFloodCheck::cleanMsgTimesList() {

	QMapIterator<unsigned, TextFloodInfos> it(msgTimesList);
	while (it.hasNext()) {
		 it.next();
		 if(timer.elapsed().total_seconds()-it.value().timeStamp > 3) {
			 
			if(it.value().floodLevel == 0)
				msgTimesList.remove(it.key());
				
			else {
				myTextFloodInfos.floodLevel--;
				myTextFloodInfos.timeStamp = it.value().timeStamp;
				msgTimesList.insert(it.key(), myTextFloodInfos);
			}
		}
//		qDebug() << msgTimesList.count() << it.key() << ": " << it.value().floodLevel << it.value().timeStamp << endl;
	}
}

void TextFloodCheck::removeNickFromList(unsigned playerId) {
	
//	qDebug() << "id " << playerId << "removed from textfloodcheck list" << endl;
	msgTimesList.remove(playerId);
}
