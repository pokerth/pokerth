#include "cleanerserver.h"

#include <QtNetwork>
#include <QtCore>
#include <stdlib.h>

#include "messagefilter.h"

CleanerServer::CleanerServer()
{
	 myMessageFilter = new MessageFilter;
	 tcpServer = new QTcpServer();
     if (!tcpServer->listen(QHostAddress("127.0.0.1"), 7777) ) {
         qDebug() << QString("Unable to start the server: %1.").arg(tcpServer->errorString());
         return;
     }

	 qDebug() << QString("The server is running on port %1.").arg(tcpServer->serverPort());
	 
     connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newCon()));
}

CleanerServer::~CleanerServer() 
{
}

void CleanerServer::newCon()
 {
	tcpSocket = tcpServer->nextPendingConnection();
	connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(onRead())); 	
 }

void CleanerServer::onRead()
{
	char buf[20000];
    tcpSocket->readLine(buf, sizeof(buf));
	QString message = QString::fromUtf8("%1").arg(buf);
	QString checkMessage = myMessageFilter->check(message);
	tcpSocket->write(checkMessage.toAscii().data(), checkMessage.length());
}
