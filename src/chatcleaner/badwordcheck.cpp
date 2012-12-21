#include "badwordcheck.h"

#include <QtCore>

BadWordCheck::BadWordCheck()
{
}

bool BadWordCheck::run(QString msg)
{
	msg = msg.toLower();
	bool badMessage(false);

	QStringListIterator it(badWords);
	while (it.hasNext()) {
		QString bw=it.next();
		if(msg.contains(bw)) {
			badMessage=true;

			//exception check
			QStringListIterator it2(badWordsException);
			while (it2.hasNext()) {
				QString bwe=it2.next();
				if(bwe.contains(bw) && msg.contains(bwe)) {
					badMessage=false;
				}
			}
		}
	}

	if(badMessage) return true;
	else return false;

}
