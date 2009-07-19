#include <QtCore/QCoreApplication>
#include "cleanerserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
	CleanerServer server;
    return a.exec();
}
