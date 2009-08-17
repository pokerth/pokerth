#ifndef CLEANERSERVER_H
#define CLEANERSERVER_H

#include <QtCore>
#include <QtNetwork>
#include <net/internalchatcleanerpacket.h>

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
	 bool handleMessage(InternalChatCleanerPacket &msg);
	 void socketStateChanged(QAbstractSocket::SocketState);
	 void refreshConfig();
	 void sendMessageToClient(InternalChatCleanerPacket &msg);

private:
     QTcpServer *tcpServer;
	 QTcpSocket *tcpSocket;
	 QTimer *configRefreshTimer;
	 MessageFilter *myMessageFilter;
		 
	 CleanerConfig *config;
	 bool blockConnection;
	 QString clientSecret;
	 QString serverSecret;
	 
	 unsigned char m_recvBuf[2*MAX_CLEANER_PACKET_SIZE];
	 unsigned m_recvBufUsed;
	 
	 int secondsSinceLastConfigChange;
};

#endif // CLEANERSERVER_H
