/* Network access manager factory for the QML web views (rankings & player
 * pages). pokerth.net / bbc.pokerth.net / wec.pokerth.net sit behind a
 * Cloudflare bot filter that allowlists the PokerTH client User-Agent. QML's
 * XMLHttpRequest cannot set the User-Agent header itself (Qt forbids it), so it
 * is injected here for every request, matching the Qt-Widgets client
 * (upload-/downloadhelper: "PokerTH/2.0 (Qt Network)"). A cookie jar is kept so
 * the Laravel CSRF session cookie (BBC/WEC) survives between the GET and POST.
 */
#ifndef _WEBNETWORKFACTORY_H_
#define _WEBNETWORKFACTORY_H_

#include <QQmlNetworkAccessManagerFactory>
#include <QNetworkAccessManager>

class WebNetworkAccessManager : public QNetworkAccessManager
{
	Q_OBJECT
public:
	explicit WebNetworkAccessManager(QObject *parent = nullptr);

protected:
	QNetworkReply *createRequest(Operation op, const QNetworkRequest &request,
	                             QIODevice *outgoingData = nullptr) override;
};

class WebNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:
	QNetworkAccessManager *create(QObject *parent) override;
};

#endif // _WEBNETWORKFACTORY_H_
