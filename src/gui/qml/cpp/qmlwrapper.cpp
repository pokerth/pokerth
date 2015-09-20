#include <QtCore>
#include <QtQml>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <configfile.h>

#include "qmlconfig.h"
#include "qmlwrapper.h"
#include "startviewimpl.h"
#include "createlocalgameviewimpl.h"

QmlWrapper::QmlWrapper(boost::shared_ptr<ConfigFile> c)
    :QObject(), myConfig(c)
{
    myQmlEngine = new QQmlApplicationEngine;
    myQmlConfig = new QmlConfig(myConfig);

    //TODO create Session and Log here

    myStartViewImpl = new StartViewImpl(this);
    myCreateLocalGameViewImpl = new CreateLocalGameViewImpl(this, myQmlEngine, myConfig);

    //Add c++ content to QML here
    myQmlEngine->rootContext()->setContextProperty("Config", myQmlConfig);
    myQmlEngine->rootContext()->setContextProperty("StartViewImpl", myStartViewImpl);
    myQmlEngine->rootContext()->setContextProperty("CreateLocalGameViewImpl", myCreateLocalGameViewImpl);

    myQmlEngine->load(QUrl(QStringLiteral("qrc:/main.qml")));
}

QmlWrapper::~QmlWrapper()
{
    myQmlEngine->deleteLater();
}

QmlWrapper::QmlWrapper(const QmlWrapper&)
    :QObject()
{

}


