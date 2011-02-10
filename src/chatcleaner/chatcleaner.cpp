#include <QtCore/QCoreApplication>
#include "cleanerserver.h"

#ifdef _WIN32
	#include <process.h>
#else
	#include <unistd.h>
	#ifndef daemon
		int daemon(int, int);
	#endif
#endif

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
	CleanerServer server;
#ifndef _WIN32
	if (daemon(0, 0) != 0)
		return -1;
#endif
    return a.exec();
}
