#include "capsfloodcheck.h"

#include <QtCore>

CapsFloodCheck::CapsFloodCheck()
	: capsNumberToTrigger(0)
{
}

bool CapsFloodCheck::run(QString msg)
{
	msg = msg.simplified().remove(" ");
	QRegExp e(QString("[A-Z]{%1,}").arg(capsNumberToTrigger));
	if(e.isValid()) {
		e.setCaseSensitivity(Qt::CaseSensitive);
		e.indexIn(msg);
		if(e.matchedLength() != -1 ) return true;
		else return false;
	} else {
//		qDebug() << "The current Caps Flood RegExp is invalid" << endl;
		return false;
	}
}
