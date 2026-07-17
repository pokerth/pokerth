#include "chattranslator.h"
#include "configfile.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

// Symbole. 🌐 = anklickbar (übersetzen), ⏳ = läuft. Beide werden über die
// System-Emoji-Fallback-Schrift des TextEdit gerendert (wie die restlichen
// Unicode-Emojis im Chat).
static const QString kGlobeGlyph   = QStringLiteral("\xF0\x9F\x8C\x90"); // U+1F310
static const QString kSpinnerGlyph = QStringLiteral("\xE2\x8F\xB3");     // U+23F3

ChatTranslator::ChatTranslator(QStringList *chatLog, QObject *parent)
	: QObject(parent)
	, m_chatLog(chatLog)
{
}

void ChatTranslator::setConfig(ConfigFile *config)
{
	m_config = config;
	emit enabledChanged();
}

bool ChatTranslator::enabled() const
{
	return m_config && m_config->readConfigInt("AllowChatTranslation") != 0;
}

QString ChatTranslator::targetLang() const
{
	QString code = m_config
		? QString::fromStdString(m_config->readConfigString("Language"))
		: QString();
	if (code.isEmpty())
		code = QStringLiteral("en_US");
	// pt_BR/pt_PT behalten die Region (Google und MyMemory unterscheiden sie),
	// für alle übrigen Sprachen genügt der 2-Buchstaben-Sprachcode.
	if (code.startsWith(QLatin1String("pt_BR")))
		return QStringLiteral("pt-BR");
	if (code.startsWith(QLatin1String("pt_PT")))
		return QStringLiteral("pt-PT");
	const int us = code.indexOf(QLatin1Char('_'));
	return us > 0 ? code.left(us) : code;
}

QString ChatTranslator::anchorFor(int id, const QString &glyph)
{
	// text-decoration:none, damit das Symbol nicht als unterstrichener Link
	// erscheint. Der Scheme-Präfix "pokerthtranslate:" wird von der ChatBox
	// abgefangen (nicht extern geöffnet).
	return QStringLiteral("<a href=\"pokerthtranslate:%1\" style=\"text-decoration:none;\">%2</a>")
		.arg(id)
		.arg(glyph);
}

QString ChatTranslator::decorate(const QString &formattedLine, const QString &sourceText)
{
	if (!enabled() || sourceText.trimmed().isEmpty())
		return formattedLine;

	const int id = m_nextId++;
	const QString anchor = anchorFor(id, kGlobeGlyph);

	Pending p;
	p.sourceText   = sourceText;
	p.currentAnchor = anchor;
	m_entries.insert(id, p);

	return formattedLine + QStringLiteral(" ") + anchor;
}

void ChatTranslator::requestTranslation(int id)
{
	auto it = m_entries.find(id);
	// Deaktiviert, unbekannt oder bereits laufend -> ignorieren (verhindert
	// Doppelanfragen bei schnellem Doppeltippen auf das ⏳-Symbol).
	if (!enabled() || it == m_entries.end() || it->inFlight)
		return;

	it->inFlight = true;
	// Symbol auf "läuft" umstellen (bleibt über den href auffindbar).
	replaceAnchor(id, anchorFor(id, kSpinnerGlyph));
	startPrimary(id);
}

void ChatTranslator::startPrimary(int id)
{
	auto it = m_entries.find(id);
	if (it == m_entries.end())
		return;

	QUrl url(QStringLiteral("https://translate.googleapis.com/translate_a/single"));
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("client"), QStringLiteral("gtx"));
	query.addQueryItem(QStringLiteral("sl"), QStringLiteral("auto"));
	query.addQueryItem(QStringLiteral("tl"), targetLang());
	query.addQueryItem(QStringLiteral("dt"), QStringLiteral("t"));
	query.addQueryItem(QStringLiteral("q"), it->sourceText);
	url.setQuery(query);

	QNetworkRequest req(url);
	req.setHeader(QNetworkRequest::UserAgentHeader,
	              QByteArrayLiteral("Mozilla/5.0 (compatible; PokerTH)"));
	QNetworkReply *reply = m_nam.get(req);
	reply->setProperty("xlate_id", id);
	connect(reply, &QNetworkReply::finished, this, &ChatTranslator::onPrimaryReply);
}

