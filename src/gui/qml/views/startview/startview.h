#ifndef STARTVIEW_H
#define STARTVIEW_H

#include <boost/shared_ptr.hpp>

class QQmlApplicationEngine;

class startView
{
public:
    startView();
    ~startView();
    startView(const startView&);

    void show();

private:
    boost::shared_ptr<QQmlApplicationEngine> myEngine;
};

#endif // STARTVIEW_H
