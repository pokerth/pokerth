#ifndef _CHATTRANSLATOR_H_
#define _CHATTRANSLATOR_H_

#include <QObject>
#include <QStringList>
#include <QHash>
#include <QNetworkAccessManager>

class ConfigFile;
class QNetworkReply;

/* Übersetzt einzelne Chat-Zeilen über kostenlose, keyfreie Web-Dienste. Wird
 * pro Chat-Handler (Lobby/Game) instanziiert und operiert direkt auf dessen
 * chatLog-Liste:
 *
 *   • decorate() hängt an jede übersetzbare Zeile ein Globus-Symbol an – als
 *     spezieller Link "pokerthtranslate:<id>", den die ChatBox abfängt (das
 *     Icon-pro-Zeile-Muster des Web-Clients, ohne den Single-RichText-Chat
 *     umzubauen).
 *   • requestTranslation() startet – vom QML beim Antippen des Symbols – die
 *     asynchrone Übersetzung und ersetzt das Symbol in genau dieser Zeile
 *     durch die Übersetzung.
 *
 * Zielsprache ist die im Client eingestellte Sprache (Config "Language").
 * Primärquelle: Google-Translate "gtx"-Endpoint (kein Key, Auto-Quellsprache,
 * die Anfrage kommt von der Client-IP). Fallback: MyMemory. Rechtliche Hinweise
 * zu beiden Diensten: docs/third_party_services.md.
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

	// Hängt (falls aktiviert) das Globus-Symbol an eine frisch gebaute Chat-
	// Zeile und merkt sich den Rohtext für die spätere Übersetzung. Gibt die
	// Zeile unverändert zurück, wenn die Funktion deaktiviert ist oder der
	// Quelltext leer ist.
	QString decorate(const QString &formattedLine, const QString &sourceText);

	// Vom QML aufgerufen, wenn auf das Globus-Symbol getippt wird.
	Q_INVOKABLE void requestTranslation(int id);

signals:
	void enabledChanged();
	// Eine Zeile in chatLog wurde verändert – der Handler verbindet dies mit
	// seinem eigenen chatLogChanged(), damit die QML-Bindung neu rendert.
	void chatLogMutated();

private slots:
	void onPrimaryReply();
	void onFallbackReply();

private:
	QString targetLang() const;            // Config "Language" -> BCP47/2-Buchstaben
	void startPrimary(int id);
	void startFallback(int id);
	void finish(int id, const QString &translated, bool ok);
	// Ersetzt in der zugehörigen chatLog-Zeile das aktuell eingebettete Anchor-
	// HTML durch newAnchor. Liefert false, wenn die Zeile nicht (mehr) existiert
	// (z. B. aus dem 400-Zeilen-Verlauf herausgetrimmt).
	bool replaceAnchor(int id, const QString &newAnchor);
	static QString anchorFor(int id, const QString &glyph);

	struct Pending {
		QString sourceText;     // Rohtext der Nachricht (vor HTML/Style-Markup)
		QString currentAnchor;  // exaktes Anchor-HTML, das gerade in der Zeile steht
		bool inFlight = false;
	};

	QStringList *m_chatLog;
	ConfigFile *m_config = nullptr;
	QNetworkAccessManager m_nam;
	QHash<int, Pending> m_entries;
	int m_nextId = 1;
};

#endif // _CHATTRANSLATOR_H_
