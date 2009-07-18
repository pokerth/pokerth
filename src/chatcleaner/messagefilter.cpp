#include "messagefilter.h"

#include <QtCore>
#include "badwordcheck.h"
#include "textfloodcheck.h"

enum OffenceType {
	BAD_WORD,
	TEXT_FLOOD_LINES };

MessageFilter::MessageFilter()
{
	myBadWordCheck = new BadWordCheck;
	myTextFloodCheck = new TextFloodCheck;
}

QString MessageFilter::check(QString msg) 
{
	if(myBadWordCheck->run(msg)) {
		return QString("<PokerTHCleaner> Der Blitz soll dich beim Scheißen erschlagen du Spammer\n");
	}
	if(myTextFloodCheck->run(1)) {
		return QString("<PokerTHCleaner> Bitte langsamer schreiben!!!\n");
	}
	
	return QString("");
}
