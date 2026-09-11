#ifndef _CHATTRANSLATOR_H_
#define _CHATTRANSLATOR_H_

#include <QObject>
#include <QStringList>
#include <QHash>

class ConfigFile;
class ChatTranslatorCore;

/* Chat translation on the QML side. It is instantiated per chat handler (lobby/game)
 * and operates directly on its chatLog list:
 *
 *   • decorate() appends a globe symbol to every translatable line – as
 *     a special link "pokerthtranslate:<id>" that the ChatBox intercepts (the
 *     icon-per-line pattern of the web client, without rebuilding the
 *     single-rich-text chat). The symbol is only visible on the line under the
 *     mouse cursor (setHoveredLine) – the anchor itself is always in the line,
 *     but otherwise carries only an invisible placeholder.
 *   • requestTranslation() starts – from QML when the symbol is tapped – the
 *     asynchronous translation and replaces the symbol in exactly that line
 *     with the translation.
 *
 * The actual network/service logic lives in ChatTranslatorCore (shared with the
 * widgets client); this class only takes care of embedding and
 * replacing the symbol in the QStringList chat list.
 */
class ChatTranslator : public QObject
{
	Q_OBJECT
	// Globally on/off (config "AllowChatTranslation"). Controls whether decorate()
	// embeds the symbol at all; for QML only informative.
	Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
public:
	// chatLog: the formatted line list of the owning handler. It has to outlive
	// the ChatTranslator (usually a member of the same handler).
	explicit ChatTranslator(QStringList *chatLog, QObject *parent = nullptr);

	void setConfig(ConfigFile *config);
	bool enabled() const;

	// Called by the owning handler when its chatLog was cleared
	// (new connection/new login): discards all line states.
	void reset();

	// Appends (if enabled) the globe symbol to a freshly built chat
	// line and remembers the raw text + the message body for the later
	// translation. bodyHtml is the exact HTML substring of the message
	// inside formattedLine (styledMsg or escapedMsg) – it is REPLACED by the
	// translation when that is shown. Returns the line unchanged
	// if the function is disabled or the source text is empty.
	QString decorate(const QString &formattedLine, const QString &sourceText,
					 const QString &bodyHtml);

	// Called from QML when the globe symbol is tapped.
	Q_INVOKABLE void requestTranslation(int id);

	// Chat line (index in chatLog) under the mouse cursor as reported by QML;
	// -1 = none. The globe symbol only appears on this line (without effect on touch
	// platforms without hover – there all symbols stay visible).
	Q_INVOKABLE void setHoveredLine(int lineIndex);

	// Called from QML when the config switch "AllowChatTranslation"
	// changes. Applies the new state to the VISIBLE history: on
	// deactivation all existing globe symbols/translations are removed
	// immediately (new messages are handled live by decorate() anyway).
	Q_INVOKABLE void refreshEnabled();

signals:
	void enabledChanged();
	// A line in chatLog has changed – the handler connects this to
	// its own chatLogChanged(), so that the QML binding renders anew.
	void chatLogMutated();

private slots:
	void onCoreTranslated(int requestId, const QString &text, bool ok);

private:
	void finish(int id, const QString &translated, bool ok);
	// Index of the chatLog line that contains the globe anchor of this id (-1 if
	// it no longer exists, e.g. trimmed out of the 400 line history).
	int findLineIndex(int id) const;
	// Sets the symbol of the line to the current state (spinner / globe /
	// invisible placeholder) – a no-op if nothing changes by that.
	void updateGlobe(int id);
	// Appends a local notice to the history when BOTH translation services
	// have failed. Without it the symbol jumps back wordlessly from the hourglass
	// to the globe – indistinguishable from "broken" for the user.
	// Throttled so that several clicks do not repeat the same notice.
	void postFailureNote();
	// Shows/hides the translation by REPLACING the message body between
	// the original and the translation.
	void setBodyShown(int id, bool shown);
	static QString anchorFor(int id, const QString &glyph);

	struct Pending {
		QString sourceText;     // raw text of the message (before the HTML/style markup)
		QString bodyHtml;       // original message body (HTML) in the line
		QString currentAnchor;  // the exact globe anchor HTML currently standing in the line
		QString translated;     // cached translation (empty = not fetched yet)
		bool inFlight = false;
		bool shown = false;     // translation currently shown? (toggle)
	};

	// The symbol the line should carry in its current state.
	QString glyphFor(const Pending &p, int id) const;

	QStringList *m_chatLog;
	ChatTranslatorCore *m_core;
	QHash<int, Pending> m_entries;   // Zeilen-id -> Zustand
	QHash<int, int> m_reqToLine;     // Core-Request-id -> Zeilen-id
	int m_nextId = 1;
	int m_hoveredId = 0;             // line under the mouse cursor (0 = none)
	qint64 m_lastFailNoteMs = 0;     // time of the last failure notice
};

#endif // _CHATTRANSLATOR_H_
