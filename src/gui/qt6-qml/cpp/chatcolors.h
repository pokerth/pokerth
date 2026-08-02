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
 * Nicht für den Spiel-Chat/-Verlauf: der liegt auf dem Tisch-Theme
 * (StyleProvider.chatLog*), das unabhängig vom App-Modus immer dunkel ist.
 */
namespace ChatColors {

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
    case Accent: return dark ? QStringLiteral("#E3C800") : QStringLiteral("#b09a00");
    case Danger: return dark ? QStringLiteral("#e05050") : QStringLiteral("#c62828");
    case Muted:  return dark ? QStringLiteral("#a0acc4") : QStringLiteral("#576378");
    case Info:   return dark ? QStringLiteral("#8ab4f8") : QStringLiteral("#1a5fb4");
    case Reject: return dark ? QStringLiteral("#e0686d") : QStringLiteral("#c62828");
    case Text:
    default:     return dark ? QStringLiteral("#cdd3e0") : QStringLiteral("#394150");
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

#endif // CHATCOLORS_H