void ChatTranslator::onPrimaryReply()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	if (!reply)
		return;
	reply->deleteLater();
	const int id = reply->property("xlate_id").toInt();

	if (reply->error() != QNetworkReply::NoError) {
		startFallback(id);
		return;
	}

	// Antwort: [[["Übersetzung","Original",…],[…]], null, "en", …]
	const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
	if (!doc.isArray()) {
		startFallback(id);
		return;
	}
	const QJsonArray outer = doc.array();
	if (outer.isEmpty() || !outer.at(0).isArray()) {
		startFallback(id);
		return;
	}
	QString translated;
	const QJsonArray sentences = outer.at(0).toArray();
	for (const QJsonValue &seg : sentences) {
		if (seg.isArray()) {
			const QJsonArray a = seg.toArray();
			if (!a.isEmpty())
				translated += a.at(0).toString();
		}
	}
	if (translated.trimmed().isEmpty()) {
		startFallback(id);
		return;
	}
	finish(id, translated, true);
}

void ChatTranslator::startFallback(int id)
{
	auto it = m_entries.find(id);
	if (it == m_entries.end())
		return;

	const QString tl = targetLang();
	// MyMemory verlangt eine Quellsprache; die Auto-Erkennung von Google ist an
	// dieser Stelle ausgefallen. Heuristik: Im internationalen Lobby-Chat ist
	// Englisch die häufigste Fremdsprache. Ist der Client selbst englisch, lässt
	// sich die Quelle nicht sinnvoll raten -> sauber abbrechen (Retry möglich).
	const QString src = tl.startsWith(QLatin1String("en"))
		? QString()
		: QStringLiteral("en");
	if (src.isEmpty()) {
		finish(id, QString(), false);
		return;
	}

	QUrl url(QStringLiteral("https://api.mymemory.translated.net/get"));
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("q"), it->sourceText);
	query.addQueryItem(QStringLiteral("langpair"), src + QLatin1Char('|') + tl);
	url.setQuery(query);

	QNetworkRequest req(url);
	req.setHeader(QNetworkRequest::UserAgentHeader,
	              QByteArrayLiteral("Mozilla/5.0 (compatible; PokerTH)"));
	QNetworkReply *reply = m_nam.get(req);
	reply->setProperty("xlate_id", id);
	connect(reply, &QNetworkReply::finished, this, &ChatTranslator::onFallbackReply);
}

void ChatTranslator::onFallbackReply()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
	if (!reply)
		return;
	reply->deleteLater();
	const int id = reply->property("xlate_id").toInt();

	if (reply->error() != QNetworkReply::NoError) {
		finish(id, QString(), false);
		return;
	}
	// Antwort: { "responseData": { "translatedText": "…" }, "responseStatus": 200 }
	const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
	QString translated;
	if (doc.isObject()) {
		const QJsonObject rd = doc.object().value(QStringLiteral("responseData")).toObject();
		translated = rd.value(QStringLiteral("translatedText")).toString();
	}
	finish(id, translated, !translated.trimmed().isEmpty());
}

void ChatTranslator::finish(int id, const QString &translated, bool ok)
{
	if (!m_entries.contains(id))
		return;

	if (ok && !translated.trimmed().isEmpty()) {
		// Terminal: Symbol durch die Übersetzung ersetzen (kein Link mehr, damit
		// nicht erneut angefragt wird). Gedämpfte, kursive Darstellung, dem
		// Original nachgestellt.
		const QString html =
			QStringLiteral("<span style=\"color:#8899bb; font-style:italic;\">")
			+ kGlobeGlyph + QStringLiteral(" ")
			+ translated.toHtmlEscaped()
			+ QStringLiteral("</span>");
		replaceAnchor(id, html);
		m_entries.remove(id);
	} else {
		// Fehlgeschlagen: wieder anklickbares Globus-Symbol (Retry ist möglich).
		m_entries[id].inFlight = false;
		replaceAnchor(id, anchorFor(id, kGlobeGlyph));
	}
}

bool ChatTranslator::replaceAnchor(int id, const QString &newAnchor)
{
	auto it = m_entries.find(id);
	if (it == m_entries.end())
		return false;
	const QString oldAnchor = it->currentAnchor;
	if (oldAnchor.isEmpty() || !m_chatLog)
		return false;

	// Von hinten suchen: die betroffene Zeile ist praktisch immer eine der
	// jüngsten. Das Anchor-HTML enthält die eindeutige id, ist also pro Zeile
	// einmalig.
	for (int i = m_chatLog->size() - 1; i >= 0; --i) {
		const int pos = (*m_chatLog)[i].indexOf(oldAnchor);
		if (pos >= 0) {
			(*m_chatLog)[i].replace(pos, oldAnchor.size(), newAnchor);
			it->currentAnchor = newAnchor;
			emit chatLogMutated();
			return true;
		}
	}
	return false;
}
