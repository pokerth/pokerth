#include "chattranslator.h"
#include "chattranslatorcore.h"

#include <QRegularExpression>

// Symbole. 🌐 = Toggle (Übersetzung ein/aus), ⏳ = läuft. Über den Codepoint
// bauen (nicht per "\xF0…"-Literal – das würde als Latin-1 gelesen und "ð…"-
// Mojibake statt Emoji ergeben).
static const QString kGlobeGlyph   = QString::fromUcs4(U"\U0001F310"); // 🌐
static const QString kSpinnerGlyph = QString::fromUcs4(U"\U000023F3"); // ⏳
// Unsichtbarer Platzhalter für Zeilen, die gerade NICHT unter dem Mauszeiger
// liegen. Der Anker bleibt damit in der Zeile (und die Zeile über ihren href
// auffindbar), zeigt aber nichts an. Geschütztes Leerzeichen, weil normale
// Leerzeichen am Zeilenende vom RichText-Renderer wegfallen können.
static const QString kHiddenGlyph  = QStringLiteral("&nbsp;");

// Hover-Modus: Auf Desktop erscheint der Globus nur an der Zeile unter dem
// Mauszeiger (der Verlauf war sonst mit Symbolen zugepflastert). Auf Touch-
// Geräten gibt es kein Hover – dort bleiben die Symbole sichtbar, sonst wäre
// die Funktion nicht mehr erreichbar.
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
static const bool kHoverOnly = false;
#else
static const bool kHoverOnly = true;
#endif

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
	// abgefangen (nicht extern geöffnet). Das Symbol wird wie ein Chat-Emoji
	// gerendert (Noto Color Emoji), aber bewusst etwas kleiner als die
	// Nachrichten-Emojis (enlargeEmojis: 22px) – gut erkennbar, ohne die Zeile
	// zu dominieren. Größe ist fix, also unabhängig von der Chat-Textgröße.
	return QStringLiteral("<a href=\"pokerthtranslate:%1\" style=\"text-decoration:none;\">"
	                      "<span style=\"font-size:15px; font-family:'Noto Color Emoji';\">%2</span></a>")
		.arg(id)
		.arg(glyph);
}

QString ChatTranslator::glyphFor(const Pending &p, int id) const
{
	if (p.inFlight)
		return kSpinnerGlyph;
	// Sichtbar, solange die Übersetzung eingeblendet ist (zeigt an, dass die
	// Zeile übersetzt ist, und ist der Rückweg zum Original) – sonst nur an der
	// Zeile unter dem Mauszeiger.
	if (!kHoverOnly || p.shown || m_hoveredId == id)
		return kGlobeGlyph;
	return kHiddenGlyph;
}

QString ChatTranslator::decorate(const QString &formattedLine, const QString &sourceText,
                                 const QString &bodyHtml)
{
	if (!enabled() || sourceText.trimmed().isEmpty())
		return formattedLine;

	const int id = m_nextId++;

	Pending p;
	p.sourceText    = sourceText;
	p.bodyHtml      = bodyHtml;
	p.currentAnchor = anchorFor(id, glyphFor(p, id));
	m_entries.insert(id, p);

	return formattedLine + QStringLiteral(" ") + p.currentAnchor;
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

void ChatTranslator::updateGlobe(int id)
{
	auto it = m_entries.find(id);
	if (it == m_entries.end())
		return;
	const QString newAnchor = anchorFor(id, glyphFor(*it, id));
	if (newAnchor == it->currentAnchor)
		return;
	const int idx = findLineIndex(id);
	if (idx < 0)
		return;
	QString &line = (*m_chatLog)[idx];
	const int pos = line.indexOf(it->currentAnchor);
	if (pos < 0)
		return;
	line.replace(pos, it->currentAnchor.size(), newAnchor);
	it->currentAnchor = newAnchor;
	emit chatLogMutated();
}

void ChatTranslator::setHoveredLine(int lineIndex)
{
	if (!kHoverOnly || !m_chatLog)
		return;

	// Zeile -> Anker-id: der href steht als Klartext in der (HTML-)Zeile.
	int id = 0;
	if (lineIndex >= 0 && lineIndex < m_chatLog->size()) {
		static const QString kHref = QStringLiteral("pokerthtranslate:");
		const QString &line = (*m_chatLog)[lineIndex];
		const int p = line.indexOf(kHref);
		if (p >= 0) {
			const int s = p + kHref.size();
			const int e = line.indexOf(QLatin1Char('"'), s);
			if (e > s)
				id = line.mid(s, e - s).toInt();
		}
	}

	if (id == m_hoveredId)
		return;
	const int prev = m_hoveredId;
	m_hoveredId = id;
	// Symbol an der alten Zeile ausblenden, an der neuen einblenden.
	if (prev > 0)
		updateGlobe(prev);
	if (id > 0)
		updateGlobe(id);
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
	// Das Symbol hängt am „shown"-Zustand (eingeblendete Übersetzung behält den
	// Globus als Rückweg, auch wenn die Maus weiterzieht).
	updateGlobe(id);
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
	updateGlobe(id);   // Spinner anzeigen
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

	if (ok && !translated.trimmed().isEmpty()) {
		it->translated = translated;
		setBodyShown(id, true); // Übersetzung einblenden (ersetzt das Original)
	}
	// Spinner zurück auf Globus – bzw. auf den unsichtbaren Platzhalter, falls
	// die Zeile inzwischen weder eingeblendet ist noch unter der Maus liegt.
	// NACH setBodyShown, weil das Symbol vom „shown"-Zustand abhängt.
	updateGlobe(id);
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
	m_hoveredId = 0;
	emit chatLogMutated();
}
