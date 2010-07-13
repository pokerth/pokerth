
#include "messagefilter.h"

#include <QtCore>
#include "badwordcheck.h"
#include "textfloodcheck.h"
#include "cleanerconfig.h"
#include "capsfloodcheck.h"
#include "letterrepeatingcheck.h"
#include "urlcheck.h"

enum ActionType {
    NOTHING,
    WARN,
    KICK,
    KICKBAN
};

enum OffenceType {
    NONE,
    BAD_WORD,
    TEXT_FLOOD_LINES,
    CAPS_FLOOD,
    LETTER_REPEATING,
    URL
};

MessageFilter::MessageFilter(CleanerConfig *c): config(c)
{
    myBadWordCheck = new BadWordCheck;
    myTextFloodCheck = new TextFloodCheck;
    myCapsFloodCheck = new CapsFloodCheck;
    myLetterRepeatingCheck = new LetterRepeatingCheck;
    myUrlCheck = new UrlCheck;

    cleanTimer = new QTimer();
    connect(cleanTimer, SIGNAL(timeout()), this, SLOT(cleanKickCounterList()));
    cleanTimer->start(30000);
}

MessageFilter::~MessageFilter() {

    delete myBadWordCheck;
    delete myTextFloodCheck;
    delete myCapsFloodCheck;
    delete myLetterRepeatingCheck;
    delete myUrlCheck;
    delete cleanTimer;
}

QStringList MessageFilter::check(unsigned playerId, QString nick, QString msg)
{
    QStringList returnList;
    QString returnMessage;
    QString returnAction;

    OffenceType offence = NONE;
    ActionType action = NOTHING;

    if(myBadWordCheck->run(msg)) offence = BAD_WORD;
    if(myCapsFloodCheck->run(msg)) offence = CAPS_FLOOD;
    if(myLetterRepeatingCheck->run(msg)) offence = LETTER_REPEATING;
    if(myUrlCheck->run(msg)) offence = URL;
    if(myTextFloodCheck->run(playerId)) offence = TEXT_FLOOD_LINES;

    if(offence){
	
        QMap<unsigned, ClientWarnInfos>::const_iterator i = myClientWarnLevelList.find(playerId);

        if(i == myClientWarnLevelList.end()) {
            ClientWarnInfos tmpInfos;
            tmpInfos.warnLevel = 1;
            tmpInfos.lastWarnType = offence;
            tmpInfos.nick = nick;
            myClientWarnLevelList.insert(playerId, tmpInfos);
            action = WARN;
        }
        else {
            if(i.value().warnLevel == warnLevelToKick || i.value().lastWarnType == offence) {
                //				Kick Command
                action = KICK;
                //remove playerId from all lists and as LAST from myClientWarnLevelList
                myTextFloodCheck->removeNickFromList(i.key());
                myClientWarnLevelList.remove(i.key());
                //check if player is already on kickCounterList
                QMap<QString, ClientKickInfos>::const_iterator j = myClientKickCounterList.find(nick);
                if(j == myClientKickCounterList.end()) {
                    //if player is NOT on this list put the playerId on it to ban after multiple offence
                    ClientKickInfos tmpInfos;
                    tmpInfos.kickNumber = 1;
                    tmpInfos.lastKickTimestamp = timer.elapsed().total_seconds();
                    myClientKickCounterList.insert(nick, tmpInfos);
                }
                else {
                    //pleayer is already on the list: either raise kickNumber or kickban when kickNumerToBan is reached
                    if(j.value().kickNumber == kickNumberToBan) {
                        action = KICKBAN;
                        //remove player from kickCounterList
                        myClientKickCounterList.remove(j.key());
                    }
                    else {
                        ClientKickInfos tmpInfos;
                        tmpInfos.kickNumber = j.value().kickNumber+1;
                        tmpInfos.lastKickTimestamp = timer.elapsed().total_seconds();
                        myClientKickCounterList.insert(nick, tmpInfos);
                    }
                }
            }
            else {
                ClientWarnInfos tmpInfos;
                tmpInfos.warnLevel = i.value().warnLevel+1;
                tmpInfos.lastWarnType = offence;
                tmpInfos.nick = nick;
                myClientWarnLevelList.insert(playerId, tmpInfos);
                action = WARN;
            }
        }

        if(action == WARN) {

            switch(offence) {
            case BAD_WORD: {
                    returnMessage = QString ("%1: Warning! No racial, religious, or sexually inflammatory language!\n").arg(nick);
                }
                break;
            case TEXT_FLOOD_LINES: {
                    returnMessage = QString ("%1: Warning! You've triggered text flood (lines) protection, slow down your typing!\n").arg(nick);
                }
                break;
            case CAPS_FLOOD: {
                    returnMessage = QString ("%1: Warning: You've triggered caps flood protection, release your caps!\n").arg(nick);
                }
                break;
            case LETTER_REPEATING: {
                    returnMessage = QString ("%1: Warning: You've triggered letter repeating protection, stop repeating!\n").arg(nick);
                }
                break;
            case URL: {
                    returnMessage = QString ("%1: Warning: You've triggered url spam protection, stop posting urls!\n").arg(nick);
                }
                break;
            default:;
            }
            returnAction = QString("warn");
        }
        else if(action == KICK) {
            returnMessage = QString("%1 kicked! Please respect: http://chatrules.pokerth.net\n").arg(nick);
            returnAction = QString("kick");
        }
        else if(action == KICKBAN) {
            returnMessage = QString("%1 kicked and banned! Please respect: http://chatrules.pokerth.net\n").arg(nick);
            returnAction = QString("kickban");
        }
    }
    else {
        returnAction = QString("");
        returnMessage = QString("");
    }

    returnList << returnAction << returnMessage;
    return returnList;
}

