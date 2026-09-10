import QtQuick

import "../config" as Config

// Gemeinsame Grundlage der beiden Spielerboxen am Tisch: GamePlayerBox
// (Gegner) und GamePlayerSelfBox (eigener Sitz).
//
// Beide leiten dieselben Werte aus GameTable.players[seatIndex] ab - der
// eigene Sitz ist schlicht Sitz 0. Genau diese Ableitungen standen früher in
// BEIDEN Dateien wortgleich nebeneinander, weshalb Pro-Spieler-Features (z. B.
// die Länderflagge) regelmäßig nur in einer der beiden gepflegt wurden.
//
// Hier gehört ausschließlich hinein, was für JEDEN Sitz gilt. Alles, was nur
// den eigenen Sitz betrifft (Ping-Anzeige, Anti-Peek) oder nur fremde Sitze
// (Kontextmenü, Ignorieren, Spielernotiz), bleibt in der jeweiligen Datei -
// ebenso das komplette Aussehen, das sich bewusst unterscheidet.
Item {
    id: seatBox

    // Sitz, dessen Daten diese Box zeigt. Der eigene Sitz ist immer 0.
    property int seatIndex: 0
    // Box sitzt in der unteren Tischhälfte (beeinflusst Badge-Seiten).
    property bool up: false
    // Effektive Tisch-Skalierung dieser Box (oppScale × Zoom), an die Karten
    // weitergereicht, damit ihr SVG-Raster die echte Bildschirmgröße trifft.
    property real cardRenderScale: 1.0

    // Einsatz-Sockel im Sitz-Stil "inset" (Config.SeatStyle): der Einsatz steht
    // IN der Box statt daneben, die Box wächst dafür genau um diese Höhe. 0 im
    // Stil "classic". Aus jeder abgeleiteten Größe wieder herausgerechnet,
    // damit die Box nur in der HÖHE wächst - Avatar, Karten und Boxbreite
    // bleiben unverändert.
    readonly property bool betInset: Config.SeatStyle.betInset
    readonly property int betStripH: Config.SeatStyle.betStripExtra

    // Höhe des Box-KÖRPERS (ohne Sockel). `height` ist die am Tisch permanent
    // RESERVIERTE Höhe inklusive Sockel: der Körper sitzt oben im reservierten
    // Bereich, der Sockel klappt nach unten in den freigehaltenen Rest auf.
    // Dadurch wächst die Box nur, wenn wirklich ein Einsatz steht - ohne dass
    // Nachbarboxen oder die Tisch-Skalierung sich bewegen.
    readonly property int bodyH: height - betStripH
    // Sockel nur bei tatsächlichem Einsatz aufklappen.
    readonly property bool stripOpen: betInset && seatBox.bet > 0

    // ── Spielerdaten aus GameTable ───────────────────────────────────────────
    readonly property var seatData: (typeof GameTable !== "undefined" && GameTable && GameTable.players.length > seatIndex)
        ? GameTable.players[seatIndex] : null

    readonly property int card0: seatData && seatData.card0 !== undefined ? seatData.card0 : -1
    readonly property int card1: seatData && seatData.card1 !== undefined ? seatData.card1 : -1
    // Showdown-Spotlight: einzelne Hole-Card des Gewinners abblenden, wenn sie
    // nicht zum Siegerblatt zählt (vom GameHandler gesetzt).
    readonly property bool fade0: seatData && seatData.fade0 !== undefined ? seatData.fade0 : false
    readonly property bool fade1: seatData && seatData.fade1 !== undefined ? seatData.fade1 : false

    readonly property bool isMyTurn: seatData ? seatData.myTurn : false
    // Aktiver Spieler (am Zug): lokal über seatData.myTurn (Engine setzt
    // getMyTurn()), im Netzwerk-Spiel über den Action-Timeout (timeoutSeatId) -
    // dort ist myTurn clientseitig nicht gesetzt. Beides berücksichtigen, damit
    // der Highlight-Rahmen in BEIDEN Spielarten erscheint.
    readonly property bool isAtTurn: seatBox.isMyTurn
        || ((typeof GameTable !== "undefined" && GameTable) ? GameTable.timeoutSeatId === seatBox.seatIndex : false)
    readonly property bool isWinner: typeof GameTable !== "undefined" && GameTable
        && GameTable.winnerSeatIds.indexOf(seatBox.seatIndex) !== -1

    readonly property int button: seatData && seatData.button !== undefined ? seatData.button : 0
    readonly property int bet: seatData && seatData.bet !== undefined ? seatData.bet : 0
    // Einstellung „Symbole für Small/Big Blind anzeigen" (Config-Key
    // ShowBlindButtons). Wie im Qt-Widgets-Client wird der Dealer-Button (1)
    // immer gezeigt, nur Small-Blind (2) und Big-Blind (3) sind abschaltbar.
    readonly property bool showBlindButtons:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("ShowBlindButtons") !== 0 : true
    readonly property bool buttonVisible:
        button === 1 || ((button === 2 || button === 3) && showBlindButtons)

    // Spieler hat gefoldet → Karten durchscheinend (wie im Qt-Widgets-Client)
    readonly property bool folded: seatData && seatData.folded !== undefined ? seatData.folded : false

    // Avatare ignorierter Spieler werden wie im Qt-Widgets-Client
    // (MyAvatarLabel) ausgeblendet. Nur für fremde Sitze relevant - die
    // Gegnerbox bindet hier ihre Ignorier-Logik dagegen, der eigene Sitz
    // bleibt beim Default.
    property bool hideIgnoredAvatar: false
    // Gesetzter Avatar (file://-URL) bzw. "" → Platzhalter.
    readonly property string avatarSource:
        (seatData && seatData.avatar !== undefined && !hideIgnoredAvatar) ? seatData.avatar : ""

    // Letzte Aktion dieses Spielers (0=keine,1=Fold,2=Check,3=Call,4=Bet,5=Raise,6=All-In)
    readonly property int action: seatData && seatData.action !== undefined ? seatData.action : 0
    // Einstellung „Internationale Pokerausdrücke nicht übersetzen" (Config-Key
    // DontTranslateInternationalPokerStringsFromStyle): Aktions-Begriffe fest auf
    // Englisch statt lokalisiert. qsTr()-Literale bleiben für die Extraktion.
    readonly property bool dontTranslatePokerTerms:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("DontTranslateInternationalPokerStringsFromStyle") !== 0 : false
    readonly property string actionText: Config.StaticData.pokerActionWord(seatBox.action, dontTranslatePokerTerms)

    // Länderflagge: direkt aus den Sitzdaten. Der GameHandler löst sie im
    // Netzwerkspiel über die eindeutige Spieler-Id der Session auf (wie der
    // Qt-Widgets-Client), nicht über die Spielerliste des Spiels - die ist am
    // Tisch unvollständig, sobald jemand erst während des Spiels dazukommt.
    readonly property string countryCode:
        seatData && seatData.countryCode !== undefined ? seatData.countryCode : ""
}
