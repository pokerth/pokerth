#include <QtCore>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <configfile.h>
#include <boost/shared_ptr.hpp>

#include "qmlconfig.h"
#include "qmlwrapper.h"

QmlWrapper::QmlWrapper(boost::shared_ptr<ConfigFile> c)
    :QObject(), myConfig(c)
{
    myEngine.reset(new QQmlApplicationEngine);
    myQmlConfig = new QmlConfig(myConfig); //need to be a default pointer because QML doesnt support shared_ptr()

//    Add c++ content here
    myEngine->rootContext()->setContextProperty("Config", myQmlConfig);

    myEngine->load(QUrl(QStringLiteral("qrc:/main.qml")));
}

QmlWrapper::~QmlWrapper()
{
    myEngine->deleteLater();
}

QmlWrapper::QmlWrapper(const QmlWrapper&)
    :QObject()
{

}
