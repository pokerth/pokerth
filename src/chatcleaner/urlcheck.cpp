#include "urlcheck.h"

#include <QtCore>

UrlCheck::UrlCheck()
{
}

bool UrlCheck::run(QString msg)
{
	msg = msg.toLower();

	QStringListIterator it1(urlStrings);

	while (it1.hasNext()) {
		if(msg.contains(it1.next())) {
			QStringListIterator it2(urlExceptionStrings);
			bool exception = false;
			while (it2.hasNext()) {
				if(msg.contains(it2.next())) {
					exception = true;
					break;
				}
			}
			if(!exception) {
				return true;
			}
		}
	}
	return false;
}
