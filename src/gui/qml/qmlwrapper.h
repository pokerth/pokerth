#ifndef QMLWRAPPER_H
#define QMLWRAPPER_H

class startView;

class qmlWrapper
{
public:
    qmlWrapper();
    ~qmlWrapper();

    void showStartView();

private:
    startView *myStartView;
};

#endif // QMLWRAPPER_H
