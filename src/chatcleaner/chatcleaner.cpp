#include <QtCore/QCoreApplication>
#include "cleanerserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
	CleanerServer server;
	qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));
    return a.exec();
}
