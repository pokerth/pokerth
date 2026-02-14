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

#include <net/socket_helper.h>
#include <net/transferhelper.h>
#include <net/netexception.h>
#include <net/socket_msg.h>
#include <net/transferdata.h>

#include <QUrl>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>

#include <cstdio>

using namespace std;

#define QT_RECV_TIMEOUT_MSEC		50

TransferHelper::TransferHelper()
{
	m_data.reset(new TransferData);
}

TransferHelper::~TransferHelper()
{
	Cleanup();
}

void
TransferHelper::Init(const string &url, const string &targetFileName, const string &user, const string &password, size_t filesize, const string &httpPost)
{
	// Cleanup data.
	Cleanup();
	m_data->returnMessage.clear();
	m_data->finished = false;
	m_data->errorCode = 0;

	// Initialise Qt Network Access Manager.
	m_data->networkManager = new QNetworkAccessManager();
	if (!m_data->networkManager)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_INIT_FAILED, 0);

	// Store the URL
	m_data->url = url;

	InternalInit(url, targetFileName, user, password, filesize, httpPost);
}

bool
TransferHelper::Process()
{
	bool retVal = false;

	// Check if the transfer has already finished
	if (m_data->finished) {
		// Clean up the network objects.
		Cleanup();

		// Throw exception if an error occurred.
		if (m_data->errorCode != 0) {
			throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_FAILED, m_data->errorCode);
		}

		retVal = true;
	} else if (m_data->networkReply) {
		// Use a proper event loop that waits for the finished signal.
		// This is required because QNetworkAccessManager needs a running
		// Qt event loop to process network I/O, especially when called
		// from a non-Qt (boost) thread.
		QEventLoop loop;
		QObject::connect(m_data->networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
		QTimer::singleShot(30000, &loop, &QEventLoop::quit); // 30s safety timeout
		loop.exec();
	} else {
		// No reply object yet, just process events briefly
		QEventLoop loop;
		QTimer::singleShot(QT_RECV_TIMEOUT_MSEC, &loop, &QEventLoop::quit);
		loop.exec();
	}

	return retVal;
}

void
TransferHelper::Cleanup()
{
	if (m_data->multiPart) {
		delete m_data->multiPart;
		m_data->multiPart = NULL;
	}
	if (m_data->networkReply) {
		m_data->networkReply->deleteLater();
		m_data->networkReply = NULL;
	}
	if (m_data->networkManager) {
		delete m_data->networkManager;
		m_data->networkManager = NULL;
	}
	if (m_data->targetFile) {
		m_data->targetFile->flush();
		m_data->targetFile->close();
		delete m_data->targetFile;
		m_data->targetFile = NULL;
	}
}

string
TransferHelper::ResetLastMessage()
{
	string retVal(m_data->returnMessage);
	m_data->returnMessage.clear();
	return retVal;
}

boost::shared_ptr<TransferData>
TransferHelper::GetData()
{
	return m_data;
}

