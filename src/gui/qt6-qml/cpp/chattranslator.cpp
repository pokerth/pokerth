#include "chattranslator.h"
#include "chattranslatorcore.h"

#include <QRegularExpression>

// Symbole. 🌐 = Toggle (Übersetzung ein/aus), ⏳ = läuft. Über den Codepoint
// bauen (nicht per "\xF0…"-Literal – das würde als Latin-1 gelesen und "ð…"-
// Mojibake statt Emoji ergeben).
static const QString kGlobeGlyph   = QString::fromUcs4(U"\U0001F310"); // 🌐
static const QString kSpinnerGlyph = QString::fromUcs4(U"\U000023F3"); // ⏳

ChatTranslator::ChatTranslator(QStringList *chatLog, QObject *parent)
	: QObject(parent)
	, m_chatLog(chatLog)
	, m_core(new ChatTranslatorCore(nullptr, this))
{
	connect(m_core, &ChatTranslatorCore::translated,
	        this, &ChatTranslator::onCoreTranslated);
}

void ChatTranslator::setConfig(ConfigFile *config)
{
	m_core->setConfig(config);
	emit enabledChanged();
}

bool ChatTranslator::enabled() const
{
	return m_core->enabled();
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

QString ChatTranslator::decorate(const QString &formattedLine, const QString &sourceText,
                                 const QString &bodyHtml)
{
	if (!enabled() || sourceText.trimmed().isEmpty())
		return formattedLine;

	const int id = m_nextId++;
	const QString anchor = anchorFor(id, kGlobeGlyph);

	Pending p;
	p.sourceText    = sourceText;
	p.bodyHtml      = bodyHtml;
	p.currentAnchor = anchor;
	m_entries.insert(id, p);

	return formattedLine + QStringLiteral(" ") + anchor;
}

int ChatTranslator::findLineIndex(int id) const
{
	if (!m_chatLog)
		return -1;
	// Der href ist über alle Symbolzustände konstant -> eindeutige Zeilenkennung.
	const QString needle = QStringLiteral("pokerthtranslate:%1\"").arg(id);
	for (int i = m_chatLog->size() - 1; i >= 0; --i) {
		if ((*m_chatLog)[i].contains(needle))
			return i;
	}
	return -1;
}

void ChatTranslator::setGlobe(int id, const QString &glyph)
{
	auto it = m_entries.find(id);
	if (it == m_entries.end())
		return;
	const int idx = findLineIndex(id);
	if (idx < 0)
		return;
	const QString newAnchor = anchorFor(id, glyph);
	QString &line = (*m_chatLog)[idx];
	const int pos = line.indexOf(it->currentAnchor);
	if (pos < 0)
		return;
	line.replace(pos, it->currentAnchor.size(), newAnchor);
	it->currentAnchor = newAnchor;
	emit chatLogMutated();
}

void ChatTranslator::setBodyShown(int id, bool shown)
{
	auto it = m_entries.find(id);
	if (it == m_entries.end() || it->shown == shown)
		return;
	const int idx = findLineIndex(id);
	if (idx < 0)
		return;
	const QString tb = ChatTranslatorCore::styledTranslation(it->bodyHtml, it->translated);
	const QString from = it->shown ? tb : it->bodyHtml;   // gerade in der Zeile
	const QString to   = shown    ? tb : it->bodyHtml;
	QString &line = (*m_chatLog)[idx];
	const int pos = line.indexOf(from);
	if (pos < 0)
		return;
	line.replace(pos, from.size(), to);
	it->shown = shown;
	emit chatLogMutated();
}

void ChatTranslator::requestTranslation(int id)
{
	auto it = m_entries.find(id);
	if (!enabled() || it == m_entries.end() || it->inFlight)
		return;

	// Toggle: eingeblendete Übersetzung wieder ausblenden (Original zeigen).
	if (it->shown) {
		setBodyShown(id, false);
		return;
	}
	// Schon einmal übersetzt -> aus dem Cache einblenden (keine neue Anfrage).
	if (!it->translated.isEmpty()) {
		setBodyShown(id, true);
		return;
	}

	it->inFlight = true;
	setGlobe(id, kSpinnerGlyph);
	const int req = m_core->translate(it->sourceText);
	m_reqToLine.insert(req, id);
}

void ChatTranslator::onCoreTranslated(int requestId, const QString &text, bool ok)
{
	auto rit = m_reqToLine.find(requestId);
	if (rit == m_reqToLine.end())
		return;
	const int id = rit.value();
	m_reqToLine.erase(rit);
	finish(id, text, ok);
}

void ChatTranslator::finish(int id, const QString &translated, bool ok)
{
	auto it = m_entries.find(id);
	if (it == m_entries.end())
		return;
	it->inFlight = false;
	setGlobe(id, kGlobeGlyph); // Spinner zurück auf Globus

	if (ok && !translated.trimmed().isEmpty()) {
		it->translated = translated;
		setBodyShown(id, true); // Übersetzung einblenden (ersetzt das Original)
	}
	// Bei Fehler bleibt das Original stehen; Globus erlaubt einen erneuten Versuch.
}

void ChatTranslator::refreshEnabled()
{
	emit enabledChanged();
	if (enabled())
		return; // Aktivieren wirkt auf neue Nachrichten (decorate()); Bestehende bleiben.

	// Deaktiviert: in jeder betroffenen Zeile die Übersetzung (falls sichtbar)
	// durch das Original ersetzen und das Globus-Symbol entfernen.
	if (m_chatLog) {
		for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
			const int idx = findLineIndex(it.key());
			if (idx < 0)
				continue;
			QString &line = (*m_chatLog)[idx];
			if (it->shown) {
				const QString tb = ChatTranslatorCore::styledTranslation(it->bodyHtml, it->translated);
				const int bp = line.indexOf(tb);
				if (bp >= 0)
					line.replace(bp, tb.size(), it->bodyHtml);
			}
			const QString withSpace = QStringLiteral(" ") + it->currentAnchor;
			int gp = line.indexOf(withSpace);
			if (gp >= 0)
				line.remove(gp, withSpace.size());
			else if ((gp = line.indexOf(it->currentAnchor)) >= 0)
				line.remove(gp, it->currentAnchor.size());
		}
	}
	m_entries.clear();
	m_reqToLine.clear();
	emit chatLogMutated();
}
