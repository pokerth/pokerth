#ifndef QMLWRAPPER_H
#define QMLWRAPPER_H
#include <QtCore>
#include <boost/shared_ptr.hpp>

class QQmlApplicationEngine;

class qmlWrapper: public QObject
{
    Q_OBJECT
public:
    qmlWrapper();
    ~qmlWrapper();
    qmlWrapper(const qmlWrapper&);

public slots:

private:
    boost::shared_ptr<QQmlApplicationEngine> myEngine;
};

#endif // QMLWRAPPER_H
