#include "chattranslator.h"
#include "chattranslatorcore.h"

// Symbole. 🌐 = anklickbar (übersetzen), ⏳ = läuft. Beide werden über die
// System-Emoji-Fallback-Schrift des TextEdit gerendert (wie die restlichen
// Unicode-Emojis im Chat). WICHTIG: über den Codepoint (fromUcs4) bauen, NICHT
// per QStringLiteral("\xF0…") – dort würden die UTF-8-Bytes als Latin-1 gelesen
// (jedes Byte ein Zeichen -> "ð…"-Mojibake statt Emoji).
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

QString ChatTranslator::anchorShown(int id, const QString &translated)
{
	// Eingeblendete Übersetzung: bleibt EIN Anchor (gleicher href) – ein
	// erneuter Klick blendet sie wieder aus. Gedämpft/kursiv, dem Original
	// nachgestellt, das Globus-Symbol bleibt als Kennzeichen vorangestellt.
	return QStringLiteral("<a href=\"pokerthtranslate:%1\" style=\"text-decoration:none; color:#8899bb; font-style:italic;\">%2 %3</a>")
		.arg(id)
		.arg(kGlobeGlyph)
		.arg(translated.toHtmlEscaped());
}

QString ChatTranslator::decorate(const QString &formattedLine, const QString &sourceText)
{
	if (!enabled() || sourceText.trimmed().isEmpty())
		return formattedLine;

	const int id = m_nextId++;
	const QString anchor = anchorFor(id, kGlobeGlyph);

	Pending p;
	p.sourceText    = sourceText;
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

	// Toggle: eingeblendete Übersetzung wieder ausblenden.
	if (it->shown) {
		replaceAnchor(id, anchorFor(id, kGlobeGlyph));
		it->shown = false;
		return;
	}
	// Schon einmal übersetzt -> aus dem Cache einblenden (keine neue Anfrage).
	if (!it->translated.isEmpty()) {
		replaceAnchor(id, anchorShown(id, it->translated));
		it->shown = true;
		return;
	}

	it->inFlight = true;
	// Symbol auf "läuft" umstellen (bleibt über den href auffindbar).
	replaceAnchor(id, anchorFor(id, kSpinnerGlyph));

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
		// Übersetzung einblenden und cachen (Toggle: erneuter Klick blendet
		// sie wieder aus, ohne neue Anfrage).
		it->translated = translated;
		it->shown = true;
		replaceAnchor(id, anchorShown(id, translated));
	} else {
		// Fehlgeschlagen: wieder anklickbares Globus-Symbol (Retry ist möglich).
		it->shown = false;
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
