#ifndef MESSAGEFILTER_H
#define MESSAGEFILTER_H

#include <QtCore>
#include <stdlib.h>

class BadWordCheck;
class TextFloodCheck;
class CleanerConfig;
class CapsFloodCheck;
class LetterRepeatingCheck;
class UrlCheck;

class MessageFilter: public QObject {
Q_OBJECT
public:
    MessageFilter(CleanerConfig*);
	~MessageFilter();
	
	QString check(unsigned, QString, QString);
	void refreshConfig();	
	
private: 
	BadWordCheck *myBadWordCheck;
	TextFloodCheck *myTextFloodCheck;
	CapsFloodCheck *myCapsFloodCheck;
	LetterRepeatingCheck *myLetterRepeatingCheck;
	UrlCheck *myUrlCheck;
	
	struct ClientWarnInfos { 
		QString nick; 
		int lastWarnType; 
		int warnLevel; 
	};
	
	QMap<unsigned, ClientWarnInfos> myClientWarnLevelList;
	
	int warnLevelToKick;
	
	CleanerConfig *config;
};

#endif // MESSAGEFILTER_H
