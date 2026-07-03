#ifndef CHAT_EMOTE_SHORTCUTS_H
#define CHAT_EMOTE_SHORTCUTS_H

#include <QString>
#include <QLatin1String>
#include <QStringList>
#include <QHash>
#include <QRegularExpression>

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

    // ── URLs schützen (Vorrang vor Emote-Kürzeln) ───────────────────────────
    // Zeichenfolgen in Links (z. B. "?v=Dxyz" → "=D", "…xD…", "…:P…") dürfen
    // NICHT zu Emojis werden. http/https-URLs werden daher vor der Ersetzung
    // aus dem Text geschnitten, durch einen kollisionsfreien Platzhalter
    // (Steuerzeichen \x01<index>\x02) ersetzt und am Ende unverändert wieder
    // eingesetzt. Der Text ist bereits HTML-escaped; "\S+" matcht deshalb auch
    // "&amp;" in Query-Strings.
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

    // ── Discord-/GitHub-Style-Shortcodes (":smile:", ":moon:", ":fire:", …) ──
    // MUSS vor den ASCII-Kürzeln laufen: Sonst würde z. B. ":s" aus ":smile:"
    // vorzeitig zu 😟. Nur kleingeschriebene Namen ([a-z0-9_+-]) zwischen zwei
    // Doppelpunkten matchen – so kollidieren ":D"/":P"/":)" usw. NICHT damit.
    // Unbekannte Codes (":foobar:") bleiben unverändert stehen.
    static const QHash<QString, QString> shortcodes = [] {
        auto e = [](char32_t cp) { return QString::fromUcs4(&cp, 1); };
        QHash<QString, QString> m;
        // Gesichter
        m.insert(QStringLiteral("grinning"),     e(0x1F600));
        m.insert(QStringLiteral("smiley"),       e(0x1F603));
        m.insert(QStringLiteral("smile"),        e(0x1F604));
        m.insert(QStringLiteral("grin"),         e(0x1F601));
        m.insert(QStringLiteral("laughing"),     e(0x1F606));
        m.insert(QStringLiteral("satisfied"),    e(0x1F606));
        m.insert(QStringLiteral("sweat_smile"),  e(0x1F605));
        m.insert(QStringLiteral("joy"),          e(0x1F602));
        m.insert(QStringLiteral("rofl"),         e(0x1F923));
        m.insert(QStringLiteral("slightly_smiling_face"), e(0x1F642));
        m.insert(QStringLiteral("slight_smile"), e(0x1F642));
        m.insert(QStringLiteral("upside_down_face"), e(0x1F643));
        m.insert(QStringLiteral("wink"),         e(0x1F609));
        m.insert(QStringLiteral("blush"),        e(0x1F60A));
        m.insert(QStringLiteral("innocent"),     e(0x1F607));
        m.insert(QStringLiteral("heart_eyes"),   e(0x1F60D));
        m.insert(QStringLiteral("kissing_heart"), e(0x1F618));
        m.insert(QStringLiteral("kiss"),         e(0x1F48B));
        m.insert(QStringLiteral("stuck_out_tongue"), e(0x1F61B));
        m.insert(QStringLiteral("stuck_out_tongue_winking_eye"), e(0x1F61C));
        m.insert(QStringLiteral("sunglasses"),   e(0x1F60E));
        m.insert(QStringLiteral("smirk"),        e(0x1F60F));
        m.insert(QStringLiteral("thinking"),     e(0x1F914));
        m.insert(QStringLiteral("neutral_face"), e(0x1F610));
        m.insert(QStringLiteral("expressionless"), e(0x1F611));
        m.insert(QStringLiteral("unamused"),     e(0x1F612));
        m.insert(QStringLiteral("relieved"),     e(0x1F60C));
        m.insert(QStringLiteral("pensive"),      e(0x1F614));
        m.insert(QStringLiteral("confused"),     e(0x1F615));
        m.insert(QStringLiteral("worried"),      e(0x1F61F));
        m.insert(QStringLiteral("cry"),          e(0x1F622));
        m.insert(QStringLiteral("sob"),          e(0x1F62D));
        m.insert(QStringLiteral("angry"),        e(0x1F620));
        m.insert(QStringLiteral("rage"),         e(0x1F621));
        m.insert(QStringLiteral("sleeping"),     e(0x1F634));
        m.insert(QStringLiteral("mask"),         e(0x1F637));
        m.insert(QStringLiteral("dizzy_face"),   e(0x1F635));
        m.insert(QStringLiteral("scream"),       e(0x1F631));
        m.insert(QStringLiteral("fearful"),      e(0x1F628));
        m.insert(QStringLiteral("eyes"),         e(0x1F440));
        // Hände / Gesten
        m.insert(QStringLiteral("thumbsup"),     e(0x1F44D));
        m.insert(QStringLiteral("+1"),           e(0x1F44D));
        m.insert(QStringLiteral("thumbsdown"),   e(0x1F44E));
        m.insert(QStringLiteral("-1"),           e(0x1F44E));
        m.insert(QStringLiteral("ok_hand"),      e(0x1F44C));
        m.insert(QStringLiteral("clap"),         e(0x1F44F));
        m.insert(QStringLiteral("pray"),         e(0x1F64F));
        m.insert(QStringLiteral("muscle"),       e(0x1F4AA));
        m.insert(QStringLiteral("wave"),         e(0x1F44B));
        // Herzen
        m.insert(QStringLiteral("heart"),        e(0x2764));
        m.insert(QStringLiteral("broken_heart"), e(0x1F494));
        m.insert(QStringLiteral("two_hearts"),   e(0x1F495));
        // Natur / Himmel
        m.insert(QStringLiteral("fire"),         e(0x1F525));
        m.insert(QStringLiteral("star"),         e(0x2B50));
        m.insert(QStringLiteral("star2"),        e(0x1F31F));
        m.insert(QStringLiteral("sparkles"),     e(0x2728));
        m.insert(QStringLiteral("zap"),          e(0x26A1));
        m.insert(QStringLiteral("sunny"),        e(0x2600));
        m.insert(QStringLiteral("moon"),         e(0x1F314));
        m.insert(QStringLiteral("crescent_moon"), e(0x1F319));
        m.insert(QStringLiteral("full_moon"),    e(0x1F315));
        m.insert(QStringLiteral("new_moon"),     e(0x1F311));
        m.insert(QStringLiteral("snowflake"),    e(0x2744));
        m.insert(QStringLiteral("zzz"),          e(0x1F4A4));
        // Objekte / Symbole
        m.insert(QStringLiteral("tada"),         e(0x1F389));
        m.insert(QStringLiteral("rocket"),       e(0x1F680));
        m.insert(QStringLiteral("100"),          e(0x1F4AF));
        m.insert(QStringLiteral("bulb"),         e(0x1F4A1));
        m.insert(QStringLiteral("gift"),         e(0x1F381));
        m.insert(QStringLiteral("trophy"),       e(0x1F3C6));
        m.insert(QStringLiteral("crown"),        e(0x1F451));
        m.insert(QStringLiteral("white_check_mark"), e(0x2705));
        m.insert(QStringLiteral("heavy_check_mark"),  e(0x2714));
        m.insert(QStringLiteral("x"),            e(0x274C));
        m.insert(QStringLiteral("warning"),      e(0x26A0));
        m.insert(QStringLiteral("question"),     e(0x2753));
        m.insert(QStringLiteral("exclamation"),  e(0x2757));
        // Figuren / Spaß
        m.insert(QStringLiteral("poop"),         e(0x1F4A9));
        m.insert(QStringLiteral("hankey"),       e(0x1F4A9));
        m.insert(QStringLiteral("ghost"),        e(0x1F47B));
        m.insert(QStringLiteral("skull"),        e(0x1F480));
        m.insert(QStringLiteral("alien"),        e(0x1F47D));
        m.insert(QStringLiteral("robot"),        e(0x1F916));
        m.insert(QStringLiteral("clown"),        e(0x1F921));
        // Essen / Trinken
        m.insert(QStringLiteral("beer"),         e(0x1F37A));
        m.insert(QStringLiteral("beers"),        e(0x1F37B));
        m.insert(QStringLiteral("coffee"),       e(0x2615));
        m.insert(QStringLiteral("pizza"),        e(0x1F355));
        // Poker-typisch
        m.insert(QStringLiteral("game_die"),     e(0x1F3B2));
        m.insert(QStringLiteral("spades"),       e(0x2660));
        m.insert(QStringLiteral("hearts"),       e(0x2665));
        m.insert(QStringLiteral("diamonds"),     e(0x2666));
        m.insert(QStringLiteral("clubs"),        e(0x2663));
        m.insert(QStringLiteral("moneybag"),     e(0x1F4B0));
        m.insert(QStringLiteral("dollar"),       e(0x1F4B5));
        return m;
    }();
    {
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
    // ":/" ist jetzt unbedenklich – "http(s)://"-URLs wurden oben geschützt.
    text.replace(QLatin1String(":/"),       emo(0x1F615)); // 😕

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

    // ── Geschützte URLs unverändert wieder einsetzen ──
    for (int i = 0; i < savedUrls.size(); ++i)
        text.replace(QChar(0x0001) + QString::number(i) + QChar(0x0002), savedUrls.at(i));

    return text;
}

#endif // CHAT_EMOTE_SHORTCUTS_H
