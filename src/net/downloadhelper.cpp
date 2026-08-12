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

#include <core/loghelper.h>

#include <QUrl>
#include <QNetworkRequest>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslSocket>

#include <cstdio>

using namespace std;

namespace
{

bool
WriteReplyData(TransferData *transferData, size_t maxFileSize)
{
	QByteArray data = transferData->networkReply->readAll();
	if (data.isEmpty() || !transferData->targetFile)
		return true;

	const size_t receivedSize = static_cast<size_t>(data.size());
	const size_t currentSize = static_cast<size_t>(transferData->targetFile->size());
	if (maxFileSize != 0
			&& (currentSize > maxFileSize || receivedSize > maxFileSize - currentSize)) {
		LOG_ERROR("Download exceeds configured size limit.");
		return false;
	}

	return transferData->targetFile->write(data) == data.size();
}

// Root certificates for verifying the download on Android.
//
// Android keeps its trust store as individual files named after a hash of the
// subject, which the OpenSSL build behind Qt does not discover on its own. That
// gap used to be papered over by accepting every certificate error, which left
// the server list download unauthenticated (issue #516).
//
// The device store is preferred because the system keeps it current, so a root
// withdrawn or added after this release still takes effect. Only if it cannot be
// read does the bundle shipped with the app apply. Returning an empty list makes
// the caller fail the download instead of falling back to no verification.
//
// Note that these are the system roots only. Certificates a user installed by
// hand live elsewhere and stay untrusted, which is what Android does for apps
// since version 7 anyway - a device with an added interception CA cannot read
// this traffic.
//
// Compiled on every platform (not just Android) so that the desktop build keeps
// this code honest; it is only ever called there.
QList<QSslCertificate>
LoadSystemCaCertificates()
{
	// Android 14 moved the store into the conscrypt APEX, older releases keep
	// it below /system. The entries are PEM with a trailing text dump.
	static const char *const systemStores[] = {
		"/apex/com.android.conscrypt/cacerts",
		"/system/etc/security/cacerts"
	};

	for (const char *const store : systemStores) {
		const QString pattern = QString::fromLatin1(store) + QLatin1String("/*");
		QList<QSslCertificate> certs = QSslCertificate::fromPath(
										   pattern, QSsl::Pem, QSslCertificate::PatternSyntax::Wildcard);
		if (certs.isEmpty()) {
			certs = QSslCertificate::fromPath(
						pattern, QSsl::Der, QSslCertificate::PatternSyntax::Wildcard);
		}
		if (!certs.isEmpty()) {
			LOG_MSG("TLS: using " << certs.size() << " root certificates from " << store << ".");
			return certs;
		}
	}

	// Shipped fallback, bundled into the Android resource next to the other
	// data/ files (see the android data bundle in the QML client CMakeLists).
	QList<QSslCertificate> bundled = QSslCertificate::fromPath(
										 QLatin1String(":/android/android-data/misc/cacert.pem"), QSsl::Pem);
	if (!bundled.isEmpty()) {
		LOG_MSG("TLS: system trust store unreadable, using " << bundled.size()
				<< " bundled root certificates.");
		return bundled;
	}

	LOG_ERROR("TLS: no root certificates available - the download cannot be verified and will fail.");
	return QList<QSslCertificate>();
}

}


DownloadHelper::DownloadHelper()
{
}

DownloadHelper::~DownloadHelper()
{
}

void
DownloadHelper::InternalInit(const string &/*url*/, const string &targetFileName, const string &/*user*/, const string &/*password*/, size_t filesize, const string &/*httpPost*/)
{
	// Open target file for writing.
	const QString targetPath = QString::fromStdString(targetFileName);
	// Make sure the target directory exists. The cache directory is normally
	// created on startup (ConfigFile), but that mkdir is best-effort and may
	// have failed silently (read-only home, missing HOME, deleted cache dir,
	// snap revision change). Without this, a missing directory surfaces as the
	// cryptic "Network error (25)" (ERR_SOCK_TRANSFER_OPEN_FAILED) instead of
	// self-healing here.
	const QString targetDir = QFileInfo(targetPath).absolutePath();
	if (!targetDir.isEmpty())
		QDir().mkpath(targetDir);
	GetData()->targetFile = new QFile(targetPath);
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

#ifdef ANDROID
	// Verify the peer against the device trust store (see LoadSystemCaCertificates).
	// Everywhere else Qt already verifies with the platform defaults.
	{
		QSslConfiguration sslConfig = request.sslConfiguration();
		sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
		sslConfig.setCaCertificates(LoadSystemCaCertificates());
		request.setSslConfiguration(sslConfig);
	}
#endif

	GetData()->networkReply = GetData()->networkManager->get(request);
	if (filesize != 0)
		GetData()->networkReply->setReadBufferSize(static_cast<qint64>(filesize));

	QObject::connect(GetData()->networkReply, &QNetworkReply::metaDataChanged, [this, filesize]() {
		const QVariant contentLength = GetData()->networkReply->header(QNetworkRequest::ContentLengthHeader);
		if (filesize != 0 && contentLength.isValid()
				&& contentLength.toULongLong() > filesize) {
			LOG_ERROR("Download exceeds configured size limit.");
			GetData()->networkReply->abort();
		}
	});

	// Connect signals to write data as it arrives
	QObject::connect(GetData()->networkReply, &QNetworkReply::readyRead, [this, filesize]() {
		if (!WriteReplyData(GetData().get(), filesize))
			GetData()->networkReply->abort();
	});

	// Certificate errors are never ignored: doing so would let anyone on the
	// network serve a forged server list (issue #516). Log them, then let the
	// request fail - QNetworkReply aborts on its own when they are not accepted.
	QObject::connect(GetData()->networkReply,
		static_cast<void(QNetworkReply::*)(const QList<QSslError>&)>(&QNetworkReply::sslErrors),
		[](const QList<QSslError> &errors) {
			for (const QSslError &error : errors)
				LOG_ERROR("TLS: rejecting download - " << error.errorString().toStdString());
		});

	QObject::connect(GetData()->networkReply, &QNetworkReply::finished, [this, filesize]() {
		if (GetData()->networkReply->error() == QNetworkReply::NoError) {
			// Write any remaining data
			if (WriteReplyData(GetData().get(), filesize))
				GetData()->errorCode = 0;
			else
				GetData()->errorCode = QNetworkReply::UnknownNetworkError;
		} else {
			GetData()->errorCode = GetData()->networkReply->error();
		}
		GetData()->finished = true;
	});
}
