#ifndef TEXTFLOODCHECK_H
#define TEXTFLOODCHECK_H

#include <QtCore>
#include <third_party/boost/timers.hpp>
#include <stdlib.h>


class TextFloodCheck: public QObject
{
	Q_OBJECT
public:
    TextFloodCheck();
	~TextFloodCheck();
	
	void setTextFloodLevelToTrigger(int level) { textFloodLevelToTrigger = level; }
	
	bool run(unsigned);
	
public slots:
	void cleanMsgTimesList();
	void removeNickFromList(unsigned);
	
private: 
	QTimer *cleanTimer;
	boost::timers::portable::second_timer timer;
	struct TextFloodInfos {
		int floodLevel;
		size_t timeStamp;
	};
	QMap<unsigned, TextFloodInfos> msgTimesList;
	
	int textFloodLevelToTrigger;
};

#endif // TEXTFLOODCHECK_H
