#ifndef ANDROIDAPI8DUMMY_H
#define ANDROIDAPI8DUMMY_H

#include <QObject>
class ConfigFile;

class AndroidApi8Dummy : public QObject
{
    Q_OBJECT
public:
    explicit AndroidApi8Dummy(ConfigFile *c, QObject *parent = 0);
    ~AndroidApi8Dummy();

signals:

public slots:

    void registerSound(const QString&, const QString&){}
    void reallyPlaySound(const QString&){}
    void playSound(const std::string&, int){}
    void initAudio(){}
    void closeAudio(){}
    void reInit(){}

private:
    ConfigFile *myConfig;

};

#endif // ANDROIDAPI8DUMMY
