#ifndef QMLWRAPPER_H
#define QMLWRAPPER_H
#include <QtCore>
#include "boost/shared_ptr.hpp"

class QQmlApplicationEngine;
class ConfigFile;
class QmlConfig;
class CreateLocalGameViewImpl;
class StartViewImpl;

class QmlWrapper: public QObject
{
    Q_OBJECT
public:
    QmlWrapper(boost::shared_ptr<ConfigFile>);
    ~QmlWrapper();
    QmlWrapper(const QmlWrapper&);

public slots:

private:
    QQmlApplicationEngine *myQmlEngine;
    boost::shared_ptr<ConfigFile> myConfig;
    QmlConfig *myQmlConfig;
    CreateLocalGameViewImpl *myCreateLocalGameViewImpl;
    StartViewImpl *myStartViewImpl;
};

#endif // QMLWRAPPER_H
