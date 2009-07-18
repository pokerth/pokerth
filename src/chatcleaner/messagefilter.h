#ifndef MESSAGEFILTER_H
#define MESSAGEFILTER_H

#include <QtCore>
#include <stdlib.h>

class BadWordCheck;
class TextFloodCheck;

class MessageFilter: public QObject {
Q_OBJECT
public:
    MessageFilter();

	QString check(QString);
			
private: 
	BadWordCheck *myBadWordCheck;
	TextFloodCheck *myTextFloodCheck;
	
	struct ClientWarnInfos { 
		QString nick; 
		int lastWarnType; 
		int warnLevel; 
	};
	
	QMap<unsigned, ClientWarnInfos> clientWarnLevelList;
	
};

#endif // MESSAGEFILTER_H
