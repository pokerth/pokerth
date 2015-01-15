#include <QQmlApplicationEngine>

#include "startview.h"

startView::startView()
{
    qDebug("startViewKonstruktor");
    myEngine.reset(new QQmlApplicationEngine);
}

startView::~startView()
{

}

startView::startView(const startView&)
{

}

void startView::show()
{
    qDebug("show() in startView");
    myEngine->load(QUrl(QStringLiteral("qrc:/main.qml")));
}
