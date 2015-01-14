#ifndef STARTVIEW_H
#define STARTVIEW_H

class QQmlApplicationEngine;
class startView
{
public:
    startView();
    ~startView();
    startView(const startView&);

    void show();

private:
    QQmlApplicationEngine *myEngine;
};

#endif // STARTVIEW_H
