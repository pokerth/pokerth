#ifndef CLEANERSERVER_H
#define CLEANERSERVER_H

#include <QtCore>
#include <QtNetwork>

class MessageFilter;

class CleanerServer: public QObject {
Q_OBJECT
	
public:
    CleanerServer();
	~CleanerServer();
	
private slots:
     void newCon();
	 void onRead();

private:
     QTcpServer *tcpServer;
	 QTcpSocket *tcpSocket;
	 MessageFilter *myMessageFilter;
		 
};

#endif // CLEANERSERVER_H
