#include "texttranslator.h"
#include "chattranslatorcore.h"

TextTranslator::TextTranslator(ConfigFile *config, QObject *parent)
	: QObject(parent)
	, m_core(new ChatTranslatorCore(config, this))
{
	connect(m_core, &ChatTranslatorCore::translated,
			this, &TextTranslator::translated);
}

bool TextTranslator::enabled() const
{
	return m_core->enabled();
}

int TextTranslator::translate(const QString &text)
{
	if (!m_core->enabled() || text.trimmed().isEmpty())
		return -1;
	return m_core->translate(text);
}

void TextTranslator::refreshEnabled()
{
	// enabled() reads the config value live; here only the binding is
	// re-evaluated (the switch lives in the settings dialog).
	emit enabledChanged();
}
