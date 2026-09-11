/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *****************************************************************************/
#ifndef _CHATTRANSLATORCORE_H_
#define _CHATTRANSLATORCORE_H_

#include <QObject>
#include <QString>
#include <QHash>
#include <QNetworkAccessManager>

class ConfigFile;
class QNetworkReply;

/* Toolkit independent network core of the chat translation. Used by BOTH
 * clients (QML: ChatTranslator; widgets: ChatTools), so that the service/
 * language logic exists in one place only.
 *
 * Target language = the language set in the client (config "Language").
 * Primary source: the Google Translate "gtx" endpoint (no key, automatic source
 * language, request from the client IP). Fallback: MyMemory. Legal notes on both
 * services: docs/third_party_services.md.
 *
 * The core knows neither chat lines nor HTML – it takes text and returns the
 * translation asynchronously via translated(). Embedding the symbol and
 * replacing it in the respective display is done by the caller.
 */
class ChatTranslatorCore : public QObject
{
	Q_OBJECT
public:
	explicit ChatTranslatorCore(ConfigFile *config, QObject *parent = nullptr);

	void setConfig(ConfigFile *config);
	// Globally on/off (config "AllowChatTranslation").
	bool enabled() const;

	// Starts a translation of the text into the client language. Returns a
	// request ID; the result arrives asynchronously via translated(requestId, …).
	int translate(const QString &text);

	// Target language as an API code ("de", "pt-BR", "zh-CN", …). The source is
	// the ConfigFile key "Language", which both clients maintain.
	QString targetLang() const;

	// Normalizes a language code to a code understood by the services. Covers
	// QML locales ("de_DE") as well as the PokerTH abbreviations of the
	// widget ts files ("cz", "dk", "gr", "jp", "ptbr", "zhcn" …), which partly
	// deviate from ISO 639-1.
	static QString normalizeLangCode(const QString &raw);

	// Builds the HTML for the inserted translation, which REPLACES the original
	// text in place. If possible it adopts the enclosing <span> (and thereby the
	// colour) of the original message, so that the translation appears in the
	// same look (correct for the theme/table colours), only in italics as a
	// marker. Used by both clients.
	static QString styledTranslation(const QString &originalBodyHtml,
									 const QString &translated);

signals:
	void translated(int requestId, const QString &text, bool ok);

private slots:
	void onPrimaryReply();
	void onFallbackReply();

private:
	void startPrimary(int id, const QString &text);
	void startFallback(int id, const QString &text);

	ConfigFile *m_config;
	QNetworkAccessManager m_nam;
	QHash<int, QString> m_sourceById;  // raw text per request (for the fallback)
	int m_nextId = 1;
};

#endif // _CHATTRANSLATORCORE_H_
