#include "cleanerserver.h"

#include <QtNetwork>
#include <QtCore>
#include <stdlib.h>

#include "messagefilter.h"
#include "cleanerconfig.h"

CleanerServer::CleanerServer(): config(0)
{
	config = new CleanerConfig;
	
	myMessageFilter = new MessageFilter(config);
	tcpServer = new QTcpServer();
	tcpServer->setMaxPendingConnections(1);
	
	if (!tcpServer->listen(QHostAddress(QString::fromUtf8(config->readConfigString("HostAddress").c_str())), config->readConfigInt("DefaultListenPort")) ) {
	 qDebug() << QString("Unable to start the server: %1.").arg(tcpServer->errorString());
	 return;
	}
	
	qDebug() << QString("The server is running on port %1.").arg(tcpServer->serverPort());

	connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newCon()));
	
	refreshConfig();
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
	char buf[1024];
    tcpSocket->readLine(buf, sizeof(buf));
	QString message = QString::fromUtf8("%1").arg(buf);
	
	// TESTING DEFAULT VALUES
	QString nick = "PlayerNick";
	unsigned playerId = 1;
	// TESTING DEFAULT VALUES
	
	QString checkMessage = myMessageFilter->check(playerId, nick, message);
	tcpSocket->write(checkMessage.toAscii().data(), checkMessage.length());
}

void CleanerServer::refreshConfig() {

//	config->fillBuffer();
	myMessageFilter->refreshConfig();
}
