#include <QQmlApplicationEngine>
#include "startview.h"

startView::startView()
{
    qDebug("startViewKonstruktor");
    myEngine = new QQmlApplicationEngine();
}

startView::~startView()
{

}

void startView::show()
{
    qDebug("show mal bitte");
    myEngine->load(QUrl(QStringLiteral("qrc:/main.qml")));
}
