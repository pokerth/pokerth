#ifndef _CHATTRANSLATOR_H_
#define _CHATTRANSLATOR_H_

#include <QObject>
#include <QStringList>
#include <QHash>

class ConfigFile;
class ChatTranslatorCore;

/* QML-seitige Chat-Übersetzung. Wird pro Chat-Handler (Lobby/Game) instanziiert
 * und operiert direkt auf dessen chatLog-Liste:
 *
 *   • decorate() hängt an jede übersetzbare Zeile ein Globus-Symbol an – als
 *     spezieller Link "pokerthtranslate:<id>", den die ChatBox abfängt (das
 *     Icon-pro-Zeile-Muster des Web-Clients, ohne den Single-RichText-Chat
 *     umzubauen). Sichtbar ist das Symbol nur an der Zeile unter dem Maus-
 *     zeiger (setHoveredLine) – der Anker selbst steht immer in der Zeile,
 *     trägt aber sonst nur einen unsichtbaren Platzhalter.
 *   • requestTranslation() startet – vom QML beim Antippen des Symbols – die
 *     asynchrone Übersetzung und ersetzt das Symbol in genau dieser Zeile
 *     durch die Übersetzung.
 *
 * Die eigentliche Netzwerk-/Dienst-Logik liegt in ChatTranslatorCore (mit dem
 * Widgets-Client geteilt); diese Klasse kümmert sich nur um das Einbetten und
 * Ersetzen des Symbols in der QStringList-Chatliste.
 */
class ChatTranslator : public QObject
{
	Q_OBJECT
	// Global an/aus (Config "AllowChatTranslation"). Steuert, ob decorate() das
	// Symbol überhaupt einbettet; für QML nur informativ.
	Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
public:
	// chatLog: die formatierte Zeilenliste des besitzenden Handlers. Muss den
	// ChatTranslator überleben (üblicherweise ein Member desselben Handlers).
	explicit ChatTranslator(QStringList *chatLog, QObject *parent = nullptr);

	void setConfig(ConfigFile *config);
	bool enabled() const;

	// Vom besitzenden Handler aufgerufen, wenn dessen chatLog geleert wurde
	// (neue Verbindung/neuer Login): verwirft alle Zeilen-Zustände.
	void reset();

	// Hängt (falls aktiviert) das Globus-Symbol an eine frisch gebaute Chat-
	// Zeile und merkt sich Rohtext + Nachrichtenkörper für die spätere
	// Übersetzung. bodyHtml ist der exakte HTML-Teilstring der Nachricht
	// innerhalb von formattedLine (styledMsg bzw. escapedMsg) – er wird beim
	// Einblenden durch die Übersetzung ERSETZT. Gibt die Zeile unverändert
	// zurück, wenn die Funktion deaktiviert ist oder der Quelltext leer ist.
	QString decorate(const QString &formattedLine, const QString &sourceText,
	                 const QString &bodyHtml);

	// Vom QML aufgerufen, wenn auf das Globus-Symbol getippt wird.
	Q_INVOKABLE void requestTranslation(int id);

	// Vom QML gemeldete Chat-Zeile (Index in chatLog) unter dem Mauszeiger;
	// -1 = keine. Nur an dieser Zeile erscheint das Globus-Symbol (auf Touch-
	// Plattformen ohne Hover wirkungslos – dort bleiben alle Symbole sichtbar).
	Q_INVOKABLE void setHoveredLine(int lineIndex);

	// Vom QML aufgerufen, wenn sich der Config-Schalter "AllowChatTranslation"
	// ändert. Wendet den neuen Zustand auf den SICHTBAREN Verlauf an: bei
	// Deaktivierung werden alle vorhandenen Globus-Symbole/Übersetzungen sofort
	// entfernt (neue Nachrichten regelt decorate() ohnehin live).
	Q_INVOKABLE void refreshEnabled();

signals:
	void enabledChanged();
	// Eine Zeile in chatLog wurde verändert – der Handler verbindet dies mit
	// seinem eigenen chatLogChanged(), damit die QML-Bindung neu rendert.
	void chatLogMutated();

private slots:
	void onCoreTranslated(int requestId, const QString &text, bool ok);

private:
	void finish(int id, const QString &translated, bool ok);
	// Index der chatLog-Zeile, die den Globus-Anker dieser id enthält (-1, wenn
	// nicht mehr vorhanden, z. B. aus dem 400-Zeilen-Verlauf herausgetrimmt).
	int findLineIndex(int id) const;
	// Setzt das Symbol der Zeile auf den aktuellen Zustand (Spinner / Globus /
	// unsichtbarer Platzhalter) – no-op, wenn sich dadurch nichts ändert.
	void updateGlobe(int id);
	// Hängt einen lokalen Hinweis an den Verlauf, wenn BEIDE Übersetzungsdienste
	// ausgefallen sind. Ohne ihn springt das Symbol nur wortlos von der Sanduhr
	// zurück auf den Globus – für den Nutzer nicht von "kaputt" unterscheidbar.
	// Gedrosselt, damit mehrere Klicks nicht denselben Hinweis wiederholen.
	void postFailureNote();
	// Blendet die Übersetzung ein/aus, indem der Nachrichtenkörper zwischen
	// Original und Übersetzung ERSETZT wird.
	void setBodyShown(int id, bool shown);
	static QString anchorFor(int id, const QString &glyph);

	struct Pending {
		QString sourceText;     // Rohtext der Nachricht (vor HTML/Style-Markup)
		QString bodyHtml;       // Original-Nachrichtenkörper (HTML) in der Zeile
		QString currentAnchor;  // exaktes Globus-Anker-HTML, das gerade in der Zeile steht
		QString translated;     // gecachte Übersetzung (leer = noch nicht geholt)
		bool inFlight = false;
		bool shown = false;     // Übersetzung aktuell eingeblendet? (Toggle)
	};

	// Symbol, das die Zeile im aktuellen Zustand tragen soll.
	QString glyphFor(const Pending &p, int id) const;

	QStringList *m_chatLog;
	ChatTranslatorCore *m_core;
	QHash<int, Pending> m_entries;   // Zeilen-id -> Zustand
	QHash<int, int> m_reqToLine;     // Core-Request-id -> Zeilen-id
	int m_nextId = 1;
	int m_hoveredId = 0;             // Zeile unter dem Mauszeiger (0 = keine)
	qint64 m_lastFailNoteMs = 0;     // Zeitpunkt des letzten Fehlschlag-Hinweises
};

#endif // _CHATTRANSLATOR_H_
