/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#include "cleanerserver.h"

#include <QtNetwork>
#include <QtCore>
#include <QtEndian>
#include <cstdlib>
#include <string>
#include <third_party/protobuf/chatcleaner.pb.h>

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
#if QT_VERSION >= 0x050100
		tcpServer->pauseAccepting();
#endif
	}
}

void CleanerServer::onRead()
{
	qint64 bytesRead = tcpSocket->read((char *)m_recvBuf + m_recvBufUsed, sizeof(m_recvBuf) - m_recvBufUsed);
	bool error = bytesRead < 1;
	if (!error) {
		m_recvBufUsed += bytesRead;
		bool valid;
		do {
			valid = false;
			if (m_recvBufUsed >= CLEANER_NET_HEADER_SIZE) {
				// Read the size of the packet (first 4 bytes in network byte order).
				uint32_t nativeVal;
				memcpy(&nativeVal, &m_recvBuf[0], sizeof(uint32_t));
				size_t packetSize = qFromBigEndian(nativeVal);
				if (packetSize > MAX_CLEANER_PACKET_SIZE) {
					m_recvBufUsed = 0;
					qDebug() << "Invalid packet size: " << packetSize;
				} else if (m_recvBufUsed >= packetSize + CLEANER_NET_HEADER_SIZE) {
					try {
						// Try to decode the packet.
						boost::shared_ptr<ChatCleanerMessage> recvMsg(ChatCleanerMessage::default_instance().New());
						if (recvMsg->ParseFromArray(&m_recvBuf[CLEANER_NET_HEADER_SIZE], static_cast<int>(packetSize))) {
							m_recvBufUsed -= (packetSize + CLEANER_NET_HEADER_SIZE);
							if (m_recvBufUsed) {
								memmove(m_recvBuf, m_recvBuf + packetSize + CLEANER_NET_HEADER_SIZE, m_recvBufUsed);
							}
						}
						// Handle the packet.
						error = handleMessage(*recvMsg);
						valid = true;
					} catch (const exception &e) {
						// Reset buffer on error.
						m_recvBufUsed = 0;
						qDebug() << "Exception while decoding packet: " << e.what();
					}
				}
			}
		} while (valid && !error);
	}

	if (error) {
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

bool CleanerServer::handleMessage(ChatCleanerMessage &msg)
{
	bool error = true;
	if (msg.messagetype() == ChatCleanerMessage::Type_CleanerInitMessage) {
		const CleanerInitMessage &netInit = msg.cleanerinitmessage();
		if (netInit.requestedversion() == CLEANER_PROTOCOL_VERSION) {
			if (clientSecret == QString::fromStdString(netInit.clientsecret())) {
				error = false;

				boost::shared_ptr<ChatCleanerMessage> tmpAck(ChatCleanerMessage::default_instance().New());
				tmpAck->set_messagetype(ChatCleanerMessage::Type_CleanerInitAckMessage);
				CleanerInitAckMessage *netAck = tmpAck->mutable_cleanerinitackmessage();
				netAck->set_serverversion(CLEANER_PROTOCOL_VERSION);
				netAck->set_serversecret(serverSecret.toStdString());
				sendMessageToClient(*tmpAck);
			} else
				qDebug() << "Invalid client secret.";
		} else
			qDebug() << "Invalid client version: " << netInit.requestedversion();
	} else if (msg.messagetype() == ChatCleanerMessage::Type_CleanerChatRequestMessage) {
		error = false;
		const CleanerChatRequestMessage &netRequest = msg.cleanerchatrequestmessage();
		unsigned playerId = netRequest.playerid();
		QString nick(QString::fromUtf8(netRequest.playername().c_str()));
		QString message(QString::fromUtf8(netRequest.chatmessage().c_str()));
		unsigned gameId = netRequest.gameid();

		QStringList checkreturn = myMessageFilter->check(gameId, playerId, nick, message);
		QString checkAction = checkreturn.at(0);
		QString checkMessage = checkreturn.at(1);

		if (!checkAction.isEmpty()) {
			boost::shared_ptr<ChatCleanerMessage> tmpReply(ChatCleanerMessage::default_instance().New());
			tmpReply->set_messagetype(ChatCleanerMessage::Type_CleanerChatReplyMessage);
			CleanerChatReplyMessage *netReply = tmpReply->mutable_cleanerchatreplymessage();
			netReply->set_requestid(netRequest.requestid());
			netReply->set_gameid(netRequest.gameid());
			netReply->set_cleanerchattype(netRequest.cleanerchattype());
			netReply->set_playerid(netRequest.playerid());

			if(checkAction == "warn") {
				netReply->set_cleaneractiontype(CleanerChatReplyMessage_CleanerActionType_cleanerActionWarning);
			} else if (checkAction == "kick") {
				netReply->set_cleaneractiontype(CleanerChatReplyMessage_CleanerActionType_cleanerActionKick);
			} else if (checkAction == "kickban") {
				netReply->set_cleaneractiontype(CleanerChatReplyMessage_CleanerActionType_cleanerActionBan);
			} else if (checkAction == "mute") {
				netReply->set_cleaneractiontype(CleanerChatReplyMessage_CleanerActionType_cleanerActionMute);
			}

			netReply->set_cleanertext(checkMessage.toUtf8());
			sendMessageToClient(*tmpReply);
		}
	}
	return error;
}

void CleanerServer::socketStateChanged(QAbstractSocket::SocketState state)
{
	qDebug() << "Socket state changed to: " << state;
	if (state == QAbstractSocket::UnconnectedState) {
		blockConnection = false;
#if QT_VERSION >= 0x050100
		tcpServer->resumeAccepting();
#endif
	}
}

void CleanerServer::refreshConfig()
{

	QFileInfo configFileInfo(QString::fromUtf8(config->getConfigFileName().c_str()));

	if(configFileInfo.lastModified().secsTo(QDateTime::currentDateTime()) < 20) {
		config->fillBuffer();
	}

	myMessageFilter->refreshConfig();
}

void CleanerServer::sendMessageToClient(ChatCleanerMessage &msg)
{
	uint32_t packetSize = msg.ByteSize();
	google::protobuf::uint8 *buf = new google::protobuf::uint8[packetSize + CLEANER_NET_HEADER_SIZE];
	*((uint32_t *)buf) = qToBigEndian(packetSize);
	msg.SerializeWithCachedSizesToArray(&buf[CLEANER_NET_HEADER_SIZE]);
	tcpSocket->write((const char *)buf, packetSize + CLEANER_NET_HEADER_SIZE);
	delete[] buf;
}

