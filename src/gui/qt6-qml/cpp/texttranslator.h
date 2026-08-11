#ifndef _TEXTTRANSLATOR_H_
#define _TEXTTRANSLATOR_H_

#include <QObject>
#include <QString>

class ConfigFile;
class ChatTranslatorCore;

/* Übersetzung beliebiger Texte für QML. Dünne Hülle um ChatTranslatorCore
 * (derselbe Dienst und dieselbe Zielsprache wie die Chat-Übersetzung), aber
 * ohne dessen Chat-Zeilen-Bezug: ChatTranslator operiert auf der chatLog-Liste
 * eines Handlers und taugt deshalb nicht für Texte außerhalb des Chats.
 *
 * Genutzt vom Globus-Symbol der Forum-Beitragsseite (ForumPostPage). Als
 * Kontext-Property "Translator" global registriert (pokerth.cpp).
 *
 *   var id = Translator.translate(text)      // -> Request-ID
 *   Connections { target: Translator
 *       function onTranslated(requestId, text, ok) { … } }
 *
 * Es wird erst etwas gesendet, wenn der Nutzer das Symbol antippt; der globale
 * Schalter ist derselbe wie beim Chat (Config "AllowChatTranslation").
 */
class TextTranslator : public QObject
{
	Q_OBJECT
	// Global an/aus (Config "AllowChatTranslation") – steuert die Sichtbarkeit
	// des Globus-Symbols.
	Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
public:
	explicit TextTranslator(ConfigFile *config, QObject *parent = nullptr);

	bool enabled() const;

	// Startet die Übersetzung in die eingestellte Client-Sprache und liefert
	// die Request-ID; das Ergebnis kommt asynchron über translated().
	Q_INVOKABLE int translate(const QString &text);

	// Vom Einstellungsdialog aufgerufen, wenn "AllowChatTranslation" umgelegt
	// wurde – meldet den neuen Zustand an die QML-Bindungen.
	Q_INVOKABLE void refreshEnabled();

signals:
	void enabledChanged();
	void translated(int requestId, const QString &text, bool ok);

private:
	ChatTranslatorCore *m_core;
};

#endif // _TEXTTRANSLATOR_H_
