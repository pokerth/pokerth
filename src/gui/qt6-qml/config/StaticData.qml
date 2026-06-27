pragma Singleton
import QtQuick 6.5
import QtCore

// import "../resources"

QtObject {
    id: root

    // 0 = Hell (Light), 1 = Dunkel (Dark), 2 = Automatisch
    // Synced from pokerth.qml Component.onCompleted and GuiSettings DarkMode ComboBox
    property int darkMode: 1

    readonly property bool isDark: darkMode !== 0  // 0=Hell → false, alles andere (1=Dunkel, 2=Auto) → true

    readonly property var languages: [
            { langName: "Deutsch (Deutsch)", code: "de_DE"},
            { langName: "English (English)", code: "en_US"},
            { langName: "French (Français)", code: "fr_FR"}
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

    // Poker-Aktion (0=keine,1=Fold,2=Check,3=Call,4=Bet,5=Raise,6=All-In) → das
    // anzuzeigende Wort. dontTranslate=true (Config-Key
    // DontTranslateInternationalPokerStringsFromStyle) hält die Begriffe fest auf
    // Englisch, sonst lokalisiert. Zentral hier, damit GamePlayerBox,
    // GamePlayerSelfBox und GameActionBar denselben Switch nicht mehr duplizieren.
    // qsTr()-Literale bleiben für die lupdate-Extraktion erhalten.
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
    // Using plain JS objects + property var so that changing `palette` triggers
    // re-evaluation of all `palette.secondary.colXXX` bindings reliably.
    readonly property var _dark: ({
        secondary: { col100:"#eff1f5", col200:"#cdd3e0", col300:"#a0acc4",
                     col400:"#7787a3", col500:"#576378", col600:"#394150", col700:"#1d222b" }
    })
    readonly property var _light: ({
        secondary: { col100:"#1d222b", col200:"#394150", col300:"#576378",
                     col400:"#7787a3", col500:"#a0acc4", col600:"#dce2ec", col700:"#f0f3f8" }
    })
    property var palette: isDark ? _dark : _light

    readonly property QtObject loadedFont: FontLoader {
        source: "../resources/Inter-VariableFont.ttf"
    }

    // Gebündelter Farb-Emoji-Font (Noto Color Emoji, OFL), damit Emojis überall
    // identisch und farbig erscheinen – unabhängig von der System-Schrift.
    readonly property QtObject emojiFont: FontLoader {
        source: "../resources/NotoColorEmoji.ttf"
    }
    readonly property string emojiFamily: emojiFont.name !== "" ? emojiFont.name : "Noto Color Emoji"

    // Spektrum-Farben (angelehnt an pokerth.net Chart-Palette, Platz 1–10)
    readonly property var chartColors: [
        "#3dbd72",  // 1  – Smaragdgrün
        "#7bc64b",  // 2  – Limette
        "#b4c83f",  // 3  – Gelbgrün
        "#e2bf35",  // 4  – Gold
        "#e28230",  // 5  – Orange
        "#d44545",  // 6  – Rot
        "#cc3480",  // 7  – Pink
        "#a833c5",  // 8  – Violett
        "#6040cc",  // 9  – Indigo
        "#4060e0"   // 10 – Blau
    ]

    // Responsiver Kartenpfad: {rank}{suit}.svg (responsive-playing-cards Namenskonvention)
    // rank: 1=Ass, 2-10, 11=Bube, 12=Dame, 13=König  |  suit: d=Karo, h=Herz, s=Pik, c=Kreuz
    function cardSourceResponsive(cardIndex) {
        if (cardIndex < 0 || cardIndex > 51) return "qrc:resources/cardBackground.svg"
        var suits  = ["d", "h", "s", "c"]
        var ranks  = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 1]
        var si = Math.floor(cardIndex / 13)
        var ri = cardIndex % 13
        return "qrc:resources/responsive-cards/" + ranks[ri] + suits[si] + ".svg"
    }

    // Gemeinsame Chat-History des LOBBY-Kanals: Lobby-Chat (compact + wide)
    // und GameWait-Chat senden alle über Lobby.sendChatMessage – ihre
    // ChatBoxen binden dieses Array als historyStore und teilen sich damit
    // die ↑/↓-History. (Der Game-Chat hat seinen eigenen Kanal und behält
    // seine instanz-lokale History.)
    readonly property var lobbyChatHistory: []

    // Tab-Nick-Vervollständigung – Logik wie ChatTools::nickAutoCompletition
    // im Qt-Widgets-Client: der erste Tab sammelt alle Nicks, die mit dem
    // letzten Wort beginnen; jeder weitere Tab iteriert durch die Treffer.
    // `state` ({counter, base, matches}) hält das aufrufende Eingabefeld und
    // setzt bei Nutzereingabe counter=0 zurück (onTextEdited). Liefert den
    // neuen Eingabetext oder null (kein Treffer).
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

    // Gibt eine kontrastgerechte Chart-Farbe zurück (hell in Dark-Mode, dunkel in Light-Mode)
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
