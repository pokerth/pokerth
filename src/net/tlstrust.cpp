/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *****************************************************************************/

#include <net/tlstrust.h>

#include <core/loghelper.h>

#include <QSslConfiguration>
#include <QSslSocket>

#ifdef ANDROID
#include <QDir>
#include <QSslCertificate>
#endif

namespace
{

#ifdef ANDROID
QList<QSslCertificate>
LoadSystemCaCertificates()
{
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

	QList<QSslCertificate> bundled = QSslCertificate::fromPath(
			QLatin1String(":/android/android-data/misc/cacert.pem"), QSsl::Pem);
	if (!bundled.isEmpty()) {
		LOG_MSG("TLS: system trust store unreadable, using " << bundled.size()
				<< " bundled root certificates.");
		return bundled;
	}

	LOG_ERROR("TLS: no root certificates available - the request cannot be verified and will fail.");
	return QList<QSslCertificate>();
}
#endif

}

void
TlsTrust::ConfigureRequest(QNetworkRequest &request)
{
	QSslConfiguration sslConfig = request.sslConfiguration();
	sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
#ifdef ANDROID
	sslConfig.setCaCertificates(LoadSystemCaCertificates());
#endif
	request.setSslConfiguration(sslConfig);
}
