#include <QtCore>
#include "qmlwrapper.h"

#include "views/startview/startview.h"


qmlWrapper::qmlWrapper()
{
    qDebug("qmlWrapperKonstruktor");

    startView myStartView;
    myStartView.show();
}

qmlWrapper::~qmlWrapper()
{

}

