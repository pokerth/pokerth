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
#include <net/downloadhelper.h>
#include <net/netexception.h>
#include <net/socket_msg.h>
#include <net/transferdata.h>

#include <QUrl>
#include <QNetworkRequest>
#include <QFile>

#include <cstdio>

using namespace std;


DownloadHelper::DownloadHelper()
{
}

DownloadHelper::~DownloadHelper()
{
}

void
DownloadHelper::InternalInit(const string &/*url*/, const string &targetFileName, const string &/*user*/, const string &/*password*/, size_t /*filesize*/, const string &/*httpPost*/)
{
	// Open target file for writing.
	GetData()->targetFile = new QFile(QString::fromStdString(targetFileName));
	if (!GetData()->targetFile->open(QIODevice::WriteOnly))
		throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_OPEN_FAILED, 0);

	// Ensure URL has a protocol prefix (http:// or https://)
	string urlWithProtocol = GetData()->url;
	if (urlWithProtocol.find("://") == string::npos) {
		urlWithProtocol = "https://" + urlWithProtocol;
	}

	QUrl qUrl(QString::fromStdString(urlWithProtocol));
	QNetworkRequest request(qUrl);
	request.setRawHeader("User-Agent", "PokerTH/2.0 (Qt Network)");

	GetData()->networkReply = GetData()->networkManager->get(request);

	// Connect signals to write data as it arrives
	QObject::connect(GetData()->networkReply, &QNetworkReply::readyRead, [this]() {
		QByteArray data = GetData()->networkReply->readAll();
		if (GetData()->targetFile) {
			GetData()->targetFile->write(data);
		}
	});

	// Ignore SSL errors on Android (certificate validation issues with system CA store)
#ifdef ANDROID
	QObject::connect(GetData()->networkReply, 
		static_cast<void(QNetworkReply::*)(const QList<QSslError>&)>(&QNetworkReply::sslErrors),
		[this](const QList<QSslError> &errors) {
			GetData()->networkReply->ignoreSslErrors(errors);
		});
#endif

	QObject::connect(GetData()->networkReply, &QNetworkReply::finished, [this]() {
		if (GetData()->networkReply->error() == QNetworkReply::NoError) {
			// Write any remaining data
			QByteArray data = GetData()->networkReply->readAll();
			if (GetData()->targetFile && !data.isEmpty()) {
				GetData()->targetFile->write(data);
			}
			GetData()->errorCode = 0;
		} else {
			GetData()->errorCode = GetData()->networkReply->error();
		}
		GetData()->finished = true;
	});
}