void MessageFilter::refreshConfig() {

    //	global settings
    warnLevelToKick = config->readConfigInt("WarnLevelToKick");
    kickNumberToBan = config->readConfigInt("KickNumberToBan");

    // special check settings
    //Bad Words
    std::list<std::string> badWordsList = config->readConfigStringList("BadWordsList");
    std::list<std::string>::iterator it1;
    QStringList bwList;
    for(it1= badWordsList.begin(); it1 != badWordsList.end(); it1++) {
        bwList << QString::fromUtf8(it1->c_str());
    }
    myBadWordCheck->setBadWords(bwList);

    std::list<std::string> urlStringsList = config->readConfigStringList("UrlStringsList");
    std::list<std::string>::iterator it2;
    QStringList urlList;
    for(it2= urlStringsList.begin(); it2 != urlStringsList.end(); it2++) {
        urlList << QString::fromUtf8(it2->c_str());
    }
    myUrlCheck->setUrlStrings(urlList);

    std::list<std::string> urlExceptionStringsList = config->readConfigStringList("UrlExceptionStringsList");
    std::list<std::string>::iterator it3;
    QStringList urlExceptionList;
    for(it3= urlExceptionStringsList.begin(); it3 != urlExceptionStringsList.end(); it3++) {
        urlExceptionList << QString::fromUtf8(it3->c_str());
    }
    myUrlCheck->setUrlExceptionStrings(urlExceptionList);

    myTextFloodCheck->setTextFloodLevelToTrigger(config->readConfigInt("TextFloodLevelToTrigger"));
    myCapsFloodCheck->setCapsNumberToTrigger(config->readConfigInt("CapsFloodCapsNumberToTrigger"));
    myLetterRepeatingCheck->setLetterNumberToTrigger(config->readConfigInt("LetterRepeatingNumberToTrigger"));

}

void MessageFilter::cleanKickCounterList()
{
    QMapIterator<QString, ClientKickInfos> it(myClientKickCounterList);
    while (it.hasNext()) {
        it.next();
        if(timer.elapsed().total_seconds()-it.value().lastKickTimestamp > config->readConfigInt("SecondsToForgetAboutKick")) {
            qDebug() << it.key() << "removed from kick counter list" << endl;
            myClientKickCounterList.remove(it.key());
        }
    }
}
