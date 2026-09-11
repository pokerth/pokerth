#ifndef CHAT_EMOTE_SHORTCUTS_H
#define CHAT_EMOTE_SHORTCUTS_H

#include <QString>
#include <QLatin1String>
#include <QStringList>
#include <QHash>
#include <QRegularExpression>

#include "chat_emote_shortcode_table.h"

// Converts ASCII emoticon shortcuts (":-)", "8-)", "<3", ">_<", …) into the
// corresponding Unicode emojis. Shared by the Qt widgets client
// (chattools.cpp) and the QML client (gamehandler.cpp / lobbyhandler.cpp),
// so that both know the same, as comprehensive as possible set of shortcuts.
//
// IMPORTANT:
//  * The function expects text that is already HTML escaped and is applied BEFORE
//    style/link markup is added. That way short shortcuts never hit
//    our own markup such as "color:#..." or "font-weight:bold".
//  * '<' / '>' are present as "&lt;" / "&gt;" – shortcuts with arrows are
//    therefore matched in their escaped form.
//  * Order: longer or more specific shortcuts (angel/devil/laughing,
//    ">:(" etc.) MUST be replaced before their shorter prefixes (":)", ":("),
//    otherwise the shorter shortcut eats a part of the longer one.
//  * Deliberately NOT contained are ambiguous shortcuts that occur frequently
//    in normal text: ":0"/":3" (times of day), ":\\"/"D:" (Windows paths),
//    a bare "8)"/"B)" (e.g. "(plan B)").
// Discord/GitHub style shortcodes (":smile:", ":moon:", ":fire:", …) →
// Unicode emoji. The complete GitHub shortcode list
// (https://gist.github.com/rxaviers/7360908), generated in
// chat_emote_shortcode_table.h. Accessible separately, so that the chat
// auto-completion (ChatBox.qml, via LobbyHandler::chatEmoteShortcodes)
// offers EXACTLY the codes that applyChatEmoteShortcuts really replaces.
inline const QHash<QString, QString> &chatEmoteShortcodeMap()
{
	static const QHash<QString, QString> shortcodes = buildChatEmoteShortcodeMap();
	return shortcodes;
}

