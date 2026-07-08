#ifndef CHAT_EMOTE_SHORTCODE_TABLE_H
#define CHAT_EMOTE_SHORTCODE_TABLE_H

#include <QString>
#include <QHash>
#include <initializer_list>

// GENERIERT – nicht von Hand editieren.
//
// Vollständige GitHub-Emoji-Shortcode-Liste (":smile:", ":fire:", …), wie im
// bekannten Cheat-Sheet https://gist.github.com/rxaviers/7360908. Quelle:
// https://api.github.com/emojis (Stand: Juli 2026, 1913 Unicode-Emojis).
// Regenerieren: JSON laden, "/unicode/<cp>[-<cp>…].png"-URLs zu Codepoint-
// Listen parsen und als insert-Zeilen ausgeben; Einträge ohne "/unicode/"
// (GitHub-Bild-Emojis wie :octocat:, :shipit:) überspringen.
//
// Abweichungen von den GitHub-Rohdaten (die URLs lassen FE0F/ZWJ weg und
// wären so gebrochene Sequenzen):
//  * Keycaps (:one:, :hash:, …): U+FE0F ergänzt (RGI-Sequenz Basis+FE0F+20E3),
//    sonst rendert die Basisziffer als Text.
//  * Mehrpunkt-Sequenzen (:family_man_woman_girl_boy:, :health_worker:, …):
//    über die offizielle RGI-Liste (unicode.org emoji-zwj-sequences.txt)
//    zur vollen Sequenz inkl. ZWJ/FE0F rekonstruiert.
//  * Am Ende einige zusätzliche Aliase, die GitHub nicht kennt (Discord-
//    üblich bzw. bisheriger PokerTH-Kürzelsatz).
inline QHash<QString, QString> buildChatEmoteShortcodeMap()
{
    auto e = [](std::initializer_list<char32_t> cps) {
        return QString::fromUcs4(cps.begin(), qsizetype(cps.size()));
    };
    QHash<QString, QString> m;
    m.reserve(2048);
    m.insert(QStringLiteral("+1"), e({0x1F44D})); // 👍
    m.insert(QStringLiteral("-1"), e({0x1F44E})); // 👎
    m.insert(QStringLiteral("100"), e({0x1F4AF})); // 💯
    m.insert(QStringLiteral("1234"), e({0x1F522})); // 🔢
    m.insert(QStringLiteral("1st_place_medal"), e({0x1F947})); // 🥇
    m.insert(QStringLiteral("2nd_place_medal"), e({0x1F948})); // 🥈
    m.insert(QStringLiteral("3rd_place_medal"), e({0x1F949})); // 🥉
    m.insert(QStringLiteral("8ball"), e({0x1F3B1})); // 🎱
    m.insert(QStringLiteral("a"), e({0x1F170})); // 🅰
    m.insert(QStringLiteral("ab"), e({0x1F18E})); // 🆎
    m.insert(QStringLiteral("abacus"), e({0x1F9EE})); // 🧮
    m.insert(QStringLiteral("abc"), e({0x1F524})); // 🔤
    m.insert(QStringLiteral("abcd"), e({0x1F521})); // 🔡
    m.insert(QStringLiteral("accept"), e({0x1F251})); // 🉑
    m.insert(QStringLiteral("accordion"), e({0x1FA97})); // 🪗
    m.insert(QStringLiteral("adhesive_bandage"), e({0x1FA79})); // 🩹
    m.insert(QStringLiteral("adult"), e({0x1F9D1})); // 🧑
    m.insert(QStringLiteral("aerial_tramway"), e({0x1F6A1})); // 🚡
    m.insert(QStringLiteral("afghanistan"), e({0x1F1E6, 0x1F1EB})); // 🇦🇫
    m.insert(QStringLiteral("airplane"), e({0x2708})); // ✈
    m.insert(QStringLiteral("aland_islands"), e({0x1F1E6, 0x1F1FD})); // 🇦🇽
    m.insert(QStringLiteral("alarm_clock"), e({0x23F0})); // ⏰
    m.insert(QStringLiteral("albania"), e({0x1F1E6, 0x1F1F1})); // 🇦🇱
    m.insert(QStringLiteral("alembic"), e({0x2697})); // ⚗
    m.insert(QStringLiteral("algeria"), e({0x1F1E9, 0x1F1FF})); // 🇩🇿
    m.insert(QStringLiteral("alien"), e({0x1F47D})); // 👽
    m.insert(QStringLiteral("ambulance"), e({0x1F691})); // 🚑
    m.insert(QStringLiteral("american_samoa"), e({0x1F1E6, 0x1F1F8})); // 🇦🇸
    m.insert(QStringLiteral("amphora"), e({0x1F3FA})); // 🏺
    m.insert(QStringLiteral("anatomical_heart"), e({0x1FAC0})); // 🫀
    m.insert(QStringLiteral("anchor"), e({0x2693})); // ⚓
    m.insert(QStringLiteral("andorra"), e({0x1F1E6, 0x1F1E9})); // 🇦🇩
    m.insert(QStringLiteral("angel"), e({0x1F47C})); // 👼
    m.insert(QStringLiteral("anger"), e({0x1F4A2})); // 💢
    m.insert(QStringLiteral("angola"), e({0x1F1E6, 0x1F1F4})); // 🇦🇴
    m.insert(QStringLiteral("angry"), e({0x1F620})); // 😠
    m.insert(QStringLiteral("anguilla"), e({0x1F1E6, 0x1F1EE})); // 🇦🇮
    m.insert(QStringLiteral("anguished"), e({0x1F627})); // 😧
    m.insert(QStringLiteral("ant"), e({0x1F41C})); // 🐜
    m.insert(QStringLiteral("antarctica"), e({0x1F1E6, 0x1F1F6})); // 🇦🇶
    m.insert(QStringLiteral("antigua_barbuda"), e({0x1F1E6, 0x1F1EC})); // 🇦🇬
    m.insert(QStringLiteral("apple"), e({0x1F34E})); // 🍎
    m.insert(QStringLiteral("aquarius"), e({0x2652})); // ♒
    m.insert(QStringLiteral("argentina"), e({0x1F1E6, 0x1F1F7})); // 🇦🇷
    m.insert(QStringLiteral("aries"), e({0x2648})); // ♈
    m.insert(QStringLiteral("armenia"), e({0x1F1E6, 0x1F1F2})); // 🇦🇲
    m.insert(QStringLiteral("arrow_backward"), e({0x25C0})); // ◀
    m.insert(QStringLiteral("arrow_double_down"), e({0x23EC})); // ⏬
    m.insert(QStringLiteral("arrow_double_up"), e({0x23EB})); // ⏫
    m.insert(QStringLiteral("arrow_down"), e({0x2B07})); // ⬇
    m.insert(QStringLiteral("arrow_down_small"), e({0x1F53D})); // 🔽
    m.insert(QStringLiteral("arrow_forward"), e({0x25B6})); // ▶
    m.insert(QStringLiteral("arrow_heading_down"), e({0x2935})); // ⤵
    m.insert(QStringLiteral("arrow_heading_up"), e({0x2934})); // ⤴
    m.insert(QStringLiteral("arrow_left"), e({0x2B05})); // ⬅
    m.insert(QStringLiteral("arrow_lower_left"), e({0x2199})); // ↙
    m.insert(QStringLiteral("arrow_lower_right"), e({0x2198})); // ↘
    m.insert(QStringLiteral("arrow_right"), e({0x27A1})); // ➡
    m.insert(QStringLiteral("arrow_right_hook"), e({0x21AA})); // ↪
    m.insert(QStringLiteral("arrow_up"), e({0x2B06})); // ⬆
    m.insert(QStringLiteral("arrow_up_down"), e({0x2195})); // ↕
    m.insert(QStringLiteral("arrow_up_small"), e({0x1F53C})); // 🔼
    m.insert(QStringLiteral("arrow_upper_left"), e({0x2196})); // ↖
    m.insert(QStringLiteral("arrow_upper_right"), e({0x2197})); // ↗
    m.insert(QStringLiteral("arrows_clockwise"), e({0x1F503})); // 🔃
    m.insert(QStringLiteral("arrows_counterclockwise"), e({0x1F504})); // 🔄
    m.insert(QStringLiteral("art"), e({0x1F3A8})); // 🎨
    m.insert(QStringLiteral("articulated_lorry"), e({0x1F69B})); // 🚛
    m.insert(QStringLiteral("artificial_satellite"), e({0x1F6F0})); // 🛰
    m.insert(QStringLiteral("artist"), e({0x1F9D1, 0x200D, 0x1F3A8})); // 🧑‍🎨
    m.insert(QStringLiteral("aruba"), e({0x1F1E6, 0x1F1FC})); // 🇦🇼
    m.insert(QStringLiteral("ascension_island"), e({0x1F1E6, 0x1F1E8})); // 🇦🇨
    m.insert(QStringLiteral("asterisk"), e({0x2A, 0xFE0F, 0x20E3})); // *️⃣
    m.insert(QStringLiteral("astonished"), e({0x1F632})); // 😲
    m.insert(QStringLiteral("astronaut"), e({0x1F9D1, 0x200D, 0x1F680})); // 🧑‍🚀
    m.insert(QStringLiteral("athletic_shoe"), e({0x1F45F})); // 👟
    m.insert(QStringLiteral("atm"), e({0x1F3E7})); // 🏧
    m.insert(QStringLiteral("atom_symbol"), e({0x269B})); // ⚛
    m.insert(QStringLiteral("australia"), e({0x1F1E6, 0x1F1FA})); // 🇦🇺
    m.insert(QStringLiteral("austria"), e({0x1F1E6, 0x1F1F9})); // 🇦🇹
    m.insert(QStringLiteral("auto_rickshaw"), e({0x1F6FA})); // 🛺
    m.insert(QStringLiteral("avocado"), e({0x1F951})); // 🥑
    m.insert(QStringLiteral("axe"), e({0x1FA93})); // 🪓
    m.insert(QStringLiteral("azerbaijan"), e({0x1F1E6, 0x1F1FF})); // 🇦🇿
    m.insert(QStringLiteral("b"), e({0x1F171})); // 🅱
    m.insert(QStringLiteral("baby"), e({0x1F476})); // 👶
    m.insert(QStringLiteral("baby_bottle"), e({0x1F37C})); // 🍼
    m.insert(QStringLiteral("baby_chick"), e({0x1F424})); // 🐤
    m.insert(QStringLiteral("baby_symbol"), e({0x1F6BC})); // 🚼
    m.insert(QStringLiteral("back"), e({0x1F519})); // 🔙
    m.insert(QStringLiteral("bacon"), e({0x1F953})); // 🥓
    m.insert(QStringLiteral("badger"), e({0x1F9A1})); // 🦡
    m.insert(QStringLiteral("badminton"), e({0x1F3F8})); // 🏸
    m.insert(QStringLiteral("bagel"), e({0x1F96F})); // 🥯
    m.insert(QStringLiteral("baggage_claim"), e({0x1F6C4})); // 🛄
    m.insert(QStringLiteral("baguette_bread"), e({0x1F956})); // 🥖
    m.insert(QStringLiteral("bahamas"), e({0x1F1E7, 0x1F1F8})); // 🇧🇸
    m.insert(QStringLiteral("bahrain"), e({0x1F1E7, 0x1F1ED})); // 🇧🇭
    m.insert(QStringLiteral("balance_scale"), e({0x2696})); // ⚖
    m.insert(QStringLiteral("bald_man"), e({0x1F468, 0x200D, 0x1F9B2})); // 👨‍🦲
    m.insert(QStringLiteral("bald_woman"), e({0x1F469, 0x200D, 0x1F9B2})); // 👩‍🦲
    m.insert(QStringLiteral("ballet_shoes"), e({0x1FA70})); // 🩰
    m.insert(QStringLiteral("balloon"), e({0x1F388})); // 🎈
    m.insert(QStringLiteral("ballot_box"), e({0x1F5F3})); // 🗳
    m.insert(QStringLiteral("ballot_box_with_check"), e({0x2611})); // ☑
    m.insert(QStringLiteral("bamboo"), e({0x1F38D})); // 🎍
    m.insert(QStringLiteral("banana"), e({0x1F34C})); // 🍌
    m.insert(QStringLiteral("bangbang"), e({0x203C})); // ‼
    m.insert(QStringLiteral("bangladesh"), e({0x1F1E7, 0x1F1E9})); // 🇧🇩
    m.insert(QStringLiteral("banjo"), e({0x1FA95})); // 🪕
    m.insert(QStringLiteral("bank"), e({0x1F3E6})); // 🏦
    m.insert(QStringLiteral("bar_chart"), e({0x1F4CA})); // 📊
    m.insert(QStringLiteral("barbados"), e({0x1F1E7, 0x1F1E7})); // 🇧🇧
    m.insert(QStringLiteral("barber"), e({0x1F488})); // 💈
    m.insert(QStringLiteral("baseball"), e({0x26BE})); // ⚾
    m.insert(QStringLiteral("basket"), e({0x1F9FA})); // 🧺
    m.insert(QStringLiteral("basketball"), e({0x1F3C0})); // 🏀
    m.insert(QStringLiteral("basketball_man"), e({0x26F9, 0xFE0F, 0x200D, 0x2642, 0xFE0F})); // ⛹️‍♂️
    m.insert(QStringLiteral("basketball_woman"), e({0x26F9, 0xFE0F, 0x200D, 0x2640, 0xFE0F})); // ⛹️‍♀️
    m.insert(QStringLiteral("bat"), e({0x1F987})); // 🦇
    m.insert(QStringLiteral("bath"), e({0x1F6C0})); // 🛀
    m.insert(QStringLiteral("bathtub"), e({0x1F6C1})); // 🛁
    m.insert(QStringLiteral("battery"), e({0x1F50B})); // 🔋
    m.insert(QStringLiteral("beach_umbrella"), e({0x1F3D6})); // 🏖
    m.insert(QStringLiteral("beans"), e({0x1FAD8})); // 🫘
    m.insert(QStringLiteral("bear"), e({0x1F43B})); // 🐻
    m.insert(QStringLiteral("bearded_person"), e({0x1F9D4})); // 🧔
    m.insert(QStringLiteral("beaver"), e({0x1F9AB})); // 🦫
    m.insert(QStringLiteral("bed"), e({0x1F6CF})); // 🛏
    m.insert(QStringLiteral("bee"), e({0x1F41D})); // 🐝
    m.insert(QStringLiteral("beer"), e({0x1F37A})); // 🍺
    m.insert(QStringLiteral("beers"), e({0x1F37B})); // 🍻
    m.insert(QStringLiteral("beetle"), e({0x1FAB2})); // 🪲
    m.insert(QStringLiteral("beginner"), e({0x1F530})); // 🔰
    m.insert(QStringLiteral("belarus"), e({0x1F1E7, 0x1F1FE})); // 🇧🇾
    m.insert(QStringLiteral("belgium"), e({0x1F1E7, 0x1F1EA})); // 🇧🇪
    m.insert(QStringLiteral("belize"), e({0x1F1E7, 0x1F1FF})); // 🇧🇿
    m.insert(QStringLiteral("bell"), e({0x1F514})); // 🔔
    m.insert(QStringLiteral("bell_pepper"), e({0x1FAD1})); // 🫑
    m.insert(QStringLiteral("bellhop_bell"), e({0x1F6CE})); // 🛎
    m.insert(QStringLiteral("benin"), e({0x1F1E7, 0x1F1EF})); // 🇧🇯
    m.insert(QStringLiteral("bento"), e({0x1F371})); // 🍱
    m.insert(QStringLiteral("bermuda"), e({0x1F1E7, 0x1F1F2})); // 🇧🇲
    m.insert(QStringLiteral("beverage_box"), e({0x1F9C3})); // 🧃
    m.insert(QStringLiteral("bhutan"), e({0x1F1E7, 0x1F1F9})); // 🇧🇹
    m.insert(QStringLiteral("bicyclist"), e({0x1F6B4})); // 🚴
    m.insert(QStringLiteral("bike"), e({0x1F6B2})); // 🚲
    m.insert(QStringLiteral("biking_man"), e({0x1F6B4, 0x200D, 0x2642, 0xFE0F})); // 🚴‍♂️
    m.insert(QStringLiteral("biking_woman"), e({0x1F6B4, 0x200D, 0x2640, 0xFE0F})); // 🚴‍♀️
    m.insert(QStringLiteral("bikini"), e({0x1F459})); // 👙
    m.insert(QStringLiteral("billed_cap"), e({0x1F9E2})); // 🧢
    m.insert(QStringLiteral("biohazard"), e({0x2623})); // ☣
    m.insert(QStringLiteral("bird"), e({0x1F426})); // 🐦
    m.insert(QStringLiteral("birthday"), e({0x1F382})); // 🎂
    m.insert(QStringLiteral("bison"), e({0x1F9AC})); // 🦬
    m.insert(QStringLiteral("biting_lip"), e({0x1FAE6})); // 🫦
    m.insert(QStringLiteral("black_bird"), e({0x1F426, 0x200D, 0x2B1B})); // 🐦‍⬛
    m.insert(QStringLiteral("black_cat"), e({0x1F408, 0x200D, 0x2B1B})); // 🐈‍⬛
    m.insert(QStringLiteral("black_circle"), e({0x26AB})); // ⚫
    m.insert(QStringLiteral("black_flag"), e({0x1F3F4})); // 🏴
    m.insert(QStringLiteral("black_heart"), e({0x1F5A4})); // 🖤
    m.insert(QStringLiteral("black_joker"), e({0x1F0CF})); // 🃏
    m.insert(QStringLiteral("black_large_square"), e({0x2B1B})); // ⬛
    m.insert(QStringLiteral("black_medium_small_square"), e({0x25FE})); // ◾
    m.insert(QStringLiteral("black_medium_square"), e({0x25FC})); // ◼
    m.insert(QStringLiteral("black_nib"), e({0x2712})); // ✒
    m.insert(QStringLiteral("black_small_square"), e({0x25AA})); // ▪
    m.insert(QStringLiteral("black_square_button"), e({0x1F532})); // 🔲
    m.insert(QStringLiteral("blond_haired_man"), e({0x1F471, 0x200D, 0x2642, 0xFE0F})); // 👱‍♂️
    m.insert(QStringLiteral("blond_haired_person"), e({0x1F471})); // 👱
    m.insert(QStringLiteral("blond_haired_woman"), e({0x1F471, 0x200D, 0x2640, 0xFE0F})); // 👱‍♀️
    m.insert(QStringLiteral("blonde_woman"), e({0x1F471, 0x200D, 0x2640, 0xFE0F})); // 👱‍♀️
    m.insert(QStringLiteral("blossom"), e({0x1F33C})); // 🌼
    m.insert(QStringLiteral("blowfish"), e({0x1F421})); // 🐡
    m.insert(QStringLiteral("blue_book"), e({0x1F4D8})); // 📘
    m.insert(QStringLiteral("blue_car"), e({0x1F699})); // 🚙
    m.insert(QStringLiteral("blue_heart"), e({0x1F499})); // 💙
    m.insert(QStringLiteral("blue_square"), e({0x1F7E6})); // 🟦
    m.insert(QStringLiteral("blueberries"), e({0x1FAD0})); // 🫐
    m.insert(QStringLiteral("blush"), e({0x1F60A})); // 😊
    m.insert(QStringLiteral("boar"), e({0x1F417})); // 🐗
    m.insert(QStringLiteral("boat"), e({0x26F5})); // ⛵
    m.insert(QStringLiteral("bolivia"), e({0x1F1E7, 0x1F1F4})); // 🇧🇴
    m.insert(QStringLiteral("bomb"), e({0x1F4A3})); // 💣
    m.insert(QStringLiteral("bone"), e({0x1F9B4})); // 🦴
    m.insert(QStringLiteral("book"), e({0x1F4D6})); // 📖
    m.insert(QStringLiteral("bookmark"), e({0x1F516})); // 🔖
    m.insert(QStringLiteral("bookmark_tabs"), e({0x1F4D1})); // 📑
    m.insert(QStringLiteral("books"), e({0x1F4DA})); // 📚
    m.insert(QStringLiteral("boom"), e({0x1F4A5})); // 💥
    m.insert(QStringLiteral("boomerang"), e({0x1FA83})); // 🪃
    m.insert(QStringLiteral("boot"), e({0x1F462})); // 👢
    m.insert(QStringLiteral("bosnia_herzegovina"), e({0x1F1E7, 0x1F1E6})); // 🇧🇦
    m.insert(QStringLiteral("botswana"), e({0x1F1E7, 0x1F1FC})); // 🇧🇼
    m.insert(QStringLiteral("bouncing_ball_man"), e({0x26F9, 0xFE0F, 0x200D, 0x2642, 0xFE0F})); // ⛹️‍♂️
    m.insert(QStringLiteral("bouncing_ball_person"), e({0x26F9})); // ⛹
    m.insert(QStringLiteral("bouncing_ball_woman"), e({0x26F9, 0xFE0F, 0x200D, 0x2640, 0xFE0F})); // ⛹️‍♀️
    m.insert(QStringLiteral("bouquet"), e({0x1F490})); // 💐
    m.insert(QStringLiteral("bouvet_island"), e({0x1F1E7, 0x1F1FB})); // 🇧🇻
    m.insert(QStringLiteral("bow"), e({0x1F647})); // 🙇
    m.insert(QStringLiteral("bow_and_arrow"), e({0x1F3F9})); // 🏹
    m.insert(QStringLiteral("bowing_man"), e({0x1F647, 0x200D, 0x2642, 0xFE0F})); // 🙇‍♂️
    m.insert(QStringLiteral("bowing_woman"), e({0x1F647, 0x200D, 0x2640, 0xFE0F})); // 🙇‍♀️
    m.insert(QStringLiteral("bowl_with_spoon"), e({0x1F963})); // 🥣
    m.insert(QStringLiteral("bowling"), e({0x1F3B3})); // 🎳
    m.insert(QStringLiteral("boxing_glove"), e({0x1F94A})); // 🥊
    m.insert(QStringLiteral("boy"), e({0x1F466})); // 👦
    m.insert(QStringLiteral("brain"), e({0x1F9E0})); // 🧠
    m.insert(QStringLiteral("brazil"), e({0x1F1E7, 0x1F1F7})); // 🇧🇷
    m.insert(QStringLiteral("bread"), e({0x1F35E})); // 🍞
    m.insert(QStringLiteral("breast_feeding"), e({0x1F931})); // 🤱
    m.insert(QStringLiteral("bricks"), e({0x1F9F1})); // 🧱
    m.insert(QStringLiteral("bride_with_veil"), e({0x1F470, 0x200D, 0x2640, 0xFE0F})); // 👰‍♀️
    m.insert(QStringLiteral("bridge_at_night"), e({0x1F309})); // 🌉
    m.insert(QStringLiteral("briefcase"), e({0x1F4BC})); // 💼
    m.insert(QStringLiteral("british_indian_ocean_territory"), e({0x1F1EE, 0x1F1F4})); // 🇮🇴
    m.insert(QStringLiteral("british_virgin_islands"), e({0x1F1FB, 0x1F1EC})); // 🇻🇬
    m.insert(QStringLiteral("broccoli"), e({0x1F966})); // 🥦
    m.insert(QStringLiteral("broken_heart"), e({0x1F494})); // 💔
    m.insert(QStringLiteral("broom"), e({0x1F9F9})); // 🧹
    m.insert(QStringLiteral("brown_circle"), e({0x1F7E4})); // 🟤
    m.insert(QStringLiteral("brown_heart"), e({0x1F90E})); // 🤎
    m.insert(QStringLiteral("brown_square"), e({0x1F7EB})); // 🟫
    m.insert(QStringLiteral("brunei"), e({0x1F1E7, 0x1F1F3})); // 🇧🇳
    m.insert(QStringLiteral("bubble_tea"), e({0x1F9CB})); // 🧋
    m.insert(QStringLiteral("bubbles"), e({0x1FAE7})); // 🫧
    m.insert(QStringLiteral("bucket"), e({0x1FAA3})); // 🪣
    m.insert(QStringLiteral("bug"), e({0x1F41B})); // 🐛
    m.insert(QStringLiteral("building_construction"), e({0x1F3D7})); // 🏗
    m.insert(QStringLiteral("bulb"), e({0x1F4A1})); // 💡
    m.insert(QStringLiteral("bulgaria"), e({0x1F1E7, 0x1F1EC})); // 🇧🇬
    m.insert(QStringLiteral("bullettrain_front"), e({0x1F685})); // 🚅
    m.insert(QStringLiteral("bullettrain_side"), e({0x1F684})); // 🚄
    m.insert(QStringLiteral("burkina_faso"), e({0x1F1E7, 0x1F1EB})); // 🇧🇫
    m.insert(QStringLiteral("burrito"), e({0x1F32F})); // 🌯
    m.insert(QStringLiteral("burundi"), e({0x1F1E7, 0x1F1EE})); // 🇧🇮
    m.insert(QStringLiteral("bus"), e({0x1F68C})); // 🚌
    m.insert(QStringLiteral("business_suit_levitating"), e({0x1F574})); // 🕴
    m.insert(QStringLiteral("busstop"), e({0x1F68F})); // 🚏
    m.insert(QStringLiteral("bust_in_silhouette"), e({0x1F464})); // 👤
    m.insert(QStringLiteral("busts_in_silhouette"), e({0x1F465})); // 👥
    m.insert(QStringLiteral("butter"), e({0x1F9C8})); // 🧈
    m.insert(QStringLiteral("butterfly"), e({0x1F98B})); // 🦋
    m.insert(QStringLiteral("cactus"), e({0x1F335})); // 🌵
    m.insert(QStringLiteral("cake"), e({0x1F370})); // 🍰
    m.insert(QStringLiteral("calendar"), e({0x1F4C6})); // 📆
    m.insert(QStringLiteral("call_me_hand"), e({0x1F919})); // 🤙
    m.insert(QStringLiteral("calling"), e({0x1F4F2})); // 📲
    m.insert(QStringLiteral("cambodia"), e({0x1F1F0, 0x1F1ED})); // 🇰🇭
    m.insert(QStringLiteral("camel"), e({0x1F42B})); // 🐫
    m.insert(QStringLiteral("camera"), e({0x1F4F7})); // 📷
    m.insert(QStringLiteral("camera_flash"), e({0x1F4F8})); // 📸
    m.insert(QStringLiteral("cameroon"), e({0x1F1E8, 0x1F1F2})); // 🇨🇲
    m.insert(QStringLiteral("camping"), e({0x1F3D5})); // 🏕
    m.insert(QStringLiteral("canada"), e({0x1F1E8, 0x1F1E6})); // 🇨🇦
    m.insert(QStringLiteral("canary_islands"), e({0x1F1EE, 0x1F1E8})); // 🇮🇨
    m.insert(QStringLiteral("cancer"), e({0x264B})); // ♋
    m.insert(QStringLiteral("candle"), e({0x1F56F})); // 🕯
    m.insert(QStringLiteral("candy"), e({0x1F36C})); // 🍬
    m.insert(QStringLiteral("canned_food"), e({0x1F96B})); // 🥫
    m.insert(QStringLiteral("canoe"), e({0x1F6F6})); // 🛶
    m.insert(QStringLiteral("cape_verde"), e({0x1F1E8, 0x1F1FB})); // 🇨🇻
    m.insert(QStringLiteral("capital_abcd"), e({0x1F520})); // 🔠
    m.insert(QStringLiteral("capricorn"), e({0x2651})); // ♑
    m.insert(QStringLiteral("car"), e({0x1F697})); // 🚗
    m.insert(QStringLiteral("card_file_box"), e({0x1F5C3})); // 🗃
    m.insert(QStringLiteral("card_index"), e({0x1F4C7})); // 📇
    m.insert(QStringLiteral("card_index_dividers"), e({0x1F5C2})); // 🗂
    m.insert(QStringLiteral("caribbean_netherlands"), e({0x1F1E7, 0x1F1F6})); // 🇧🇶
    m.insert(QStringLiteral("carousel_horse"), e({0x1F3A0})); // 🎠
    m.insert(QStringLiteral("carpentry_saw"), e({0x1FA9A})); // 🪚
    m.insert(QStringLiteral("carrot"), e({0x1F955})); // 🥕
    m.insert(QStringLiteral("cartwheeling"), e({0x1F938})); // 🤸
    m.insert(QStringLiteral("cat"), e({0x1F431})); // 🐱
    m.insert(QStringLiteral("cat2"), e({0x1F408})); // 🐈
    m.insert(QStringLiteral("cayman_islands"), e({0x1F1F0, 0x1F1FE})); // 🇰🇾
    m.insert(QStringLiteral("cd"), e({0x1F4BF})); // 💿
    m.insert(QStringLiteral("central_african_republic"), e({0x1F1E8, 0x1F1EB})); // 🇨🇫
    m.insert(QStringLiteral("ceuta_melilla"), e({0x1F1EA, 0x1F1E6})); // 🇪🇦
    m.insert(QStringLiteral("chad"), e({0x1F1F9, 0x1F1E9})); // 🇹🇩
    m.insert(QStringLiteral("chains"), e({0x26D3})); // ⛓
    m.insert(QStringLiteral("chair"), e({0x1FA91})); // 🪑
    m.insert(QStringLiteral("champagne"), e({0x1F37E})); // 🍾
    m.insert(QStringLiteral("chart"), e({0x1F4B9})); // 💹
    m.insert(QStringLiteral("chart_with_downwards_trend"), e({0x1F4C9})); // 📉
    m.insert(QStringLiteral("chart_with_upwards_trend"), e({0x1F4C8})); // 📈
    m.insert(QStringLiteral("checkered_flag"), e({0x1F3C1})); // 🏁
    m.insert(QStringLiteral("cheese"), e({0x1F9C0})); // 🧀
    m.insert(QStringLiteral("cherries"), e({0x1F352})); // 🍒
    m.insert(QStringLiteral("cherry_blossom"), e({0x1F338})); // 🌸
    m.insert(QStringLiteral("chess_pawn"), e({0x265F})); // ♟
    m.insert(QStringLiteral("chestnut"), e({0x1F330})); // 🌰
    m.insert(QStringLiteral("chicken"), e({0x1F414})); // 🐔
    m.insert(QStringLiteral("child"), e({0x1F9D2})); // 🧒
    m.insert(QStringLiteral("children_crossing"), e({0x1F6B8})); // 🚸
    m.insert(QStringLiteral("chile"), e({0x1F1E8, 0x1F1F1})); // 🇨🇱
    m.insert(QStringLiteral("chipmunk"), e({0x1F43F})); // 🐿
    m.insert(QStringLiteral("chocolate_bar"), e({0x1F36B})); // 🍫
    m.insert(QStringLiteral("chopsticks"), e({0x1F962})); // 🥢
    m.insert(QStringLiteral("christmas_island"), e({0x1F1E8, 0x1F1FD})); // 🇨🇽
    m.insert(QStringLiteral("christmas_tree"), e({0x1F384})); // 🎄
    m.insert(QStringLiteral("church"), e({0x26EA})); // ⛪
    m.insert(QStringLiteral("cinema"), e({0x1F3A6})); // 🎦
    m.insert(QStringLiteral("circus_tent"), e({0x1F3AA})); // 🎪
    m.insert(QStringLiteral("city_sunrise"), e({0x1F307})); // 🌇
    m.insert(QStringLiteral("city_sunset"), e({0x1F306})); // 🌆
    m.insert(QStringLiteral("cityscape"), e({0x1F3D9})); // 🏙
    m.insert(QStringLiteral("cl"), e({0x1F191})); // 🆑
    m.insert(QStringLiteral("clamp"), e({0x1F5DC})); // 🗜
    m.insert(QStringLiteral("clap"), e({0x1F44F})); // 👏
    m.insert(QStringLiteral("clapper"), e({0x1F3AC})); // 🎬
    m.insert(QStringLiteral("classical_building"), e({0x1F3DB})); // 🏛
    m.insert(QStringLiteral("climbing"), e({0x1F9D7})); // 🧗
    m.insert(QStringLiteral("climbing_man"), e({0x1F9D7, 0x200D, 0x2642, 0xFE0F})); // 🧗‍♂️
    m.insert(QStringLiteral("climbing_woman"), e({0x1F9D7, 0x200D, 0x2640, 0xFE0F})); // 🧗‍♀️
    m.insert(QStringLiteral("clinking_glasses"), e({0x1F942})); // 🥂
    m.insert(QStringLiteral("clipboard"), e({0x1F4CB})); // 📋
    m.insert(QStringLiteral("clipperton_island"), e({0x1F1E8, 0x1F1F5})); // 🇨🇵
    m.insert(QStringLiteral("clock1"), e({0x1F550})); // 🕐
    m.insert(QStringLiteral("clock10"), e({0x1F559})); // 🕙
    m.insert(QStringLiteral("clock1030"), e({0x1F565})); // 🕥
    m.insert(QStringLiteral("clock11"), e({0x1F55A})); // 🕚
    m.insert(QStringLiteral("clock1130"), e({0x1F566})); // 🕦
    m.insert(QStringLiteral("clock12"), e({0x1F55B})); // 🕛
    m.insert(QStringLiteral("clock1230"), e({0x1F567})); // 🕧
    m.insert(QStringLiteral("clock130"), e({0x1F55C})); // 🕜
    m.insert(QStringLiteral("clock2"), e({0x1F551})); // 🕑
    m.insert(QStringLiteral("clock230"), e({0x1F55D})); // 🕝
    m.insert(QStringLiteral("clock3"), e({0x1F552})); // 🕒
    m.insert(QStringLiteral("clock330"), e({0x1F55E})); // 🕞
    m.insert(QStringLiteral("clock4"), e({0x1F553})); // 🕓
    m.insert(QStringLiteral("clock430"), e({0x1F55F})); // 🕟
    m.insert(QStringLiteral("clock5"), e({0x1F554})); // 🕔
    m.insert(QStringLiteral("clock530"), e({0x1F560})); // 🕠
    m.insert(QStringLiteral("clock6"), e({0x1F555})); // 🕕
    m.insert(QStringLiteral("clock630"), e({0x1F561})); // 🕡
    m.insert(QStringLiteral("clock7"), e({0x1F556})); // 🕖
    m.insert(QStringLiteral("clock730"), e({0x1F562})); // 🕢
    m.insert(QStringLiteral("clock8"), e({0x1F557})); // 🕗
    m.insert(QStringLiteral("clock830"), e({0x1F563})); // 🕣
    m.insert(QStringLiteral("clock9"), e({0x1F558})); // 🕘
    m.insert(QStringLiteral("clock930"), e({0x1F564})); // 🕤
    m.insert(QStringLiteral("closed_book"), e({0x1F4D5})); // 📕
    m.insert(QStringLiteral("closed_lock_with_key"), e({0x1F510})); // 🔐
    m.insert(QStringLiteral("closed_umbrella"), e({0x1F302})); // 🌂
    m.insert(QStringLiteral("cloud"), e({0x2601})); // ☁
    m.insert(QStringLiteral("cloud_with_lightning"), e({0x1F329})); // 🌩
    m.insert(QStringLiteral("cloud_with_lightning_and_rain"), e({0x26C8})); // ⛈
    m.insert(QStringLiteral("cloud_with_rain"), e({0x1F327})); // 🌧
    m.insert(QStringLiteral("cloud_with_snow"), e({0x1F328})); // 🌨
    m.insert(QStringLiteral("clown_face"), e({0x1F921})); // 🤡
    m.insert(QStringLiteral("clubs"), e({0x2663})); // ♣
    m.insert(QStringLiteral("cn"), e({0x1F1E8, 0x1F1F3})); // 🇨🇳
    m.insert(QStringLiteral("coat"), e({0x1F9E5})); // 🧥
    m.insert(QStringLiteral("cockroach"), e({0x1FAB3})); // 🪳
    m.insert(QStringLiteral("cocktail"), e({0x1F378})); // 🍸
    m.insert(QStringLiteral("coconut"), e({0x1F965})); // 🥥
    m.insert(QStringLiteral("cocos_islands"), e({0x1F1E8, 0x1F1E8})); // 🇨🇨
    m.insert(QStringLiteral("coffee"), e({0x2615})); // ☕
    m.insert(QStringLiteral("coffin"), e({0x26B0})); // ⚰
    m.insert(QStringLiteral("coin"), e({0x1FA99})); // 🪙
    m.insert(QStringLiteral("cold_face"), e({0x1F976})); // 🥶
    m.insert(QStringLiteral("cold_sweat"), e({0x1F630})); // 😰
    m.insert(QStringLiteral("collision"), e({0x1F4A5})); // 💥
    m.insert(QStringLiteral("colombia"), e({0x1F1E8, 0x1F1F4})); // 🇨🇴
    m.insert(QStringLiteral("comet"), e({0x2604})); // ☄
    m.insert(QStringLiteral("comoros"), e({0x1F1F0, 0x1F1F2})); // 🇰🇲
    m.insert(QStringLiteral("compass"), e({0x1F9ED})); // 🧭
    m.insert(QStringLiteral("computer"), e({0x1F4BB})); // 💻
    m.insert(QStringLiteral("computer_mouse"), e({0x1F5B1})); // 🖱
    m.insert(QStringLiteral("confetti_ball"), e({0x1F38A})); // 🎊
    m.insert(QStringLiteral("confounded"), e({0x1F616})); // 😖
    m.insert(QStringLiteral("confused"), e({0x1F615})); // 😕
    m.insert(QStringLiteral("congo_brazzaville"), e({0x1F1E8, 0x1F1EC})); // 🇨🇬
    m.insert(QStringLiteral("congo_kinshasa"), e({0x1F1E8, 0x1F1E9})); // 🇨🇩
    m.insert(QStringLiteral("congratulations"), e({0x3297})); // ㊗
    m.insert(QStringLiteral("construction"), e({0x1F6A7})); // 🚧
    m.insert(QStringLiteral("construction_worker"), e({0x1F477})); // 👷
    m.insert(QStringLiteral("construction_worker_man"), e({0x1F477, 0x200D, 0x2642, 0xFE0F})); // 👷‍♂️
    m.insert(QStringLiteral("construction_worker_woman"), e({0x1F477, 0x200D, 0x2640, 0xFE0F})); // 👷‍♀️
    m.insert(QStringLiteral("control_knobs"), e({0x1F39B})); // 🎛
    m.insert(QStringLiteral("convenience_store"), e({0x1F3EA})); // 🏪
    m.insert(QStringLiteral("cook"), e({0x1F9D1, 0x200D, 0x1F373})); // 🧑‍🍳
    m.insert(QStringLiteral("cook_islands"), e({0x1F1E8, 0x1F1F0})); // 🇨🇰
    m.insert(QStringLiteral("cookie"), e({0x1F36A})); // 🍪
    m.insert(QStringLiteral("cool"), e({0x1F192})); // 🆒
    m.insert(QStringLiteral("cop"), e({0x1F46E})); // 👮
    m.insert(QStringLiteral("copyright"), e({0xA9})); // ©
    m.insert(QStringLiteral("coral"), e({0x1FAB8})); // 🪸
    m.insert(QStringLiteral("corn"), e({0x1F33D})); // 🌽
    m.insert(QStringLiteral("costa_rica"), e({0x1F1E8, 0x1F1F7})); // 🇨🇷
    m.insert(QStringLiteral("cote_divoire"), e({0x1F1E8, 0x1F1EE})); // 🇨🇮
    m.insert(QStringLiteral("couch_and_lamp"), e({0x1F6CB})); // 🛋
    m.insert(QStringLiteral("couple"), e({0x1F46B})); // 👫
    m.insert(QStringLiteral("couple_with_heart"), e({0x1F491})); // 💑
    m.insert(QStringLiteral("couple_with_heart_man_man"), e({0x1F468, 0x200D, 0x2764, 0xFE0F, 0x200D, 0x1F468})); // 👨‍❤️‍👨
    m.insert(QStringLiteral("couple_with_heart_woman_man"), e({0x1F469, 0x200D, 0x2764, 0xFE0F, 0x200D, 0x1F468})); // 👩‍❤️‍👨
    m.insert(QStringLiteral("couple_with_heart_woman_woman"), e({0x1F469, 0x200D, 0x2764, 0xFE0F, 0x200D, 0x1F469})); // 👩‍❤️‍👩
    m.insert(QStringLiteral("couplekiss"), e({0x1F48F})); // 💏
    m.insert(QStringLiteral("couplekiss_man_man"), e({0x1F468, 0x200D, 0x2764, 0xFE0F, 0x200D, 0x1F48B, 0x200D, 0x1F468})); // 👨‍❤️‍💋‍👨
    m.insert(QStringLiteral("couplekiss_man_woman"), e({0x1F469, 0x200D, 0x2764, 0xFE0F, 0x200D, 0x1F48B, 0x200D, 0x1F468})); // 👩‍❤️‍💋‍👨
    m.insert(QStringLiteral("couplekiss_woman_woman"), e({0x1F469, 0x200D, 0x2764, 0xFE0F, 0x200D, 0x1F48B, 0x200D, 0x1F469})); // 👩‍❤️‍💋‍👩
    m.insert(QStringLiteral("cow"), e({0x1F42E})); // 🐮
    m.insert(QStringLiteral("cow2"), e({0x1F404})); // 🐄
    m.insert(QStringLiteral("cowboy_hat_face"), e({0x1F920})); // 🤠
    m.insert(QStringLiteral("crab"), e({0x1F980})); // 🦀
    m.insert(QStringLiteral("crayon"), e({0x1F58D})); // 🖍
    m.insert(QStringLiteral("credit_card"), e({0x1F4B3})); // 💳
    m.insert(QStringLiteral("crescent_moon"), e({0x1F319})); // 🌙
    m.insert(QStringLiteral("cricket"), e({0x1F997})); // 🦗
    m.insert(QStringLiteral("cricket_game"), e({0x1F3CF})); // 🏏
    m.insert(QStringLiteral("croatia"), e({0x1F1ED, 0x1F1F7})); // 🇭🇷
    m.insert(QStringLiteral("crocodile"), e({0x1F40A})); // 🐊
    m.insert(QStringLiteral("croissant"), e({0x1F950})); // 🥐
    m.insert(QStringLiteral("crossed_fingers"), e({0x1F91E})); // 🤞
    m.insert(QStringLiteral("crossed_flags"), e({0x1F38C})); // 🎌
    m.insert(QStringLiteral("crossed_swords"), e({0x2694})); // ⚔
    m.insert(QStringLiteral("crown"), e({0x1F451})); // 👑
    m.insert(QStringLiteral("crutch"), e({0x1FA7C})); // 🩼
    m.insert(QStringLiteral("cry"), e({0x1F622})); // 😢
    m.insert(QStringLiteral("crying_cat_face"), e({0x1F63F})); // 😿
    m.insert(QStringLiteral("crystal_ball"), e({0x1F52E})); // 🔮
    m.insert(QStringLiteral("cuba"), e({0x1F1E8, 0x1F1FA})); // 🇨🇺
    m.insert(QStringLiteral("cucumber"), e({0x1F952})); // 🥒
    m.insert(QStringLiteral("cup_with_straw"), e({0x1F964})); // 🥤
    m.insert(QStringLiteral("cupcake"), e({0x1F9C1})); // 🧁
    m.insert(QStringLiteral("cupid"), e({0x1F498})); // 💘
    m.insert(QStringLiteral("curacao"), e({0x1F1E8, 0x1F1FC})); // 🇨🇼
    m.insert(QStringLiteral("curling_stone"), e({0x1F94C})); // 🥌
    m.insert(QStringLiteral("curly_haired_man"), e({0x1F468, 0x200D, 0x1F9B1})); // 👨‍🦱
    m.insert(QStringLiteral("curly_haired_woman"), e({0x1F469, 0x200D, 0x1F9B1})); // 👩‍🦱
    m.insert(QStringLiteral("curly_loop"), e({0x27B0})); // ➰
    m.insert(QStringLiteral("currency_exchange"), e({0x1F4B1})); // 💱
    m.insert(QStringLiteral("curry"), e({0x1F35B})); // 🍛
    m.insert(QStringLiteral("cursing_face"), e({0x1F92C})); // 🤬
    m.insert(QStringLiteral("custard"), e({0x1F36E})); // 🍮
    m.insert(QStringLiteral("customs"), e({0x1F6C3})); // 🛃
    m.insert(QStringLiteral("cut_of_meat"), e({0x1F969})); // 🥩
    m.insert(QStringLiteral("cyclone"), e({0x1F300})); // 🌀
    m.insert(QStringLiteral("cyprus"), e({0x1F1E8, 0x1F1FE})); // 🇨🇾
    m.insert(QStringLiteral("czech_republic"), e({0x1F1E8, 0x1F1FF})); // 🇨🇿
    m.insert(QStringLiteral("dagger"), e({0x1F5E1})); // 🗡
    m.insert(QStringLiteral("dancer"), e({0x1F483})); // 💃
    m.insert(QStringLiteral("dancers"), e({0x1F46F})); // 👯
    m.insert(QStringLiteral("dancing_men"), e({0x1F46F, 0x200D, 0x2642, 0xFE0F})); // 👯‍♂️
    m.insert(QStringLiteral("dancing_women"), e({0x1F46F, 0x200D, 0x2640, 0xFE0F})); // 👯‍♀️
    m.insert(QStringLiteral("dango"), e({0x1F361})); // 🍡
    m.insert(QStringLiteral("dark_sunglasses"), e({0x1F576})); // 🕶
    m.insert(QStringLiteral("dart"), e({0x1F3AF})); // 🎯
    m.insert(QStringLiteral("dash"), e({0x1F4A8})); // 💨
    m.insert(QStringLiteral("date"), e({0x1F4C5})); // 📅
    m.insert(QStringLiteral("de"), e({0x1F1E9, 0x1F1EA})); // 🇩🇪
    m.insert(QStringLiteral("deaf_man"), e({0x1F9CF, 0x200D, 0x2642, 0xFE0F})); // 🧏‍♂️
    m.insert(QStringLiteral("deaf_person"), e({0x1F9CF})); // 🧏
    m.insert(QStringLiteral("deaf_woman"), e({0x1F9CF, 0x200D, 0x2640, 0xFE0F})); // 🧏‍♀️
    m.insert(QStringLiteral("deciduous_tree"), e({0x1F333})); // 🌳
    m.insert(QStringLiteral("deer"), e({0x1F98C})); // 🦌
    m.insert(QStringLiteral("denmark"), e({0x1F1E9, 0x1F1F0})); // 🇩🇰
    m.insert(QStringLiteral("department_store"), e({0x1F3EC})); // 🏬
    m.insert(QStringLiteral("derelict_house"), e({0x1F3DA})); // 🏚
    m.insert(QStringLiteral("desert"), e({0x1F3DC})); // 🏜
    m.insert(QStringLiteral("desert_island"), e({0x1F3DD})); // 🏝
    m.insert(QStringLiteral("desktop_computer"), e({0x1F5A5})); // 🖥
    m.insert(QStringLiteral("detective"), e({0x1F575})); // 🕵
    m.insert(QStringLiteral("diamond_shape_with_a_dot_inside"), e({0x1F4A0})); // 💠
    m.insert(QStringLiteral("diamonds"), e({0x2666})); // ♦
    m.insert(QStringLiteral("diego_garcia"), e({0x1F1E9, 0x1F1EC})); // 🇩🇬
    m.insert(QStringLiteral("disappointed"), e({0x1F61E})); // 😞
    m.insert(QStringLiteral("disappointed_relieved"), e({0x1F625})); // 😥
    m.insert(QStringLiteral("disguised_face"), e({0x1F978})); // 🥸
    m.insert(QStringLiteral("diving_mask"), e({0x1F93F})); // 🤿
    m.insert(QStringLiteral("diya_lamp"), e({0x1FA94})); // 🪔
    m.insert(QStringLiteral("dizzy"), e({0x1F4AB})); // 💫
    m.insert(QStringLiteral("dizzy_face"), e({0x1F635})); // 😵
    m.insert(QStringLiteral("djibouti"), e({0x1F1E9, 0x1F1EF})); // 🇩🇯
    m.insert(QStringLiteral("dna"), e({0x1F9EC})); // 🧬
    m.insert(QStringLiteral("do_not_litter"), e({0x1F6AF})); // 🚯
    m.insert(QStringLiteral("dodo"), e({0x1F9A4})); // 🦤
    m.insert(QStringLiteral("dog"), e({0x1F436})); // 🐶
    m.insert(QStringLiteral("dog2"), e({0x1F415})); // 🐕
    m.insert(QStringLiteral("dollar"), e({0x1F4B5})); // 💵
    m.insert(QStringLiteral("dolls"), e({0x1F38E})); // 🎎
    m.insert(QStringLiteral("dolphin"), e({0x1F42C})); // 🐬
    m.insert(QStringLiteral("dominica"), e({0x1F1E9, 0x1F1F2})); // 🇩🇲
    m.insert(QStringLiteral("dominican_republic"), e({0x1F1E9, 0x1F1F4})); // 🇩🇴
    m.insert(QStringLiteral("donkey"), e({0x1FACF})); // 🫏
    m.insert(QStringLiteral("door"), e({0x1F6AA})); // 🚪
    m.insert(QStringLiteral("dotted_line_face"), e({0x1FAE5})); // 🫥
    m.insert(QStringLiteral("doughnut"), e({0x1F369})); // 🍩
    m.insert(QStringLiteral("dove"), e({0x1F54A})); // 🕊
    m.insert(QStringLiteral("dragon"), e({0x1F409})); // 🐉
    m.insert(QStringLiteral("dragon_face"), e({0x1F432})); // 🐲
    m.insert(QStringLiteral("dress"), e({0x1F457})); // 👗
    m.insert(QStringLiteral("dromedary_camel"), e({0x1F42A})); // 🐪
    m.insert(QStringLiteral("drooling_face"), e({0x1F924})); // 🤤
    m.insert(QStringLiteral("drop_of_blood"), e({0x1FA78})); // 🩸
    m.insert(QStringLiteral("droplet"), e({0x1F4A7})); // 💧
    m.insert(QStringLiteral("drum"), e({0x1F941})); // 🥁
    m.insert(QStringLiteral("duck"), e({0x1F986})); // 🦆
    m.insert(QStringLiteral("dumpling"), e({0x1F95F})); // 🥟
    m.insert(QStringLiteral("dvd"), e({0x1F4C0})); // 📀
    m.insert(QStringLiteral("e-mail"), e({0x1F4E7})); // 📧
    m.insert(QStringLiteral("eagle"), e({0x1F985})); // 🦅
    m.insert(QStringLiteral("ear"), e({0x1F442})); // 👂
    m.insert(QStringLiteral("ear_of_rice"), e({0x1F33E})); // 🌾
    m.insert(QStringLiteral("ear_with_hearing_aid"), e({0x1F9BB})); // 🦻
    m.insert(QStringLiteral("earth_africa"), e({0x1F30D})); // 🌍
    m.insert(QStringLiteral("earth_americas"), e({0x1F30E})); // 🌎
    m.insert(QStringLiteral("earth_asia"), e({0x1F30F})); // 🌏
    m.insert(QStringLiteral("ecuador"), e({0x1F1EA, 0x1F1E8})); // 🇪🇨
    m.insert(QStringLiteral("egg"), e({0x1F95A})); // 🥚
    m.insert(QStringLiteral("eggplant"), e({0x1F346})); // 🍆
    m.insert(QStringLiteral("egypt"), e({0x1F1EA, 0x1F1EC})); // 🇪🇬
    m.insert(QStringLiteral("eight"), e({0x38, 0xFE0F, 0x20E3})); // 8️⃣
    m.insert(QStringLiteral("eight_pointed_black_star"), e({0x2734})); // ✴
    m.insert(QStringLiteral("eight_spoked_asterisk"), e({0x2733})); // ✳
    m.insert(QStringLiteral("eject_button"), e({0x23CF})); // ⏏
    m.insert(QStringLiteral("el_salvador"), e({0x1F1F8, 0x1F1FB})); // 🇸🇻
    m.insert(QStringLiteral("electric_plug"), e({0x1F50C})); // 🔌
    m.insert(QStringLiteral("elephant"), e({0x1F418})); // 🐘
    m.insert(QStringLiteral("elevator"), e({0x1F6D7})); // 🛗
    m.insert(QStringLiteral("elf"), e({0x1F9DD})); // 🧝
    m.insert(QStringLiteral("elf_man"), e({0x1F9DD, 0x200D, 0x2642, 0xFE0F})); // 🧝‍♂️
    m.insert(QStringLiteral("elf_woman"), e({0x1F9DD, 0x200D, 0x2640, 0xFE0F})); // 🧝‍♀️
    m.insert(QStringLiteral("email"), e({0x1F4E7})); // 📧
    m.insert(QStringLiteral("empty_nest"), e({0x1FAB9})); // 🪹
    m.insert(QStringLiteral("end"), e({0x1F51A})); // 🔚
    m.insert(QStringLiteral("england"), e({0x1F3F4, 0xE0067, 0xE0062, 0xE0065, 0xE006E, 0xE0067, 0xE007F})); // 🏴󠁧󠁢󠁥󠁮󠁧󠁿
    m.insert(QStringLiteral("envelope"), e({0x2709})); // ✉
    m.insert(QStringLiteral("envelope_with_arrow"), e({0x1F4E9})); // 📩
    m.insert(QStringLiteral("equatorial_guinea"), e({0x1F1EC, 0x1F1F6})); // 🇬🇶
    m.insert(QStringLiteral("eritrea"), e({0x1F1EA, 0x1F1F7})); // 🇪🇷
    m.insert(QStringLiteral("es"), e({0x1F1EA, 0x1F1F8})); // 🇪🇸
    m.insert(QStringLiteral("estonia"), e({0x1F1EA, 0x1F1EA})); // 🇪🇪
    m.insert(QStringLiteral("ethiopia"), e({0x1F1EA, 0x1F1F9})); // 🇪🇹
    m.insert(QStringLiteral("eu"), e({0x1F1EA, 0x1F1FA})); // 🇪🇺
    m.insert(QStringLiteral("euro"), e({0x1F4B6})); // 💶
    m.insert(QStringLiteral("european_castle"), e({0x1F3F0})); // 🏰
    m.insert(QStringLiteral("european_post_office"), e({0x1F3E4})); // 🏤
    m.insert(QStringLiteral("european_union"), e({0x1F1EA, 0x1F1FA})); // 🇪🇺
    m.insert(QStringLiteral("evergreen_tree"), e({0x1F332})); // 🌲
    m.insert(QStringLiteral("exclamation"), e({0x2757})); // ❗
    m.insert(QStringLiteral("exploding_head"), e({0x1F92F})); // 🤯
    m.insert(QStringLiteral("expressionless"), e({0x1F611})); // 😑
    m.insert(QStringLiteral("eye"), e({0x1F441})); // 👁
    m.insert(QStringLiteral("eye_speech_bubble"), e({0x1F441, 0xFE0F, 0x200D, 0x1F5E8, 0xFE0F})); // 👁️‍🗨️
    m.insert(QStringLiteral("eyeglasses"), e({0x1F453})); // 👓
    m.insert(QStringLiteral("eyes"), e({0x1F440})); // 👀
    m.insert(QStringLiteral("face_exhaling"), e({0x1F62E, 0x200D, 0x1F4A8})); // 😮‍💨
    m.insert(QStringLiteral("face_holding_back_tears"), e({0x1F979})); // 🥹
    m.insert(QStringLiteral("face_in_clouds"), e({0x1F636, 0x200D, 0x1F32B, 0xFE0F})); // 😶‍🌫️
    m.insert(QStringLiteral("face_with_diagonal_mouth"), e({0x1FAE4})); // 🫤
    m.insert(QStringLiteral("face_with_head_bandage"), e({0x1F915})); // 🤕
    m.insert(QStringLiteral("face_with_open_eyes_and_hand_over_mouth"), e({0x1FAE2})); // 🫢
    m.insert(QStringLiteral("face_with_peeking_eye"), e({0x1FAE3})); // 🫣
    m.insert(QStringLiteral("face_with_spiral_eyes"), e({0x1F635, 0x200D, 0x1F4AB})); // 😵‍💫
    m.insert(QStringLiteral("face_with_thermometer"), e({0x1F912})); // 🤒
    m.insert(QStringLiteral("facepalm"), e({0x1F926})); // 🤦
    m.insert(QStringLiteral("facepunch"), e({0x1F44A})); // 👊
    m.insert(QStringLiteral("factory"), e({0x1F3ED})); // 🏭
    m.insert(QStringLiteral("factory_worker"), e({0x1F9D1, 0x200D, 0x1F3ED})); // 🧑‍🏭
    m.insert(QStringLiteral("fairy"), e({0x1F9DA})); // 🧚
    m.insert(QStringLiteral("fairy_man"), e({0x1F9DA, 0x200D, 0x2642, 0xFE0F})); // 🧚‍♂️
    m.insert(QStringLiteral("fairy_woman"), e({0x1F9DA, 0x200D, 0x2640, 0xFE0F})); // 🧚‍♀️
    m.insert(QStringLiteral("falafel"), e({0x1F9C6})); // 🧆
    m.insert(QStringLiteral("falkland_islands"), e({0x1F1EB, 0x1F1F0})); // 🇫🇰
    m.insert(QStringLiteral("fallen_leaf"), e({0x1F342})); // 🍂
    m.insert(QStringLiteral("family"), e({0x1F46A})); // 👪
    m.insert(QStringLiteral("family_man_boy"), e({0x1F468, 0x200D, 0x1F466})); // 👨‍👦
    m.insert(QStringLiteral("family_man_boy_boy"), e({0x1F468, 0x200D, 0x1F466, 0x200D, 0x1F466})); // 👨‍👦‍👦
    m.insert(QStringLiteral("family_man_girl"), e({0x1F468, 0x200D, 0x1F467})); // 👨‍👧
    m.insert(QStringLiteral("family_man_girl_boy"), e({0x1F468, 0x200D, 0x1F467, 0x200D, 0x1F466})); // 👨‍👧‍👦
    m.insert(QStringLiteral("family_man_girl_girl"), e({0x1F468, 0x200D, 0x1F467, 0x200D, 0x1F467})); // 👨‍👧‍👧
    m.insert(QStringLiteral("family_man_man_boy"), e({0x1F468, 0x200D, 0x1F468, 0x200D, 0x1F466})); // 👨‍👨‍👦
    m.insert(QStringLiteral("family_man_man_boy_boy"), e({0x1F468, 0x200D, 0x1F468, 0x200D, 0x1F466, 0x200D, 0x1F466})); // 👨‍👨‍👦‍👦
    m.insert(QStringLiteral("family_man_man_girl"), e({0x1F468, 0x200D, 0x1F468, 0x200D, 0x1F467})); // 👨‍👨‍👧
    m.insert(QStringLiteral("family_man_man_girl_boy"), e({0x1F468, 0x200D, 0x1F468, 0x200D, 0x1F467, 0x200D, 0x1F466})); // 👨‍👨‍👧‍👦
    m.insert(QStringLiteral("family_man_man_girl_girl"), e({0x1F468, 0x200D, 0x1F468, 0x200D, 0x1F467, 0x200D, 0x1F467})); // 👨‍👨‍👧‍👧
    m.insert(QStringLiteral("family_man_woman_boy"), e({0x1F468, 0x200D, 0x1F469, 0x200D, 0x1F466})); // 👨‍👩‍👦
    m.insert(QStringLiteral("family_man_woman_boy_boy"), e({0x1F468, 0x200D, 0x1F469, 0x200D, 0x1F466, 0x200D, 0x1F466})); // 👨‍👩‍👦‍👦
    m.insert(QStringLiteral("family_man_woman_girl"), e({0x1F468, 0x200D, 0x1F469, 0x200D, 0x1F467})); // 👨‍👩‍👧
    m.insert(QStringLiteral("family_man_woman_girl_boy"), e({0x1F468, 0x200D, 0x1F469, 0x200D, 0x1F467, 0x200D, 0x1F466})); // 👨‍👩‍👧‍👦
    m.insert(QStringLiteral("family_man_woman_girl_girl"), e({0x1F468, 0x200D, 0x1F469, 0x200D, 0x1F467, 0x200D, 0x1F467})); // 👨‍👩‍👧‍👧
    m.insert(QStringLiteral("family_woman_boy"), e({0x1F469, 0x200D, 0x1F466})); // 👩‍👦
    m.insert(QStringLiteral("family_woman_boy_boy"), e({0x1F469, 0x200D, 0x1F466, 0x200D, 0x1F466})); // 👩‍👦‍👦
    m.insert(QStringLiteral("family_woman_girl"), e({0x1F469, 0x200D, 0x1F467})); // 👩‍👧
    m.insert(QStringLiteral("family_woman_girl_boy"), e({0x1F469, 0x200D, 0x1F467, 0x200D, 0x1F466})); // 👩‍👧‍👦
    m.insert(QStringLiteral("family_woman_girl_girl"), e({0x1F469, 0x200D, 0x1F467, 0x200D, 0x1F467})); // 👩‍👧‍👧
    m.insert(QStringLiteral("family_woman_woman_boy"), e({0x1F469, 0x200D, 0x1F469, 0x200D, 0x1F466})); // 👩‍👩‍👦
    m.insert(QStringLiteral("family_woman_woman_boy_boy"), e({0x1F469, 0x200D, 0x1F469, 0x200D, 0x1F466, 0x200D, 0x1F466})); // 👩‍👩‍👦‍👦
    m.insert(QStringLiteral("family_woman_woman_girl"), e({0x1F469, 0x200D, 0x1F469, 0x200D, 0x1F467})); // 👩‍👩‍👧
    m.insert(QStringLiteral("family_woman_woman_girl_boy"), e({0x1F469, 0x200D, 0x1F469, 0x200D, 0x1F467, 0x200D, 0x1F466})); // 👩‍👩‍👧‍👦
    m.insert(QStringLiteral("family_woman_woman_girl_girl"), e({0x1F469, 0x200D, 0x1F469, 0x200D, 0x1F467, 0x200D, 0x1F467})); // 👩‍👩‍👧‍👧
    m.insert(QStringLiteral("farmer"), e({0x1F9D1, 0x200D, 0x1F33E})); // 🧑‍🌾
    m.insert(QStringLiteral("faroe_islands"), e({0x1F1EB, 0x1F1F4})); // 🇫🇴
    m.insert(QStringLiteral("fast_forward"), e({0x23E9})); // ⏩
    m.insert(QStringLiteral("fax"), e({0x1F4E0})); // 📠
    m.insert(QStringLiteral("fearful"), e({0x1F628})); // 😨
    m.insert(QStringLiteral("feather"), e({0x1FAB6})); // 🪶
    m.insert(QStringLiteral("feet"), e({0x1F43E})); // 🐾
    m.insert(QStringLiteral("female_detective"), e({0x1F575, 0xFE0F, 0x200D, 0x2640, 0xFE0F})); // 🕵️‍♀️
    m.insert(QStringLiteral("female_sign"), e({0x2640})); // ♀
    m.insert(QStringLiteral("ferris_wheel"), e({0x1F3A1})); // 🎡
    m.insert(QStringLiteral("ferry"), e({0x26F4})); // ⛴
    m.insert(QStringLiteral("field_hockey"), e({0x1F3D1})); // 🏑
    m.insert(QStringLiteral("fiji"), e({0x1F1EB, 0x1F1EF})); // 🇫🇯
    m.insert(QStringLiteral("file_cabinet"), e({0x1F5C4})); // 🗄
    m.insert(QStringLiteral("file_folder"), e({0x1F4C1})); // 📁
    m.insert(QStringLiteral("film_projector"), e({0x1F4FD})); // 📽
    m.insert(QStringLiteral("film_strip"), e({0x1F39E})); // 🎞
    m.insert(QStringLiteral("finland"), e({0x1F1EB, 0x1F1EE})); // 🇫🇮
    m.insert(QStringLiteral("fire"), e({0x1F525})); // 🔥
    m.insert(QStringLiteral("fire_engine"), e({0x1F692})); // 🚒
    m.insert(QStringLiteral("fire_extinguisher"), e({0x1F9EF})); // 🧯
    m.insert(QStringLiteral("firecracker"), e({0x1F9E8})); // 🧨
    m.insert(QStringLiteral("firefighter"), e({0x1F9D1, 0x200D, 0x1F692})); // 🧑‍🚒
    m.insert(QStringLiteral("fireworks"), e({0x1F386})); // 🎆
    m.insert(QStringLiteral("first_quarter_moon"), e({0x1F313})); // 🌓
    m.insert(QStringLiteral("first_quarter_moon_with_face"), e({0x1F31B})); // 🌛
    m.insert(QStringLiteral("fish"), e({0x1F41F})); // 🐟
    m.insert(QStringLiteral("fish_cake"), e({0x1F365})); // 🍥
    m.insert(QStringLiteral("fishing_pole_and_fish"), e({0x1F3A3})); // 🎣
    m.insert(QStringLiteral("fist"), e({0x270A})); // ✊
    m.insert(QStringLiteral("fist_left"), e({0x1F91B})); // 🤛
    m.insert(QStringLiteral("fist_oncoming"), e({0x1F44A})); // 👊
    m.insert(QStringLiteral("fist_raised"), e({0x270A})); // ✊
    m.insert(QStringLiteral("fist_right"), e({0x1F91C})); // 🤜
    m.insert(QStringLiteral("five"), e({0x35, 0xFE0F, 0x20E3})); // 5️⃣
    m.insert(QStringLiteral("flags"), e({0x1F38F})); // 🎏
    m.insert(QStringLiteral("flamingo"), e({0x1F9A9})); // 🦩
    m.insert(QStringLiteral("flashlight"), e({0x1F526})); // 🔦
    m.insert(QStringLiteral("flat_shoe"), e({0x1F97F})); // 🥿
    m.insert(QStringLiteral("flatbread"), e({0x1FAD3})); // 🫓
    m.insert(QStringLiteral("fleur_de_lis"), e({0x269C})); // ⚜
    m.insert(QStringLiteral("flight_arrival"), e({0x1F6EC})); // 🛬
    m.insert(QStringLiteral("flight_departure"), e({0x1F6EB})); // 🛫
    m.insert(QStringLiteral("flipper"), e({0x1F42C})); // 🐬
    m.insert(QStringLiteral("floppy_disk"), e({0x1F4BE})); // 💾
    m.insert(QStringLiteral("flower_playing_cards"), e({0x1F3B4})); // 🎴
    m.insert(QStringLiteral("flushed"), e({0x1F633})); // 😳
    m.insert(QStringLiteral("flute"), e({0x1FA88})); // 🪈
    m.insert(QStringLiteral("fly"), e({0x1FAB0})); // 🪰
    m.insert(QStringLiteral("flying_disc"), e({0x1F94F})); // 🥏
    m.insert(QStringLiteral("flying_saucer"), e({0x1F6F8})); // 🛸
    m.insert(QStringLiteral("fog"), e({0x1F32B})); // 🌫
    m.insert(QStringLiteral("foggy"), e({0x1F301})); // 🌁
    m.insert(QStringLiteral("folding_hand_fan"), e({0x1FAAD})); // 🪭
    m.insert(QStringLiteral("fondue"), e({0x1FAD5})); // 🫕
    m.insert(QStringLiteral("foot"), e({0x1F9B6})); // 🦶
    m.insert(QStringLiteral("football"), e({0x1F3C8})); // 🏈
    m.insert(QStringLiteral("footprints"), e({0x1F463})); // 👣
    m.insert(QStringLiteral("fork_and_knife"), e({0x1F374})); // 🍴
    m.insert(QStringLiteral("fortune_cookie"), e({0x1F960})); // 🥠
    m.insert(QStringLiteral("fountain"), e({0x26F2})); // ⛲
    m.insert(QStringLiteral("fountain_pen"), e({0x1F58B})); // 🖋
    m.insert(QStringLiteral("four"), e({0x34, 0xFE0F, 0x20E3})); // 4️⃣
    m.insert(QStringLiteral("four_leaf_clover"), e({0x1F340})); // 🍀
    m.insert(QStringLiteral("fox_face"), e({0x1F98A})); // 🦊
    m.insert(QStringLiteral("fr"), e({0x1F1EB, 0x1F1F7})); // 🇫🇷
    m.insert(QStringLiteral("framed_picture"), e({0x1F5BC})); // 🖼
    m.insert(QStringLiteral("free"), e({0x1F193})); // 🆓
    m.insert(QStringLiteral("french_guiana"), e({0x1F1EC, 0x1F1EB})); // 🇬🇫
    m.insert(QStringLiteral("french_polynesia"), e({0x1F1F5, 0x1F1EB})); // 🇵🇫
    m.insert(QStringLiteral("french_southern_territories"), e({0x1F1F9, 0x1F1EB})); // 🇹🇫
    m.insert(QStringLiteral("fried_egg"), e({0x1F373})); // 🍳
    m.insert(QStringLiteral("fried_shrimp"), e({0x1F364})); // 🍤
    m.insert(QStringLiteral("fries"), e({0x1F35F})); // 🍟
    m.insert(QStringLiteral("frog"), e({0x1F438})); // 🐸
    m.insert(QStringLiteral("frowning"), e({0x1F626})); // 😦
    m.insert(QStringLiteral("frowning_face"), e({0x2639})); // ☹
    m.insert(QStringLiteral("frowning_man"), e({0x1F64D, 0x200D, 0x2642, 0xFE0F})); // 🙍‍♂️
    m.insert(QStringLiteral("frowning_person"), e({0x1F64D})); // 🙍
    m.insert(QStringLiteral("frowning_woman"), e({0x1F64D, 0x200D, 0x2640, 0xFE0F})); // 🙍‍♀️
    m.insert(QStringLiteral("fu"), e({0x1F595})); // 🖕
    m.insert(QStringLiteral("fuelpump"), e({0x26FD})); // ⛽
    m.insert(QStringLiteral("full_moon"), e({0x1F315})); // 🌕
    m.insert(QStringLiteral("full_moon_with_face"), e({0x1F31D})); // 🌝
    m.insert(QStringLiteral("funeral_urn"), e({0x26B1})); // ⚱
    m.insert(QStringLiteral("gabon"), e({0x1F1EC, 0x1F1E6})); // 🇬🇦
    m.insert(QStringLiteral("gambia"), e({0x1F1EC, 0x1F1F2})); // 🇬🇲
    m.insert(QStringLiteral("game_die"), e({0x1F3B2})); // 🎲
    m.insert(QStringLiteral("garlic"), e({0x1F9C4})); // 🧄
    m.insert(QStringLiteral("gb"), e({0x1F1EC, 0x1F1E7})); // 🇬🇧
    m.insert(QStringLiteral("gear"), e({0x2699})); // ⚙
    m.insert(QStringLiteral("gem"), e({0x1F48E})); // 💎
    m.insert(QStringLiteral("gemini"), e({0x264A})); // ♊
    m.insert(QStringLiteral("genie"), e({0x1F9DE})); // 🧞
    m.insert(QStringLiteral("genie_man"), e({0x1F9DE, 0x200D, 0x2642, 0xFE0F})); // 🧞‍♂️
    m.insert(QStringLiteral("genie_woman"), e({0x1F9DE, 0x200D, 0x2640, 0xFE0F})); // 🧞‍♀️
    m.insert(QStringLiteral("georgia"), e({0x1F1EC, 0x1F1EA})); // 🇬🇪
    m.insert(QStringLiteral("ghana"), e({0x1F1EC, 0x1F1ED})); // 🇬🇭
    m.insert(QStringLiteral("ghost"), e({0x1F47B})); // 👻
    m.insert(QStringLiteral("gibraltar"), e({0x1F1EC, 0x1F1EE})); // 🇬🇮
    m.insert(QStringLiteral("gift"), e({0x1F381})); // 🎁
    m.insert(QStringLiteral("gift_heart"), e({0x1F49D})); // 💝
    m.insert(QStringLiteral("ginger_root"), e({0x1FADA})); // 🫚
    m.insert(QStringLiteral("giraffe"), e({0x1F992})); // 🦒
    m.insert(QStringLiteral("girl"), e({0x1F467})); // 👧
    m.insert(QStringLiteral("globe_with_meridians"), e({0x1F310})); // 🌐
    m.insert(QStringLiteral("gloves"), e({0x1F9E4})); // 🧤
    m.insert(QStringLiteral("goal_net"), e({0x1F945})); // 🥅
    m.insert(QStringLiteral("goat"), e({0x1F410})); // 🐐
    m.insert(QStringLiteral("goggles"), e({0x1F97D})); // 🥽
    m.insert(QStringLiteral("golf"), e({0x26F3})); // ⛳
    m.insert(QStringLiteral("golfing"), e({0x1F3CC})); // 🏌
    m.insert(QStringLiteral("golfing_man"), e({0x1F3CC, 0xFE0F, 0x200D, 0x2642, 0xFE0F})); // 🏌️‍♂️
    m.insert(QStringLiteral("golfing_woman"), e({0x1F3CC, 0xFE0F, 0x200D, 0x2640, 0xFE0F})); // 🏌️‍♀️
    m.insert(QStringLiteral("goose"), e({0x1FABF})); // 🪿
    m.insert(QStringLiteral("gorilla"), e({0x1F98D})); // 🦍
    m.insert(QStringLiteral("grapes"), e({0x1F347})); // 🍇
    m.insert(QStringLiteral("greece"), e({0x1F1EC, 0x1F1F7})); // 🇬🇷
    m.insert(QStringLiteral("green_apple"), e({0x1F34F})); // 🍏
    m.insert(QStringLiteral("green_book"), e({0x1F4D7})); // 📗
    m.insert(QStringLiteral("green_circle"), e({0x1F7E2})); // 🟢
    m.insert(QStringLiteral("green_heart"), e({0x1F49A})); // 💚
    m.insert(QStringLiteral("green_salad"), e({0x1F957})); // 🥗
    m.insert(QStringLiteral("green_square"), e({0x1F7E9})); // 🟩
    m.insert(QStringLiteral("greenland"), e({0x1F1EC, 0x1F1F1})); // 🇬🇱
    m.insert(QStringLiteral("grenada"), e({0x1F1EC, 0x1F1E9})); // 🇬🇩
    m.insert(QStringLiteral("grey_exclamation"), e({0x2755})); // ❕
    m.insert(QStringLiteral("grey_heart"), e({0x1FA76})); // 🩶
    m.insert(QStringLiteral("grey_question"), e({0x2754})); // ❔
    m.insert(QStringLiteral("grimacing"), e({0x1F62C})); // 😬
    m.insert(QStringLiteral("grin"), e({0x1F601})); // 😁
    m.insert(QStringLiteral("grinning"), e({0x1F600})); // 😀
    m.insert(QStringLiteral("guadeloupe"), e({0x1F1EC, 0x1F1F5})); // 🇬🇵
    m.insert(QStringLiteral("guam"), e({0x1F1EC, 0x1F1FA})); // 🇬🇺
    m.insert(QStringLiteral("guard"), e({0x1F482})); // 💂
    m.insert(QStringLiteral("guardsman"), e({0x1F482, 0x200D, 0x2642, 0xFE0F})); // 💂‍♂️
    m.insert(QStringLiteral("guardswoman"), e({0x1F482, 0x200D, 0x2640, 0xFE0F})); // 💂‍♀️
    m.insert(QStringLiteral("guatemala"), e({0x1F1EC, 0x1F1F9})); // 🇬🇹
    m.insert(QStringLiteral("guernsey"), e({0x1F1EC, 0x1F1EC})); // 🇬🇬
    m.insert(QStringLiteral("guide_dog"), e({0x1F9AE})); // 🦮
    m.insert(QStringLiteral("guinea"), e({0x1F1EC, 0x1F1F3})); // 🇬🇳
    m.insert(QStringLiteral("guinea_bissau"), e({0x1F1EC, 0x1F1FC})); // 🇬🇼
    m.insert(QStringLiteral("guitar"), e({0x1F3B8})); // 🎸
    m.insert(QStringLiteral("gun"), e({0x1F52B})); // 🔫
    m.insert(QStringLiteral("guyana"), e({0x1F1EC, 0x1F1FE})); // 🇬🇾
    m.insert(QStringLiteral("hair_pick"), e({0x1FAAE})); // 🪮
    m.insert(QStringLiteral("haircut"), e({0x1F487})); // 💇
    m.insert(QStringLiteral("haircut_man"), e({0x1F487, 0x200D, 0x2642, 0xFE0F})); // 💇‍♂️
    m.insert(QStringLiteral("haircut_woman"), e({0x1F487, 0x200D, 0x2640, 0xFE0F})); // 💇‍♀️
    m.insert(QStringLiteral("haiti"), e({0x1F1ED, 0x1F1F9})); // 🇭🇹
    m.insert(QStringLiteral("hamburger"), e({0x1F354})); // 🍔
    m.insert(QStringLiteral("hammer"), e({0x1F528})); // 🔨
    m.insert(QStringLiteral("hammer_and_pick"), e({0x2692})); // ⚒
    m.insert(QStringLiteral("hammer_and_wrench"), e({0x1F6E0})); // 🛠
    m.insert(QStringLiteral("hamsa"), e({0x1FAAC})); // 🪬
    m.insert(QStringLiteral("hamster"), e({0x1F439})); // 🐹
    m.insert(QStringLiteral("hand"), e({0x270B})); // ✋
    m.insert(QStringLiteral("hand_over_mouth"), e({0x1F92D})); // 🤭
    m.insert(QStringLiteral("hand_with_index_finger_and_thumb_crossed"), e({0x1FAF0})); // 🫰
    m.insert(QStringLiteral("handbag"), e({0x1F45C})); // 👜
    m.insert(QStringLiteral("handball_person"), e({0x1F93E})); // 🤾
    m.insert(QStringLiteral("handshake"), e({0x1F91D})); // 🤝
    m.insert(QStringLiteral("hankey"), e({0x1F4A9})); // 💩
    m.insert(QStringLiteral("hash"), e({0x23, 0xFE0F, 0x20E3})); // #️⃣
    m.insert(QStringLiteral("hatched_chick"), e({0x1F425})); // 🐥
    m.insert(QStringLiteral("hatching_chick"), e({0x1F423})); // 🐣
    m.insert(QStringLiteral("headphones"), e({0x1F3A7})); // 🎧
    m.insert(QStringLiteral("headstone"), e({0x1FAA6})); // 🪦
    m.insert(QStringLiteral("health_worker"), e({0x1F9D1, 0x200D, 0x2695, 0xFE0F})); // 🧑‍⚕️
    m.insert(QStringLiteral("hear_no_evil"), e({0x1F649})); // 🙉
    m.insert(QStringLiteral("heard_mcdonald_islands"), e({0x1F1ED, 0x1F1F2})); // 🇭🇲
    m.insert(QStringLiteral("heart"), e({0x2764})); // ❤
    m.insert(QStringLiteral("heart_decoration"), e({0x1F49F})); // 💟
    m.insert(QStringLiteral("heart_eyes"), e({0x1F60D})); // 😍
    m.insert(QStringLiteral("heart_eyes_cat"), e({0x1F63B})); // 😻
    m.insert(QStringLiteral("heart_hands"), e({0x1FAF6})); // 🫶
    m.insert(QStringLiteral("heart_on_fire"), e({0x2764, 0xFE0F, 0x200D, 0x1F525})); // ❤️‍🔥
    m.insert(QStringLiteral("heartbeat"), e({0x1F493})); // 💓
    m.insert(QStringLiteral("heartpulse"), e({0x1F497})); // 💗
    m.insert(QStringLiteral("hearts"), e({0x2665})); // ♥
    m.insert(QStringLiteral("heavy_check_mark"), e({0x2714})); // ✔
    m.insert(QStringLiteral("heavy_division_sign"), e({0x2797})); // ➗
    m.insert(QStringLiteral("heavy_dollar_sign"), e({0x1F4B2})); // 💲
    m.insert(QStringLiteral("heavy_equals_sign"), e({0x1F7F0})); // 🟰
    m.insert(QStringLiteral("heavy_exclamation_mark"), e({0x2757})); // ❗
    m.insert(QStringLiteral("heavy_heart_exclamation"), e({0x2763})); // ❣
    m.insert(QStringLiteral("heavy_minus_sign"), e({0x2796})); // ➖
    m.insert(QStringLiteral("heavy_multiplication_x"), e({0x2716})); // ✖
    m.insert(QStringLiteral("heavy_plus_sign"), e({0x2795})); // ➕
    m.insert(QStringLiteral("hedgehog"), e({0x1F994})); // 🦔
    m.insert(QStringLiteral("helicopter"), e({0x1F681})); // 🚁
    m.insert(QStringLiteral("herb"), e({0x1F33F})); // 🌿
    m.insert(QStringLiteral("hibiscus"), e({0x1F33A})); // 🌺
    m.insert(QStringLiteral("high_brightness"), e({0x1F506})); // 🔆
    m.insert(QStringLiteral("high_heel"), e({0x1F460})); // 👠
    m.insert(QStringLiteral("hiking_boot"), e({0x1F97E})); // 🥾
    m.insert(QStringLiteral("hindu_temple"), e({0x1F6D5})); // 🛕
    m.insert(QStringLiteral("hippopotamus"), e({0x1F99B})); // 🦛
    m.insert(QStringLiteral("hocho"), e({0x1F52A})); // 🔪
    m.insert(QStringLiteral("hole"), e({0x1F573})); // 🕳
    m.insert(QStringLiteral("honduras"), e({0x1F1ED, 0x1F1F3})); // 🇭🇳
    m.insert(QStringLiteral("honey_pot"), e({0x1F36F})); // 🍯
    m.insert(QStringLiteral("honeybee"), e({0x1F41D})); // 🐝
    m.insert(QStringLiteral("hong_kong"), e({0x1F1ED, 0x1F1F0})); // 🇭🇰
    m.insert(QStringLiteral("hook"), e({0x1FA9D})); // 🪝
    m.insert(QStringLiteral("horse"), e({0x1F434})); // 🐴
    m.insert(QStringLiteral("horse_racing"), e({0x1F3C7})); // 🏇
    m.insert(QStringLiteral("hospital"), e({0x1F3E5})); // 🏥
    m.insert(QStringLiteral("hot_face"), e({0x1F975})); // 🥵
    m.insert(QStringLiteral("hot_pepper"), e({0x1F336})); // 🌶
    m.insert(QStringLiteral("hotdog"), e({0x1F32D})); // 🌭
    m.insert(QStringLiteral("hotel"), e({0x1F3E8})); // 🏨
    m.insert(QStringLiteral("hotsprings"), e({0x2668})); // ♨
    m.insert(QStringLiteral("hourglass"), e({0x231B})); // ⌛
    m.insert(QStringLiteral("hourglass_flowing_sand"), e({0x23F3})); // ⏳
    m.insert(QStringLiteral("house"), e({0x1F3E0})); // 🏠
    m.insert(QStringLiteral("house_with_garden"), e({0x1F3E1})); // 🏡
    m.insert(QStringLiteral("houses"), e({0x1F3D8})); // 🏘
    m.insert(QStringLiteral("hugs"), e({0x1F917})); // 🤗
    m.insert(QStringLiteral("hungary"), e({0x1F1ED, 0x1F1FA})); // 🇭🇺
    m.insert(QStringLiteral("hushed"), e({0x1F62F})); // 😯
    m.insert(QStringLiteral("hut"), e({0x1F6D6})); // 🛖
    m.insert(QStringLiteral("hyacinth"), e({0x1FABB})); // 🪻
    m.insert(QStringLiteral("ice_cream"), e({0x1F368})); // 🍨
    m.insert(QStringLiteral("ice_cube"), e({0x1F9CA})); // 🧊
    m.insert(QStringLiteral("ice_hockey"), e({0x1F3D2})); // 🏒
    m.insert(QStringLiteral("ice_skate"), e({0x26F8})); // ⛸
    m.insert(QStringLiteral("icecream"), e({0x1F366})); // 🍦
    m.insert(QStringLiteral("iceland"), e({0x1F1EE, 0x1F1F8})); // 🇮🇸
    m.insert(QStringLiteral("id"), e({0x1F194})); // 🆔
    m.insert(QStringLiteral("identification_card"), e({0x1FAAA})); // 🪪
    m.insert(QStringLiteral("ideograph_advantage"), e({0x1F250})); // 🉐
    m.insert(QStringLiteral("imp"), e({0x1F47F})); // 👿
    m.insert(QStringLiteral("inbox_tray"), e({0x1F4E5})); // 📥
    m.insert(QStringLiteral("incoming_envelope"), e({0x1F4E8})); // 📨
    m.insert(QStringLiteral("index_pointing_at_the_viewer"), e({0x1FAF5})); // 🫵
    m.insert(QStringLiteral("india"), e({0x1F1EE, 0x1F1F3})); // 🇮🇳
    m.insert(QStringLiteral("indonesia"), e({0x1F1EE, 0x1F1E9})); // 🇮🇩
    m.insert(QStringLiteral("infinity"), e({0x267E})); // ♾
    m.insert(QStringLiteral("information_desk_person"), e({0x1F481})); // 💁
    m.insert(QStringLiteral("information_source"), e({0x2139})); // ℹ
    m.insert(QStringLiteral("innocent"), e({0x1F607})); // 😇
    m.insert(QStringLiteral("interrobang"), e({0x2049})); // ⁉
    m.insert(QStringLiteral("iphone"), e({0x1F4F1})); // 📱
    m.insert(QStringLiteral("iran"), e({0x1F1EE, 0x1F1F7})); // 🇮🇷
    m.insert(QStringLiteral("iraq"), e({0x1F1EE, 0x1F1F6})); // 🇮🇶
    m.insert(QStringLiteral("ireland"), e({0x1F1EE, 0x1F1EA})); // 🇮🇪
    m.insert(QStringLiteral("isle_of_man"), e({0x1F1EE, 0x1F1F2})); // 🇮🇲
    m.insert(QStringLiteral("israel"), e({0x1F1EE, 0x1F1F1})); // 🇮🇱
    m.insert(QStringLiteral("it"), e({0x1F1EE, 0x1F1F9})); // 🇮🇹
    m.insert(QStringLiteral("izakaya_lantern"), e({0x1F3EE})); // 🏮
    m.insert(QStringLiteral("jack_o_lantern"), e({0x1F383})); // 🎃
    m.insert(QStringLiteral("jamaica"), e({0x1F1EF, 0x1F1F2})); // 🇯🇲
    m.insert(QStringLiteral("japan"), e({0x1F5FE})); // 🗾
    m.insert(QStringLiteral("japanese_castle"), e({0x1F3EF})); // 🏯
    m.insert(QStringLiteral("japanese_goblin"), e({0x1F47A})); // 👺
    m.insert(QStringLiteral("japanese_ogre"), e({0x1F479})); // 👹
    m.insert(QStringLiteral("jar"), e({0x1FAD9})); // 🫙
    m.insert(QStringLiteral("jeans"), e({0x1F456})); // 👖
    m.insert(QStringLiteral("jellyfish"), e({0x1FABC})); // 🪼
    m.insert(QStringLiteral("jersey"), e({0x1F1EF, 0x1F1EA})); // 🇯🇪
    m.insert(QStringLiteral("jigsaw"), e({0x1F9E9})); // 🧩
    m.insert(QStringLiteral("jordan"), e({0x1F1EF, 0x1F1F4})); // 🇯🇴
    m.insert(QStringLiteral("joy"), e({0x1F602})); // 😂
    m.insert(QStringLiteral("joy_cat"), e({0x1F639})); // 😹
    m.insert(QStringLiteral("joystick"), e({0x1F579})); // 🕹
    m.insert(QStringLiteral("jp"), e({0x1F1EF, 0x1F1F5})); // 🇯🇵
    m.insert(QStringLiteral("judge"), e({0x1F9D1, 0x200D, 0x2696, 0xFE0F})); // 🧑‍⚖️
    m.insert(QStringLiteral("juggling_person"), e({0x1F939})); // 🤹
    m.insert(QStringLiteral("kaaba"), e({0x1F54B})); // 🕋
    m.insert(QStringLiteral("kangaroo"), e({0x1F998})); // 🦘
    m.insert(QStringLiteral("kazakhstan"), e({0x1F1F0, 0x1F1FF})); // 🇰🇿
    m.insert(QStringLiteral("kenya"), e({0x1F1F0, 0x1F1EA})); // 🇰🇪
    m.insert(QStringLiteral("key"), e({0x1F511})); // 🔑
    m.insert(QStringLiteral("keyboard"), e({0x2328})); // ⌨
    m.insert(QStringLiteral("keycap_ten"), e({0x1F51F})); // 🔟
    m.insert(QStringLiteral("khanda"), e({0x1FAAF})); // 🪯
    m.insert(QStringLiteral("kick_scooter"), e({0x1F6F4})); // 🛴
    m.insert(QStringLiteral("kimono"), e({0x1F458})); // 👘
    m.insert(QStringLiteral("kiribati"), e({0x1F1F0, 0x1F1EE})); // 🇰🇮
    m.insert(QStringLiteral("kiss"), e({0x1F48B})); // 💋
    m.insert(QStringLiteral("kissing"), e({0x1F617})); // 😗
    m.insert(QStringLiteral("kissing_cat"), e({0x1F63D})); // 😽
    m.insert(QStringLiteral("kissing_closed_eyes"), e({0x1F61A})); // 😚
    m.insert(QStringLiteral("kissing_heart"), e({0x1F618})); // 😘
    m.insert(QStringLiteral("kissing_smiling_eyes"), e({0x1F619})); // 😙
    m.insert(QStringLiteral("kite"), e({0x1FA81})); // 🪁
    m.insert(QStringLiteral("kiwi_fruit"), e({0x1F95D})); // 🥝
    m.insert(QStringLiteral("kneeling_man"), e({0x1F9CE, 0x200D, 0x2642, 0xFE0F})); // 🧎‍♂️
    m.insert(QStringLiteral("kneeling_person"), e({0x1F9CE})); // 🧎
    m.insert(QStringLiteral("kneeling_woman"), e({0x1F9CE, 0x200D, 0x2640, 0xFE0F})); // 🧎‍♀️
    m.insert(QStringLiteral("knife"), e({0x1F52A})); // 🔪
    m.insert(QStringLiteral("knot"), e({0x1FAA2})); // 🪢
    m.insert(QStringLiteral("koala"), e({0x1F428})); // 🐨
    m.insert(QStringLiteral("koko"), e({0x1F201})); // 🈁
    m.insert(QStringLiteral("kosovo"), e({0x1F1FD, 0x1F1F0})); // 🇽🇰
    m.insert(QStringLiteral("kr"), e({0x1F1F0, 0x1F1F7})); // 🇰🇷
    m.insert(QStringLiteral("kuwait"), e({0x1F1F0, 0x1F1FC})); // 🇰🇼
    m.insert(QStringLiteral("kyrgyzstan"), e({0x1F1F0, 0x1F1EC})); // 🇰🇬
    m.insert(QStringLiteral("lab_coat"), e({0x1F97C})); // 🥼
    m.insert(QStringLiteral("label"), e({0x1F3F7})); // 🏷
    m.insert(QStringLiteral("lacrosse"), e({0x1F94D})); // 🥍
    m.insert(QStringLiteral("ladder"), e({0x1FA9C})); // 🪜
    m.insert(QStringLiteral("lady_beetle"), e({0x1F41E})); // 🐞
    m.insert(QStringLiteral("lantern"), e({0x1F3EE})); // 🏮
    m.insert(QStringLiteral("laos"), e({0x1F1F1, 0x1F1E6})); // 🇱🇦
    m.insert(QStringLiteral("large_blue_circle"), e({0x1F535})); // 🔵
    m.insert(QStringLiteral("large_blue_diamond"), e({0x1F537})); // 🔷
    m.insert(QStringLiteral("large_orange_diamond"), e({0x1F536})); // 🔶
    m.insert(QStringLiteral("last_quarter_moon"), e({0x1F317})); // 🌗
    m.insert(QStringLiteral("last_quarter_moon_with_face"), e({0x1F31C})); // 🌜
    m.insert(QStringLiteral("latin_cross"), e({0x271D})); // ✝
    m.insert(QStringLiteral("latvia"), e({0x1F1F1, 0x1F1FB})); // 🇱🇻
    m.insert(QStringLiteral("laughing"), e({0x1F606})); // 😆
    m.insert(QStringLiteral("leafy_green"), e({0x1F96C})); // 🥬
    m.insert(QStringLiteral("leaves"), e({0x1F343})); // 🍃
    m.insert(QStringLiteral("lebanon"), e({0x1F1F1, 0x1F1E7})); // 🇱🇧
    m.insert(QStringLiteral("ledger"), e({0x1F4D2})); // 📒
    m.insert(QStringLiteral("left_luggage"), e({0x1F6C5})); // 🛅
    m.insert(QStringLiteral("left_right_arrow"), e({0x2194})); // ↔
    m.insert(QStringLiteral("left_speech_bubble"), e({0x1F5E8})); // 🗨
    m.insert(QStringLiteral("leftwards_arrow_with_hook"), e({0x21A9})); // ↩
    m.insert(QStringLiteral("leftwards_hand"), e({0x1FAF2})); // 🫲
    m.insert(QStringLiteral("leftwards_pushing_hand"), e({0x1FAF7})); // 🫷
    m.insert(QStringLiteral("leg"), e({0x1F9B5})); // 🦵
    m.insert(QStringLiteral("lemon"), e({0x1F34B})); // 🍋
    m.insert(QStringLiteral("leo"), e({0x264C})); // ♌
    m.insert(QStringLiteral("leopard"), e({0x1F406})); // 🐆
    m.insert(QStringLiteral("lesotho"), e({0x1F1F1, 0x1F1F8})); // 🇱🇸
    m.insert(QStringLiteral("level_slider"), e({0x1F39A})); // 🎚
    m.insert(QStringLiteral("liberia"), e({0x1F1F1, 0x1F1F7})); // 🇱🇷
    m.insert(QStringLiteral("libra"), e({0x264E})); // ♎
    m.insert(QStringLiteral("libya"), e({0x1F1F1, 0x1F1FE})); // 🇱🇾
    m.insert(QStringLiteral("liechtenstein"), e({0x1F1F1, 0x1F1EE})); // 🇱🇮
    m.insert(QStringLiteral("light_blue_heart"), e({0x1FA75})); // 🩵
    m.insert(QStringLiteral("light_rail"), e({0x1F688})); // 🚈
    m.insert(QStringLiteral("link"), e({0x1F517})); // 🔗
    m.insert(QStringLiteral("lion"), e({0x1F981})); // 🦁
    m.insert(QStringLiteral("lips"), e({0x1F444})); // 👄
    m.insert(QStringLiteral("lipstick"), e({0x1F484})); // 💄
    m.insert(QStringLiteral("lithuania"), e({0x1F1F1, 0x1F1F9})); // 🇱🇹
    m.insert(QStringLiteral("lizard"), e({0x1F98E})); // 🦎
    m.insert(QStringLiteral("llama"), e({0x1F999})); // 🦙
    m.insert(QStringLiteral("lobster"), e({0x1F99E})); // 🦞
    m.insert(QStringLiteral("lock"), e({0x1F512})); // 🔒
    m.insert(QStringLiteral("lock_with_ink_pen"), e({0x1F50F})); // 🔏
    m.insert(QStringLiteral("lollipop"), e({0x1F36D})); // 🍭
    m.insert(QStringLiteral("long_drum"), e({0x1FA98})); // 🪘
    m.insert(QStringLiteral("loop"), e({0x27BF})); // ➿
    m.insert(QStringLiteral("lotion_bottle"), e({0x1F9F4})); // 🧴
    m.insert(QStringLiteral("lotus"), e({0x1FAB7})); // 🪷
    m.insert(QStringLiteral("lotus_position"), e({0x1F9D8})); // 🧘
    m.insert(QStringLiteral("lotus_position_man"), e({0x1F9D8, 0x200D, 0x2642, 0xFE0F})); // 🧘‍♂️
    m.insert(QStringLiteral("lotus_position_woman"), e({0x1F9D8, 0x200D, 0x2640, 0xFE0F})); // 🧘‍♀️
    m.insert(QStringLiteral("loud_sound"), e({0x1F50A})); // 🔊
    m.insert(QStringLiteral("loudspeaker"), e({0x1F4E2})); // 📢
    m.insert(QStringLiteral("love_hotel"), e({0x1F3E9})); // 🏩
    m.insert(QStringLiteral("love_letter"), e({0x1F48C})); // 💌
    m.insert(QStringLiteral("love_you_gesture"), e({0x1F91F})); // 🤟
    m.insert(QStringLiteral("low_battery"), e({0x1FAAB})); // 🪫
    m.insert(QStringLiteral("low_brightness"), e({0x1F505})); // 🔅
    m.insert(QStringLiteral("luggage"), e({0x1F9F3})); // 🧳
    m.insert(QStringLiteral("lungs"), e({0x1FAC1})); // 🫁
    m.insert(QStringLiteral("luxembourg"), e({0x1F1F1, 0x1F1FA})); // 🇱🇺
    m.insert(QStringLiteral("lying_face"), e({0x1F925})); // 🤥
    m.insert(QStringLiteral("m"), e({0x24C2})); // Ⓜ
    m.insert(QStringLiteral("macau"), e({0x1F1F2, 0x1F1F4})); // 🇲🇴
    m.insert(QStringLiteral("macedonia"), e({0x1F1F2, 0x1F1F0})); // 🇲🇰
    m.insert(QStringLiteral("madagascar"), e({0x1F1F2, 0x1F1EC})); // 🇲🇬
    m.insert(QStringLiteral("mag"), e({0x1F50D})); // 🔍
    m.insert(QStringLiteral("mag_right"), e({0x1F50E})); // 🔎
    m.insert(QStringLiteral("mage"), e({0x1F9D9})); // 🧙
    m.insert(QStringLiteral("mage_man"), e({0x1F9D9, 0x200D, 0x2642, 0xFE0F})); // 🧙‍♂️
    m.insert(QStringLiteral("mage_woman"), e({0x1F9D9, 0x200D, 0x2640, 0xFE0F})); // 🧙‍♀️
    m.insert(QStringLiteral("magic_wand"), e({0x1FA84})); // 🪄
    m.insert(QStringLiteral("magnet"), e({0x1F9F2})); // 🧲
    m.insert(QStringLiteral("mahjong"), e({0x1F004})); // 🀄
    m.insert(QStringLiteral("mailbox"), e({0x1F4EB})); // 📫
    m.insert(QStringLiteral("mailbox_closed"), e({0x1F4EA})); // 📪
    m.insert(QStringLiteral("mailbox_with_mail"), e({0x1F4EC})); // 📬
    m.insert(QStringLiteral("mailbox_with_no_mail"), e({0x1F4ED})); // 📭
    m.insert(QStringLiteral("malawi"), e({0x1F1F2, 0x1F1FC})); // 🇲🇼
    m.insert(QStringLiteral("malaysia"), e({0x1F1F2, 0x1F1FE})); // 🇲🇾
    m.insert(QStringLiteral("maldives"), e({0x1F1F2, 0x1F1FB})); // 🇲🇻
    m.insert(QStringLiteral("male_detective"), e({0x1F575, 0xFE0F, 0x200D, 0x2642, 0xFE0F})); // 🕵️‍♂️
    m.insert(QStringLiteral("male_sign"), e({0x2642})); // ♂
    m.insert(QStringLiteral("mali"), e({0x1F1F2, 0x1F1F1})); // 🇲🇱
    m.insert(QStringLiteral("malta"), e({0x1F1F2, 0x1F1F9})); // 🇲🇹
    m.insert(QStringLiteral("mammoth"), e({0x1F9A3})); // 🦣
    m.insert(QStringLiteral("man"), e({0x1F468})); // 👨
    m.insert(QStringLiteral("man_artist"), e({0x1F468, 0x200D, 0x1F3A8})); // 👨‍🎨
    m.insert(QStringLiteral("man_astronaut"), e({0x1F468, 0x200D, 0x1F680})); // 👨‍🚀
    m.insert(QStringLiteral("man_beard"), e({0x1F9D4, 0x200D, 0x2642, 0xFE0F})); // 🧔‍♂️
    m.insert(QStringLiteral("man_cartwheeling"), e({0x1F938, 0x200D, 0x2642, 0xFE0F})); // 🤸‍♂️
    m.insert(QStringLiteral("man_cook"), e({0x1F468, 0x200D, 0x1F373})); // 👨‍🍳
    m.insert(QStringLiteral("man_dancing"), e({0x1F57A})); // 🕺
    m.insert(QStringLiteral("man_facepalming"), e({0x1F926, 0x200D, 0x2642, 0xFE0F})); // 🤦‍♂️
    m.insert(QStringLiteral("man_factory_worker"), e({0x1F468, 0x200D, 0x1F3ED})); // 👨‍🏭
    m.insert(QStringLiteral("man_farmer"), e({0x1F468, 0x200D, 0x1F33E})); // 👨‍🌾
    m.insert(QStringLiteral("man_feeding_baby"), e({0x1F468, 0x200D, 0x1F37C})); // 👨‍🍼
    m.insert(QStringLiteral("man_firefighter"), e({0x1F468, 0x200D, 0x1F692})); // 👨‍🚒
    m.insert(QStringLiteral("man_health_worker"), e({0x1F468, 0x200D, 0x2695, 0xFE0F})); // 👨‍⚕️
    m.insert(QStringLiteral("man_in_manual_wheelchair"), e({0x1F468, 0x200D, 0x1F9BD})); // 👨‍🦽
    m.insert(QStringLiteral("man_in_motorized_wheelchair"), e({0x1F468, 0x200D, 0x1F9BC})); // 👨‍🦼
    m.insert(QStringLiteral("man_in_tuxedo"), e({0x1F935, 0x200D, 0x2642, 0xFE0F})); // 🤵‍♂️
    m.insert(QStringLiteral("man_judge"), e({0x1F468, 0x200D, 0x2696, 0xFE0F})); // 👨‍⚖️
    m.insert(QStringLiteral("man_juggling"), e({0x1F939, 0x200D, 0x2642, 0xFE0F})); // 🤹‍♂️
    m.insert(QStringLiteral("man_mechanic"), e({0x1F468, 0x200D, 0x1F527})); // 👨‍🔧
    m.insert(QStringLiteral("man_office_worker"), e({0x1F468, 0x200D, 0x1F4BC})); // 👨‍💼
    m.insert(QStringLiteral("man_pilot"), e({0x1F468, 0x200D, 0x2708, 0xFE0F})); // 👨‍✈️
    m.insert(QStringLiteral("man_playing_handball"), e({0x1F93E, 0x200D, 0x2642, 0xFE0F})); // 🤾‍♂️
    m.insert(QStringLiteral("man_playing_water_polo"), e({0x1F93D, 0x200D, 0x2642, 0xFE0F})); // 🤽‍♂️
    m.insert(QStringLiteral("man_scientist"), e({0x1F468, 0x200D, 0x1F52C})); // 👨‍🔬
    m.insert(QStringLiteral("man_shrugging"), e({0x1F937, 0x200D, 0x2642, 0xFE0F})); // 🤷‍♂️
    m.insert(QStringLiteral("man_singer"), e({0x1F468, 0x200D, 0x1F3A4})); // 👨‍🎤
    m.insert(QStringLiteral("man_student"), e({0x1F468, 0x200D, 0x1F393})); // 👨‍🎓
    m.insert(QStringLiteral("man_teacher"), e({0x1F468, 0x200D, 0x1F3EB})); // 👨‍🏫
    m.insert(QStringLiteral("man_technologist"), e({0x1F468, 0x200D, 0x1F4BB})); // 👨‍💻
    m.insert(QStringLiteral("man_with_gua_pi_mao"), e({0x1F472})); // 👲
    m.insert(QStringLiteral("man_with_probing_cane"), e({0x1F468, 0x200D, 0x1F9AF})); // 👨‍🦯
    m.insert(QStringLiteral("man_with_turban"), e({0x1F473, 0x200D, 0x2642, 0xFE0F})); // 👳‍♂️
    m.insert(QStringLiteral("man_with_veil"), e({0x1F470, 0x200D, 0x2642, 0xFE0F})); // 👰‍♂️
    m.insert(QStringLiteral("mandarin"), e({0x1F34A})); // 🍊
    m.insert(QStringLiteral("mango"), e({0x1F96D})); // 🥭
    m.insert(QStringLiteral("mans_shoe"), e({0x1F45E})); // 👞
    m.insert(QStringLiteral("mantelpiece_clock"), e({0x1F570})); // 🕰
    m.insert(QStringLiteral("manual_wheelchair"), e({0x1F9BD})); // 🦽
    m.insert(QStringLiteral("maple_leaf"), e({0x1F341})); // 🍁
    m.insert(QStringLiteral("maracas"), e({0x1FA87})); // 🪇
    m.insert(QStringLiteral("marshall_islands"), e({0x1F1F2, 0x1F1ED})); // 🇲🇭
    m.insert(QStringLiteral("martial_arts_uniform"), e({0x1F94B})); // 🥋
    m.insert(QStringLiteral("martinique"), e({0x1F1F2, 0x1F1F6})); // 🇲🇶
    m.insert(QStringLiteral("mask"), e({0x1F637})); // 😷
    m.insert(QStringLiteral("massage"), e({0x1F486})); // 💆
    m.insert(QStringLiteral("massage_man"), e({0x1F486, 0x200D, 0x2642, 0xFE0F})); // 💆‍♂️
    m.insert(QStringLiteral("massage_woman"), e({0x1F486, 0x200D, 0x2640, 0xFE0F})); // 💆‍♀️
    m.insert(QStringLiteral("mate"), e({0x1F9C9})); // 🧉
    m.insert(QStringLiteral("mauritania"), e({0x1F1F2, 0x1F1F7})); // 🇲🇷
    m.insert(QStringLiteral("mauritius"), e({0x1F1F2, 0x1F1FA})); // 🇲🇺
    m.insert(QStringLiteral("mayotte"), e({0x1F1FE, 0x1F1F9})); // 🇾🇹
    m.insert(QStringLiteral("meat_on_bone"), e({0x1F356})); // 🍖
    m.insert(QStringLiteral("mechanic"), e({0x1F9D1, 0x200D, 0x1F527})); // 🧑‍🔧
    m.insert(QStringLiteral("mechanical_arm"), e({0x1F9BE})); // 🦾
    m.insert(QStringLiteral("mechanical_leg"), e({0x1F9BF})); // 🦿
    m.insert(QStringLiteral("medal_military"), e({0x1F396})); // 🎖
    m.insert(QStringLiteral("medal_sports"), e({0x1F3C5})); // 🏅
    m.insert(QStringLiteral("medical_symbol"), e({0x2695})); // ⚕
    m.insert(QStringLiteral("mega"), e({0x1F4E3})); // 📣
    m.insert(QStringLiteral("melon"), e({0x1F348})); // 🍈
    m.insert(QStringLiteral("melting_face"), e({0x1FAE0})); // 🫠
    m.insert(QStringLiteral("memo"), e({0x1F4DD})); // 📝
    m.insert(QStringLiteral("men_wrestling"), e({0x1F93C, 0x200D, 0x2642, 0xFE0F})); // 🤼‍♂️
    m.insert(QStringLiteral("mending_heart"), e({0x2764, 0xFE0F, 0x200D, 0x1FA79})); // ❤️‍🩹
    m.insert(QStringLiteral("menorah"), e({0x1F54E})); // 🕎
    m.insert(QStringLiteral("mens"), e({0x1F6B9})); // 🚹
    m.insert(QStringLiteral("mermaid"), e({0x1F9DC, 0x200D, 0x2640, 0xFE0F})); // 🧜‍♀️
    m.insert(QStringLiteral("merman"), e({0x1F9DC, 0x200D, 0x2642, 0xFE0F})); // 🧜‍♂️
    m.insert(QStringLiteral("merperson"), e({0x1F9DC})); // 🧜
    m.insert(QStringLiteral("metal"), e({0x1F918})); // 🤘
    m.insert(QStringLiteral("metro"), e({0x1F687})); // 🚇
    m.insert(QStringLiteral("mexico"), e({0x1F1F2, 0x1F1FD})); // 🇲🇽
    m.insert(QStringLiteral("microbe"), e({0x1F9A0})); // 🦠
    m.insert(QStringLiteral("micronesia"), e({0x1F1EB, 0x1F1F2})); // 🇫🇲
    m.insert(QStringLiteral("microphone"), e({0x1F3A4})); // 🎤
    m.insert(QStringLiteral("microscope"), e({0x1F52C})); // 🔬
    m.insert(QStringLiteral("middle_finger"), e({0x1F595})); // 🖕
    m.insert(QStringLiteral("military_helmet"), e({0x1FA96})); // 🪖
    m.insert(QStringLiteral("milk_glass"), e({0x1F95B})); // 🥛
    m.insert(QStringLiteral("milky_way"), e({0x1F30C})); // 🌌
    m.insert(QStringLiteral("minibus"), e({0x1F690})); // 🚐
    m.insert(QStringLiteral("minidisc"), e({0x1F4BD})); // 💽
    m.insert(QStringLiteral("mirror"), e({0x1FA9E})); // 🪞
    m.insert(QStringLiteral("mirror_ball"), e({0x1FAA9})); // 🪩
    m.insert(QStringLiteral("mobile_phone_off"), e({0x1F4F4})); // 📴
    m.insert(QStringLiteral("moldova"), e({0x1F1F2, 0x1F1E9})); // 🇲🇩
    m.insert(QStringLiteral("monaco"), e({0x1F1F2, 0x1F1E8})); // 🇲🇨
    m.insert(QStringLiteral("money_mouth_face"), e({0x1F911})); // 🤑
    m.insert(QStringLiteral("money_with_wings"), e({0x1F4B8})); // 💸
    m.insert(QStringLiteral("moneybag"), e({0x1F4B0})); // 💰
    m.insert(QStringLiteral("mongolia"), e({0x1F1F2, 0x1F1F3})); // 🇲🇳
    m.insert(QStringLiteral("monkey"), e({0x1F412})); // 🐒
    m.insert(QStringLiteral("monkey_face"), e({0x1F435})); // 🐵
    m.insert(QStringLiteral("monocle_face"), e({0x1F9D0})); // 🧐
    m.insert(QStringLiteral("monorail"), e({0x1F69D})); // 🚝
    m.insert(QStringLiteral("montenegro"), e({0x1F1F2, 0x1F1EA})); // 🇲🇪
    m.insert(QStringLiteral("montserrat"), e({0x1F1F2, 0x1F1F8})); // 🇲🇸
    m.insert(QStringLiteral("moon"), e({0x1F314})); // 🌔
    m.insert(QStringLiteral("moon_cake"), e({0x1F96E})); // 🥮
    m.insert(QStringLiteral("moose"), e({0x1FACE})); // 🫎
    m.insert(QStringLiteral("morocco"), e({0x1F1F2, 0x1F1E6})); // 🇲🇦
    m.insert(QStringLiteral("mortar_board"), e({0x1F393})); // 🎓
    m.insert(QStringLiteral("mosque"), e({0x1F54C})); // 🕌
    m.insert(QStringLiteral("mosquito"), e({0x1F99F})); // 🦟
    m.insert(QStringLiteral("motor_boat"), e({0x1F6E5})); // 🛥
    m.insert(QStringLiteral("motor_scooter"), e({0x1F6F5})); // 🛵
    m.insert(QStringLiteral("motorcycle"), e({0x1F3CD})); // 🏍
    m.insert(QStringLiteral("motorized_wheelchair"), e({0x1F9BC})); // 🦼
    m.insert(QStringLiteral("motorway"), e({0x1F6E3})); // 🛣
    m.insert(QStringLiteral("mount_fuji"), e({0x1F5FB})); // 🗻
    m.insert(QStringLiteral("mountain"), e({0x26F0})); // ⛰
    m.insert(QStringLiteral("mountain_bicyclist"), e({0x1F6B5})); // 🚵
    m.insert(QStringLiteral("mountain_biking_man"), e({0x1F6B5, 0x200D, 0x2642, 0xFE0F})); // 🚵‍♂️
    m.insert(QStringLiteral("mountain_biking_woman"), e({0x1F6B5, 0x200D, 0x2640, 0xFE0F})); // 🚵‍♀️
    m.insert(QStringLiteral("mountain_cableway"), e({0x1F6A0})); // 🚠
    m.insert(QStringLiteral("mountain_railway"), e({0x1F69E})); // 🚞
    m.insert(QStringLiteral("mountain_snow"), e({0x1F3D4})); // 🏔
    m.insert(QStringLiteral("mouse"), e({0x1F42D})); // 🐭
    m.insert(QStringLiteral("mouse2"), e({0x1F401})); // 🐁
    m.insert(QStringLiteral("mouse_trap"), e({0x1FAA4})); // 🪤
    m.insert(QStringLiteral("movie_camera"), e({0x1F3A5})); // 🎥
    m.insert(QStringLiteral("moyai"), e({0x1F5FF})); // 🗿
    m.insert(QStringLiteral("mozambique"), e({0x1F1F2, 0x1F1FF})); // 🇲🇿
    m.insert(QStringLiteral("mrs_claus"), e({0x1F936})); // 🤶
    m.insert(QStringLiteral("muscle"), e({0x1F4AA})); // 💪
    m.insert(QStringLiteral("mushroom"), e({0x1F344})); // 🍄
    m.insert(QStringLiteral("musical_keyboard"), e({0x1F3B9})); // 🎹
    m.insert(QStringLiteral("musical_note"), e({0x1F3B5})); // 🎵
    m.insert(QStringLiteral("musical_score"), e({0x1F3BC})); // 🎼
    m.insert(QStringLiteral("mute"), e({0x1F507})); // 🔇
    m.insert(QStringLiteral("mx_claus"), e({0x1F9D1, 0x200D, 0x1F384})); // 🧑‍🎄
    m.insert(QStringLiteral("myanmar"), e({0x1F1F2, 0x1F1F2})); // 🇲🇲
    m.insert(QStringLiteral("nail_care"), e({0x1F485})); // 💅
    m.insert(QStringLiteral("name_badge"), e({0x1F4DB})); // 📛
    m.insert(QStringLiteral("namibia"), e({0x1F1F3, 0x1F1E6})); // 🇳🇦
    m.insert(QStringLiteral("national_park"), e({0x1F3DE})); // 🏞
    m.insert(QStringLiteral("nauru"), e({0x1F1F3, 0x1F1F7})); // 🇳🇷
    m.insert(QStringLiteral("nauseated_face"), e({0x1F922})); // 🤢
    m.insert(QStringLiteral("nazar_amulet"), e({0x1F9FF})); // 🧿
    m.insert(QStringLiteral("necktie"), e({0x1F454})); // 👔
    m.insert(QStringLiteral("negative_squared_cross_mark"), e({0x274E})); // ❎
    m.insert(QStringLiteral("nepal"), e({0x1F1F3, 0x1F1F5})); // 🇳🇵
    m.insert(QStringLiteral("nerd_face"), e({0x1F913})); // 🤓
    m.insert(QStringLiteral("nest_with_eggs"), e({0x1FABA})); // 🪺
    m.insert(QStringLiteral("nesting_dolls"), e({0x1FA86})); // 🪆
    m.insert(QStringLiteral("netherlands"), e({0x1F1F3, 0x1F1F1})); // 🇳🇱
    m.insert(QStringLiteral("neutral_face"), e({0x1F610})); // 😐
    m.insert(QStringLiteral("new"), e({0x1F195})); // 🆕
    m.insert(QStringLiteral("new_caledonia"), e({0x1F1F3, 0x1F1E8})); // 🇳🇨
    m.insert(QStringLiteral("new_moon"), e({0x1F311})); // 🌑
    m.insert(QStringLiteral("new_moon_with_face"), e({0x1F31A})); // 🌚
    m.insert(QStringLiteral("new_zealand"), e({0x1F1F3, 0x1F1FF})); // 🇳🇿
    m.insert(QStringLiteral("newspaper"), e({0x1F4F0})); // 📰
    m.insert(QStringLiteral("newspaper_roll"), e({0x1F5DE})); // 🗞
    m.insert(QStringLiteral("next_track_button"), e({0x23ED})); // ⏭
    m.insert(QStringLiteral("ng"), e({0x1F196})); // 🆖
    m.insert(QStringLiteral("ng_man"), e({0x1F645, 0x200D, 0x2642, 0xFE0F})); // 🙅‍♂️
    m.insert(QStringLiteral("ng_woman"), e({0x1F645, 0x200D, 0x2640, 0xFE0F})); // 🙅‍♀️
    m.insert(QStringLiteral("nicaragua"), e({0x1F1F3, 0x1F1EE})); // 🇳🇮
    m.insert(QStringLiteral("niger"), e({0x1F1F3, 0x1F1EA})); // 🇳🇪
    m.insert(QStringLiteral("nigeria"), e({0x1F1F3, 0x1F1EC})); // 🇳🇬
    m.insert(QStringLiteral("night_with_stars"), e({0x1F303})); // 🌃
    m.insert(QStringLiteral("nine"), e({0x39, 0xFE0F, 0x20E3})); // 9️⃣
    m.insert(QStringLiteral("ninja"), e({0x1F977})); // 🥷
    m.insert(QStringLiteral("niue"), e({0x1F1F3, 0x1F1FA})); // 🇳🇺
    m.insert(QStringLiteral("no_bell"), e({0x1F515})); // 🔕
    m.insert(QStringLiteral("no_bicycles"), e({0x1F6B3})); // 🚳
    m.insert(QStringLiteral("no_entry"), e({0x26D4})); // ⛔
    m.insert(QStringLiteral("no_entry_sign"), e({0x1F6AB})); // 🚫
    m.insert(QStringLiteral("no_good"), e({0x1F645})); // 🙅
    m.insert(QStringLiteral("no_good_man"), e({0x1F645, 0x200D, 0x2642, 0xFE0F})); // 🙅‍♂️
    m.insert(QStringLiteral("no_good_woman"), e({0x1F645, 0x200D, 0x2640, 0xFE0F})); // 🙅‍♀️
    m.insert(QStringLiteral("no_mobile_phones"), e({0x1F4F5})); // 📵
    m.insert(QStringLiteral("no_mouth"), e({0x1F636})); // 😶
    m.insert(QStringLiteral("no_pedestrians"), e({0x1F6B7})); // 🚷
    m.insert(QStringLiteral("no_smoking"), e({0x1F6AD})); // 🚭
    m.insert(QStringLiteral("non-potable_water"), e({0x1F6B1})); // 🚱
    m.insert(QStringLiteral("norfolk_island"), e({0x1F1F3, 0x1F1EB})); // 🇳🇫
    m.insert(QStringLiteral("north_korea"), e({0x1F1F0, 0x1F1F5})); // 🇰🇵
    m.insert(QStringLiteral("northern_mariana_islands"), e({0x1F1F2, 0x1F1F5})); // 🇲🇵
    m.insert(QStringLiteral("norway"), e({0x1F1F3, 0x1F1F4})); // 🇳🇴
    m.insert(QStringLiteral("nose"), e({0x1F443})); // 👃
    m.insert(QStringLiteral("notebook"), e({0x1F4D3})); // 📓
    m.insert(QStringLiteral("notebook_with_decorative_cover"), e({0x1F4D4})); // 📔
    m.insert(QStringLiteral("notes"), e({0x1F3B6})); // 🎶
    m.insert(QStringLiteral("nut_and_bolt"), e({0x1F529})); // 🔩
    m.insert(QStringLiteral("o"), e({0x2B55})); // ⭕
    m.insert(QStringLiteral("o2"), e({0x1F17E})); // 🅾
    m.insert(QStringLiteral("ocean"), e({0x1F30A})); // 🌊
    m.insert(QStringLiteral("octopus"), e({0x1F419})); // 🐙
    m.insert(QStringLiteral("oden"), e({0x1F362})); // 🍢
    m.insert(QStringLiteral("office"), e({0x1F3E2})); // 🏢
    m.insert(QStringLiteral("office_worker"), e({0x1F9D1, 0x200D, 0x1F4BC})); // 🧑‍💼
    m.insert(QStringLiteral("oil_drum"), e({0x1F6E2})); // 🛢
    m.insert(QStringLiteral("ok"), e({0x1F197})); // 🆗
    m.insert(QStringLiteral("ok_hand"), e({0x1F44C})); // 👌
    m.insert(QStringLiteral("ok_man"), e({0x1F646, 0x200D, 0x2642, 0xFE0F})); // 🙆‍♂️
    m.insert(QStringLiteral("ok_person"), e({0x1F646})); // 🙆
    m.insert(QStringLiteral("ok_woman"), e({0x1F646, 0x200D, 0x2640, 0xFE0F})); // 🙆‍♀️
    m.insert(QStringLiteral("old_key"), e({0x1F5DD})); // 🗝
    m.insert(QStringLiteral("older_adult"), e({0x1F9D3})); // 🧓
    m.insert(QStringLiteral("older_man"), e({0x1F474})); // 👴
    m.insert(QStringLiteral("older_woman"), e({0x1F475})); // 👵
    m.insert(QStringLiteral("olive"), e({0x1FAD2})); // 🫒
    m.insert(QStringLiteral("om"), e({0x1F549})); // 🕉
    m.insert(QStringLiteral("oman"), e({0x1F1F4, 0x1F1F2})); // 🇴🇲
    m.insert(QStringLiteral("on"), e({0x1F51B})); // 🔛
    m.insert(QStringLiteral("oncoming_automobile"), e({0x1F698})); // 🚘
    m.insert(QStringLiteral("oncoming_bus"), e({0x1F68D})); // 🚍
    m.insert(QStringLiteral("oncoming_police_car"), e({0x1F694})); // 🚔
    m.insert(QStringLiteral("oncoming_taxi"), e({0x1F696})); // 🚖
    m.insert(QStringLiteral("one"), e({0x31, 0xFE0F, 0x20E3})); // 1️⃣
    m.insert(QStringLiteral("one_piece_swimsuit"), e({0x1FA71})); // 🩱
    m.insert(QStringLiteral("onion"), e({0x1F9C5})); // 🧅
    m.insert(QStringLiteral("open_book"), e({0x1F4D6})); // 📖
    m.insert(QStringLiteral("open_file_folder"), e({0x1F4C2})); // 📂
    m.insert(QStringLiteral("open_hands"), e({0x1F450})); // 👐
    m.insert(QStringLiteral("open_mouth"), e({0x1F62E})); // 😮
    m.insert(QStringLiteral("open_umbrella"), e({0x2602})); // ☂
    m.insert(QStringLiteral("ophiuchus"), e({0x26CE})); // ⛎
    m.insert(QStringLiteral("orange"), e({0x1F34A})); // 🍊
    m.insert(QStringLiteral("orange_book"), e({0x1F4D9})); // 📙
    m.insert(QStringLiteral("orange_circle"), e({0x1F7E0})); // 🟠
    m.insert(QStringLiteral("orange_heart"), e({0x1F9E1})); // 🧡
    m.insert(QStringLiteral("orange_square"), e({0x1F7E7})); // 🟧
    m.insert(QStringLiteral("orangutan"), e({0x1F9A7})); // 🦧
    m.insert(QStringLiteral("orthodox_cross"), e({0x2626})); // ☦
    m.insert(QStringLiteral("otter"), e({0x1F9A6})); // 🦦
    m.insert(QStringLiteral("outbox_tray"), e({0x1F4E4})); // 📤
    m.insert(QStringLiteral("owl"), e({0x1F989})); // 🦉
    m.insert(QStringLiteral("ox"), e({0x1F402})); // 🐂
    m.insert(QStringLiteral("oyster"), e({0x1F9AA})); // 🦪
    m.insert(QStringLiteral("package"), e({0x1F4E6})); // 📦
    m.insert(QStringLiteral("page_facing_up"), e({0x1F4C4})); // 📄
    m.insert(QStringLiteral("page_with_curl"), e({0x1F4C3})); // 📃
    m.insert(QStringLiteral("pager"), e({0x1F4DF})); // 📟
    m.insert(QStringLiteral("paintbrush"), e({0x1F58C})); // 🖌
    m.insert(QStringLiteral("pakistan"), e({0x1F1F5, 0x1F1F0})); // 🇵🇰
    m.insert(QStringLiteral("palau"), e({0x1F1F5, 0x1F1FC})); // 🇵🇼
    m.insert(QStringLiteral("palestinian_territories"), e({0x1F1F5, 0x1F1F8})); // 🇵🇸
    m.insert(QStringLiteral("palm_down_hand"), e({0x1FAF3})); // 🫳
    m.insert(QStringLiteral("palm_tree"), e({0x1F334})); // 🌴
    m.insert(QStringLiteral("palm_up_hand"), e({0x1FAF4})); // 🫴
    m.insert(QStringLiteral("palms_up_together"), e({0x1F932})); // 🤲
    m.insert(QStringLiteral("panama"), e({0x1F1F5, 0x1F1E6})); // 🇵🇦
    m.insert(QStringLiteral("pancakes"), e({0x1F95E})); // 🥞
    m.insert(QStringLiteral("panda_face"), e({0x1F43C})); // 🐼
    m.insert(QStringLiteral("paperclip"), e({0x1F4CE})); // 📎
    m.insert(QStringLiteral("paperclips"), e({0x1F587})); // 🖇
    m.insert(QStringLiteral("papua_new_guinea"), e({0x1F1F5, 0x1F1EC})); // 🇵🇬
    m.insert(QStringLiteral("parachute"), e({0x1FA82})); // 🪂
    m.insert(QStringLiteral("paraguay"), e({0x1F1F5, 0x1F1FE})); // 🇵🇾
    m.insert(QStringLiteral("parasol_on_ground"), e({0x26F1})); // ⛱
    m.insert(QStringLiteral("parking"), e({0x1F17F})); // 🅿
    m.insert(QStringLiteral("parrot"), e({0x1F99C})); // 🦜
    m.insert(QStringLiteral("part_alternation_mark"), e({0x303D})); // 〽
    m.insert(QStringLiteral("partly_sunny"), e({0x26C5})); // ⛅
    m.insert(QStringLiteral("partying_face"), e({0x1F973})); // 🥳
    m.insert(QStringLiteral("passenger_ship"), e({0x1F6F3})); // 🛳
    m.insert(QStringLiteral("passport_control"), e({0x1F6C2})); // 🛂
    m.insert(QStringLiteral("pause_button"), e({0x23F8})); // ⏸
    m.insert(QStringLiteral("paw_prints"), e({0x1F43E})); // 🐾
    m.insert(QStringLiteral("pea_pod"), e({0x1FADB})); // 🫛
    m.insert(QStringLiteral("peace_symbol"), e({0x262E})); // ☮
    m.insert(QStringLiteral("peach"), e({0x1F351})); // 🍑
    m.insert(QStringLiteral("peacock"), e({0x1F99A})); // 🦚
    m.insert(QStringLiteral("peanuts"), e({0x1F95C})); // 🥜
    m.insert(QStringLiteral("pear"), e({0x1F350})); // 🍐
    m.insert(QStringLiteral("pen"), e({0x1F58A})); // 🖊
    m.insert(QStringLiteral("pencil"), e({0x1F4DD})); // 📝
    m.insert(QStringLiteral("pencil2"), e({0x270F})); // ✏
    m.insert(QStringLiteral("penguin"), e({0x1F427})); // 🐧
    m.insert(QStringLiteral("pensive"), e({0x1F614})); // 😔
    m.insert(QStringLiteral("people_holding_hands"), e({0x1F9D1, 0x200D, 0x1F91D, 0x200D, 0x1F9D1})); // 🧑‍🤝‍🧑
    m.insert(QStringLiteral("people_hugging"), e({0x1FAC2})); // 🫂
    m.insert(QStringLiteral("performing_arts"), e({0x1F3AD})); // 🎭
    m.insert(QStringLiteral("persevere"), e({0x1F623})); // 😣
    m.insert(QStringLiteral("person_bald"), e({0x1F9D1, 0x200D, 0x1F9B2})); // 🧑‍🦲
    m.insert(QStringLiteral("person_curly_hair"), e({0x1F9D1, 0x200D, 0x1F9B1})); // 🧑‍🦱
    m.insert(QStringLiteral("person_feeding_baby"), e({0x1F9D1, 0x200D, 0x1F37C})); // 🧑‍🍼
    m.insert(QStringLiteral("person_fencing"), e({0x1F93A})); // 🤺
    m.insert(QStringLiteral("person_in_manual_wheelchair"), e({0x1F9D1, 0x200D, 0x1F9BD})); // 🧑‍🦽
    m.insert(QStringLiteral("person_in_motorized_wheelchair"), e({0x1F9D1, 0x200D, 0x1F9BC})); // 🧑‍🦼
    m.insert(QStringLiteral("person_in_tuxedo"), e({0x1F935})); // 🤵
    m.insert(QStringLiteral("person_red_hair"), e({0x1F9D1, 0x200D, 0x1F9B0})); // 🧑‍🦰
    m.insert(QStringLiteral("person_white_hair"), e({0x1F9D1, 0x200D, 0x1F9B3})); // 🧑‍🦳
    m.insert(QStringLiteral("person_with_crown"), e({0x1FAC5})); // 🫅
    m.insert(QStringLiteral("person_with_probing_cane"), e({0x1F9D1, 0x200D, 0x1F9AF})); // 🧑‍🦯
    m.insert(QStringLiteral("person_with_turban"), e({0x1F473})); // 👳
    m.insert(QStringLiteral("person_with_veil"), e({0x1F470})); // 👰
    m.insert(QStringLiteral("peru"), e({0x1F1F5, 0x1F1EA})); // 🇵🇪
    m.insert(QStringLiteral("petri_dish"), e({0x1F9EB})); // 🧫
    m.insert(QStringLiteral("philippines"), e({0x1F1F5, 0x1F1ED})); // 🇵🇭
    m.insert(QStringLiteral("phone"), e({0x260E})); // ☎
    m.insert(QStringLiteral("pick"), e({0x26CF})); // ⛏
    m.insert(QStringLiteral("pickup_truck"), e({0x1F6FB})); // 🛻
    m.insert(QStringLiteral("pie"), e({0x1F967})); // 🥧
    m.insert(QStringLiteral("pig"), e({0x1F437})); // 🐷
    m.insert(QStringLiteral("pig2"), e({0x1F416})); // 🐖
    m.insert(QStringLiteral("pig_nose"), e({0x1F43D})); // 🐽
    m.insert(QStringLiteral("pill"), e({0x1F48A})); // 💊
    m.insert(QStringLiteral("pilot"), e({0x1F9D1, 0x200D, 0x2708, 0xFE0F})); // 🧑‍✈️
    m.insert(QStringLiteral("pinata"), e({0x1FA85})); // 🪅
    m.insert(QStringLiteral("pinched_fingers"), e({0x1F90C})); // 🤌
    m.insert(QStringLiteral("pinching_hand"), e({0x1F90F})); // 🤏
    m.insert(QStringLiteral("pineapple"), e({0x1F34D})); // 🍍
    m.insert(QStringLiteral("ping_pong"), e({0x1F3D3})); // 🏓
    m.insert(QStringLiteral("pink_heart"), e({0x1FA77})); // 🩷
    m.insert(QStringLiteral("pirate_flag"), e({0x1F3F4, 0x200D, 0x2620, 0xFE0F})); // 🏴‍☠️
    m.insert(QStringLiteral("pisces"), e({0x2653})); // ♓
    m.insert(QStringLiteral("pitcairn_islands"), e({0x1F1F5, 0x1F1F3})); // 🇵🇳
    m.insert(QStringLiteral("pizza"), e({0x1F355})); // 🍕
    m.insert(QStringLiteral("placard"), e({0x1FAA7})); // 🪧
    m.insert(QStringLiteral("place_of_worship"), e({0x1F6D0})); // 🛐
    m.insert(QStringLiteral("plate_with_cutlery"), e({0x1F37D})); // 🍽
    m.insert(QStringLiteral("play_or_pause_button"), e({0x23EF})); // ⏯
    m.insert(QStringLiteral("playground_slide"), e({0x1F6DD})); // 🛝
    m.insert(QStringLiteral("pleading_face"), e({0x1F97A})); // 🥺
    m.insert(QStringLiteral("plunger"), e({0x1FAA0})); // 🪠
    m.insert(QStringLiteral("point_down"), e({0x1F447})); // 👇
    m.insert(QStringLiteral("point_left"), e({0x1F448})); // 👈
    m.insert(QStringLiteral("point_right"), e({0x1F449})); // 👉
    m.insert(QStringLiteral("point_up"), e({0x261D})); // ☝
    m.insert(QStringLiteral("point_up_2"), e({0x1F446})); // 👆
    m.insert(QStringLiteral("poland"), e({0x1F1F5, 0x1F1F1})); // 🇵🇱
    m.insert(QStringLiteral("polar_bear"), e({0x1F43B, 0x200D, 0x2744, 0xFE0F})); // 🐻‍❄️
    m.insert(QStringLiteral("police_car"), e({0x1F693})); // 🚓
    m.insert(QStringLiteral("police_officer"), e({0x1F46E})); // 👮
    m.insert(QStringLiteral("policeman"), e({0x1F46E, 0x200D, 0x2642, 0xFE0F})); // 👮‍♂️
    m.insert(QStringLiteral("policewoman"), e({0x1F46E, 0x200D, 0x2640, 0xFE0F})); // 👮‍♀️
    m.insert(QStringLiteral("poodle"), e({0x1F429})); // 🐩
    m.insert(QStringLiteral("poop"), e({0x1F4A9})); // 💩
    m.insert(QStringLiteral("popcorn"), e({0x1F37F})); // 🍿
    m.insert(QStringLiteral("portugal"), e({0x1F1F5, 0x1F1F9})); // 🇵🇹
    m.insert(QStringLiteral("post_office"), e({0x1F3E3})); // 🏣
    m.insert(QStringLiteral("postal_horn"), e({0x1F4EF})); // 📯
    m.insert(QStringLiteral("postbox"), e({0x1F4EE})); // 📮
    m.insert(QStringLiteral("potable_water"), e({0x1F6B0})); // 🚰
    m.insert(QStringLiteral("potato"), e({0x1F954})); // 🥔
    m.insert(QStringLiteral("potted_plant"), e({0x1FAB4})); // 🪴
    m.insert(QStringLiteral("pouch"), e({0x1F45D})); // 👝
    m.insert(QStringLiteral("poultry_leg"), e({0x1F357})); // 🍗
    m.insert(QStringLiteral("pound"), e({0x1F4B7})); // 💷
    m.insert(QStringLiteral("pouring_liquid"), e({0x1FAD7})); // 🫗
    m.insert(QStringLiteral("pout"), e({0x1F621})); // 😡
    m.insert(QStringLiteral("pouting_cat"), e({0x1F63E})); // 😾
    m.insert(QStringLiteral("pouting_face"), e({0x1F64E})); // 🙎
    m.insert(QStringLiteral("pouting_man"), e({0x1F64E, 0x200D, 0x2642, 0xFE0F})); // 🙎‍♂️
    m.insert(QStringLiteral("pouting_woman"), e({0x1F64E, 0x200D, 0x2640, 0xFE0F})); // 🙎‍♀️
    m.insert(QStringLiteral("pray"), e({0x1F64F})); // 🙏
    m.insert(QStringLiteral("prayer_beads"), e({0x1F4FF})); // 📿
    m.insert(QStringLiteral("pregnant_man"), e({0x1FAC3})); // 🫃
    m.insert(QStringLiteral("pregnant_person"), e({0x1FAC4})); // 🫄
    m.insert(QStringLiteral("pregnant_woman"), e({0x1F930})); // 🤰
    m.insert(QStringLiteral("pretzel"), e({0x1F968})); // 🥨
    m.insert(QStringLiteral("previous_track_button"), e({0x23EE})); // ⏮
    m.insert(QStringLiteral("prince"), e({0x1F934})); // 🤴
    m.insert(QStringLiteral("princess"), e({0x1F478})); // 👸
    m.insert(QStringLiteral("printer"), e({0x1F5A8})); // 🖨
    m.insert(QStringLiteral("probing_cane"), e({0x1F9AF})); // 🦯
    m.insert(QStringLiteral("puerto_rico"), e({0x1F1F5, 0x1F1F7})); // 🇵🇷
    m.insert(QStringLiteral("punch"), e({0x1F44A})); // 👊
    m.insert(QStringLiteral("purple_circle"), e({0x1F7E3})); // 🟣
    m.insert(QStringLiteral("purple_heart"), e({0x1F49C})); // 💜
    m.insert(QStringLiteral("purple_square"), e({0x1F7EA})); // 🟪
    m.insert(QStringLiteral("purse"), e({0x1F45B})); // 👛
    m.insert(QStringLiteral("pushpin"), e({0x1F4CC})); // 📌
    m.insert(QStringLiteral("put_litter_in_its_place"), e({0x1F6AE})); // 🚮
    m.insert(QStringLiteral("qatar"), e({0x1F1F6, 0x1F1E6})); // 🇶🇦
    m.insert(QStringLiteral("question"), e({0x2753})); // ❓
    m.insert(QStringLiteral("rabbit"), e({0x1F430})); // 🐰
    m.insert(QStringLiteral("rabbit2"), e({0x1F407})); // 🐇
    m.insert(QStringLiteral("raccoon"), e({0x1F99D})); // 🦝
    m.insert(QStringLiteral("racehorse"), e({0x1F40E})); // 🐎
    m.insert(QStringLiteral("racing_car"), e({0x1F3CE})); // 🏎
    m.insert(QStringLiteral("radio"), e({0x1F4FB})); // 📻
    m.insert(QStringLiteral("radio_button"), e({0x1F518})); // 🔘
    m.insert(QStringLiteral("radioactive"), e({0x2622})); // ☢
    m.insert(QStringLiteral("rage"), e({0x1F621})); // 😡
    m.insert(QStringLiteral("railway_car"), e({0x1F683})); // 🚃
    m.insert(QStringLiteral("railway_track"), e({0x1F6E4})); // 🛤
    m.insert(QStringLiteral("rainbow"), e({0x1F308})); // 🌈
    m.insert(QStringLiteral("rainbow_flag"), e({0x1F3F3, 0xFE0F, 0x200D, 0x1F308})); // 🏳️‍🌈
    m.insert(QStringLiteral("raised_back_of_hand"), e({0x1F91A})); // 🤚
    m.insert(QStringLiteral("raised_eyebrow"), e({0x1F928})); // 🤨
    m.insert(QStringLiteral("raised_hand"), e({0x270B})); // ✋
    m.insert(QStringLiteral("raised_hand_with_fingers_splayed"), e({0x1F590})); // 🖐
    m.insert(QStringLiteral("raised_hands"), e({0x1F64C})); // 🙌
    m.insert(QStringLiteral("raising_hand"), e({0x1F64B})); // 🙋
    m.insert(QStringLiteral("raising_hand_man"), e({0x1F64B, 0x200D, 0x2642, 0xFE0F})); // 🙋‍♂️
    m.insert(QStringLiteral("raising_hand_woman"), e({0x1F64B, 0x200D, 0x2640, 0xFE0F})); // 🙋‍♀️
    m.insert(QStringLiteral("ram"), e({0x1F40F})); // 🐏
    m.insert(QStringLiteral("ramen"), e({0x1F35C})); // 🍜
    m.insert(QStringLiteral("rat"), e({0x1F400})); // 🐀
    m.insert(QStringLiteral("razor"), e({0x1FA92})); // 🪒
    m.insert(QStringLiteral("receipt"), e({0x1F9FE})); // 🧾
    m.insert(QStringLiteral("record_button"), e({0x23FA})); // ⏺
    m.insert(QStringLiteral("recycle"), e({0x267B})); // ♻
    m.insert(QStringLiteral("red_car"), e({0x1F697})); // 🚗
    m.insert(QStringLiteral("red_circle"), e({0x1F534})); // 🔴
    m.insert(QStringLiteral("red_envelope"), e({0x1F9E7})); // 🧧
    m.insert(QStringLiteral("red_haired_man"), e({0x1F468, 0x200D, 0x1F9B0})); // 👨‍🦰
    m.insert(QStringLiteral("red_haired_woman"), e({0x1F469, 0x200D, 0x1F9B0})); // 👩‍🦰
    m.insert(QStringLiteral("red_square"), e({0x1F7E5})); // 🟥
    m.insert(QStringLiteral("registered"), e({0xAE})); // ®
    m.insert(QStringLiteral("relaxed"), e({0x263A})); // ☺
    m.insert(QStringLiteral("relieved"), e({0x1F60C})); // 😌
    m.insert(QStringLiteral("reminder_ribbon"), e({0x1F397})); // 🎗
    m.insert(QStringLiteral("repeat"), e({0x1F501})); // 🔁
    m.insert(QStringLiteral("repeat_one"), e({0x1F502})); // 🔂
    m.insert(QStringLiteral("rescue_worker_helmet"), e({0x26D1})); // ⛑
    m.insert(QStringLiteral("restroom"), e({0x1F6BB})); // 🚻
    m.insert(QStringLiteral("reunion"), e({0x1F1F7, 0x1F1EA})); // 🇷🇪
    m.insert(QStringLiteral("revolving_hearts"), e({0x1F49E})); // 💞
    m.insert(QStringLiteral("rewind"), e({0x23EA})); // ⏪
    m.insert(QStringLiteral("rhinoceros"), e({0x1F98F})); // 🦏
    m.insert(QStringLiteral("ribbon"), e({0x1F380})); // 🎀
    m.insert(QStringLiteral("rice"), e({0x1F35A})); // 🍚
    m.insert(QStringLiteral("rice_ball"), e({0x1F359})); // 🍙
    m.insert(QStringLiteral("rice_cracker"), e({0x1F358})); // 🍘
    m.insert(QStringLiteral("rice_scene"), e({0x1F391})); // 🎑
    m.insert(QStringLiteral("right_anger_bubble"), e({0x1F5EF})); // 🗯
    m.insert(QStringLiteral("rightwards_hand"), e({0x1FAF1})); // 🫱
    m.insert(QStringLiteral("rightwards_pushing_hand"), e({0x1FAF8})); // 🫸
    m.insert(QStringLiteral("ring"), e({0x1F48D})); // 💍
    m.insert(QStringLiteral("ring_buoy"), e({0x1F6DF})); // 🛟
    m.insert(QStringLiteral("ringed_planet"), e({0x1FA90})); // 🪐
    m.insert(QStringLiteral("robot"), e({0x1F916})); // 🤖
    m.insert(QStringLiteral("rock"), e({0x1FAA8})); // 🪨
    m.insert(QStringLiteral("rocket"), e({0x1F680})); // 🚀
    m.insert(QStringLiteral("rofl"), e({0x1F923})); // 🤣
    m.insert(QStringLiteral("roll_eyes"), e({0x1F644})); // 🙄
    m.insert(QStringLiteral("roll_of_paper"), e({0x1F9FB})); // 🧻
    m.insert(QStringLiteral("roller_coaster"), e({0x1F3A2})); // 🎢
    m.insert(QStringLiteral("roller_skate"), e({0x1F6FC})); // 🛼
    m.insert(QStringLiteral("romania"), e({0x1F1F7, 0x1F1F4})); // 🇷🇴
    m.insert(QStringLiteral("rooster"), e({0x1F413})); // 🐓
    m.insert(QStringLiteral("rose"), e({0x1F339})); // 🌹
    m.insert(QStringLiteral("rosette"), e({0x1F3F5})); // 🏵
    m.insert(QStringLiteral("rotating_light"), e({0x1F6A8})); // 🚨
    m.insert(QStringLiteral("round_pushpin"), e({0x1F4CD})); // 📍
    m.insert(QStringLiteral("rowboat"), e({0x1F6A3})); // 🚣
    m.insert(QStringLiteral("rowing_man"), e({0x1F6A3, 0x200D, 0x2642, 0xFE0F})); // 🚣‍♂️
    m.insert(QStringLiteral("rowing_woman"), e({0x1F6A3, 0x200D, 0x2640, 0xFE0F})); // 🚣‍♀️
    m.insert(QStringLiteral("ru"), e({0x1F1F7, 0x1F1FA})); // 🇷🇺
    m.insert(QStringLiteral("rugby_football"), e({0x1F3C9})); // 🏉
    m.insert(QStringLiteral("runner"), e({0x1F3C3})); // 🏃
    m.insert(QStringLiteral("running"), e({0x1F3C3})); // 🏃
    m.insert(QStringLiteral("running_man"), e({0x1F3C3, 0x200D, 0x2642, 0xFE0F})); // 🏃‍♂️
    m.insert(QStringLiteral("running_shirt_with_sash"), e({0x1F3BD})); // 🎽
    m.insert(QStringLiteral("running_woman"), e({0x1F3C3, 0x200D, 0x2640, 0xFE0F})); // 🏃‍♀️
    m.insert(QStringLiteral("rwanda"), e({0x1F1F7, 0x1F1FC})); // 🇷🇼
    m.insert(QStringLiteral("sa"), e({0x1F202})); // 🈂
    m.insert(QStringLiteral("safety_pin"), e({0x1F9F7})); // 🧷
    m.insert(QStringLiteral("safety_vest"), e({0x1F9BA})); // 🦺
    m.insert(QStringLiteral("sagittarius"), e({0x2650})); // ♐
    m.insert(QStringLiteral("sailboat"), e({0x26F5})); // ⛵
    m.insert(QStringLiteral("sake"), e({0x1F376})); // 🍶
    m.insert(QStringLiteral("salt"), e({0x1F9C2})); // 🧂
    m.insert(QStringLiteral("saluting_face"), e({0x1FAE1})); // 🫡
    m.insert(QStringLiteral("samoa"), e({0x1F1FC, 0x1F1F8})); // 🇼🇸
    m.insert(QStringLiteral("san_marino"), e({0x1F1F8, 0x1F1F2})); // 🇸🇲
    m.insert(QStringLiteral("sandal"), e({0x1F461})); // 👡
    m.insert(QStringLiteral("sandwich"), e({0x1F96A})); // 🥪
    m.insert(QStringLiteral("santa"), e({0x1F385})); // 🎅
    m.insert(QStringLiteral("sao_tome_principe"), e({0x1F1F8, 0x1F1F9})); // 🇸🇹
    m.insert(QStringLiteral("sari"), e({0x1F97B})); // 🥻
    m.insert(QStringLiteral("sassy_man"), e({0x1F481, 0x200D, 0x2642, 0xFE0F})); // 💁‍♂️
    m.insert(QStringLiteral("sassy_woman"), e({0x1F481, 0x200D, 0x2640, 0xFE0F})); // 💁‍♀️
    m.insert(QStringLiteral("satellite"), e({0x1F4E1})); // 📡
    m.insert(QStringLiteral("satisfied"), e({0x1F606})); // 😆
    m.insert(QStringLiteral("saudi_arabia"), e({0x1F1F8, 0x1F1E6})); // 🇸🇦
    m.insert(QStringLiteral("sauna_man"), e({0x1F9D6, 0x200D, 0x2642, 0xFE0F})); // 🧖‍♂️
    m.insert(QStringLiteral("sauna_person"), e({0x1F9D6})); // 🧖
    m.insert(QStringLiteral("sauna_woman"), e({0x1F9D6, 0x200D, 0x2640, 0xFE0F})); // 🧖‍♀️
    m.insert(QStringLiteral("sauropod"), e({0x1F995})); // 🦕
    m.insert(QStringLiteral("saxophone"), e({0x1F3B7})); // 🎷
    m.insert(QStringLiteral("scarf"), e({0x1F9E3})); // 🧣
    m.insert(QStringLiteral("school"), e({0x1F3EB})); // 🏫
    m.insert(QStringLiteral("school_satchel"), e({0x1F392})); // 🎒
    m.insert(QStringLiteral("scientist"), e({0x1F9D1, 0x200D, 0x1F52C})); // 🧑‍🔬
    m.insert(QStringLiteral("scissors"), e({0x2702})); // ✂
    m.insert(QStringLiteral("scorpion"), e({0x1F982})); // 🦂
    m.insert(QStringLiteral("scorpius"), e({0x264F})); // ♏
    m.insert(QStringLiteral("scotland"), e({0x1F3F4, 0xE0067, 0xE0062, 0xE0073, 0xE0063, 0xE0074, 0xE007F})); // 🏴󠁧󠁢󠁳󠁣󠁴󠁿
    m.insert(QStringLiteral("scream"), e({0x1F631})); // 😱
    m.insert(QStringLiteral("scream_cat"), e({0x1F640})); // 🙀
    m.insert(QStringLiteral("screwdriver"), e({0x1FA9B})); // 🪛
    m.insert(QStringLiteral("scroll"), e({0x1F4DC})); // 📜
    m.insert(QStringLiteral("seal"), e({0x1F9AD})); // 🦭
    m.insert(QStringLiteral("seat"), e({0x1F4BA})); // 💺
    m.insert(QStringLiteral("secret"), e({0x3299})); // ㊙
    m.insert(QStringLiteral("see_no_evil"), e({0x1F648})); // 🙈
    m.insert(QStringLiteral("seedling"), e({0x1F331})); // 🌱
    m.insert(QStringLiteral("selfie"), e({0x1F933})); // 🤳
    m.insert(QStringLiteral("senegal"), e({0x1F1F8, 0x1F1F3})); // 🇸🇳
    m.insert(QStringLiteral("serbia"), e({0x1F1F7, 0x1F1F8})); // 🇷🇸
    m.insert(QStringLiteral("service_dog"), e({0x1F415, 0x200D, 0x1F9BA})); // 🐕‍🦺
    m.insert(QStringLiteral("seven"), e({0x37, 0xFE0F, 0x20E3})); // 7️⃣
    m.insert(QStringLiteral("sewing_needle"), e({0x1FAA1})); // 🪡
    m.insert(QStringLiteral("seychelles"), e({0x1F1F8, 0x1F1E8})); // 🇸🇨
    m.insert(QStringLiteral("shaking_face"), e({0x1FAE8})); // 🫨
    m.insert(QStringLiteral("shallow_pan_of_food"), e({0x1F958})); // 🥘
    m.insert(QStringLiteral("shamrock"), e({0x2618})); // ☘
    m.insert(QStringLiteral("shark"), e({0x1F988})); // 🦈
    m.insert(QStringLiteral("shaved_ice"), e({0x1F367})); // 🍧
    m.insert(QStringLiteral("sheep"), e({0x1F411})); // 🐑
    m.insert(QStringLiteral("shell"), e({0x1F41A})); // 🐚
    m.insert(QStringLiteral("shield"), e({0x1F6E1})); // 🛡
    m.insert(QStringLiteral("shinto_shrine"), e({0x26E9})); // ⛩
    m.insert(QStringLiteral("ship"), e({0x1F6A2})); // 🚢
    m.insert(QStringLiteral("shirt"), e({0x1F455})); // 👕
    m.insert(QStringLiteral("shit"), e({0x1F4A9})); // 💩
    m.insert(QStringLiteral("shoe"), e({0x1F45E})); // 👞
    m.insert(QStringLiteral("shopping"), e({0x1F6CD})); // 🛍
    m.insert(QStringLiteral("shopping_cart"), e({0x1F6D2})); // 🛒
    m.insert(QStringLiteral("shorts"), e({0x1FA73})); // 🩳
    m.insert(QStringLiteral("shower"), e({0x1F6BF})); // 🚿
    m.insert(QStringLiteral("shrimp"), e({0x1F990})); // 🦐
    m.insert(QStringLiteral("shrug"), e({0x1F937})); // 🤷
    m.insert(QStringLiteral("shushing_face"), e({0x1F92B})); // 🤫
    m.insert(QStringLiteral("sierra_leone"), e({0x1F1F8, 0x1F1F1})); // 🇸🇱
    m.insert(QStringLiteral("signal_strength"), e({0x1F4F6})); // 📶
    m.insert(QStringLiteral("singapore"), e({0x1F1F8, 0x1F1EC})); // 🇸🇬
    m.insert(QStringLiteral("singer"), e({0x1F9D1, 0x200D, 0x1F3A4})); // 🧑‍🎤
    m.insert(QStringLiteral("sint_maarten"), e({0x1F1F8, 0x1F1FD})); // 🇸🇽
    m.insert(QStringLiteral("six"), e({0x36, 0xFE0F, 0x20E3})); // 6️⃣
    m.insert(QStringLiteral("six_pointed_star"), e({0x1F52F})); // 🔯
    m.insert(QStringLiteral("skateboard"), e({0x1F6F9})); // 🛹
    m.insert(QStringLiteral("ski"), e({0x1F3BF})); // 🎿
    m.insert(QStringLiteral("skier"), e({0x26F7})); // ⛷
    m.insert(QStringLiteral("skull"), e({0x1F480})); // 💀
    m.insert(QStringLiteral("skull_and_crossbones"), e({0x2620})); // ☠
    m.insert(QStringLiteral("skunk"), e({0x1F9A8})); // 🦨
    m.insert(QStringLiteral("sled"), e({0x1F6F7})); // 🛷
    m.insert(QStringLiteral("sleeping"), e({0x1F634})); // 😴
    m.insert(QStringLiteral("sleeping_bed"), e({0x1F6CC})); // 🛌
    m.insert(QStringLiteral("sleepy"), e({0x1F62A})); // 😪
    m.insert(QStringLiteral("slightly_frowning_face"), e({0x1F641})); // 🙁
    m.insert(QStringLiteral("slightly_smiling_face"), e({0x1F642})); // 🙂
    m.insert(QStringLiteral("slot_machine"), e({0x1F3B0})); // 🎰
    m.insert(QStringLiteral("sloth"), e({0x1F9A5})); // 🦥
    m.insert(QStringLiteral("slovakia"), e({0x1F1F8, 0x1F1F0})); // 🇸🇰
    m.insert(QStringLiteral("slovenia"), e({0x1F1F8, 0x1F1EE})); // 🇸🇮
    m.insert(QStringLiteral("small_airplane"), e({0x1F6E9})); // 🛩
    m.insert(QStringLiteral("small_blue_diamond"), e({0x1F539})); // 🔹
    m.insert(QStringLiteral("small_orange_diamond"), e({0x1F538})); // 🔸
    m.insert(QStringLiteral("small_red_triangle"), e({0x1F53A})); // 🔺
    m.insert(QStringLiteral("small_red_triangle_down"), e({0x1F53B})); // 🔻
    m.insert(QStringLiteral("smile"), e({0x1F604})); // 😄
    m.insert(QStringLiteral("smile_cat"), e({0x1F638})); // 😸
    m.insert(QStringLiteral("smiley"), e({0x1F603})); // 😃
    m.insert(QStringLiteral("smiley_cat"), e({0x1F63A})); // 😺
    m.insert(QStringLiteral("smiling_face_with_tear"), e({0x1F972})); // 🥲
    m.insert(QStringLiteral("smiling_face_with_three_hearts"), e({0x1F970})); // 🥰
    m.insert(QStringLiteral("smiling_imp"), e({0x1F608})); // 😈
    m.insert(QStringLiteral("smirk"), e({0x1F60F})); // 😏
    m.insert(QStringLiteral("smirk_cat"), e({0x1F63C})); // 😼
    m.insert(QStringLiteral("smoking"), e({0x1F6AC})); // 🚬
    m.insert(QStringLiteral("snail"), e({0x1F40C})); // 🐌
    m.insert(QStringLiteral("snake"), e({0x1F40D})); // 🐍
    m.insert(QStringLiteral("sneezing_face"), e({0x1F927})); // 🤧
    m.insert(QStringLiteral("snowboarder"), e({0x1F3C2})); // 🏂
    m.insert(QStringLiteral("snowflake"), e({0x2744})); // ❄
    m.insert(QStringLiteral("snowman"), e({0x26C4})); // ⛄
    m.insert(QStringLiteral("snowman_with_snow"), e({0x2603})); // ☃
    m.insert(QStringLiteral("soap"), e({0x1F9FC})); // 🧼
    m.insert(QStringLiteral("sob"), e({0x1F62D})); // 😭
    m.insert(QStringLiteral("soccer"), e({0x26BD})); // ⚽
    m.insert(QStringLiteral("socks"), e({0x1F9E6})); // 🧦
    m.insert(QStringLiteral("softball"), e({0x1F94E})); // 🥎
    m.insert(QStringLiteral("solomon_islands"), e({0x1F1F8, 0x1F1E7})); // 🇸🇧
    m.insert(QStringLiteral("somalia"), e({0x1F1F8, 0x1F1F4})); // 🇸🇴
    m.insert(QStringLiteral("soon"), e({0x1F51C})); // 🔜
    m.insert(QStringLiteral("sos"), e({0x1F198})); // 🆘
    m.insert(QStringLiteral("sound"), e({0x1F509})); // 🔉
    m.insert(QStringLiteral("south_africa"), e({0x1F1FF, 0x1F1E6})); // 🇿🇦
    m.insert(QStringLiteral("south_georgia_south_sandwich_islands"), e({0x1F1EC, 0x1F1F8})); // 🇬🇸
    m.insert(QStringLiteral("south_sudan"), e({0x1F1F8, 0x1F1F8})); // 🇸🇸
    m.insert(QStringLiteral("space_invader"), e({0x1F47E})); // 👾
    m.insert(QStringLiteral("spades"), e({0x2660})); // ♠
    m.insert(QStringLiteral("spaghetti"), e({0x1F35D})); // 🍝
    m.insert(QStringLiteral("sparkle"), e({0x2747})); // ❇
    m.insert(QStringLiteral("sparkler"), e({0x1F387})); // 🎇
    m.insert(QStringLiteral("sparkles"), e({0x2728})); // ✨
    m.insert(QStringLiteral("sparkling_heart"), e({0x1F496})); // 💖
    m.insert(QStringLiteral("speak_no_evil"), e({0x1F64A})); // 🙊
    m.insert(QStringLiteral("speaker"), e({0x1F508})); // 🔈
    m.insert(QStringLiteral("speaking_head"), e({0x1F5E3})); // 🗣
    m.insert(QStringLiteral("speech_balloon"), e({0x1F4AC})); // 💬
    m.insert(QStringLiteral("speedboat"), e({0x1F6A4})); // 🚤
    m.insert(QStringLiteral("spider"), e({0x1F577})); // 🕷
    m.insert(QStringLiteral("spider_web"), e({0x1F578})); // 🕸
    m.insert(QStringLiteral("spiral_calendar"), e({0x1F5D3})); // 🗓
    m.insert(QStringLiteral("spiral_notepad"), e({0x1F5D2})); // 🗒
    m.insert(QStringLiteral("sponge"), e({0x1F9FD})); // 🧽
    m.insert(QStringLiteral("spoon"), e({0x1F944})); // 🥄
    m.insert(QStringLiteral("squid"), e({0x1F991})); // 🦑
    m.insert(QStringLiteral("sri_lanka"), e({0x1F1F1, 0x1F1F0})); // 🇱🇰
    m.insert(QStringLiteral("st_barthelemy"), e({0x1F1E7, 0x1F1F1})); // 🇧🇱
    m.insert(QStringLiteral("st_helena"), e({0x1F1F8, 0x1F1ED})); // 🇸🇭
    m.insert(QStringLiteral("st_kitts_nevis"), e({0x1F1F0, 0x1F1F3})); // 🇰🇳
    m.insert(QStringLiteral("st_lucia"), e({0x1F1F1, 0x1F1E8})); // 🇱🇨
    m.insert(QStringLiteral("st_martin"), e({0x1F1F2, 0x1F1EB})); // 🇲🇫
    m.insert(QStringLiteral("st_pierre_miquelon"), e({0x1F1F5, 0x1F1F2})); // 🇵🇲
    m.insert(QStringLiteral("st_vincent_grenadines"), e({0x1F1FB, 0x1F1E8})); // 🇻🇨
    m.insert(QStringLiteral("stadium"), e({0x1F3DF})); // 🏟
    m.insert(QStringLiteral("standing_man"), e({0x1F9CD, 0x200D, 0x2642, 0xFE0F})); // 🧍‍♂️
    m.insert(QStringLiteral("standing_person"), e({0x1F9CD})); // 🧍
    m.insert(QStringLiteral("standing_woman"), e({0x1F9CD, 0x200D, 0x2640, 0xFE0F})); // 🧍‍♀️
    m.insert(QStringLiteral("star"), e({0x2B50})); // ⭐
    m.insert(QStringLiteral("star2"), e({0x1F31F})); // 🌟
    m.insert(QStringLiteral("star_and_crescent"), e({0x262A})); // ☪
    m.insert(QStringLiteral("star_of_david"), e({0x2721})); // ✡
    m.insert(QStringLiteral("star_struck"), e({0x1F929})); // 🤩
    m.insert(QStringLiteral("stars"), e({0x1F320})); // 🌠
    m.insert(QStringLiteral("station"), e({0x1F689})); // 🚉
    m.insert(QStringLiteral("statue_of_liberty"), e({0x1F5FD})); // 🗽
    m.insert(QStringLiteral("steam_locomotive"), e({0x1F682})); // 🚂
    m.insert(QStringLiteral("stethoscope"), e({0x1FA7A})); // 🩺
    m.insert(QStringLiteral("stew"), e({0x1F372})); // 🍲
    m.insert(QStringLiteral("stop_button"), e({0x23F9})); // ⏹
    m.insert(QStringLiteral("stop_sign"), e({0x1F6D1})); // 🛑
    m.insert(QStringLiteral("stopwatch"), e({0x23F1})); // ⏱
    m.insert(QStringLiteral("straight_ruler"), e({0x1F4CF})); // 📏
    m.insert(QStringLiteral("strawberry"), e({0x1F353})); // 🍓
    m.insert(QStringLiteral("stuck_out_tongue"), e({0x1F61B})); // 😛
    m.insert(QStringLiteral("stuck_out_tongue_closed_eyes"), e({0x1F61D})); // 😝
    m.insert(QStringLiteral("stuck_out_tongue_winking_eye"), e({0x1F61C})); // 😜
    m.insert(QStringLiteral("student"), e({0x1F9D1, 0x200D, 0x1F393})); // 🧑‍🎓
    m.insert(QStringLiteral("studio_microphone"), e({0x1F399})); // 🎙
    m.insert(QStringLiteral("stuffed_flatbread"), e({0x1F959})); // 🥙
    m.insert(QStringLiteral("sudan"), e({0x1F1F8, 0x1F1E9})); // 🇸🇩
    m.insert(QStringLiteral("sun_behind_large_cloud"), e({0x1F325})); // 🌥
    m.insert(QStringLiteral("sun_behind_rain_cloud"), e({0x1F326})); // 🌦
    m.insert(QStringLiteral("sun_behind_small_cloud"), e({0x1F324})); // 🌤
    m.insert(QStringLiteral("sun_with_face"), e({0x1F31E})); // 🌞
    m.insert(QStringLiteral("sunflower"), e({0x1F33B})); // 🌻
    m.insert(QStringLiteral("sunglasses"), e({0x1F60E})); // 😎
    m.insert(QStringLiteral("sunny"), e({0x2600})); // ☀
    m.insert(QStringLiteral("sunrise"), e({0x1F305})); // 🌅
    m.insert(QStringLiteral("sunrise_over_mountains"), e({0x1F304})); // 🌄
    m.insert(QStringLiteral("superhero"), e({0x1F9B8})); // 🦸
    m.insert(QStringLiteral("superhero_man"), e({0x1F9B8, 0x200D, 0x2642, 0xFE0F})); // 🦸‍♂️
    m.insert(QStringLiteral("superhero_woman"), e({0x1F9B8, 0x200D, 0x2640, 0xFE0F})); // 🦸‍♀️
    m.insert(QStringLiteral("supervillain"), e({0x1F9B9})); // 🦹
    m.insert(QStringLiteral("supervillain_man"), e({0x1F9B9, 0x200D, 0x2642, 0xFE0F})); // 🦹‍♂️
    m.insert(QStringLiteral("supervillain_woman"), e({0x1F9B9, 0x200D, 0x2640, 0xFE0F})); // 🦹‍♀️
    m.insert(QStringLiteral("surfer"), e({0x1F3C4})); // 🏄
    m.insert(QStringLiteral("surfing_man"), e({0x1F3C4, 0x200D, 0x2642, 0xFE0F})); // 🏄‍♂️
    m.insert(QStringLiteral("surfing_woman"), e({0x1F3C4, 0x200D, 0x2640, 0xFE0F})); // 🏄‍♀️
    m.insert(QStringLiteral("suriname"), e({0x1F1F8, 0x1F1F7})); // 🇸🇷
    m.insert(QStringLiteral("sushi"), e({0x1F363})); // 🍣
    m.insert(QStringLiteral("suspension_railway"), e({0x1F69F})); // 🚟
    m.insert(QStringLiteral("svalbard_jan_mayen"), e({0x1F1F8, 0x1F1EF})); // 🇸🇯
    m.insert(QStringLiteral("swan"), e({0x1F9A2})); // 🦢
    m.insert(QStringLiteral("swaziland"), e({0x1F1F8, 0x1F1FF})); // 🇸🇿
    m.insert(QStringLiteral("sweat"), e({0x1F613})); // 😓
    m.insert(QStringLiteral("sweat_drops"), e({0x1F4A6})); // 💦
    m.insert(QStringLiteral("sweat_smile"), e({0x1F605})); // 😅
    m.insert(QStringLiteral("sweden"), e({0x1F1F8, 0x1F1EA})); // 🇸🇪
    m.insert(QStringLiteral("sweet_potato"), e({0x1F360})); // 🍠
    m.insert(QStringLiteral("swim_brief"), e({0x1FA72})); // 🩲
    m.insert(QStringLiteral("swimmer"), e({0x1F3CA})); // 🏊
    m.insert(QStringLiteral("swimming_man"), e({0x1F3CA, 0x200D, 0x2642, 0xFE0F})); // 🏊‍♂️
    m.insert(QStringLiteral("swimming_woman"), e({0x1F3CA, 0x200D, 0x2640, 0xFE0F})); // 🏊‍♀️
    m.insert(QStringLiteral("switzerland"), e({0x1F1E8, 0x1F1ED})); // 🇨🇭
    m.insert(QStringLiteral("symbols"), e({0x1F523})); // 🔣
    m.insert(QStringLiteral("synagogue"), e({0x1F54D})); // 🕍
    m.insert(QStringLiteral("syria"), e({0x1F1F8, 0x1F1FE})); // 🇸🇾
    m.insert(QStringLiteral("syringe"), e({0x1F489})); // 💉
    m.insert(QStringLiteral("t-rex"), e({0x1F996})); // 🦖
    m.insert(QStringLiteral("taco"), e({0x1F32E})); // 🌮
    m.insert(QStringLiteral("tada"), e({0x1F389})); // 🎉
    m.insert(QStringLiteral("taiwan"), e({0x1F1F9, 0x1F1FC})); // 🇹🇼
    m.insert(QStringLiteral("tajikistan"), e({0x1F1F9, 0x1F1EF})); // 🇹🇯
    m.insert(QStringLiteral("takeout_box"), e({0x1F961})); // 🥡
    m.insert(QStringLiteral("tamale"), e({0x1FAD4})); // 🫔
    m.insert(QStringLiteral("tanabata_tree"), e({0x1F38B})); // 🎋
    m.insert(QStringLiteral("tangerine"), e({0x1F34A})); // 🍊
    m.insert(QStringLiteral("tanzania"), e({0x1F1F9, 0x1F1FF})); // 🇹🇿
    m.insert(QStringLiteral("taurus"), e({0x2649})); // ♉
    m.insert(QStringLiteral("taxi"), e({0x1F695})); // 🚕
    m.insert(QStringLiteral("tea"), e({0x1F375})); // 🍵
    m.insert(QStringLiteral("teacher"), e({0x1F9D1, 0x200D, 0x1F3EB})); // 🧑‍🏫
    m.insert(QStringLiteral("teapot"), e({0x1FAD6})); // 🫖
    m.insert(QStringLiteral("technologist"), e({0x1F9D1, 0x200D, 0x1F4BB})); // 🧑‍💻
    m.insert(QStringLiteral("teddy_bear"), e({0x1F9F8})); // 🧸
    m.insert(QStringLiteral("telephone"), e({0x260E})); // ☎
    m.insert(QStringLiteral("telephone_receiver"), e({0x1F4DE})); // 📞
    m.insert(QStringLiteral("telescope"), e({0x1F52D})); // 🔭
    m.insert(QStringLiteral("tennis"), e({0x1F3BE})); // 🎾
    m.insert(QStringLiteral("tent"), e({0x26FA})); // ⛺
    m.insert(QStringLiteral("test_tube"), e({0x1F9EA})); // 🧪
    m.insert(QStringLiteral("thailand"), e({0x1F1F9, 0x1F1ED})); // 🇹🇭
    m.insert(QStringLiteral("thermometer"), e({0x1F321})); // 🌡
    m.insert(QStringLiteral("thinking"), e({0x1F914})); // 🤔
    m.insert(QStringLiteral("thong_sandal"), e({0x1FA74})); // 🩴
    m.insert(QStringLiteral("thought_balloon"), e({0x1F4AD})); // 💭
    m.insert(QStringLiteral("thread"), e({0x1F9F5})); // 🧵
    m.insert(QStringLiteral("three"), e({0x33, 0xFE0F, 0x20E3})); // 3️⃣
    m.insert(QStringLiteral("thumbsdown"), e({0x1F44E})); // 👎
    m.insert(QStringLiteral("thumbsup"), e({0x1F44D})); // 👍
    m.insert(QStringLiteral("ticket"), e({0x1F3AB})); // 🎫
    m.insert(QStringLiteral("tickets"), e({0x1F39F})); // 🎟
    m.insert(QStringLiteral("tiger"), e({0x1F42F})); // 🐯
    m.insert(QStringLiteral("tiger2"), e({0x1F405})); // 🐅
    m.insert(QStringLiteral("timer_clock"), e({0x23F2})); // ⏲
    m.insert(QStringLiteral("timor_leste"), e({0x1F1F9, 0x1F1F1})); // 🇹🇱
    m.insert(QStringLiteral("tipping_hand_man"), e({0x1F481, 0x200D, 0x2642, 0xFE0F})); // 💁‍♂️
    m.insert(QStringLiteral("tipping_hand_person"), e({0x1F481})); // 💁
    m.insert(QStringLiteral("tipping_hand_woman"), e({0x1F481, 0x200D, 0x2640, 0xFE0F})); // 💁‍♀️
    m.insert(QStringLiteral("tired_face"), e({0x1F62B})); // 😫
    m.insert(QStringLiteral("tm"), e({0x2122})); // ™
    m.insert(QStringLiteral("togo"), e({0x1F1F9, 0x1F1EC})); // 🇹🇬
    m.insert(QStringLiteral("toilet"), e({0x1F6BD})); // 🚽
    m.insert(QStringLiteral("tokelau"), e({0x1F1F9, 0x1F1F0})); // 🇹🇰
    m.insert(QStringLiteral("tokyo_tower"), e({0x1F5FC})); // 🗼
    m.insert(QStringLiteral("tomato"), e({0x1F345})); // 🍅
    m.insert(QStringLiteral("tonga"), e({0x1F1F9, 0x1F1F4})); // 🇹🇴
    m.insert(QStringLiteral("tongue"), e({0x1F445})); // 👅
    m.insert(QStringLiteral("toolbox"), e({0x1F9F0})); // 🧰
    m.insert(QStringLiteral("tooth"), e({0x1F9B7})); // 🦷
    m.insert(QStringLiteral("toothbrush"), e({0x1FAA5})); // 🪥
    m.insert(QStringLiteral("top"), e({0x1F51D})); // 🔝
    m.insert(QStringLiteral("tophat"), e({0x1F3A9})); // 🎩
    m.insert(QStringLiteral("tornado"), e({0x1F32A})); // 🌪
    m.insert(QStringLiteral("tr"), e({0x1F1F9, 0x1F1F7})); // 🇹🇷
    m.insert(QStringLiteral("trackball"), e({0x1F5B2})); // 🖲
    m.insert(QStringLiteral("tractor"), e({0x1F69C})); // 🚜
    m.insert(QStringLiteral("traffic_light"), e({0x1F6A5})); // 🚥
    m.insert(QStringLiteral("train"), e({0x1F68B})); // 🚋
    m.insert(QStringLiteral("train2"), e({0x1F686})); // 🚆
    m.insert(QStringLiteral("tram"), e({0x1F68A})); // 🚊
    m.insert(QStringLiteral("transgender_flag"), e({0x1F3F3, 0xFE0F, 0x200D, 0x26A7, 0xFE0F})); // 🏳️‍⚧️
    m.insert(QStringLiteral("transgender_symbol"), e({0x26A7})); // ⚧
    m.insert(QStringLiteral("triangular_flag_on_post"), e({0x1F6A9})); // 🚩
    m.insert(QStringLiteral("triangular_ruler"), e({0x1F4D0})); // 📐
    m.insert(QStringLiteral("trident"), e({0x1F531})); // 🔱
    m.insert(QStringLiteral("trinidad_tobago"), e({0x1F1F9, 0x1F1F9})); // 🇹🇹
    m.insert(QStringLiteral("tristan_da_cunha"), e({0x1F1F9, 0x1F1E6})); // 🇹🇦
    m.insert(QStringLiteral("triumph"), e({0x1F624})); // 😤
    m.insert(QStringLiteral("troll"), e({0x1F9CC})); // 🧌
    m.insert(QStringLiteral("trolleybus"), e({0x1F68E})); // 🚎
    m.insert(QStringLiteral("trophy"), e({0x1F3C6})); // 🏆
    m.insert(QStringLiteral("tropical_drink"), e({0x1F379})); // 🍹
    m.insert(QStringLiteral("tropical_fish"), e({0x1F420})); // 🐠
    m.insert(QStringLiteral("truck"), e({0x1F69A})); // 🚚
    m.insert(QStringLiteral("trumpet"), e({0x1F3BA})); // 🎺
    m.insert(QStringLiteral("tshirt"), e({0x1F455})); // 👕
    m.insert(QStringLiteral("tulip"), e({0x1F337})); // 🌷
    m.insert(QStringLiteral("tumbler_glass"), e({0x1F943})); // 🥃
    m.insert(QStringLiteral("tunisia"), e({0x1F1F9, 0x1F1F3})); // 🇹🇳
    m.insert(QStringLiteral("turkey"), e({0x1F983})); // 🦃
    m.insert(QStringLiteral("turkmenistan"), e({0x1F1F9, 0x1F1F2})); // 🇹🇲
    m.insert(QStringLiteral("turks_caicos_islands"), e({0x1F1F9, 0x1F1E8})); // 🇹🇨
    m.insert(QStringLiteral("turtle"), e({0x1F422})); // 🐢
    m.insert(QStringLiteral("tuvalu"), e({0x1F1F9, 0x1F1FB})); // 🇹🇻
    m.insert(QStringLiteral("tv"), e({0x1F4FA})); // 📺
    m.insert(QStringLiteral("twisted_rightwards_arrows"), e({0x1F500})); // 🔀
    m.insert(QStringLiteral("two"), e({0x32, 0xFE0F, 0x20E3})); // 2️⃣
    m.insert(QStringLiteral("two_hearts"), e({0x1F495})); // 💕
    m.insert(QStringLiteral("two_men_holding_hands"), e({0x1F46C})); // 👬
    m.insert(QStringLiteral("two_women_holding_hands"), e({0x1F46D})); // 👭
    m.insert(QStringLiteral("u5272"), e({0x1F239})); // 🈹
    m.insert(QStringLiteral("u5408"), e({0x1F234})); // 🈴
    m.insert(QStringLiteral("u55b6"), e({0x1F23A})); // 🈺
    m.insert(QStringLiteral("u6307"), e({0x1F22F})); // 🈯
    m.insert(QStringLiteral("u6708"), e({0x1F237})); // 🈷
    m.insert(QStringLiteral("u6709"), e({0x1F236})); // 🈶
    m.insert(QStringLiteral("u6e80"), e({0x1F235})); // 🈵
    m.insert(QStringLiteral("u7121"), e({0x1F21A})); // 🈚
    m.insert(QStringLiteral("u7533"), e({0x1F238})); // 🈸
    m.insert(QStringLiteral("u7981"), e({0x1F232})); // 🈲
    m.insert(QStringLiteral("u7a7a"), e({0x1F233})); // 🈳
    m.insert(QStringLiteral("uganda"), e({0x1F1FA, 0x1F1EC})); // 🇺🇬
    m.insert(QStringLiteral("uk"), e({0x1F1EC, 0x1F1E7})); // 🇬🇧
    m.insert(QStringLiteral("ukraine"), e({0x1F1FA, 0x1F1E6})); // 🇺🇦
    m.insert(QStringLiteral("umbrella"), e({0x2614})); // ☔
    m.insert(QStringLiteral("unamused"), e({0x1F612})); // 😒
    m.insert(QStringLiteral("underage"), e({0x1F51E})); // 🔞
    m.insert(QStringLiteral("unicorn"), e({0x1F984})); // 🦄
    m.insert(QStringLiteral("united_arab_emirates"), e({0x1F1E6, 0x1F1EA})); // 🇦🇪
    m.insert(QStringLiteral("united_nations"), e({0x1F1FA, 0x1F1F3})); // 🇺🇳
    m.insert(QStringLiteral("unlock"), e({0x1F513})); // 🔓
    m.insert(QStringLiteral("up"), e({0x1F199})); // 🆙
    m.insert(QStringLiteral("upside_down_face"), e({0x1F643})); // 🙃
    m.insert(QStringLiteral("uruguay"), e({0x1F1FA, 0x1F1FE})); // 🇺🇾
    m.insert(QStringLiteral("us"), e({0x1F1FA, 0x1F1F8})); // 🇺🇸
    m.insert(QStringLiteral("us_outlying_islands"), e({0x1F1FA, 0x1F1F2})); // 🇺🇲
    m.insert(QStringLiteral("us_virgin_islands"), e({0x1F1FB, 0x1F1EE})); // 🇻🇮
    m.insert(QStringLiteral("uzbekistan"), e({0x1F1FA, 0x1F1FF})); // 🇺🇿
    m.insert(QStringLiteral("v"), e({0x270C})); // ✌
    m.insert(QStringLiteral("vampire"), e({0x1F9DB})); // 🧛
    m.insert(QStringLiteral("vampire_man"), e({0x1F9DB, 0x200D, 0x2642, 0xFE0F})); // 🧛‍♂️
    m.insert(QStringLiteral("vampire_woman"), e({0x1F9DB, 0x200D, 0x2640, 0xFE0F})); // 🧛‍♀️
    m.insert(QStringLiteral("vanuatu"), e({0x1F1FB, 0x1F1FA})); // 🇻🇺
    m.insert(QStringLiteral("vatican_city"), e({0x1F1FB, 0x1F1E6})); // 🇻🇦
    m.insert(QStringLiteral("venezuela"), e({0x1F1FB, 0x1F1EA})); // 🇻🇪
    m.insert(QStringLiteral("vertical_traffic_light"), e({0x1F6A6})); // 🚦
    m.insert(QStringLiteral("vhs"), e({0x1F4FC})); // 📼
    m.insert(QStringLiteral("vibration_mode"), e({0x1F4F3})); // 📳
    m.insert(QStringLiteral("video_camera"), e({0x1F4F9})); // 📹
    m.insert(QStringLiteral("video_game"), e({0x1F3AE})); // 🎮
    m.insert(QStringLiteral("vietnam"), e({0x1F1FB, 0x1F1F3})); // 🇻🇳
    m.insert(QStringLiteral("violin"), e({0x1F3BB})); // 🎻
    m.insert(QStringLiteral("virgo"), e({0x264D})); // ♍
    m.insert(QStringLiteral("volcano"), e({0x1F30B})); // 🌋
    m.insert(QStringLiteral("volleyball"), e({0x1F3D0})); // 🏐
    m.insert(QStringLiteral("vomiting_face"), e({0x1F92E})); // 🤮
    m.insert(QStringLiteral("vs"), e({0x1F19A})); // 🆚
    m.insert(QStringLiteral("vulcan_salute"), e({0x1F596})); // 🖖
    m.insert(QStringLiteral("waffle"), e({0x1F9C7})); // 🧇
    m.insert(QStringLiteral("wales"), e({0x1F3F4, 0xE0067, 0xE0062, 0xE0077, 0xE006C, 0xE0073, 0xE007F})); // 🏴󠁧󠁢󠁷󠁬󠁳󠁿
    m.insert(QStringLiteral("walking"), e({0x1F6B6})); // 🚶
    m.insert(QStringLiteral("walking_man"), e({0x1F6B6, 0x200D, 0x2642, 0xFE0F})); // 🚶‍♂️
    m.insert(QStringLiteral("walking_woman"), e({0x1F6B6, 0x200D, 0x2640, 0xFE0F})); // 🚶‍♀️
    m.insert(QStringLiteral("wallis_futuna"), e({0x1F1FC, 0x1F1EB})); // 🇼🇫
    m.insert(QStringLiteral("waning_crescent_moon"), e({0x1F318})); // 🌘
    m.insert(QStringLiteral("waning_gibbous_moon"), e({0x1F316})); // 🌖
    m.insert(QStringLiteral("warning"), e({0x26A0})); // ⚠
    m.insert(QStringLiteral("wastebasket"), e({0x1F5D1})); // 🗑
    m.insert(QStringLiteral("watch"), e({0x231A})); // ⌚
    m.insert(QStringLiteral("water_buffalo"), e({0x1F403})); // 🐃
    m.insert(QStringLiteral("water_polo"), e({0x1F93D})); // 🤽
    m.insert(QStringLiteral("watermelon"), e({0x1F349})); // 🍉
    m.insert(QStringLiteral("wave"), e({0x1F44B})); // 👋
    m.insert(QStringLiteral("wavy_dash"), e({0x3030})); // 〰
    m.insert(QStringLiteral("waxing_crescent_moon"), e({0x1F312})); // 🌒
    m.insert(QStringLiteral("waxing_gibbous_moon"), e({0x1F314})); // 🌔
    m.insert(QStringLiteral("wc"), e({0x1F6BE})); // 🚾
    m.insert(QStringLiteral("weary"), e({0x1F629})); // 😩
    m.insert(QStringLiteral("wedding"), e({0x1F492})); // 💒
    m.insert(QStringLiteral("weight_lifting"), e({0x1F3CB})); // 🏋
    m.insert(QStringLiteral("weight_lifting_man"), e({0x1F3CB, 0xFE0F, 0x200D, 0x2642, 0xFE0F})); // 🏋️‍♂️
    m.insert(QStringLiteral("weight_lifting_woman"), e({0x1F3CB, 0xFE0F, 0x200D, 0x2640, 0xFE0F})); // 🏋️‍♀️
    m.insert(QStringLiteral("western_sahara"), e({0x1F1EA, 0x1F1ED})); // 🇪🇭
    m.insert(QStringLiteral("whale"), e({0x1F433})); // 🐳
    m.insert(QStringLiteral("whale2"), e({0x1F40B})); // 🐋
    m.insert(QStringLiteral("wheel"), e({0x1F6DE})); // 🛞
    m.insert(QStringLiteral("wheel_of_dharma"), e({0x2638})); // ☸
    m.insert(QStringLiteral("wheelchair"), e({0x267F})); // ♿
    m.insert(QStringLiteral("white_check_mark"), e({0x2705})); // ✅
    m.insert(QStringLiteral("white_circle"), e({0x26AA})); // ⚪
    m.insert(QStringLiteral("white_flag"), e({0x1F3F3})); // 🏳
    m.insert(QStringLiteral("white_flower"), e({0x1F4AE})); // 💮
    m.insert(QStringLiteral("white_haired_man"), e({0x1F468, 0x200D, 0x1F9B3})); // 👨‍🦳
    m.insert(QStringLiteral("white_haired_woman"), e({0x1F469, 0x200D, 0x1F9B3})); // 👩‍🦳
    m.insert(QStringLiteral("white_heart"), e({0x1F90D})); // 🤍
    m.insert(QStringLiteral("white_large_square"), e({0x2B1C})); // ⬜
    m.insert(QStringLiteral("white_medium_small_square"), e({0x25FD})); // ◽
    m.insert(QStringLiteral("white_medium_square"), e({0x25FB})); // ◻
    m.insert(QStringLiteral("white_small_square"), e({0x25AB})); // ▫
    m.insert(QStringLiteral("white_square_button"), e({0x1F533})); // 🔳
    m.insert(QStringLiteral("wilted_flower"), e({0x1F940})); // 🥀
    m.insert(QStringLiteral("wind_chime"), e({0x1F390})); // 🎐
    m.insert(QStringLiteral("wind_face"), e({0x1F32C})); // 🌬
    m.insert(QStringLiteral("window"), e({0x1FA9F})); // 🪟
    m.insert(QStringLiteral("wine_glass"), e({0x1F377})); // 🍷
    m.insert(QStringLiteral("wing"), e({0x1FABD})); // 🪽
    m.insert(QStringLiteral("wink"), e({0x1F609})); // 😉
    m.insert(QStringLiteral("wireless"), e({0x1F6DC})); // 🛜
    m.insert(QStringLiteral("wolf"), e({0x1F43A})); // 🐺
    m.insert(QStringLiteral("woman"), e({0x1F469})); // 👩
    m.insert(QStringLiteral("woman_artist"), e({0x1F469, 0x200D, 0x1F3A8})); // 👩‍🎨
    m.insert(QStringLiteral("woman_astronaut"), e({0x1F469, 0x200D, 0x1F680})); // 👩‍🚀
    m.insert(QStringLiteral("woman_beard"), e({0x1F9D4, 0x200D, 0x2640, 0xFE0F})); // 🧔‍♀️
    m.insert(QStringLiteral("woman_cartwheeling"), e({0x1F938, 0x200D, 0x2640, 0xFE0F})); // 🤸‍♀️
    m.insert(QStringLiteral("woman_cook"), e({0x1F469, 0x200D, 0x1F373})); // 👩‍🍳
    m.insert(QStringLiteral("woman_dancing"), e({0x1F483})); // 💃
    m.insert(QStringLiteral("woman_facepalming"), e({0x1F926, 0x200D, 0x2640, 0xFE0F})); // 🤦‍♀️
    m.insert(QStringLiteral("woman_factory_worker"), e({0x1F469, 0x200D, 0x1F3ED})); // 👩‍🏭
    m.insert(QStringLiteral("woman_farmer"), e({0x1F469, 0x200D, 0x1F33E})); // 👩‍🌾
    m.insert(QStringLiteral("woman_feeding_baby"), e({0x1F469, 0x200D, 0x1F37C})); // 👩‍🍼
    m.insert(QStringLiteral("woman_firefighter"), e({0x1F469, 0x200D, 0x1F692})); // 👩‍🚒
    m.insert(QStringLiteral("woman_health_worker"), e({0x1F469, 0x200D, 0x2695, 0xFE0F})); // 👩‍⚕️
    m.insert(QStringLiteral("woman_in_manual_wheelchair"), e({0x1F469, 0x200D, 0x1F9BD})); // 👩‍🦽
    m.insert(QStringLiteral("woman_in_motorized_wheelchair"), e({0x1F469, 0x200D, 0x1F9BC})); // 👩‍🦼
    m.insert(QStringLiteral("woman_in_tuxedo"), e({0x1F935, 0x200D, 0x2640, 0xFE0F})); // 🤵‍♀️
    m.insert(QStringLiteral("woman_judge"), e({0x1F469, 0x200D, 0x2696, 0xFE0F})); // 👩‍⚖️
    m.insert(QStringLiteral("woman_juggling"), e({0x1F939, 0x200D, 0x2640, 0xFE0F})); // 🤹‍♀️
    m.insert(QStringLiteral("woman_mechanic"), e({0x1F469, 0x200D, 0x1F527})); // 👩‍🔧
    m.insert(QStringLiteral("woman_office_worker"), e({0x1F469, 0x200D, 0x1F4BC})); // 👩‍💼
    m.insert(QStringLiteral("woman_pilot"), e({0x1F469, 0x200D, 0x2708, 0xFE0F})); // 👩‍✈️
    m.insert(QStringLiteral("woman_playing_handball"), e({0x1F93E, 0x200D, 0x2640, 0xFE0F})); // 🤾‍♀️
    m.insert(QStringLiteral("woman_playing_water_polo"), e({0x1F93D, 0x200D, 0x2640, 0xFE0F})); // 🤽‍♀️
    m.insert(QStringLiteral("woman_scientist"), e({0x1F469, 0x200D, 0x1F52C})); // 👩‍🔬
    m.insert(QStringLiteral("woman_shrugging"), e({0x1F937, 0x200D, 0x2640, 0xFE0F})); // 🤷‍♀️
    m.insert(QStringLiteral("woman_singer"), e({0x1F469, 0x200D, 0x1F3A4})); // 👩‍🎤
    m.insert(QStringLiteral("woman_student"), e({0x1F469, 0x200D, 0x1F393})); // 👩‍🎓
    m.insert(QStringLiteral("woman_teacher"), e({0x1F469, 0x200D, 0x1F3EB})); // 👩‍🏫
    m.insert(QStringLiteral("woman_technologist"), e({0x1F469, 0x200D, 0x1F4BB})); // 👩‍💻
    m.insert(QStringLiteral("woman_with_headscarf"), e({0x1F9D5})); // 🧕
    m.insert(QStringLiteral("woman_with_probing_cane"), e({0x1F469, 0x200D, 0x1F9AF})); // 👩‍🦯
    m.insert(QStringLiteral("woman_with_turban"), e({0x1F473, 0x200D, 0x2640, 0xFE0F})); // 👳‍♀️
    m.insert(QStringLiteral("woman_with_veil"), e({0x1F470, 0x200D, 0x2640, 0xFE0F})); // 👰‍♀️
    m.insert(QStringLiteral("womans_clothes"), e({0x1F45A})); // 👚
    m.insert(QStringLiteral("womans_hat"), e({0x1F452})); // 👒
    m.insert(QStringLiteral("women_wrestling"), e({0x1F93C, 0x200D, 0x2640, 0xFE0F})); // 🤼‍♀️
    m.insert(QStringLiteral("womens"), e({0x1F6BA})); // 🚺
    m.insert(QStringLiteral("wood"), e({0x1FAB5})); // 🪵
    m.insert(QStringLiteral("woozy_face"), e({0x1F974})); // 🥴
    m.insert(QStringLiteral("world_map"), e({0x1F5FA})); // 🗺
    m.insert(QStringLiteral("worm"), e({0x1FAB1})); // 🪱
    m.insert(QStringLiteral("worried"), e({0x1F61F})); // 😟
    m.insert(QStringLiteral("wrench"), e({0x1F527})); // 🔧
    m.insert(QStringLiteral("wrestling"), e({0x1F93C})); // 🤼
    m.insert(QStringLiteral("writing_hand"), e({0x270D})); // ✍
    m.insert(QStringLiteral("x"), e({0x274C})); // ❌
    m.insert(QStringLiteral("x_ray"), e({0x1FA7B})); // 🩻
    m.insert(QStringLiteral("yarn"), e({0x1F9F6})); // 🧶
    m.insert(QStringLiteral("yawning_face"), e({0x1F971})); // 🥱
    m.insert(QStringLiteral("yellow_circle"), e({0x1F7E1})); // 🟡
    m.insert(QStringLiteral("yellow_heart"), e({0x1F49B})); // 💛
    m.insert(QStringLiteral("yellow_square"), e({0x1F7E8})); // 🟨
    m.insert(QStringLiteral("yemen"), e({0x1F1FE, 0x1F1EA})); // 🇾🇪
    m.insert(QStringLiteral("yen"), e({0x1F4B4})); // 💴
    m.insert(QStringLiteral("yin_yang"), e({0x262F})); // ☯
    m.insert(QStringLiteral("yo_yo"), e({0x1FA80})); // 🪀
    m.insert(QStringLiteral("yum"), e({0x1F60B})); // 😋
    m.insert(QStringLiteral("zambia"), e({0x1F1FF, 0x1F1F2})); // 🇿🇲
    m.insert(QStringLiteral("zany_face"), e({0x1F92A})); // 🤪
    m.insert(QStringLiteral("zap"), e({0x26A1})); // ⚡
    m.insert(QStringLiteral("zebra"), e({0x1F993})); // 🦓
    m.insert(QStringLiteral("zero"), e({0x30, 0xFE0F, 0x20E3})); // 0️⃣
    m.insert(QStringLiteral("zimbabwe"), e({0x1F1FF, 0x1F1FC})); // 🇿🇼
    m.insert(QStringLiteral("zipper_mouth_face"), e({0x1F910})); // 🤐
    m.insert(QStringLiteral("zombie"), e({0x1F9DF})); // 🧟
    m.insert(QStringLiteral("zombie_man"), e({0x1F9DF, 0x200D, 0x2642, 0xFE0F})); // 🧟‍♂️
    m.insert(QStringLiteral("zombie_woman"), e({0x1F9DF, 0x200D, 0x2640, 0xFE0F})); // 🧟‍♀️
    m.insert(QStringLiteral("zzz"), e({0x1F4A4})); // 💤
    // Zusätzliche Aliase (nicht in der GitHub-Liste)
    m.insert(QStringLiteral("slight_smile"), e({0x1F642})); // 🙂
    m.insert(QStringLiteral("clown"), e({0x1F921})); // 🤡
    return m;
}

#endif // CHAT_EMOTE_SHORTCODE_TABLE_H
