#include <QtCore>
#include "qmlwrapper.h"

qmlWrapper::qmlWrapper()
{
    qDebug("qmlWrapperKonstruktor");
}

qmlWrapper::~qmlWrapper()
{
}

qmlWrapper::qmlWrapper(const qmlWrapper&)
{
}

void qmlWrapper::showStartView()
{
    myStartView.show();
}
