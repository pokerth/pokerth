#include "qmlconfig.h"
#include "configfile.h"

QmlConfig::QmlConfig(boost::shared_ptr<ConfigFile> c)
    :QObject(), myConfig(c)
{

}

QmlConfig::~QmlConfig() {}
QmlConfig::QmlConfig(const QmlConfig&) :QObject() {}

QString QmlConfig::readConfigString(QString varName) {
    return QString::fromUtf8(myConfig->readConfigString(varName.toStdString()).c_str());
}

QStringList QmlConfig::readConfigStringList(QString listName)
{
    std::list<std::string> myList = myConfig->readConfigStringList(listName.toStdString());
    std::list<std::string>::iterator it1;
    QStringList returnList;

    for(it1= myList.begin(); it1 != myList.end(); ++it1) {
        returnList.append(QString::fromStdString(*it1));
    }
    return returnList;
}

QString QmlConfig::readConfigIntString(QString varName) {
    return QString::number(myConfig->readConfigInt(varName.toStdString()));
}

QStringList QmlConfig::readConfigIntStringList(QString listName)
{
    std::list<int> myList = myConfig->readConfigIntList(listName.toStdString());
    std::list<int>::iterator it1;
    QStringList returnList;

    for(it1= myList.begin(); it1 != myList.end(); ++it1) {
        returnList.append(QString::number(*it1,10));
    }
    return returnList;
}
