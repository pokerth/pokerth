#include "textfloodcheck.h"

TextFloodCheck::TextFloodCheck()
{
	timer.reset();
	timer.start();
}

bool TextFloodCheck::run(unsigned playerId) {
	
	QMapIterator<unsigned, TextFloodInfos> it(msgTimesList);
	while (it.hasNext()) {
		 it.next();
		 qDebug() << msgTimesList.count() << it.key() << ": " << it.value().floodLevel << it.value().timeStamp << endl;
	}
	
	QMap<unsigned, TextFloodInfos>::const_iterator i = msgTimesList.find(playerId);
	
	if(i == msgTimesList.end()) {
		myTextFloodInfos.floodLevel = 0;
		myTextFloodInfos.timeStamp = timer.elapsed().total_seconds();
		msgTimesList.insert(playerId, myTextFloodInfos);
	}
	else {
		if(timer.elapsed().total_seconds()-i.value().timeStamp <= 1) {
			if(i.value().floodLevel == 2)
				return true;
			else if(i.value().floodLevel == 1)
				myTextFloodInfos.floodLevel = 2;
			else
				myTextFloodInfos.floodLevel = 1;
				
		}
		myTextFloodInfos.timeStamp = timer.elapsed().total_seconds();
		msgTimesList.insert(playerId, myTextFloodInfos);
	}
	return false;
}

void TextFloodCheck::cleanMsgTimesList() {


}
