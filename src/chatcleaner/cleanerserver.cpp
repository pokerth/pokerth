#include "cleanerserver.h"

#include <QtNetwork>
#include <QtCore>
#include <cstdlib>
#include <string>
#include <third_party/asn1/ChatCleanerMessage.h>

#include "messagefilter.h"
#include "cleanerconfig.h"

using namespace std;

CleanerServer::CleanerServer(): config(0), blockConnection(false), m_recvBufUsed(0), secondsSinceLastConfigChange(0)
{
	config = new CleanerConfig;

	clientSecret = QString::fromUtf8(config->readConfigString("ClientAuthString").c_str());
	serverSecret = QString::fromUtf8(config->readConfigString("ServerAuthString").c_str());
	
	myMessageFilter = new MessageFilter(config);
	tcpServer = new QTcpServer();
	tcpServer->setMaxPendingConnections(1);
	
	if (!tcpServer->listen(QHostAddress(QString::fromUtf8(config->readConfigString("HostAddress").c_str())), config->readConfigInt("DefaultListenPort")) ) {
	 qDebug() << QString("Unable to start the server: %1.").arg(tcpServer->errorString());
	 return;
	}
	qDebug() << QString("The server is running on port %1.").arg(tcpServer->serverPort());

	configRefreshTimer = new QTimer();
	
	connect(configRefreshTimer, SIGNAL(timeout()), this, SLOT(refreshConfig()));
	connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newCon()));
	
	refreshConfig();
	configRefreshTimer->start(10000);
}

CleanerServer::~CleanerServer() 
{
	delete config;
	delete myMessageFilter;
	delete tcpServer;
	delete configRefreshTimer;
}

void CleanerServer::newCon()
 {  
	if(!blockConnection) {
		tcpSocket = tcpServer->nextPendingConnection();
		connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(onRead())); 	
		connect(tcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
		blockConnection = true;
    }
 }

void CleanerServer::onRead()
{
	qint64 bytesRead = tcpSocket->read((char *)m_recvBuf + m_recvBufUsed, sizeof(m_recvBuf) - m_recvBufUsed);
	bool error = bytesRead < 1;
	if (!error)
	{
		m_recvBufUsed += bytesRead;

		asn_dec_rval_t retVal;
		do
		{
			// Try to decode the packets.
			InternalChatCleanerPacket recvMsg;
			retVal = ber_decode(0, &asn_DEF_ChatCleanerMessage, (void **)recvMsg.GetMsgPtr(), m_recvBuf, m_recvBufUsed);
			if(retVal.code == RC_OK)
			{
				if (retVal.consumed < m_recvBufUsed)
				{
					m_recvBufUsed -= retVal.consumed;
					memmove(m_recvBuf, m_recvBuf + retVal.consumed, m_recvBufUsed);
				}
				else
					m_recvBufUsed = 0;

				// Handle the packets.
				error = handleMessage(recvMsg);
			}
		} while (!error && retVal.code == RC_OK);
	}

	if (error)
	{
		qDebug() << "Error handling packets from client.";
		tcpSocket->close();
	}

/*	char buf[1024];
    tcpSocket->readLine(buf, sizeof(buf));
	QString message = QString::fromUtf8("%1").arg(buf);
	
	// TESTING DEFAULT VALUES
	QString nick = "PlayerNick";
	unsigned playerId = 1;
	// TESTING DEFAULT VALUES
	
	QString checkMessage = myMessageFilter->check(playerId, nick, message);
	tcpSocket->write(checkMessage.toAscii().data(), checkMessage.length());*/
}

