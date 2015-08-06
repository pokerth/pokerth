#include <QtCore>
#include <QQmlApplicationEngine>
#include <QQmlComponent>

//#include "startview/startview.h"

#include "qmlwrapper.h"

qmlWrapper::qmlWrapper()
    :QObject()
{
    qDebug("qmlWrapperKonstruktor");
    myEngine.reset(new QQmlApplicationEngine);
    myEngine->load(QUrl(QStringLiteral("qrc:/main.qml")));

//    QTimer *timer = new QTimer(this);
//    timer->setSingleShot(true);
//    connect(timer, SIGNAL(timeout()), this, SLOT(showStartForm()));
//    timer->start(1000);
}

qmlWrapper::~qmlWrapper()
{
    myEngine->deleteLater();
}

qmlWrapper::qmlWrapper(const qmlWrapper&)
    :QObject()
{

}
