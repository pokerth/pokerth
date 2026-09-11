/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *****************************************************************************/
#include "chattranslatorcore.h"
#include "configfile.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

ChatTranslatorCore::ChatTranslatorCore(ConfigFile *config, QObject *parent)
	: QObject(parent)
	, m_config(config)
{
}

void ChatTranslatorCore::setConfig(ConfigFile *config)
{
	m_config = config;
}

bool ChatTranslatorCore::enabled() const
{
	return m_config && m_config->readConfigInt("AllowChatTranslation") != 0;
}

QString ChatTranslatorCore::normalizeLangCode(const QString &raw)
{
	QString code = raw.trimmed();
	code.replace(QLatin1Char('-'), QLatin1Char('_'));
	if (code.isEmpty())
		return QStringLiteral("en");

	// Regional variants that the services distinguish. Cover both spellings:
	// the QML locale ("pt_BR") and the PokerTH abbreviation ("ptbr").
	if (code.startsWith(QLatin1String("pt_BR"), Qt::CaseInsensitive)
			|| code.compare(QLatin1String("ptbr"), Qt::CaseInsensitive) == 0)
		return QStringLiteral("pt-BR");
	if (code.startsWith(QLatin1String("pt_PT"), Qt::CaseInsensitive)
			|| code.compare(QLatin1String("ptpt"), Qt::CaseInsensitive) == 0)
		return QStringLiteral("pt-PT");
	if (code.startsWith(QLatin1String("zh_CN"), Qt::CaseInsensitive)
			|| code.compare(QLatin1String("zhcn"), Qt::CaseInsensitive) == 0)
		return QStringLiteral("zh-CN");
	if (code.startsWith(QLatin1String("zh_TW"), Qt::CaseInsensitive)
			|| code.compare(QLatin1String("zhtw"), Qt::CaseInsensitive) == 0)
		return QStringLiteral("zh-TW");

	// Language part before the region ("de_DE" -> "de").
	const int us = code.indexOf(QLatin1Char('_'));
	const QString lang = (us > 0 ? code.left(us) : code).toLower();

	// Map the PokerTH abbreviations (ts file names) to ISO 639-1 where they differ.
	static const QHash<QString, QString> alias = {
		{ QStringLiteral("cz"), QStringLiteral("cs") },  // Czech
		{ QStringLiteral("dk"), QStringLiteral("da") },  // Danish
		{ QStringLiteral("gr"), QStringLiteral("el") },  // Greek
		{ QStringLiteral("jp"), QStringLiteral("ja") },  // Japanese
		{ QStringLiteral("nb"), QStringLiteral("no") },  // Norwegian (Bokmål)
		{ QStringLiteral("se"), QStringLiteral("sv") },  // Swedish
		{ QStringLiteral("ua"), QStringLiteral("uk") },  // Ukrainian
	};
	return alias.value(lang, lang);
}

QString ChatTranslatorCore::targetLang() const
{
	// The only source: the ConfigFile key "Language" – BOTH clients maintain the
	// same one. It is read afresh on every request, so a language change takes
	// effect immediately (without a restart).
	const QString code = m_config
						 ? QString::fromStdString(m_config->readConfigString("Language"))
						 : QString();
	return normalizeLangCode(code);
}

QString ChatTranslatorCore::styledTranslation(const QString &originalBodyHtml,
		const QString &translated)
{
	const QString esc = translated.toHtmlEscaped();
	// Reuse the enclosing <span ...> of the original message (with its colour)
	// and replace the content with the translation, set in italics.
	if (originalBodyHtml.startsWith(QLatin1String("<span"))) {
		const int gt = originalBodyHtml.indexOf(QLatin1Char('>'));
		if (gt > 0)
			return originalBodyHtml.left(gt + 1)
				   + QStringLiteral("<i>") + esc + QStringLiteral("</i></span>");
	}
	// No enclosing span (e.g. PM text) -> plain italics, inheriting the
	// colour of the surrounding context.
	return QStringLiteral("<i>") + esc + QStringLiteral("</i>");
}

int ChatTranslatorCore::translate(const QString &text)
{
	const int id = m_nextId++;
	m_sourceById.insert(id, text);
	startPrimary(id, text);
	return id;
}

void ChatTranslatorCore::startPrimary(int id, const QString &text)
{
	QUrl url(QStringLiteral("https://translate.googleapis.com/translate_a/single"));
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("client"), QStringLiteral("gtx"));
	query.addQueryItem(QStringLiteral("sl"), QStringLiteral("auto"));
	query.addQueryItem(QStringLiteral("tl"), targetLang());
	query.addQueryItem(QStringLiteral("dt"), QStringLiteral("t"));
	query.addQueryItem(QStringLiteral("q"), text);
	url.setQuery(query);

	QNetworkRequest req(url);
	req.setHeader(QNetworkRequest::UserAgentHeader,
				  QByteArrayLiteral("Mozilla/5.0 (compatible; PokerTH)"));
	QNetworkReply *reply = m_nam.get(req);
	reply->setProperty("xlate_id", id);
	connect(reply, &QNetworkReply::finished, this, &ChatTranslatorCore::onPrimaryReply);
}

