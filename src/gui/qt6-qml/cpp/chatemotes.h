#ifndef CHATEMOTES_H
#define CHATEMOTES_H

#include <QString>
#include <QRegularExpression>

// Vergrößert Unicode-Emoji in einer (bereits HTML-formatierten) Chatzeile auf
// ~22px – ähnlich den Bild-Emotes des Qt-Widgets-Clients. Zusammenhängende
// Emoji-Sequenzen (inkl. Variations-Selektoren / ZWJ / Keycaps) werden in einen
// größeren font-size-Span gewrappt. Wird von Game- und Lobby-Chat genutzt.
inline QString enlargeEmojis(const QString &html)
{
    static const QRegularExpression emojiRe(QStringLiteral(
        "([\\x{1F000}-\\x{1FAFF}\\x{2600}-\\x{27BF}\\x{2B00}-\\x{2BFF}"
        "\\x{2190}-\\x{21FF}\\x{2300}-\\x{23FF}\\x{2900}-\\x{297F}"
        "\\x{FE00}-\\x{FE0F}\\x{200D}\\x{20E3}]+)"));
    QString r = html;
    r.replace(emojiRe, QStringLiteral(
        "<span style=\"font-size:22px; font-family:'Noto Color Emoji';\">\\1</span>"));
    return r;
}

// Prüft, ob ein Reaktions-Payload ("/emoji <x>") ausschließlich aus echten
// Emoji-Zeichen besteht (inkl. Variation-Selektoren, ZWJ, Keycaps und Hautton-
// Modifiern) und mindestens ein Emoji enthält. So werden als Reaktion getarnte
// Textnachrichten ("/emoji haha") verworfen, beliebige echte Emojis (auch
// außerhalb der Picker-Liste) aber zugelassen.
inline bool isEmojiOnlyReaction(const QString &text)
{
    if (text.isEmpty())
        return false;
    // Zeichenklasse der „echten" Emoji-Codepoints (wie in enlargeEmojis).
    static const QString emojiClass = QStringLiteral(
        "\\x{1F000}-\\x{1FAFF}\\x{2600}-\\x{27BF}\\x{2B00}-\\x{2BFF}"
        "\\x{2190}-\\x{21FF}\\x{2300}-\\x{23FF}\\x{2900}-\\x{297F}");
    // Zusätzlich erlaubte Verbindungs-/Modifier-Zeichen (nur mit Emoji sinnvoll).
    static const QString joinerClass = QStringLiteral(
        "\\x{1F3FB}-\\x{1F3FF}\\x{FE00}-\\x{FE0F}\\x{200D}\\x{20E3}");
    static const QRegularExpression hasEmojiRe(QStringLiteral("[") + emojiClass + QStringLiteral("]"));
    static const QRegularExpression foreignRe(QStringLiteral("[^") + emojiClass + joinerClass + QStringLiteral("]"));
    return hasEmojiRe.match(text).hasMatch()       // mindestens ein echtes Emoji …
           && !foreignRe.match(text).hasMatch();   // … und sonst nur Emoji-Zeichen
}

#endif // CHATEMOTES_H
