#ifndef CHATEMOTES_H
#define CHATEMOTES_H

#include <QString>
#include <QRegularExpression>
#include <QDateTime>
#include <configfile.h>

// Builds the timestamp prefix "[HH:mm:ss] " of a chat line – or an empty
// string if the user has hidden the timestamp via the setting
// (config key "ShowChatTimestamp", default on). Central place for the format,
// the key and the default, so that the game and lobby chat stay identical.
inline QString chatTimestampPrefix(ConfigFile *config)
{
	if (config && config->readConfigInt("ShowChatTimestamp") == 0)
		return QString();
	return QLatin1Char('[')
		   + QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"))
		   + QLatin1String("] ");
}

// Character classes of the emoji detection – shared by enlargeEmojis
// and isEmojiOnlyReaction, so that both have the same notion of an emoji.
//
// "Real" emoji code points. Covers the complete GitHub shortcode list
// (chat_emote_shortcode_table.h) – EXCEPT ©/®/™ (:copyright:/:registered:/
// :tm:): those are ordinary text characters as well ("Qt® …") and should be
// neither enlarged in the chat nor counted as an emoji reaction.
inline const QString &emojiCharClass()
{
	static const QString cls = QStringLiteral(
								   "\\x{1F000}-\\x{1FAFF}\\x{2600}-\\x{27BF}\\x{2B00}-\\x{2BFF}"
								   "\\x{2190}-\\x{21FF}\\x{2300}-\\x{23FF}\\x{2900}-\\x{297F}"
								   "\\x{203C}\\x{2049}\\x{2139}\\x{24C2}"
								   "\\x{25AA}\\x{25AB}\\x{25B6}\\x{25C0}\\x{25FB}-\\x{25FE}"
								   "\\x{3030}\\x{303D}\\x{3297}\\x{3299}");
	return cls;
}

// Joining/modifier characters (only meaningful together with emojis): skin
// tones, variation selectors, ZWJ, keycap combiners as well as tag characters
// (flags England/Scotland/Wales, e.g. 🏴󠁧󠁢󠁥󠁮󠁧󠁿 = 1F3F4 + E0067…E007F).
inline const QString &emojiJoinerClass()
{
	static const QString cls = QStringLiteral(
								   "\\x{1F3FB}-\\x{1F3FF}\\x{FE00}-\\x{FE0F}\\x{200D}\\x{20E3}"
								   "\\x{E0020}-\\x{E007F}");
	return cls;
}

// Enlarges Unicode emojis in an (already HTML formatted) chat line to
// ~22px – similar to the image emotes of the Qt widgets client. Contiguous
// emoji sequences (incl. variation selectors / ZWJ / keycaps) are wrapped into a
// larger font-size span. Used by the game and the lobby chat.
inline QString enlargeEmojis(const QString &html)
{
	// Keycap sequences (#⃣ 1⃣ …) as an alternative BEFORE the character class: their
	// base characters (#, *, digits) only belong into the span as part of the
	// complete sequence – otherwise the digit would stay small and only the
	// combiner would be enlarged (broken rendering). Bare digits do NOT match.
	static const QRegularExpression emojiRe(
		QStringLiteral("((?:[#*0-9]\\x{FE0F}?\\x{20E3}|[")
		+ emojiCharClass() + emojiJoinerClass() + QStringLiteral("])+)"));
	QString r = html;
	r.replace(emojiRe, QStringLiteral(
				  "<span style=\"font-size:22px; font-family:'Noto Color Emoji';\">\\1</span>"));
	return r;
}

// Checks whether a reaction payload ("/emoji <x>") consists exclusively of real
// emoji characters (incl. variation selectors, ZWJ, keycaps and skin tone
// modifiers) and contains at least one emoji. That way text messages disguised
// as a reaction ("/emoji haha") are discarded, while arbitrary real emojis (also
// outside the picker list) are allowed.
inline bool isEmojiOnlyReaction(const QString &text)
{
	if (text.isEmpty())
		return false;
	// Cut out the keycap sequences first: their base characters (#, *,
	// digits) are NOT emojis on their own (otherwise "/emoji 123" would be
	// valid) and only count as a complete sequence.
	static const QRegularExpression keycapRe(
		QStringLiteral("[#*0-9]\\x{FE0F}?\\x{20E3}"));
	QString t = text;
	t.remove(keycapRe);
	const bool hadKeycap = t.size() != text.size();
	if (t.isEmpty())
		return hadKeycap;                          // keycap emojis only
	static const QRegularExpression hasEmojiRe(
		QStringLiteral("[") + emojiCharClass() + QStringLiteral("]"));
	static const QRegularExpression foreignRe(
		QStringLiteral("[^") + emojiCharClass() + emojiJoinerClass() + QStringLiteral("]"));
	return (hasEmojiRe.match(t).hasMatch() || hadKeycap) // at least one emoji …
		   && !foreignRe.match(t).hasMatch();      // … and otherwise only emoji characters
}

#endif // CHATEMOTES_H
