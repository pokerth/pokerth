pragma Singleton
import QtQuick 6.5
import QtCore

// import "../resources"

QtObject {
    id: root

    // 0 = Hell (Light), 1 = Dunkel (Dark), 2 = Automatisch
    // Synced from pokerth.qml Component.onCompleted and GuiSettings DarkMode ComboBox
    property int darkMode: 1
    // The mode reported by the operating system (SettingsManager.systemDark, C++:
    // darkmode.h). A singleton cannot read the context property itself,
    // which is why the value – like darkMode – is set by pokerth.qml and updated on
    // system theme changes.
    property bool systemDark: true

    // 0=light → false, 1=dark → true, 2=automatic → follow the system.
    readonly property bool isDark: darkMode === 2 ? systemDark : darkMode !== 0

    // German and English first (the project language and the fallback), the rest
    // alphabetically by the English name. The order is purely for
    // display: reading and writing goes exclusively via code.
    readonly property var languages: [
            { langName: "Deutsch (Deutsch)", code: "de_DE"},
            { langName: "English (English)", code: "en_US"},
            { langName: "Afrikaans (Afrikaans)", code: "af_ZA"},
            { langName: "Bulgarian (Български)", code: "bg_BG"},
            { langName: "Catalan (Català)", code: "ca_ES"},
            { langName: "Chinese, Simplified (简体中文)", code: "zh_CN"},
            { langName: "Czech (Čeština)", code: "cs_CZ"},
            { langName: "Danish (Dansk)", code: "da_DK"},
            { langName: "Dutch (Nederlands)", code: "nl_NL"},
            { langName: "Finnish (Suomi)", code: "fi_FI"},
            { langName: "French (Français)", code: "fr_FR"},
            { langName: "Galician (Galego)", code: "gl_ES"},
            { langName: "Greek (Ελληνικά)", code: "el_GR"},
            { langName: "Hungarian (Magyar)", code: "hu_HU"},
            { langName: "Italian (Italiano)", code: "it_IT"},
            { langName: "Japanese (日本語)", code: "ja_JP"},
            { langName: "Lithuanian (Lietuvių)", code: "lt_LT"},
            { langName: "Norwegian (Norsk bokmål)", code: "nb_NO"},
            { langName: "Polish (Polski)", code: "pl_PL"},
            { langName: "Portuguese (Português)", code: "pt_PT"},
            { langName: "Portuguese, Brazil (Português do Brasil)", code: "pt_BR"},
            { langName: "Russian (Русский)", code: "ru_RU"},
            { langName: "Scottish Gaelic (Gàidhlig)", code: "gd_GB"},
            { langName: "Slovak (Slovenčina)", code: "sk_SK"},
            { langName: "Spanish (Español)", code: "es_ES"},
            { langName: "Swedish (Svenska)", code: "sv_SE"},
            { langName: "Tamil (தமிழ்)", code: "ta_IN"},
            { langName: "Turkish (Türkçe)", code: "tr_TR"},
            { langName: "Vietnamese (Tiếng Việt)", code: "vi_VN"}
        ]

    function findSupportedLocale(systemName) {
        var closestMatch = "en_US" // Default
        var shortName = systemName.substring(0,2)
        for (var i = 0; i < languages.length; ++i) {
            var currentCode = languages[i].code;
            if (currentCode === systemName) {
                return systemName;
            }
            if (currentCode.substring(0,2) === shortName) {
                closestMatch = currentCode;
            }
        }

        return closestMatch;
    }

    // ── Language code mapping QML <-> ConfigFile ────────────────────────────
    // AUTHORITATIVE is the ConfigFile key "Language" – the widgets client maintains
    // the same key. Its short codes follow the ts file names
    // (data/translations/pokerth_de.qm), the QML client uses locale codes
    // (i18n/pokerth_de_DE.qm). That is why it is converted when reading/writing.
    // IMPORTANT: the SHORT CODE belongs in the config, otherwise the
    // widgets client no longer finds its .qm (pokerth.cpp loads it unstripped).
    readonly property var languageConfigCodes: ({
        "de_DE": "de", "en_US": "en", "es_ES": "es", "fr_FR": "fr",
        "it_IT": "it", "pt_BR": "ptbr", "pt_PT": "ptpt", "ru_RU": "ru",
        "cs_CZ": "cz", "da_DK": "dk", "el_GR": "gr", "hu_HU": "hu",
        "nl_NL": "nl", "pl_PL": "pl", "sv_SE": "sv", "tr_TR": "tr",
        "bg_BG": "bg", "ca_ES": "ca", "fi_FI": "fi", "nb_NO": "no",
        "af_ZA": "af", "gd_GB": "gd", "gl_ES": "gl", "ja_JP": "jp",
        "lt_LT": "lt", "sk_SK": "sk", "ta_IN": "ta", "vi_VN": "vi",
        "zh_CN": "zhcn"
    })

    // QML locale -> config short code (for writing).
    function localeToConfigLanguage(locale) {
        return languageConfigCodes[locale] !== undefined
                ? languageConfigCodes[locale] : "en"
    }

    // Config value -> QML locale (for reading). It understands both the short code of the
    // widgets client ("de", "ptbr") and a locale value ("de_DE", the
    // default is QLocale::system().name()). Languages without a QML translation
    // (e.g. "cz") end up at the next best supported language; the
    // chat translation still follows the config value nevertheless.
    function configLanguageToLocale(cfg) {
        if (!cfg || cfg === "")
            return findSupportedLocale(Qt.locale().name)
        for (var i = 0; i < languages.length; ++i) {
            if (languages[i].code === cfg)
                return languages[i].code
        }
        for (var loc in languageConfigCodes) {
            if (languageConfigCodes[loc] === cfg)
                return loc
        }
        return findSupportedLocale(cfg)
    }

    // Poker action (0=none,1=fold,2=check,3=call,4=bet,5=raise,6=all-in) → the
    // word to be displayed. dontTranslate=true (config key
    // DontTranslateInternationalPokerStringsFromStyle) keeps the terms fixed in
    // English, otherwise they are localized. Centrally here, so that GamePlayerBox,
    // GamePlayerSelfBox and GameActionBar no longer duplicate the same switch.
    // The qsTr() literals are kept for the lupdate extraction.
    function pokerActionWord(action, dontTranslate) {
        switch (action) {
        case 1: return dontTranslate ? "Fold"   : qsTr("Fold")
        case 2: return dontTranslate ? "Check"  : qsTr("Check")
        case 3: return dontTranslate ? "Call"   : qsTr("Call")
        case 4: return dontTranslate ? "Bet"    : qsTr("Bet")
        case 5: return dontTranslate ? "Raise"  : qsTr("Raise")
        case 6: return dontTranslate ? "All-In" : qsTr("All-In")
        default: return ""
        }
    }

    // col100=primary text … col700=background — inverted between dark and light.
    // col700 is ALWAYS the page background (light: grey). Content boxes lie
    // above it and have a role of their own: Config.Theme.colorPanel / colorPopup
    // (light white, dark darkened) – see Theme.qml.
    // Using plain JS objects + property var so that changing `palette` triggers
    // re-evaluation of all `palette.secondary.colXXX` bindings reliably.
    readonly property var _dark: ({
        secondary: { col100:"#eff1f5", col200:"#cdd3e0", col300:"#a0acc4",
                     col400:"#7787a3", col500:"#576378", col600:"#394150", col700:"#1d222b" }
    })
    readonly property var _light: ({
        secondary: { col100:"#1d222b", col200:"#394150", col300:"#576378",
                     col400:"#7787a3", col500:"#a0acc4", col600:"#dce2ec", col700:"#e3e8f0" }
    })
    property var palette: isDark ? _dark : _light

    readonly property QtObject loadedFont: FontLoader {
        source: "../resources/Inter-VariableFont.ttf"
    }

    // Bundled colour emoji font (Noto Color Emoji, OFL), so that emojis appear
    // identical and in colour everywhere – independently of the system font.
    readonly property QtObject emojiFont: FontLoader {
        source: "../resources/NotoColorEmoji.ttf"
    }
    readonly property string emojiFamily: emojiFont.name !== "" ? emojiFont.name : "Noto Color Emoji"

    // Spektrum-Farben (angelehnt an pokerth.net Chart-Palette, Platz 1–10)
    readonly property var chartColors: [
        "#3dbd72",  // 1  – emerald green
        "#7bc64b",  // 2  – lime
        "#b4c83f",  // 3  – yellow-green
        "#e2bf35",  // 4  – gold
        "#e28230",  // 5  – orange
        "#d44545",  // 6  – red
        "#cc3480",  // 7  – pink
        "#a833c5",  // 8  – violet
        "#6040cc",  // 9  – indigo
        "#4060e0"   // 10 – blue
    ]

    // Placement palette 1–10, exactly like the pokerth.net season charts
    // (chartColors.js → PLACEMENT_COLORS). Deliberately separate from `chartColors`
    // above: that one is a darker UI accent variant, here the season
    // stats graphic should correspond 1:1 to the website in colour.
    readonly property var placementColors: [
        "#56e289",  // 1  – green
        "#68e256",  // 2  – light green
        "#aee256",  // 3  – yellow-green
        "#e2c856",  // 4  – yellow
        "#e2a056",  // 5  – orange
        "#e25468",  // 6  – red
        "#e256ae",  // 7  – pink
        "#cf56e2",  // 8  – purple
        "#8a56e2",  // 9  – violet
        "#5668e2"   // 10 – blue
    ]

    // Green→red heat palette of the BBC/WEC result bars, exactly like
    // bbc_wec_site BarChart.vue (COLORS): place 1 (green, good) … place 10 (red).
    readonly property var heatColors: [
        "#4ba800", "#78bb00", "#a5cd00", "#d2e000", "#fff200",
        "#ffc100", "#ff9100", "#ff6100", "#ff3000", "#ff0000"
    ]

    // Responsive card path: {rank}{suit}.svg (the responsive-playing-cards naming convention)
    // rank: 1=ace, 2-10, 11=jack, 12=queen, 13=king  |  suit: d=diamonds, h=hearts, s=spades, c=clubs
    function cardSourceResponsive(cardIndex) {
        if (cardIndex < 0 || cardIndex > 51) return "qrc:resources/cardBackground.svg"
        var suits  = ["d", "h", "s", "c"]
        var ranks  = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 1]
        var si = Math.floor(cardIndex / 13)
        var ri = cardIndex % 13
        return "qrc:resources/responsive-cards/" + ranks[ri] + suits[si] + ".svg"
    }

    // Shared chat history of the LOBBY channel: the lobby chat (compact + wide)
    // and the GameWait chat all send via Lobby.sendChatMessage – their
    // ChatBoxes bind this array as the historyStore and thus share
    // the ↑/↓ history. (The game chat has a channel of its own and keeps
    // its instance-local history.)
    readonly property var lobbyChatHistory: []

    // Tab nickname completion – the logic as in ChatTools::nickAutoCompletition
    // in the Qt widgets client: the first Tab collects all nicks that begin with
    // the last word; every further Tab iterates through the matches.
    // `state` ({counter, base, matches}) holds the calling input field and
    // resets counter=0 on user input (onTextEdited). It returns the
    // new input text or null (no match).
    function nickComplete(state, text, nicks) {
        if (state.counter === 0) {
            var words = text.split(" ")
            var prefix = words[words.length - 1]
            if (prefix === "") return null
            var matches = []
            for (var i = 0; i < nicks.length; i++) {
                var n = nicks[i]
                if (n && n.toLowerCase().indexOf(prefix.toLowerCase()) === 0)
                    matches.push(n)
            }
            if (matches.length === 0) return null
            words.pop()
            state.base = words.join(" ")
            state.matches = matches
        }
        if (!state.matches || state.matches.length === 0) return null
        if (state.counter >= state.matches.length) state.counter = 0
        var nick = state.matches[state.counter]
        state.counter++
        return state.base === "" ? nick + ": " : state.base + " " + nick + " "
    }

    // Returns a chart colour with proper contrast (light in dark mode, dark in light mode)
    function chartColor(index, highlighted) {
        var c = Qt.color(chartColors[index % chartColors.length])
        if (highlighted) {
            return isDark ? c : Qt.darker(c, 1.45)
        } else {
            return isDark ? Qt.darker(c, 1.9) : Qt.darker(c, 2.8)
        }
    }

    readonly property var progressMessages: [
        "Shuffling the Decks ...",
        "Bribing the Dealer ...",
        "Manipulating the RNG ...",
        "Taking a break at the Bar ...",
        "Practicing Poker-Faces ...",
        "Going All-In ...",
        "Folding AAAA ...",
        "Raising Big Blind ...",
        "Stacking Chips ..."
    ]
}
