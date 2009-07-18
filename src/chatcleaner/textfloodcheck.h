#ifndef TEXTFLOODCHECK_H
#define TEXTFLOODCHECK_H

#include <QtCore>
#include <third_party/boost/timers.hpp>
#include <stdlib.h>


class TextFloodCheck
{
public:
    TextFloodCheck();
	bool run(unsigned);
	void cleanMsgTimesList();
	
private: 
	QTimer cleanTimer;
	boost::timers::portable::second_timer timer;
	struct TextFloodInfos {
		int floodLevel;
		size_t timeStamp;
	};
	TextFloodInfos myTextFloodInfos;
	QMap<unsigned, TextFloodInfos> msgTimesList;
};

#endif // TEXTFLOODCHECK_H
