#include "letterrepeatingcheck.h"

#include <QtCore>

LetterRepeatingCheck::LetterRepeatingCheck()
: letterNumberToTrigger(0)
{
}

bool LetterRepeatingCheck::run(QString msg) 
{
	msg = msg.simplified().remove(" ");
	QRegExp e(QString(".*(.)\\1{%1,}.*").arg(letterNumberToTrigger-1));
	if(e.isValid()) {
		if(e.exactMatch(msg)) { 
			return true; 
		}
		else {
			return false;
		}
	}
	else { 
//		qDebug() << "The current Letter Repeating RegExp is invalid" << endl;
		return false;
	}
}
