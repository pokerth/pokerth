#ifndef CHATCOLORS_H
#define CHATCOLORS_H

#include <QString>

/* Rollen-Farben der Lobby-Chatzeilen (Hell-/Dunkelmodus).
 *
 * Eine Chatzeile wird EINMAL beim Empfang zu HTML gebaut und danach nur noch
 * punktuell verändert (der ChatTranslator hängt Globus-Anker an und tauscht
 * Nachrichtenkörper gegen Übersetzungen aus) – komplett neu erzeugt wird sie
 * nie. Stünde der Hex-Wert der Textfarbe fest in dieser Zeile, behielte der
 * gesamte bereits empfangene Verlauf nach einem Hell/Dunkel-Wechsel die alten
 * Farben: heller Text (#cdd3e0) auf hellem Grund (#f0f3f8) ist mit ~1,3:1
 * schlicht unlesbar.
 *
 * Deshalb steht in der gespeicherten Zeile nur ein Rollen-Platzhalter; der
 * konkrete Hex-Wert entsteht erst beim Ausliefern an QML
 * (LobbyHandler::chatLog()). Ein Themenwechsel ist damit nur noch ein
 * chatLogChanged() – ohne Neuaufbau des Verlaufs und damit ohne die Zustände
 * des ChatTranslators (Anker-Ids, eingeblendete Übersetzungen) zu verlieren.
 *
 * Die Werte entsprechen 1:1 der QML-Palette (StaticData _dark/_light bzw.
 * Theme.colorAccent…), damit der Chat zum Rest der Oberfläche passt.
 *
 * Der Spiel-Chat/-Verlauf am Tisch arbeitet nach demselben Muster, bezieht
 * seine Hex-Werte aber nicht aus der App-Palette, sondern aus dem Tisch-Theme
 * (StyleProvider.chatLog*) – siehe TableChatColors weiter unten.
 */
namespace ChatColors
{

enum Role {
	Text = 0,   // normale Nachricht (Fließtext)
	Accent,     // Erwähnung des eigenen Nicks (Gold)
	Danger,     // Chatbot-Warnung an mich
	Muted,      // private Nachricht / lokale Hinweiszeile
	Info,       // Spiel-Einladung
	Reject,     // abgelehnte Einladung
	RoleCount
};

// Steuerzeichen als Klammern des Platzhalters: sie kommen in Chat-Text nicht
// vor und werden aus Fremdtext zusätzlich entfernt (chatEscape), können also
// nie mit Nutzerinhalten kollidieren.
inline constexpr char16_t kTokenStart = u'\x02';
inline constexpr char16_t kTokenEnd   = u'\x03';

// Platzhalter, wie er in der gespeicherten Zeile steht ("\x02<rolle>\x03").
inline QString token(Role role)
{
	return QChar(kTokenStart) + QString::number(int(role)) + QChar(kTokenEnd);
}

// Fertiges Style-Fragment für den Zeilenaufbau: "color:<platzhalter>".
inline QString colorStyle(Role role)
{
	return QStringLiteral("color:") + token(role);
}

// Hex-Wert einer Rolle im jeweiligen Modus.
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

// Platzhalter -> Hex. Wird beim Ausliefern jeder Zeile an QML angewandt.
inline QString expand(QString line, bool dark)
{
	if (!line.contains(QChar(kTokenStart)))
		return line;
	for (int r = 0; r < RoleCount; ++r)
		line.replace(token(Role(r)), value(Role(r), dark));
	return line;
}

// HTML-Escaping für Chat-Inhalte (Nachrichten, Spielernamen). Entfernt
// zusätzlich die Platzhalter-Steuerzeichen, damit Fremdtext keine Farb-
// Platzhalter einschleusen kann.
inline QString chatEscape(const QString &raw)
{
	QString s = raw;
	s.remove(QChar(kTokenStart));
	s.remove(QChar(kTokenEnd));
	return s.toHtmlEscaped();
}

} // namespace ChatColors

/* Rollen-Farben des Spiel-Chats und des Spielverlaufs AM TISCH.
 *
 * Gleiches Prinzip wie ChatColors, andere Quelle: die Hex-Werte kommen nicht
 * aus der App-Palette (Hell/Dunkel), sondern aus dem gerade gewählten
 * Tisch-Theme (StyleProvider.chatLog*). Ohne Platzhalter stünden die Farben
 * fest in der einmal gebauten Zeile – ein helles Tisch-Theme (z. B.
 * "Ivoire - Chene") bekäme dann weißen Text auf hellem Grund, und ein
 * Theme-Wechsel im laufenden Spiel färbte den bereits vorhandenen Verlauf
 * nie um.
 *
 * Die Zeilen bleiben deshalb roh (mit Platzhalter) gespeichert; GameHandler
 * expandiert sie beim Ausliefern an QML und meldet bei jedem Theme-Wechsel
 * einfach die Liste als geändert.
 */
namespace TableChatColors
{

enum Role {
	Text = 0,    // normale Chat-Nachricht / normale Verlaufszeile
	Accent,      // Erwähnung des eigenen Nicks
	Winner,      // Gewinner des Hauptpots
	WinnerSide,  // Gewinner eines Side-Pots
	Board,       // "--- Flop ---" / "… sits out"
	RoleCount
};

// Dieselben Steuerzeichen wie ChatColors: die beiden Verläufe (Lobby / Tisch)
// liegen in getrennten Listen und werden jeweils von ihrem eigenen Handler
// expandiert, können sich also nicht in die Quere kommen.
inline QString token(Role role)
{
	return QChar(ChatColors::kTokenStart) + QString::number(int(role))
		   + QChar(ChatColors::kTokenEnd);
}

// Fertiges Style-Fragment für den Zeilenaufbau: "color:<platzhalter>".
inline QString colorStyle(Role role)
{
	return QStringLiteral("color:") + token(role);
}

// Die Hex-Werte einer Rolle, wie sie das aktuelle Tisch-Theme liefert.
struct Palette {
	QString color[RoleCount];
	bool isEmpty() const
	{
		return color[Text].isEmpty();
	}
};

// Platzhalter -> Hex. Wird beim Ausliefern jeder Zeile an QML angewandt.
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
