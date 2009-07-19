#ifndef CLEANERSERVER_H
#define CLEANERSERVER_H

#include <QtCore>
#include <QtNetwork>

class MessageFilter;
class CleanerConfig;

class CleanerServer: public QObject {
Q_OBJECT
	
public:
    CleanerServer();
	~CleanerServer();
	
private slots:
     void newCon();
	 void onRead();
	 
	 void refreshConfig();

private:
     QTcpServer *tcpServer;
	 QTcpSocket *tcpSocket;
	 QTimer *configRefreshTimer;
	 MessageFilter *myMessageFilter;
		 
	 CleanerConfig *config;
};

#endif // CLEANERSERVER_H
