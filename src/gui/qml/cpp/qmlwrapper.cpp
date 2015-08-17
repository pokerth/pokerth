#include <QtCore>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <configfile.h>
#include <boost/shared_ptr.hpp>

#include "qmlconfig.h"
#include "qmlwrapper.h"
#include "startviewimpl.h"
#include "createlocalgameviewimpl.h"

QmlWrapper::QmlWrapper(boost::shared_ptr<ConfigFile> c)
    :QObject(), myConfig(c)
{
    myEngine.reset(new QQmlApplicationEngine);
    myQmlConfig = new QmlConfig(myConfig); //need to be a default pointer because QML doesnt support shared_ptr()

    //TODO create Session and Log here

    myStartViewImpl = new StartViewImpl(this);
    myCreateLocalGameViewImpl = new CreateLocalGameViewImpl(this);

    //Add c++ content to QML here
    myEngine->rootContext()->setContextProperty("Config", myQmlConfig);
    myEngine->rootContext()->setContextProperty("StartViewImpl", myStartViewImpl);

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


