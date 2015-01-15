#include <QtCore>
#include "startview/startview.h"

#include "qmlwrapper.h"

qmlWrapper::qmlWrapper()
{
    qDebug("qmlWrapperKonstruktor");
    myStartView.reset(new startView);
}

qmlWrapper::~qmlWrapper()
{
}

qmlWrapper::qmlWrapper(const qmlWrapper&)
{
}

void qmlWrapper::showStartView()
{
    qDebug("showStartView() in qmlWrapper");
    myStartView->show();
}
