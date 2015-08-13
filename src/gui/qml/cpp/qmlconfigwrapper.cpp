#include "qmlconfigwrapper.h"
#include "configfile.h"

QmlConfigWrapper::QmlConfigWrapper(ConfigFile *c)
    :QObject(), myConfig(c)
{

}

QmlConfigWrapper::~QmlConfigWrapper()
{

}

QmlConfigWrapper::QmlConfigWrapper(const QmlConfigWrapper&)
    :QObject()
{

}