inline QString applyChatEmoteShortcuts(QString text)
{
	auto emo = [](char32_t cp) -> QString { return QString::fromUcs4(&cp, 1); };

	// ── Protect URLs (they take precedence over emote shortcuts) ────────────
	// Character sequences in links (e.g. "?v=Dxyz" → "=D", "…xD…", "…:P…") must
	// NOT become emojis. http/https URLs are therefore cut out of the text before
	// the replacement, replaced by a collision free placeholder
	// (the control characters \x01<index>\x02) and reinserted unchanged
	// at the end. The text is already HTML escaped; "\S+" therefore matches
	// "&amp;" in query strings as well.
	static const QRegularExpression urlRe(QStringLiteral("https?://\\S+"));
	QStringList savedUrls;
	{
		QRegularExpressionMatchIterator it = urlRe.globalMatch(text);
		if (it.hasNext()) {
			QString out;
			out.reserve(text.size());
			int last = 0;
			while (it.hasNext()) {
				const QRegularExpressionMatch m = it.next();
				out += text.mid(last, m.capturedStart() - last);
				out += QChar(0x0001) + QString::number(savedUrls.size()) + QChar(0x0002);
				savedUrls << m.captured();
				last = m.capturedEnd();
			}
			out += text.mid(last);
			text = out;
		}
	}

	// ── Discord/GitHub style shortcodes (":smile:", ":moon:", ":fire:", …) ──
	// MUST run before the ASCII shortcuts: otherwise e.g. ":s" from ":smile:"
	// would turn into 😟 prematurely. Only lowercase names ([a-z0-9_+-]) between two
	// colons match – so ":D"/":P"/":)" etc. do NOT collide with it.
	// Unknown codes (":foobar:") stay unchanged.
	{
		const QHash<QString, QString> &shortcodes = chatEmoteShortcodeMap();
		static const QRegularExpression scRe(QStringLiteral(":([a-z0-9_+-]+):"));
		QRegularExpressionMatchIterator it = scRe.globalMatch(text);
		if (it.hasNext()) {
			QString out;
			out.reserve(text.size());
			int last = 0;
			while (it.hasNext()) {
				const QRegularExpressionMatch m = it.next();
				out += text.mid(last, m.capturedStart() - last);
				const auto found = shortcodes.constFind(m.captured(1));
				out += (found != shortcodes.constEnd()) ? *found : m.captured(0);
				last = m.capturedEnd();
			}
			out += text.mid(last);
			text = out;
		}
	}

	// ── Protect HTML entities (except &lt;/&gt;) ───────────────────────────
	// The text is HTML escaped; toHtmlEscaped() produces "&quot;" and
	// "&amp;" among others. Their closing ';' must NOT form the beginning of a shortcut such as
	// ";D"/";)"/";P" – otherwise e.g. '"D' → "&quot;D" would wrongly become
	// '&quot😜'. Entities are therefore (like URLs) replaced by placeholders
	// (\x03<index>\x04) and reinserted unchanged at the end.
	// EXCLUDED are "&lt;"/"&gt;": arrow shortcuts ("&lt;3", "&gt;:)",
	// "&gt;_&lt;") match deliberately on their escaped form.
	static const QRegularExpression entityRe(QStringLiteral(
				"&(?!lt;)(?!gt;)(?:[a-zA-Z][a-zA-Z0-9]*|#[0-9]+|#x[0-9a-fA-F]+);"));
	QStringList savedEntities;
	{
		QRegularExpressionMatchIterator it = entityRe.globalMatch(text);
		if (it.hasNext()) {
			QString out;
			out.reserve(text.size());
			int last = 0;
			while (it.hasNext()) {
				const QRegularExpressionMatch m = it.next();
				out += text.mid(last, m.capturedStart() - last);
				out += QChar(0x0003) + QString::number(savedEntities.size()) + QChar(0x0004);
				savedEntities << m.captured();
				last = m.capturedEnd();
			}
			out += text.mid(last);
			text = out;
		}
	}

	// ── Angel / devil (they contain ":-)" or ":)") ──
	text.replace(QLatin1String("0:-)"),     emo(0x1F607)); // 😇 angel
	text.replace(QLatin1String("0:)"),      emo(0x1F607)); // 😇
	text.replace(QLatin1String("O:-)"),     emo(0x1F607)); // 😇
	text.replace(QLatin1String("O:)"),      emo(0x1F607)); // 😇
	text.replace(QLatin1String("&gt;:-)"),  emo(0x1F608)); // 😈 devilish (>:-))
	text.replace(QLatin1String("&gt;:)"),   emo(0x1F608)); // 😈 (>:))
	text.replace(QLatin1String("}:-)"),     emo(0x1F608)); // 😈
	text.replace(QLatin1String("}:)"),      emo(0x1F608)); // 😈

	// ── Laughing (contains ":-)" / ":)") ──
	text.replace(QLatin1String(":-))"),     emo(0x1F602)); // 😂 laughing
	text.replace(QLatin1String(":))"),      emo(0x1F602)); // 😂

	// ── Angry / crying (contain ":(" / ":-(") ──
	text.replace(QLatin1String("&gt;:-("),  emo(0x1F621)); // 😡 angry (>:-()
	text.replace(QLatin1String("&gt;:("),   emo(0x1F621)); // 😡 (>:()
	text.replace(QLatin1String(":'-("),     emo(0x1F622)); // 😢 crying
	text.replace(QLatin1String(":'("),      emo(0x1F622)); // 😢

	// ── Tears of joy (before ":)" – but it does not collide) ──
	text.replace(QLatin1String(":'-)"),     emo(0x1F972)); // 🥲 happy tears
	text.replace(QLatin1String(":')"),      emo(0x1F972)); // 🥲
	text.replace(QLatin1String(":'D"),      emo(0x1F602)); // 😂

	// ── Big grin / laughing ──
	text.replace(QLatin1String(":-D"),      emo(0x1F603)); // 😃 big grin
	text.replace(QLatin1String(":D"),       emo(0x1F603)); // 😃
	text.replace(QLatin1String("=D"),       emo(0x1F604)); // 😄
	text.replace(QLatin1String("xD"),       emo(0x1F606)); // 😆 laughing
	text.replace(QLatin1String("XD"),       emo(0x1F606)); // 😆

	// ── Smile ──
	text.replace(QLatin1String(":-)"),      emo(0x1F60A)); // 😊 smile
	text.replace(QLatin1String(":)"),       emo(0x1F60A)); // 😊
	text.replace(QLatin1String("=)"),       emo(0x1F642)); // 🙂
	text.replace(QLatin1String(":]"),       emo(0x1F642)); // 🙂
	text.replace(QLatin1String("^_^"),      emo(0x1F604)); // 😄 happy
	text.replace(QLatin1String("^^"),       emo(0x1F604)); // 😄

	// ── Zwinkern ──
	text.replace(QLatin1String(";-)"),      emo(0x1F609)); // 😉 wink
	text.replace(QLatin1String(";)"),       emo(0x1F609)); // 😉
	text.replace(QLatin1String(";-D"),      emo(0x1F61C)); // 😜 winking grin
	text.replace(QLatin1String(";D"),       emo(0x1F61C)); // 😜

	// ── Zunge ──
	text.replace(QLatin1String(":-P"),      emo(0x1F61B)); // 😛 tongue
	text.replace(QLatin1String(":P"),       emo(0x1F61B)); // 😛
	text.replace(QLatin1String(":-p"),      emo(0x1F61B)); // 😛
	text.replace(QLatin1String(":p"),       emo(0x1F61B)); // 😛
	text.replace(QLatin1String("=P"),       emo(0x1F61B)); // 😛
	text.replace(QLatin1String(":-b"),      emo(0x1F61B)); // 😛
	text.replace(QLatin1String(";-P"),      emo(0x1F61C)); // 😜 winking tongue
	text.replace(QLatin1String(";P"),       emo(0x1F61C)); // 😜

	// ── Cool (nose variants only; "8)"/"B)" would be too collision prone) ──
	text.replace(QLatin1String("B-)"),      emo(0x1F60E)); // 😎 cool
	text.replace(QLatin1String("8-)"),      emo(0x1F60E)); // 😎

	// ── Traurig ──
	text.replace(QLatin1String(":-("),      emo(0x1F61E)); // 😞 sad
	text.replace(QLatin1String(":("),       emo(0x1F61E)); // 😞
	text.replace(QLatin1String("=("),       emo(0x1F61E)); // 😞
	text.replace(QLatin1String(":["),       emo(0x1F61E)); // 😞

	// ── Embarrassed / neutral / skeptical ──
	text.replace(QLatin1String(":-["),      emo(0x1F633)); // 😳 embarrassed
	text.replace(QLatin1String(":-|"),      emo(0x1F610)); // 😐 neutral
	text.replace(QLatin1String(":|"),       emo(0x1F610)); // 😐
	text.replace(QLatin1String(":-/"),      emo(0x1F615)); // 😕 skeptical
	text.replace(QLatin1String(":-\\"),     emo(0x1F615)); // 😕
	// ":/" is harmless now – "http(s)://" URLs were protected above.
	text.replace(QLatin1String(":/"),       emo(0x1F615)); // 😕

	// ── Worried / sick / surprised ──
	text.replace(QLatin1String(":-S"),      emo(0x1F61F)); // 😟 worried
	text.replace(QLatin1String(":-s"),      emo(0x1F61F)); // 😟
	text.replace(QLatin1String(":S"),       emo(0x1F61F)); // 😟
	text.replace(QLatin1String(":s"),       emo(0x1F61F)); // 😟
	text.replace(QLatin1String(":-&"),      emo(0x1F912)); // 🤒 sick
	text.replace(QLatin1String(":-O"),      emo(0x1F62E)); // 😮 surprised
	text.replace(QLatin1String(":-o"),      emo(0x1F62E)); // 😮
	text.replace(QLatin1String(":O"),       emo(0x1F62E)); // 😮
	text.replace(QLatin1String(":o"),       emo(0x1F62E)); // 😮
	text.replace(QLatin1String(":-0"),      emo(0x1F62E)); // 😮

	// ── Kuss ──
	text.replace(QLatin1String(":-*"),      emo(0x1F618)); // 😘 kiss
	text.replace(QLatin1String(":*"),       emo(0x1F618)); // 😘
	text.replace(QLatin1String(";-*"),      emo(0x1F618)); // 😘
	text.replace(QLatin1String(";*"),       emo(0x1F618)); // 😘

	// ── Sonstiges ──
	text.replace(QLatin1String(":-!"),      emo(0x1F60F)); // 😏 smirk
	text.replace(QLatin1String(":-#"),      emo(0x1F910)); // 🤐 sealed lips
	text.replace(QLatin1String(":-@"),      emo(0x1F621)); // 😡 angry
	text.replace(QLatin1String(":@"),       emo(0x1F621)); // 😡
	text.replace(QLatin1String("X-("),      emo(0x1F620)); // 😠 angry
	text.replace(QLatin1String("X("),       emo(0x1F623)); // 😣 persevering

	// ── Kaomoji (Pfeile escaped) ──
	text.replace(QLatin1String("&gt;_&lt;"), emo(0x1F623)); // 😣 >_<
	text.replace(QLatin1String("-_-"),       emo(0x1F611)); // 😑 expressionless
	text.replace(QLatin1String("T_T"),       emo(0x1F622)); // 😢 T_T
	text.replace(QLatin1String("T.T"),       emo(0x1F622)); // 😢
	text.replace(QLatin1String(";_;"),       emo(0x1F622)); // 😢
	text.replace(QLatin1String("o_O"),       emo(0x1F615)); // 😕 confused
	text.replace(QLatin1String("O_o"),       emo(0x1F615)); // 😕
	text.replace(QLatin1String("O_O"),       emo(0x1F633)); // 😳 shocked
	text.replace(QLatin1String("o_o"),       emo(0x1F633)); // 😳
	text.replace(QLatin1String("\\o/"),      emo(0x1F64C)); // 🙌 cheering

	// ── Heart (escaped; "</3" before "<3") ──
	text.replace(QLatin1String("&lt;/3"),    emo(0x1F494)); // 💔 broken heart
	text.replace(QLatin1String("&lt;3"),     emo(0x2764));  // ❤  heart

	// ── Reinsert the protected HTML entities unchanged ──
	for (int i = 0; i < savedEntities.size(); ++i)
		text.replace(QChar(0x0003) + QString::number(i) + QChar(0x0004), savedEntities.at(i));

	// ── Reinsert the protected URLs unchanged ──
	for (int i = 0; i < savedUrls.size(); ++i)
		text.replace(QChar(0x0001) + QString::number(i) + QChar(0x0002), savedUrls.at(i));

	return text;
}

#endif // CHAT_EMOTE_SHORTCUTS_H
