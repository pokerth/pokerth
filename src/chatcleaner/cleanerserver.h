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
	 void socketStateChanged(QAbstractSocket::SocketState);
	 void refreshConfig();

private:
     QTcpServer *tcpServer;
	 QTcpSocket *tcpSocket;
	 QTimer *configRefreshTimer;
	 MessageFilter *myMessageFilter;
		 
	 CleanerConfig *config;
	 bool blockConnection;
};

#endif // CLEANERSERVER_H