bool CleanerServer::handleMessage(InternalChatCleanerPacket &msg) {
	bool error = true;
	if (msg.GetMsg()->present == ChatCleanerMessage_PR_cleanerInitMessage)
	{
		CleanerInitMessage_t *netInit = &msg.GetMsg()->choice.cleanerInitMessage;
		if (netInit->requestedVersion == CLEANER_PROTOCOL_VERSION)
		{
			string tmpClientSecret((const char *)netInit->clientSecret.buf, netInit->clientSecret.size);
			if (clientSecret == QString::fromStdString(tmpClientSecret))
			{
				error = false;

				InternalChatCleanerPacket tmpAck;
				tmpAck.GetMsg()->present = ChatCleanerMessage_PR_cleanerInitAckMessage;
				CleanerInitAckMessage_t *netAck = &tmpAck.GetMsg()->choice.cleanerInitAckMessage;
				netAck->serverVersion = CLEANER_PROTOCOL_VERSION;
				string tmpServerSecret(serverSecret.toStdString());
				OCTET_STRING_fromBuf(&netAck->serverSecret,
									 tmpServerSecret.c_str(),
									 tmpServerSecret.length());
				sendMessageToClient(tmpAck);
			}
			else
				qDebug() << "Invalid client secret.";
		}
		else
			qDebug() << "Invalid client version: " << netInit->requestedVersion;
	}
	else if (msg.GetMsg()->present == ChatCleanerMessage_PR_cleanerChatRequestMessage)
	{
		error = false;
		CleanerChatRequestMessage_t *netRequest = &msg.GetMsg()->choice.cleanerChatRequestMessage;
		unsigned playerId = netRequest->playerId;
		QString nick(QString::fromUtf8(
				string((const char *)netRequest->playerName.buf, netRequest->playerName.size).c_str()));
		QString message(QString::fromUtf8(
				string((const char *)netRequest->chatMessage.buf, netRequest->chatMessage.size).c_str()));
		QString checkMessage = myMessageFilter->check(playerId, nick, message);

		if (!checkMessage.isEmpty())
		{
			InternalChatCleanerPacket tmpReply;
			tmpReply.GetMsg()->present = ChatCleanerMessage_PR_cleanerChatReplyMessage;
			CleanerChatReplyMessage_t *netReply = &tmpReply.GetMsg()->choice.cleanerChatReplyMessage;
			netReply->requestId = netRequest->requestId;
			netReply->playerId = netRequest->playerId;
			netReply->cleanerActionType = cleanerActionType_cleanerActionNone;
	/*	cleanerActionType_cleanerActionNone
		cleanerActionType_cleanerActionWarning
		cleanerActionType_cleanerActionKick
		cleanerActionType_cleanerActionBan */
			string tmpCheck(checkMessage.toUtf8());
			netReply->cleanerText =
				OCTET_STRING_new_fromBuf(
					&asn_DEF_OCTET_STRING,
					(const char *)tmpCheck.c_str(),
					tmpCheck.length());
			sendMessageToClient(tmpReply);
		}
	}
	return error;
}

void CleanerServer::socketStateChanged(QAbstractSocket::SocketState state) {

	qDebug() << "Socket state changed to: " << QAbstractSocket::UnconnectedState;
	if(state == QAbstractSocket::UnconnectedState) blockConnection = false;
}

void CleanerServer::refreshConfig() {
	
	QFileInfo configFileInfo(QString::fromUtf8(config->getConfigFileName().c_str()));
	
	if(configFileInfo.lastModified().secsTo(QDateTime::currentDateTime()) < 20) {
		config->fillBuffer();
	}
	
	myMessageFilter->refreshConfig();
}

void CleanerServer::sendMessageToClient(InternalChatCleanerPacket &msg)
{
	unsigned char buf[MAX_CLEANER_PACKET_SIZE];
	asn_enc_rval_t e = der_encode_to_buffer(&asn_DEF_ChatCleanerMessage, msg.GetMsg(), buf, MAX_CLEANER_PACKET_SIZE);

	if (e.encoded == -1)
		qDebug() << "Failed to encode chat cleaner packet: " << msg.GetMsg()->present;
	else
		tcpSocket->write((const char *)buf, e.encoded);
}
