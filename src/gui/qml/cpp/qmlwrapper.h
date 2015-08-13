#ifndef QMLWRAPPER_H
#define QMLWRAPPER_H
#include <QtCore>
#include <boost/shared_ptr.hpp>

class ConfigFile;
class QmlConfig;
class QQmlApplicationEngine;

class QmlWrapper: public QObject
{
    Q_OBJECT
public:
    QmlWrapper(boost::shared_ptr<ConfigFile>);
    ~QmlWrapper();
    QmlWrapper(const QmlWrapper&);

public slots:

private:
    boost::shared_ptr<QQmlApplicationEngine> myEngine;
    boost::shared_ptr<ConfigFile> myConfig;
    QmlConfig *myQmlConfig;
};

#endif // QMLWRAPPER_H
