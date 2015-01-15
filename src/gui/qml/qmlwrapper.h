#ifndef QMLWRAPPER_H
#define QMLWRAPPER_H

#include "startview/startview.h"

class qmlWrapper
{
public:
    qmlWrapper();
    ~qmlWrapper();
    qmlWrapper(const qmlWrapper&);

    void showStartView();

private:
    startView myStartView;
};

#endif // QMLWRAPPER_H
