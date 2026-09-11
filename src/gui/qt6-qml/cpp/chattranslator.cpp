#include "chattranslator.h"
#include "chattranslatorcore.h"

#include <QRegularExpression>
#include <QDateTime>

// Symbols. 🌐 = toggle (translation on/off), ⏳ = running. Built via the code
// point (not via a "\xF0…" literal – that would be read as Latin-1 and yield
// "ð…" mojibake instead of an emoji).
static const QString kGlobeGlyph   = QString::fromUcs4(U"\U0001F310"); // 🌐
static const QString kSpinnerGlyph = QString::fromUcs4(U"\U000023F3"); // ⏳
// Invisible placeholder for lines that are currently NOT under the mouse
// cursor. The anchor thus stays in the line (and the line findable via its href),
// but shows nothing. A non-breaking space, because normal
// spaces at the end of a line can be dropped by the rich text renderer.
static const QString kHiddenGlyph  = QStringLiteral("&nbsp;");

// Distance between two failure notices. If somebody clicks several lines while
// the service is down, the history should not fill up with the same
// notice.
static const qint64 kFailNoteIntervalMs = 60 * 1000;

// Hover mode: on the desktop the globe only appears on the line under the
// mouse cursor (otherwise the history was plastered with symbols). On touch
// devices there is no hover – there the symbols stay visible, otherwise the
// function would no longer be reachable.
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

void ChatTranslator::reset()
{
	// The history the line states refer to is gone – the lines themselves
	// (unlike in refreshEnabled) no longer have to be cleaned up. Core replies
	// still running then go nowhere afterwards:
	// onCoreTranslated discards unknown request ids. m_nextId keeps running,
	// so that old and new anchor ids never overlap.
	m_entries.clear();
	m_reqToLine.clear();
	m_hoveredId = 0;
}

QString ChatTranslator::anchorFor(int id, const QString &glyph)
{
	// text-decoration:none, so that the symbol does not appear as an underlined
	// link. The scheme prefix "pokerthtranslate:" is intercepted by the ChatBox
	// (not opened externally). The symbol is rendered like a chat emoji
	// (Noto Color Emoji), but deliberately a bit smaller than the
	// message emojis (enlargeEmojis: 22px) – clearly recognisable without dominating
	// the line. The size is fixed, i.e. independent of the chat text size.
	return QStringLiteral("<a href=\"pokerthtranslate:%1\" style=\"text-decoration:none;\">"
						  "<span style=\"font-size:15px; font-family:'Noto Color Emoji';\">%2</span></a>")
		   .arg(id)
		   .arg(glyph);
}

QString ChatTranslator::glyphFor(const Pending &p, int id) const
{
	if (p.inFlight)
		return kSpinnerGlyph;
	// Visible while the translation is shown (it indicates that the line is
	// translated, and is the way back to the original) – otherwise only on the
	// line under the mouse cursor.
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
	// The href is constant across all symbol states -> a unique line identifier.
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

	// Line -> anchor id: the href stands as plain text in the (HTML) line.
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
	// Hide the symbol on the old line, show it on the new one.
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
	const QString from = it->shown ? tb : it->bodyHtml;   // currently in the line
	const QString to   = shown    ? tb : it->bodyHtml;
	QString &line = (*m_chatLog)[idx];
	const int pos = line.indexOf(from);
	if (pos < 0)
		return;
	line.replace(pos, from.size(), to);
	it->shown = shown;
	emit chatLogMutated();
	// The symbol depends on the "shown" state (a shown translation keeps the
	// globe as the way back, even when the mouse moves on).
	updateGlobe(id);
}

void ChatTranslator::requestTranslation(int id)
{
	auto it = m_entries.find(id);
	if (!enabled() || it == m_entries.end() || it->inFlight)
		return;

	// Toggle: hide a shown translation again (show the original).
	if (it->shown) {
		setBodyShown(id, false);
		return;
	}
	// Already translated once -> show it from the cache (no new request).
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

	const bool haveText = ok && !translated.trimmed().isEmpty();
	if (haveText) {
		it->translated = translated;
		setBodyShown(id, true); // Show the translation (it replaces the original)
	}
	// Spinner back to the globe – or to the invisible placeholder, in case
	// the line is meanwhile neither shown nor under the mouse.
	// AFTER setBodyShown, because the symbol depends on the "shown" state.
	updateGlobe(id);
	// On an error the original stays (the globe allows another
	// attempt) – plus a notice, otherwise the user only sees a briefly
	// flashing hourglass and thinks the function is broken.
	if (!haveText)
		postFailureNote();
}

void ChatTranslator::postFailureNote()
{
	if (!m_chatLog)
		return;
	const qint64 now = QDateTime::currentMSecsSinceEpoch();
	if (m_lastFailNoteMs != 0 && (now - m_lastFailNoteMs) < kFailNoteIntervalMs)
		return;
	m_lastFailNoteMs = now;

	// Deliberately WITHOUT colour placeholders and without a timestamp: the same line
	// ends up in the lobby history AND in the table chat, and both expand their roles
	// from different sources (app palette or table theme). A neutral
	// grey is readable in both. One line = one entry – the mapping by which
	// setHoveredLine finds the symbols thus stays untouched.
	m_chatLog->append(QStringLiteral("<i><span style=\"color:#9e9e9e;\">")
					  + tr("Translation is currently unavailable. Please try again later.").toHtmlEscaped()
					  + QStringLiteral("</span></i>"));
	emit chatLogMutated();
}

void ChatTranslator::refreshEnabled()
{
	emit enabledChanged();
	if (enabled())
		return; // Enabling affects new messages (decorate()); existing ones stay.

	// Disabled: in every affected line replace the translation (if visible)
	// by the original and remove the globe symbol.
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
