#ifndef STARTVIEWIMPL_H
#define STARTVIEWIMPL_H
#include <QtCore>

class StartViewImpl : public QObject
{
    Q_OBJECT
public:
    explicit StartViewImpl(QObject *parent = 0);

signals:

public slots:
    Q_INVOKABLE void startLocalGame();

};

#endif // STARTVIEWIMPL_H
