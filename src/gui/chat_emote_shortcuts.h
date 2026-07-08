#ifndef CHAT_EMOTE_SHORTCUTS_H
#define CHAT_EMOTE_SHORTCUTS_H

#include <QString>
#include <QLatin1String>
#include <QStringList>
#include <QHash>
#include <QRegularExpression>

#include "chat_emote_shortcode_table.h"

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
// Discord-/GitHub-Style-Shortcodes (":smile:", ":moon:", ":fire:", …) →
// Unicode-Emoji. Vollständige GitHub-Shortcode-Liste
// (https://gist.github.com/rxaviers/7360908), generiert in
// chat_emote_shortcode_table.h. Separat zugänglich, damit die Chat-
// Autovervollständigung (ChatBox.qml, via LobbyHandler::chatEmoteShortcodes)
// EXAKT die Codes anbietet, die applyChatEmoteShortcuts auch wirklich ersetzt.
inline const QHash<QString, QString> &chatEmoteShortcodeMap()
{
    static const QHash<QString, QString> shortcodes = buildChatEmoteShortcodeMap();
    return shortcodes;
}

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

    // ── HTML-Entities schützen (außer &lt;/&gt;) ────────────────────────────
    // Der Text ist HTML-escaped; toHtmlEscaped() erzeugt u. a. "&quot;" und
    // "&amp;". Ihr abschließendes ';' darf NICHT den Anfang eines Kürzels wie
    // ";D"/";)"/";P" bilden – sonst würde z. B. '"D' → "&quot;D" fälschlich zu
    // '&quot😜'. Entities werden daher (wie URLs) durch Platzhalter
    // (\x03<index>\x04) ersetzt und am Ende unverändert wieder eingesetzt.
    // AUSGENOMMEN sind "&lt;"/"&gt;": Pfeil-Kürzel ("&lt;3", "&gt;:)",
    // "&gt;_&lt;") matchen bewusst auf ihrer escapten Form.
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

    // ── Geschützte HTML-Entities unverändert wieder einsetzen ──
    for (int i = 0; i < savedEntities.size(); ++i)
        text.replace(QChar(0x0003) + QString::number(i) + QChar(0x0004), savedEntities.at(i));

    // ── Geschützte URLs unverändert wieder einsetzen ──
    for (int i = 0; i < savedUrls.size(); ++i)
        text.replace(QChar(0x0001) + QString::number(i) + QChar(0x0002), savedUrls.at(i));

    return text;
}

#endif // CHAT_EMOTE_SHORTCUTS_H
