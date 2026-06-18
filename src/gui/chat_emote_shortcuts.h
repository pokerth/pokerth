#ifndef CHAT_EMOTE_SHORTCUTS_H
#define CHAT_EMOTE_SHORTCUTS_H

#include <QString>
#include <QLatin1String>

// Wandelt ASCII-Emoticon-Kürzel (":-)", "8-)", "<3", ">_<", …) in die
// entsprechenden Unicode-Emojis um. Gemeinsam genutzt vom Qt-Widgets-Client
// (chattools.cpp) und vom QML-Client (gamehandler.cpp / lobbyhandler.cpp),
// damit beide denselben, möglichst umfangreichen Kürzel-Satz kennen.
//
// WICHTIG:
//  * Die Funktion erwartet bereits HTML-escapten Text und wird VOR dem
//    Hinzufügen von Style-/Link-Markup angewendet. So treffen kurze Kürzel
//    nie auf eigenes Markup wie "color:#..." oder "font-weight:bold".
//  * '<' / '>' liegen als "&lt;" / "&gt;" vor – Kürzel mit Pfeilen werden
//    daher in ihrer escapten Form gematcht.
//  * Reihenfolge: Längere bzw. speziellere Kürzel (Engel/Teufel/Lachen,
//    ">:(" usw.) MÜSSEN vor ihren kürzeren Präfixen (":)", ":(") ersetzt
//    werden, sonst frisst das kürzere Kürzel einen Teil des längeren.
//  * Bewusst NICHT enthalten sind mehrdeutige Kürzel, die in normalem Text
//    häufig auftreten: ":0"/":3" (Uhrzeiten), ":\\"/"D:" (Windows-Pfade),
//    bare "8)"/"B)" (z. B. "(Plan B)").
inline QString applyChatEmoteShortcuts(QString text)
{
    auto emo = [](char32_t cp) -> QString { return QString::fromUcs4(&cp, 1); };

    // ── Engel / Teufel (enthalten ":-)" bzw. ":)") ──
    text.replace(QLatin1String("0:-)"),     emo(0x1F607)); // 😇 angel
    text.replace(QLatin1String("0:)"),      emo(0x1F607)); // 😇
    text.replace(QLatin1String("O:-)"),     emo(0x1F607)); // 😇
    text.replace(QLatin1String("O:)"),      emo(0x1F607)); // 😇
    text.replace(QLatin1String("&gt;:-)"),  emo(0x1F608)); // 😈 devilish (>:-))
    text.replace(QLatin1String("&gt;:)"),   emo(0x1F608)); // 😈 (>:))
    text.replace(QLatin1String("}:-)"),     emo(0x1F608)); // 😈
    text.replace(QLatin1String("}:)"),      emo(0x1F608)); // 😈

    // ── Lachen (enthält ":-)" / ":)") ──
    text.replace(QLatin1String(":-))"),     emo(0x1F602)); // 😂 laughing
    text.replace(QLatin1String(":))"),      emo(0x1F602)); // 😂

    // ── Wütend / weinend (enthalten ":(" / ":-(") ──
    text.replace(QLatin1String("&gt;:-("),  emo(0x1F621)); // 😡 angry (>:-()
    text.replace(QLatin1String("&gt;:("),   emo(0x1F621)); // 😡 (>:()
    text.replace(QLatin1String(":'-("),     emo(0x1F622)); // 😢 crying
    text.replace(QLatin1String(":'("),      emo(0x1F622)); // 😢

    // ── Freudentränen (vor ":)" – kollidiert aber nicht) ──
    text.replace(QLatin1String(":'-)"),     emo(0x1F972)); // 🥲 happy tears
    text.replace(QLatin1String(":')"),      emo(0x1F972)); // 🥲
    text.replace(QLatin1String(":'D"),      emo(0x1F602)); // 😂

    // ── Großes Grinsen / Lachen ──
    text.replace(QLatin1String(":-D"),      emo(0x1F603)); // 😃 big grin
    text.replace(QLatin1String(":D"),       emo(0x1F603)); // 😃
    text.replace(QLatin1String("=D"),       emo(0x1F604)); // 😄
    text.replace(QLatin1String("xD"),       emo(0x1F606)); // 😆 laughing
    text.replace(QLatin1String("XD"),       emo(0x1F606)); // 😆

    // ── Lächeln ──
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

    // ── Cool (nur Nasen-Varianten; "8)"/"B)" wären zu kollisionsanfällig) ──
    text.replace(QLatin1String("B-)"),      emo(0x1F60E)); // 😎 cool
    text.replace(QLatin1String("8-)"),      emo(0x1F60E)); // 😎

    // ── Traurig ──
    text.replace(QLatin1String(":-("),      emo(0x1F61E)); // 😞 sad
    text.replace(QLatin1String(":("),       emo(0x1F61E)); // 😞
    text.replace(QLatin1String("=("),       emo(0x1F61E)); // 😞
    text.replace(QLatin1String(":["),       emo(0x1F61E)); // 😞

    // ── Verlegen / neutral / skeptisch ──
    text.replace(QLatin1String(":-["),      emo(0x1F633)); // 😳 embarrassed
    text.replace(QLatin1String(":-|"),      emo(0x1F610)); // 😐 neutral
    text.replace(QLatin1String(":|"),       emo(0x1F610)); // 😐
    text.replace(QLatin1String(":-/"),      emo(0x1F615)); // 😕 skeptical
    text.replace(QLatin1String(":-\\"),     emo(0x1F615)); // 😕
    // ":/" nur außerhalb von URLs (sonst zerlegt es "http://").
    if (!text.contains(QLatin1String("http://")) && !text.contains(QLatin1String("https://")))
        text.replace(QLatin1String(":/"),   emo(0x1F615)); // 😕

    // ── Besorgt / krank / überrascht ──
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

    // ── Herz (escaped; "</3" vor "<3") ──
    text.replace(QLatin1String("&lt;/3"),    emo(0x1F494)); // 💔 broken heart
    text.replace(QLatin1String("&lt;3"),     emo(0x2764));  // ❤  heart

    return text;
}

#endif // CHAT_EMOTE_SHORTCUTS_H
