#ifndef CHATEMOTES_H
#define CHATEMOTES_H

#include <QString>
#include <QRegularExpression>

// Zeichenklassen der Emoji-Erkennung – gemeinsam genutzt von enlargeEmojis
// und isEmojiOnlyReaction, damit beide denselben Emoji-Begriff haben.
//
// „Echte" Emoji-Codepoints. Deckt die komplette GitHub-Shortcode-Liste
// (chat_emote_shortcode_table.h) ab – AUSSER ©/®/™ (:copyright:/:registered:/
// :tm:): die sind auch normale Textzeichen („Qt® …") und sollen im Chat weder
// vergrößert noch als Emoji-Reaktion gewertet werden.
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

// Verbindungs-/Modifier-Zeichen (nur zusammen mit Emojis sinnvoll): Hauttöne,
// Variation-Selektoren, ZWJ, Keycap-Kombinierer sowie Tag-Zeichen
// (Flaggen England/Schottland/Wales, z. B. 🏴󠁧󠁢󠁥󠁮󠁧󠁿 = 1F3F4 + E0067…E007F).
inline const QString &emojiJoinerClass()
{
    static const QString cls = QStringLiteral(
        "\\x{1F3FB}-\\x{1F3FF}\\x{FE00}-\\x{FE0F}\\x{200D}\\x{20E3}"
        "\\x{E0020}-\\x{E007F}");
    return cls;
}

// Vergrößert Unicode-Emoji in einer (bereits HTML-formatierten) Chatzeile auf
// ~22px – ähnlich den Bild-Emotes des Qt-Widgets-Clients. Zusammenhängende
// Emoji-Sequenzen (inkl. Variations-Selektoren / ZWJ / Keycaps) werden in einen
// größeren font-size-Span gewrappt. Wird von Game- und Lobby-Chat genutzt.
inline QString enlargeEmojis(const QString &html)
{
    // Keycap-Sequenzen (#⃣ 1⃣ …) als Alternative VOR der Zeichenklasse: ihre
    // Basiszeichen (#, *, Ziffern) gehören nur als Teil der kompletten Sequenz
    // in den Span – sonst bliebe die Ziffer klein und nur der Kombinierer
    // würde vergrößert (kaputtes Rendering). Bare Ziffern matchen NICHT.
    static const QRegularExpression emojiRe(
        QStringLiteral("((?:[#*0-9]\\x{FE0F}?\\x{20E3}|[")
        + emojiCharClass() + emojiJoinerClass() + QStringLiteral("])+)"));
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
    // Keycap-Sequenzen zuerst herausschneiden: ihre Basiszeichen (#, *,
    // Ziffern) sind für sich genommen KEINE Emojis (sonst wäre "/emoji 123"
    // gültig) und zählen nur als komplette Sequenz.
    static const QRegularExpression keycapRe(
        QStringLiteral("[#*0-9]\\x{FE0F}?\\x{20E3}"));
    QString t = text;
    t.remove(keycapRe);
    const bool hadKeycap = t.size() != text.size();
    if (t.isEmpty())
        return hadKeycap;                          // nur Keycap-Emojis
    static const QRegularExpression hasEmojiRe(
        QStringLiteral("[") + emojiCharClass() + QStringLiteral("]"));
    static const QRegularExpression foreignRe(
        QStringLiteral("[^") + emojiCharClass() + emojiJoinerClass() + QStringLiteral("]"));
    return (hasEmojiRe.match(t).hasMatch() || hadKeycap) // mindestens ein Emoji …
           && !foreignRe.match(t).hasMatch();      // … und sonst nur Emoji-Zeichen
}

#endif // CHATEMOTES_H
