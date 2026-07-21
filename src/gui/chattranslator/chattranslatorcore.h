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

	// Zielsprache als API-Code ("de", "pt-BR", "zh-CN", …). Quelle ist der
	// ConfigFile-Key "Language", den beide Clients pflegen.
	QString targetLang() const;

	// Normalisiert einen Sprachcode auf einen von den Diensten verstandenen
	// Code. Deckt QML-Locales ("de_DE") ebenso ab wie die PokerTH-Kürzel der
	// Widget-ts-Dateien ("cz", "dk", "gr", "jp", "ptbr", "zhcn" …), die teils
	// von ISO 639-1 abweichen.
	static QString normalizeLangCode(const QString &raw);

	// Baut das HTML für die eingeblendete Übersetzung, die den Originaltext an
	// gleicher Stelle ERSETZT. Übernimmt – wenn möglich – den umschließenden
	// <span> (und damit die Farbe) der Originalnachricht, damit die Übersetzung
	// im selben Look erscheint (theme-/tischfarben-korrekt), nur kursiv als
	// Kennzeichnung. Von beiden Clients genutzt.
	static QString styledTranslation(const QString &originalBodyHtml,
	                                 const QString &translated);

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
