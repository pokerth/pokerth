#ifndef QMLWRAPPER_H
#define QMLWRAPPER_H

#include <boost/shared_ptr.hpp>

class startView;

class qmlWrapper
{
public:
    qmlWrapper();
    ~qmlWrapper();
    qmlWrapper(const qmlWrapper&);

    void showStartView();

private:
    boost::shared_ptr<startView> myStartView;
};

#endif // QMLWRAPPER_H
