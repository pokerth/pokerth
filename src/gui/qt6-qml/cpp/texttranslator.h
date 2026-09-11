#ifndef _TEXTTRANSLATOR_H_
#define _TEXTTRANSLATOR_H_

#include <QObject>
#include <QString>

class ConfigFile;
class ChatTranslatorCore;

/* Translation of arbitrary texts for QML. A thin shell around ChatTranslatorCore
 * (the same service and the same target language as the chat translation), but
 * without its relation to chat lines: ChatTranslator operates on the chatLog
 * list of a handler and is therefore unsuitable for texts outside the chat.
 *
 * Used by the globe symbol of the forum post page (ForumPostPage). Registered
 * globally as the context property "Translator" (pokerth.cpp).
 *
 *   var id = Translator.translate(text)      // -> request ID
 *   Connections { target: Translator
 *       function onTranslated(requestId, text, ok) { … } }
 *
 * Nothing is sent before the user taps the symbol; the global switch is the
 * same one as for the chat (config "AllowChatTranslation").
 */
class TextTranslator : public QObject
{
	Q_OBJECT
	// Globally on/off (config "AllowChatTranslation") – controls the visibility
	// of the globe symbol.
	Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
public:
	explicit TextTranslator(ConfigFile *config, QObject *parent = nullptr);

	bool enabled() const;

	// Starts the translation into the configured client language and returns
	// the request ID; the result arrives asynchronously via translated().
	Q_INVOKABLE int translate(const QString &text);

	// Called by the settings dialog when "AllowChatTranslation" has been
	// toggled – reports the new state to the QML bindings.
	Q_INVOKABLE void refreshEnabled();

signals:
	void enabledChanged();
	void translated(int requestId, const QString &text, bool ok);

private:
	ChatTranslatorCore *m_core;
};

#endif // _TEXTTRANSLATOR_H_
