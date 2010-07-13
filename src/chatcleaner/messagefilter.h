#ifndef MESSAGEFILTER_H
#define MESSAGEFILTER_H

#include <QtCore>
#include <stdlib.h>
#include <third_party/boost/timers.hpp>

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

    QStringList check(unsigned, QString, QString);
    void refreshConfig();

public slots:
    void cleanKickCounterList();

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

    struct ClientKickInfos {
        size_t lastKickTimestamp;
        int kickNumber;
    };

    QMap<unsigned, ClientWarnInfos> myClientWarnLevelList;
    QMap<QString, ClientKickInfos> myClientKickCounterList;

    int warnLevelToKick;
    int kickNumberToBan;

    CleanerConfig *config;

    boost::timers::portable::second_timer timer;
    QTimer *cleanTimer;
};

#endif // MESSAGEFILTER_H
