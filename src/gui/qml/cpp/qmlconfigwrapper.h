#ifndef QMLCONFIGWRAPPER_H
#define QMLCONFIGWRAPPER_H
#include <QtCore>

class ConfigFile;

class QmlConfigWrapper: public QObject
{
    Q_OBJECT
public:
    QmlConfigWrapper(ConfigFile *c);
    ~QmlConfigWrapper();
    QmlConfigWrapper(const QmlConfigWrapper&);

private:
    ConfigFile *myConfig;
};

#endif // QMLCONFIGWRAPPER_H
