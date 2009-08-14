#ifndef CLEANERSERVER_H
#define CLEANERSERVER_H

#include <QtCore>
#include <QtNetwork>
#include <src/net/internalchatcleanerpacket.h>
#include <src/third_party/asn1/ChatCleanerMessage.h>

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
	 
	 unsigned char m_recvBuf[2*MAX_CLEANER_PACKET_SIZE];
	 unsigned m_recvBufUsed;
};

#endif // CLEANERSERVER_H
