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
#include <net/uploadhelper.h>
#include <net/netexception.h>
#include <net/socket_msg.h>
#include <net/transferdata.h>
#include <sys/stat.h>

#include <QUrl>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QFile>
#include <QFileInfo>
#include <QSslSocket>

#include <cstdio>

using namespace std;

UploadHelper::UploadHelper()
{
}

UploadHelper::~UploadHelper()
{
}

void
UploadHelper::InternalInit(const string &/*url*/, const string &targetFileName, const string &user, const string &password, size_t filesize, const string &httpPost)
{
	// Ensure URL has a protocol prefix (http:// or https://)
	string urlWithProtocol = GetData()->url;
	if (urlWithProtocol.find("://") == string::npos) {
		urlWithProtocol = "https://" + urlWithProtocol;
	}

	QUrl qUrl(QString::fromStdString(urlWithProtocol));
	QNetworkRequest request(qUrl);
	request.setRawHeader("User-Agent", "PokerTH/2.0 (Qt Network)");

	// Disable SSL certificate verification (matching old curl behavior)
	QSslConfiguration sslConfig = request.sslConfiguration();
	sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
	request.setSslConfiguration(sslConfig);

	// Set authentication if provided
	if (!user.empty() || !password.empty()) {
		GetData()->userCredentials = user + ":" + password;
		QString concatenated = QString::fromStdString(user) + ":" + QString::fromStdString(password);
		QByteArray data = concatenated.toLocal8Bit().toBase64();
		QString headerData = "Basic " + data;
		request.setRawHeader("Authorization", headerData.toLocal8Bit());
	}

	if (httpPost.empty()) {
		// Simple PUT upload
		GetData()->targetFile = new QFile(QString::fromStdString(targetFileName));
		if (!GetData()->targetFile->open(QIODevice::ReadOnly))
			throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_OPEN_FAILED, 0);

		GetData()->networkReply = GetData()->networkManager->put(request, GetData()->targetFile);
	} else {
		// HTTP POST with multipart form data
		QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
		GetData()->multiPart = multiPart;

		QHttpPart filePart;
		QString filename = QFileInfo(QString::fromStdString(targetFileName)).fileName();
		filePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
			QVariant("form-data; name=\"" + QString::fromStdString(httpPost) + "\"; filename=\"" + filename + "\""));

		QFile *file = new QFile(QString::fromStdString(targetFileName));
		if (!file->open(QIODevice::ReadOnly)) {
			delete file;
			throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_OPEN_FAILED, 0);
		}
		filePart.setBodyDevice(file);
		file->setParent(multiPart);

		multiPart->append(filePart);

		GetData()->networkReply = GetData()->networkManager->post(request, multiPart);
		multiPart->setParent(GetData()->networkReply);
	}

	// Connect signals
	QObject::connect(GetData()->networkReply, &QNetworkReply::finished, [this]() {
		if (GetData()->networkReply->error() == QNetworkReply::NoError) {
			// Read response data
			QByteArray response = GetData()->networkReply->readAll();
			GetData()->returnMessage = string(response.constData(), response.size());
			GetData()->errorCode = 0;
		} else {
			GetData()->errorCode = GetData()->networkReply->error();
		}
		GetData()->finished = true;
	});
}

