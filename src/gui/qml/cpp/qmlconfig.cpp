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

QString QmlConfig::readConfigInt(QString varName) {
    return QString::number(myConfig->readConfigInt(varName.toStdString()));
}
