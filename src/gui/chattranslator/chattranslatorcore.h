/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *****************************************************************************/
#ifndef _CHATTRANSLATORCORE_H_
#define _CHATTRANSLATORCORE_H_

#include <QObject>
#include <QString>
#include <QHash>
#include <QNetworkAccessManager>

class ConfigFile;
class QNetworkReply;

/* Toolkit-unabhängiger Netzwerk-Kern der Chat-Übersetzung. Von BEIDEN Clients
 * genutzt (QML: ChatTranslator; Widgets: ChatTools), damit die Dienst-/Sprach-
 * Logik nur an einer Stelle existiert.
 *
 * Zielsprache = im Client eingestellte Sprache (Config "Language").
 * Primärquelle: Google-Translate "gtx"-Endpoint (kein Key, Auto-Quellsprache,
 * Anfrage von der Client-IP). Fallback: MyMemory. Rechtliche Hinweise zu beiden
 * Diensten: docs/third_party_services.md.
 *
 * Der Kern kennt weder Chat-Zeilen noch HTML – er nimmt Text entgegen und
 * liefert die Übersetzung asynchron über translated() zurück. Das Einbetten des
 * Symbols und das Ersetzen in der jeweiligen Anzeige übernimmt der Aufrufer.
 */
class ChatTranslatorCore : public QObject
{
	Q_OBJECT
public:
	explicit ChatTranslatorCore(ConfigFile *config, QObject *parent = nullptr);

	void setConfig(ConfigFile *config);
	// Global an/aus (Config "AllowChatTranslation").
	bool enabled() const;

	// Startet eine Übersetzung des Textes in die Client-Sprache. Liefert eine
	// Request-ID; das Ergebnis kommt asynchron über translated(requestId, …).
	int translate(const QString &text);

	// Config "Language" (z. B. "de_DE") -> API-Sprachcode ("de", "pt-BR", …).
	QString targetLang() const;

signals:
	void translated(int requestId, const QString &text, bool ok);

private slots:
	void onPrimaryReply();
	void onFallbackReply();

private:
	void startPrimary(int id, const QString &text);
	void startFallback(int id, const QString &text);

	ConfigFile *m_config;
	QNetworkAccessManager m_nam;
	QHash<int, QString> m_sourceById;  // Rohtext je Request (für den Fallback)
	int m_nextId = 1;
};

#endif // _CHATTRANSLATORCORE_H_
