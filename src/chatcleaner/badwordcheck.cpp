#include "badwordcheck.h"

#include <QtCore>

BadWordCheck::BadWordCheck()
{
}

bool BadWordCheck::run(QString msg) 
{
	msg = msg.toLower();
	
	QStringListIterator it(badWords);
	while (it.hasNext()) {
		if(msg.contains(it.next()))
			return true;
	}
	return false;
}