void ChatTranslatorCore::onPrimaryReply()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	if (!reply)
		return;
	reply->deleteLater();
	const int id = reply->property("xlate_id").toInt();

	if (reply->error() != QNetworkReply::NoError) {
		// Log an outage of the primary service: Google throttles the
		// gtx endpoint per IP (HTTP 429 "Sorry..." page). Without this line
		// it is impossible to tell from the outside whether the service blocks or the
		// reply merely could not be parsed.
		qWarning() << "ChatTranslator: Google-Endpunkt fehlgeschlagen, HTTP"
				   << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
				   << reply->errorString() << "-> MyMemory";
		startFallback(id, m_sourceById.value(id));
		return;
	}

	// Reply: [[["translation","original",…],[…]], null, "en", …]
	const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
	if (!doc.isArray()) {
		startFallback(id, m_sourceById.value(id));
		return;
	}
	const QJsonArray outer = doc.array();
	if (outer.isEmpty() || !outer.at(0).isArray()) {
		startFallback(id, m_sourceById.value(id));
		return;
	}
	QString result;
	const QJsonArray sentences = outer.at(0).toArray();
	for (const QJsonValue &seg : sentences) {
		if (seg.isArray()) {
			const QJsonArray a = seg.toArray();
			if (!a.isEmpty())
				result += a.at(0).toString();
		}
	}
	if (result.trimmed().isEmpty()) {
		startFallback(id, m_sourceById.value(id));
		return;
	}
	m_sourceById.remove(id);
	emit translated(id, result, true);
}

void ChatTranslatorCore::startFallback(int id, const QString &text)
{
	const QString tl = targetLang();
	if (text.isEmpty()) {
		m_sourceById.remove(id);
		emit translated(id, QString(), false);
		return;
	}

	QUrl url(QStringLiteral("https://api.mymemory.translated.net/get"));
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("q"), text);
	// MyMemory requires a source language, but it knows "Autodetect" (the
	// detected language is in the reply as responseData.detectedLanguage).
	// Before, a fixed "en" stood here – with an abort if the client itself is set
	// to English. An English client thus had NO fallback at all:
	// if the Google endpoint fails (it throttles per IP with HTTP 429), the
	// globe briefly showed the hourglass and then visibly nothing. At the same time
	// every non-English message was translated as English.
	query.addQueryItem(QStringLiteral("langpair"),
					   QStringLiteral("Autodetect|") + tl);
	url.setQuery(query);

	QNetworkRequest req(url);
	req.setHeader(QNetworkRequest::UserAgentHeader,
				  QByteArrayLiteral("Mozilla/5.0 (compatible; PokerTH)"));
	QNetworkReply *reply = m_nam.get(req);
	reply->setProperty("xlate_id", id);
	connect(reply, &QNetworkReply::finished, this, &ChatTranslatorCore::onFallbackReply);
}

void ChatTranslatorCore::onFallbackReply()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	if (!reply)
		return;
	reply->deleteLater();
	const int id = reply->property("xlate_id").toInt();
	const QString source = m_sourceById.take(id);

	if (reply->error() != QNetworkReply::NoError) {
		emit translated(id, QString(), false);
		return;
	}
	// Antwort: { "responseData": { "translatedText": "…" }, "responseStatus": 200 }
	// responseStatus MUST be checked: in the error case (invalid language pair,
	// the daily quota of the free usage used up) MyMemory replies with
	// HTTP 200 and writes the warning text in CAPITALS into translatedText –
	// unchecked, this warning would stand as the "translation" in the chat line. The
	// field arrives sometimes as a number (200), sometimes as a string ("403"), hence via QVariant.
	const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
	QString text;
	int status = 0;
	if (doc.isObject()) {
		const QJsonObject obj = doc.object();
		status = obj.value(QStringLiteral("responseStatus")).toVariant().toInt();
		text = obj.value(QStringLiteral("responseData")).toObject()
			   .value(QStringLiteral("translatedText")).toString();
	}
	if (status != 200) {
		// Special case "source == target": MyMemory replies to that with 403
		// "PLEASE SELECT TWO DISTINCT LANGUAGES" – the message is already in
		// the language of the client, so there is nothing to translate. That is
		// NOT an error: in this case the Google endpoint simply returns the
		// original text, and that is exactly how the fallback behaves now
		// as well. It mainly affects English clients, for which English is the
		// most frequent language in the lobby chat.
		if (text.contains(QLatin1String("DISTINCT LANGUAGES"), Qt::CaseInsensitive)
				&& !source.isEmpty()) {
			emit translated(id, source, true);
			return;
		}
		qWarning() << "ChatTranslator: MyMemory-Fallback fehlgeschlagen, Status"
				   << status << text.left(120);
		emit translated(id, QString(), false);
		return;
	}
	emit translated(id, text, !text.trimmed().isEmpty());
}
