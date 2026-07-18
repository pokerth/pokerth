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

	// Regionale Varianten, die die Dienste unterscheiden. Beide Schreibweisen
	// abdecken: QML-Locale ("pt_BR") und PokerTH-Kürzel ("ptbr").
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

	// Sprachteil vor der Region ("de_DE" -> "de").
	const int us = code.indexOf(QLatin1Char('_'));
	const QString lang = (us > 0 ? code.left(us) : code).toLower();

	// PokerTH-Kürzel (ts-Dateinamen) auf ISO 639-1 abbilden, wo sie abweichen.
	static const QHash<QString, QString> alias = {
		{ QStringLiteral("cz"), QStringLiteral("cs") },  // Tschechisch
		{ QStringLiteral("dk"), QStringLiteral("da") },  // Dänisch
		{ QStringLiteral("gr"), QStringLiteral("el") },  // Griechisch
		{ QStringLiteral("jp"), QStringLiteral("ja") },  // Japanisch
		{ QStringLiteral("se"), QStringLiteral("sv") },  // Schwedisch
		{ QStringLiteral("ua"), QStringLiteral("uk") },  // Ukrainisch
	};
	return alias.value(lang, lang);
}

QString ChatTranslatorCore::targetLang() const
{
	// Einzige Quelle: der ConfigFile-Key "Language" – denselben pflegen BEIDE
	// Clients. Wird bei jeder Anfrage frisch gelesen, ein Sprachwechsel wirkt
	// damit sofort (ohne Neustart).
	const QString code = m_config
		? QString::fromStdString(m_config->readConfigString("Language"))
		: QString();
	return normalizeLangCode(code);
}

QString ChatTranslatorCore::styledTranslation(const QString &originalBodyHtml,
                                              const QString &translated)
{
	const QString esc = translated.toHtmlEscaped();
	// Umschließenden <span ...> der Originalnachricht (mit Farbe) wiederverwenden
	// und den Inhalt durch die – kursiv gesetzte – Übersetzung ersetzen.
	if (originalBodyHtml.startsWith(QLatin1String("<span"))) {
		const int gt = originalBodyHtml.indexOf(QLatin1Char('>'));
		if (gt > 0)
			return originalBodyHtml.left(gt + 1)
			       + QStringLiteral("<i>") + esc + QStringLiteral("</i></span>");
	}
	// Kein umschließender Span (z. B. PM-Text) -> schlicht kursiv, erbt die
	// Farbe des umgebenden Kontexts.
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
		startFallback(id, m_sourceById.value(id));
		return;
	}

	// Antwort: [[["Übersetzung","Original",…],[…]], null, "en", …]
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
	// MyMemory verlangt eine Quellsprache; die Auto-Erkennung von Google ist an
	// dieser Stelle ausgefallen. Heuristik: Im internationalen Lobby-Chat ist
	// Englisch die häufigste Fremdsprache. Ist der Client selbst englisch, lässt
	// sich die Quelle nicht sinnvoll raten -> sauber abbrechen (Retry möglich).
	const QString src = tl.startsWith(QLatin1String("en"))
		? QString()
		: QStringLiteral("en");
	if (src.isEmpty() || text.isEmpty()) {
		m_sourceById.remove(id);
		emit translated(id, QString(), false);
		return;
	}

	QUrl url(QStringLiteral("https://api.mymemory.translated.net/get"));
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("q"), text);
	query.addQueryItem(QStringLiteral("langpair"), src + QLatin1Char('|') + tl);
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
	m_sourceById.remove(id);

	if (reply->error() != QNetworkReply::NoError) {
		emit translated(id, QString(), false);
		return;
	}
	// Antwort: { "responseData": { "translatedText": "…" }, "responseStatus": 200 }
	const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
	QString text;
	if (doc.isObject()) {
		const QJsonObject rd = doc.object().value(QStringLiteral("responseData")).toObject();
		text = rd.value(QStringLiteral("translatedText")).toString();
	}
	emit translated(id, text, !text.trimmed().isEmpty());
}
