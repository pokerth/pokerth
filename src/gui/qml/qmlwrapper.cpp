#include <QtCore>
#include "qmlwrapper.h"

#include "views/startview/startview.h"


qmlWrapper::qmlWrapper()
{
    qDebug("qmlWrapperKonstruktor");
    myStartView = new startView();
}

qmlWrapper::~qmlWrapper()
{

}

void qmlWrapper::showStartView()
{
    myStartView->show();
}
