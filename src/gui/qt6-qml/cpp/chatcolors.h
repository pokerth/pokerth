#ifndef CHATCOLORS_H
#define CHATCOLORS_H

#include <QString>

/* Role colours of the lobby chat lines (light/dark mode).
 *
 * A chat line is built to HTML ONCE when it is received and afterwards only
 * changed selectively (the ChatTranslator appends globe anchors and swaps
 * message bodies for translations) – it is never created from scratch
 * again. If the hex value of the text colour stood fixed in that line, the
 * whole history already received would keep the old colours after a light/dark
 * switch: light text (#cdd3e0) on a light ground (#f0f3f8) is simply
 * unreadable at ~1.3:1.
 *
 * That is why the stored line only contains a role placeholder; the
 * concrete hex value only arises when it is delivered to QML
 * (LobbyHandler::chatLog()). A theme change is thereby only a
 * chatLogChanged() – without rebuilding the history and thus without losing the
 * states of the ChatTranslator (anchor ids, shown translations).
 *
 * The values correspond 1:1 to the QML palette (StaticData _dark/_light or
 * Theme.colorAccent…), so that the chat matches the rest of the user interface.
 *
 * The game chat/history at the table works by the same pattern, but takes
 * its hex values not from the app palette but from the table theme
 * (StyleProvider.chatLog*) – see TableChatColors further below.
 */
namespace ChatColors
{

enum Role {
	Text = 0,   // normal message (body text)
	Accent,     // mention of your own nick (gold)
	Danger,     // chatbot warning to me
	Muted,      // private message / local notice line
	Info,       // game invitation
	Reject,     // declined invitation
	RoleCount
};

// Control characters as the brackets of the placeholder: they do not occur in chat
// text and are additionally removed from foreign text (chatEscape), so they can
// never collide with user content.
inline constexpr char16_t kTokenStart = u'\x02';
inline constexpr char16_t kTokenEnd   = u'\x03';

// The placeholder as it stands in the stored line ("\x02<role>\x03").
inline QString token(Role role)
{
	return QChar(kTokenStart) + QString::number(int(role)) + QChar(kTokenEnd);
}

// A ready style fragment for building the line: "color:<placeholder>".
inline QString colorStyle(Role role)
{
	return QStringLiteral("color:") + token(role);
}

// Hex value of a role in the respective mode.
inline QString value(Role role, bool dark)
{
	switch (role) {
	case Accent:
		return dark ? QStringLiteral("#E3C800") : QStringLiteral("#b09a00");
	case Danger:
		return dark ? QStringLiteral("#e05050") : QStringLiteral("#c62828");
	case Muted:
		return dark ? QStringLiteral("#a0acc4") : QStringLiteral("#576378");
	case Info:
		return dark ? QStringLiteral("#8ab4f8") : QStringLiteral("#1a5fb4");
	case Reject:
		return dark ? QStringLiteral("#e0686d") : QStringLiteral("#c62828");
	case Text:
	default:
		return dark ? QStringLiteral("#cdd3e0") : QStringLiteral("#394150");
	}
}

// Placeholder -> hex. Applied when delivering every line to QML.
inline QString expand(QString line, bool dark)
{
	if (!line.contains(QChar(kTokenStart)))
		return line;
	for (int r = 0; r < RoleCount; ++r)
		line.replace(token(Role(r)), value(Role(r), dark));
	return line;
}

// HTML escaping for chat content (messages, player names). It additionally
// removes the placeholder control characters, so that foreign text cannot
// smuggle in colour placeholders.
inline QString chatEscape(const QString &raw)
{
	QString s = raw;
	s.remove(QChar(kTokenStart));
	s.remove(QChar(kTokenEnd));
	return s.toHtmlEscaped();
}

} // namespace ChatColors

/* Role colours of the game chat and the game history AT THE TABLE.
 *
 * The same principle as ChatColors, a different source: the hex values come not
 * from the app palette (light/dark) but from the table theme currently
 * selected (StyleProvider.chatLog*). Without placeholders the colours would stand
 * fixed in the line once it is built – a light table theme (e.g.
 * "Ivoire - Chene") would then get white text on a light ground, and a
 * theme change during a running game would never recolour the history
 * that is already there.
 *
 * The lines are therefore stored raw (with the placeholder); the GameHandler
 * expands them when delivering to QML and on every theme change simply
 * reports the list as changed.
 */
namespace TableChatColors
{

enum Role {
	Text = 0,    // normal chat message / normal history line
	Accent,      // mention of your own nick
	Winner,      // winner of the main pot
	WinnerSide,  // winner of a side pot
	Board,       // "--- Flop ---" / "… sits out"
	RoleCount
};

// The same control characters as in ChatColors: the two histories (lobby / table)
// live in separate lists and are each expanded by their own handler,
// so they cannot get in each other's way.
inline QString token(Role role)
{
	return QChar(ChatColors::kTokenStart) + QString::number(int(role))
		   + QChar(ChatColors::kTokenEnd);
}

// A ready style fragment for building the line: "color:<placeholder>".
inline QString colorStyle(Role role)
{
	return QStringLiteral("color:") + token(role);
}

// The hex values of a role as the current table theme delivers them.
struct Palette {
	QString color[RoleCount];
	bool isEmpty() const
	{
		return color[Text].isEmpty();
	}
};

// Placeholder -> hex. Applied when delivering every line to QML.
inline QString expand(QString line, const Palette &palette)
{
	if (palette.isEmpty() || !line.contains(QChar(ChatColors::kTokenStart)))
		return line;
	for (int r = 0; r < RoleCount; ++r)
		line.replace(token(Role(r)), palette.color[r]);
	return line;
}

} // namespace TableChatColors

#endif // CHATCOLORS_H
