#include "webnetworkfactory.h"

#include <QNetworkRequest>
#include <QNetworkCookieJar>

// Allowlisted by the pokerth.net Cloudflare configuration – identical to the
// Qt-Widgets client (src/net/upload-/downloadhelper.cpp).
static const QByteArray POKERTH_USER_AGENT = QByteArrayLiteral("PokerTH/2.0 (Qt Network)");

WebNetworkAccessManager::WebNetworkAccessManager(QObject *parent)
	: QNetworkAccessManager(parent)
{
	// Explicit in-memory cookie jar so the CSRF session cookie set by the GET
	// is sent back on the following POST (BBC/WEC leaderboard & player APIs).
	setCookieJar(new QNetworkCookieJar(this));
}

QNetworkReply *WebNetworkAccessManager::createRequest(Operation op,
        const QNetworkRequest &request, QIODevice *outgoingData)
{
	QNetworkRequest req(request);
	req.setRawHeader(QByteArrayLiteral("User-Agent"), POKERTH_USER_AGENT);
	return QNetworkAccessManager::createRequest(op, req, outgoingData);
}

QNetworkAccessManager *WebNetworkAccessManagerFactory::create(QObject *parent)
{
	return new WebNetworkAccessManager(parent);
}
