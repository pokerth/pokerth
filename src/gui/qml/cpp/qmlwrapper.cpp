#include <QtCore>
#include <QQmlApplicationEngine>
#include <configfile.h>
#include <boost/shared_ptr.hpp>

#include "qmlwrapper.h"

QmlWrapper::QmlWrapper(boost::shared_ptr<ConfigFile> c)
    :QObject(), myConfig(c)
{
    qDebug("QmlWrapperKonstruktor");
    myEngine.reset(new QQmlApplicationEngine);

//    Add c++ content here
//    myEngine.rootContext()->setContextProperty("model1", &model1);

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
