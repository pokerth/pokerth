import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import "../config" as Config

// Inhalt des rechten Info-Panels im Spiel: Tab-Leiste „Verlauf" / „Chancen".
//   • Verlauf: der Spielverlauf (Log).
//   • Chancen: 10 Pokerblatt-Kategorien mit Wahrscheinlichkeit + Balken
//     (Port des CardsChanceMonitor aus dem Qt-Widgets-Client).
// Datenquelle ist GameTable (GameHandler): gameLog, cardsChance,
// cardsChanceFolded. Wird sowohl im schwebenden Overlay (GameSidePanel) als
// auch im permanent angedockten Desktop-Modus verwendet.
ColumnLayout {
    id: root
    spacing: 8

    // Tisch-Theme-Farben (fest/dunkel, unabhängig vom Hell/Dunkel-Modus der App).
    // Der Spielverlauf-Text selbst ist serverseitig bereits hell eingefärbt
    // (GameHandler::formatLogLine) – hier nur die Chancen-Balken/-Texte.
    readonly property color colText:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogText : "#eff1f5"
    readonly property color colTextMuted:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogTextMuted : "#7787a3"
    readonly property color colBorder:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogBorder : "#576378"
    readonly property color colSurface:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogSurface : "#394150"
    readonly property color colBackground:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogBackground : "#1d222b"
    readonly property color colAccent: Config.Theme.colorAccent

    // Aktiver Tab von außen steuerbar (Shortcuts/Toggle): 0 Verlauf · 1 Chancen
    property alias currentIndex: tabs.currentIndex

    // Schriftgröße der Chancen-Texte (Kategorie + Prozent).
    // Overlay: 12 (Default), gedockt: 11 (vom Aufrufer gesetzt).
    property int messageFontSize: 12

    // Der Spielverlauf (Log) läuft wie der Chat als fortlaufender Text und wird
    // größer gesetzt als die kompakte Chancen-Liste – analog zur ChatBox
    // (messageFontSize dort). +2 hält Log und Chat auf gleicher Größe:
    // Overlay 14, gedockt 13.
    property int logFontSize: messageFontSize + 2

    // ── Datenanbindung ────────────────────────────────────────────────────────
    readonly property var chance:
        (typeof GameTable !== "undefined" && GameTable) ? GameTable.cardsChance : []
    readonly property bool folded:
        (typeof GameTable !== "undefined" && GameTable) ? GameTable.cardsChanceFolded : false

    // Kategorie-Index (0 = Höchste Karte … 9 = Royal Flush) → Name + SVG-Icon.
    // Reihenfolge identisch zur cardsChance-Indizierung im GameHandler.
    readonly property var handDefs: [
        { name: qsTr("Höchste Karte"),  icon: "highcard" },
        { name: qsTr("Paar"),           icon: "onepair" },
        { name: qsTr("Zwei Paare"),     icon: "twopair" },
        { name: qsTr("Drilling"),       icon: "threeofakind" },
        { name: qsTr("Straße"),         icon: "straight" },
        { name: qsTr("Flush"),          icon: "flush" },
        { name: qsTr("Full House"),     icon: "fullhouse" },
        { name: qsTr("Vierling"),       icon: "fourofakind" },
        { name: qsTr("Straight Flush"), icon: "straightflush" },
        { name: qsTr("Royal Flush"),    icon: "royalflush" }
    ]

    function handIcon(cat) {
        return (cat >= 0 && cat < handDefs.length)
            ? "qrc:resources/hands/" + handDefs[cat].icon + ".svg" : ""
    }

    // Rechter Freiraum für die vertikale Scrollbar, damit sie den Inhalt nie
    // überlappt (Scrollbars liegen im Overlay-Stil sonst über dem Text).
    readonly property int scrollGutter: 14

    // ── Tab-Leiste ────────────────────────────────────────────────────────────
    CustomTabBar {
        id: tabs
        // „Verlauf" und „Chancen" sind immer beide erreichbar – der
        // Kartenchancenmonitor wird nicht mehr separat in den Einstellungen
        // abgeschaltet, sondern über den Info-Panel-Toggle ein-/ausgeblendet.
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        tabHeight: 18
        tabFontPointSize: 8
        model: [qsTr("Verlauf"), qsTr("Chancen")]
    }

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: tabs.currentIndex

        // ── Tab „Verlauf" (Spielverlauf / Log) ────────────────────────────────
        // EIN zusammenhängendes RichText-Dokument (wie die ChatBox), KEINE
        // ListView: deren contentHeight ist bei variabel hohen RichText-
        // Delegates nur geschätzt und fluktuiert beim Scrollen (Delegate-
        // Recycling) – das ließ den Auto-Scroll ständig neu feuern und unten
        // festklemmen. Mit einer Flickable über deterministischer contentHeight
        // (= TextEdit.implicitHeight) ist das stabil; zusätzlich gibt es so die
        // durchgehende Maus-Selektion + Kopieren/Alles-auswählen.
        Flickable {
            id: logFlick
            clip: true
            contentWidth: width
            contentHeight: logText.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick
            ScrollBar.vertical: ScrollBar {
                id: logScrollBar
                policy: logFlick.contentHeight > logFlick.height + 4
                        ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                // Ziehen am Griff setzt contentY direkt und erzeugt KEINE
                // movementStarted/Ended-Signale – von Hand anhängen.
                onPressedChanged: pressed ? logFlick.userScrollStarted()
                                          : logFlick.userScrollEnded()
            }
            // Auto-Scroll: pausiert beim Hochscrollen, Position bleibt bei neuen
            // Zeilen erhalten, nach 15 s Inaktivität wieder ans Ende.
            property bool autoScroll: true
            property real savedContentY: 0
            Timer {
                id: logAutoScrollTimer
                interval: 15000
                onTriggered: { logFlick.autoScroll = true; logFlick.scrollToBottom() }
            }
            // Ans Ende kleben. pinBottom() prüft selbst autoScroll, damit ein
            // nachgelagerter (Qt.callLater-)Aufruf nichts tut, wenn der Nutzer
            // inzwischen weggescrollt hat.
            function pinBottom() {
                if (autoScroll) contentY = Math.max(0, contentHeight - height)
            }
            function scrollToBottom() { autoScroll = true; pinBottom() }
            function restoreScroll() {
                contentY = Math.min(savedContentY, Math.max(0, contentHeight - height))
            }
            // Bei Auto-Scroll ZWEIMAL ans Ende ziehen: sofort (contentHeight ist im
            // Change-Handler bereits der neue Wert) UND einmal per Qt.callLater.
            // QQuickTextEdit aktualisiert seine implicitHeight erst in der Polish-
            // Phase und QQuickFlickable seine interne Scroll-Grenze ebenfalls dort;
            // je nach Reihenfolge klemmt die noch alte Grenze das sofortige Setzen
            // nach unten – dann greift der callLater nach dem Polish. Einer der
            // beiden landet immer korrekt, doppeltes Setzen ist folgenlos.
            function followBottom() {
                // Während einer laufenden Nutzergeste gar nichts anfassen.
                if (moving || logScrollBar.pressed)
                    return
                if (autoScroll) { pinBottom(); Qt.callLater(pinBottom) }
                // Pausiert: gemerkte Position halten, während der Text komplett
                // ersetzt wird.
                else restoreScroll()
            }
            // An contentHeight hängen: feuert bei JEDER Höhenänderung – neue Zeile,
            // async umbrechende RichText-Zeilen und komplettes Ersetzen des Texts.
            onContentHeightChanged: followBottom()
            // Resize (z. B. geänderte Spieleranzahl) – gleich behandeln.
            onHeightChanged: followBottom()

            // Auto-Scroll-Zustand NUR aus echten Nutzergesten ableiten (siehe
            // ausführliche Begründung in ChatBox.qml): `moving` gilt auch für
            // die Positionskorrektur, die die Flickable bei jeder Höhen-
            // änderung selbst vornimmt – deren Zwischenwerte dürfen den
            // Auto-Scroll nicht abschalten, sonst bleibt die letzte Zeile
            // angeschnitten stehen.
            function userScrollStarted() {
                autoScroll = false
                logAutoScrollTimer.stop()
            }
            function userScrollEnded() {
                savedContentY = contentY
                // Mit Toleranz prüfen statt exaktem atYEnd: knapp am Ende reicht
                // (Subpixel/async wachsende RichText-Zeilen), damit der
                // Auto-Scroll am unteren Rand zuverlässig wieder anspringt.
                autoScroll = contentY >= contentHeight - height - 4
                if (autoScroll) { logAutoScrollTimer.stop(); pinBottom() }
                else logAutoScrollTimer.restart()
            }
            onMovementStarted: userScrollStarted()
            onMovementEnded: userScrollEnded()

            // Read-only TextEdit hält den gesamten Verlauf als EIN HTML-Dokument
            // (GameLogModel.html – Zeilen mit <br> verkettet). Die Zeilen sind
            // serverseitig bereits hell eingefärbt (GameHandler::formatLogLine).
            TextEdit {
                id: logText
                width: logFlick.width - root.scrollGutter
                text: (typeof GameTable !== "undefined" && GameTable)
                      ? GameTable.gameLog.html : ""
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.Wrap
                readOnly: true
                selectByMouse: true
                persistentSelection: true
                color: root.colText
                selectionColor: root.colAccent
                selectedTextColor: "#101010"
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: root.logFontSize
                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: logCtxMenu.popup()
                }
            }
        }

        // ── Tab „Chancen" ─────────────────────────────────────────────────────
        Flickable {
            id: chanceFlick
            contentHeight: chanceCol.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {
                policy: chanceFlick.contentHeight > chanceFlick.height + 2
                        ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }

            Column {
                id: chanceCol
                width: chanceFlick.width - root.scrollGutter
                spacing: 0

                // Royal Flush oben, Höchste Karte unten (wie im Widgets-Client).
                Repeater {
                    model: 10
                    delegate: Item {
                        required property int index
                        // 9 → Royal Flush … 0 → Höchste Karte
                        readonly property int cat: 9 - index
                        readonly property var entry:
                            (root.chance && root.chance.length > cat) ? root.chance[cat] : null
                        readonly property int prob: entry ? entry.prob : 0
                        readonly property bool possible: entry ? entry.possible : false

                        width: chanceCol.width
                        height: 26

                        // Wahrscheinlichkeits-Balken als Zeilen-Hintergrund – kostet
                        // keine horizontale Breite, sodass der Name den vollen Platz
                        // bekommt (statt früh mit „…" abgeschnitten zu werden).
                        // Wie der Chat: keine Bubbles/Ränder – die Zeilen füllen die
                        // volle Höhe ohne Abstand und ergeben so eine durchgehende,
                        // einheitliche Fläche; nur der Füllbalken hebt sich ab.
                        Rectangle {
                            anchors.fill: parent
                            color: Config.Theme.withAlpha(root.colBorder, 0.14)
                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: parent.width * Math.max(0, Math.min(100, prob)) / 100
                                color: possible
                                       ? Config.Theme.withAlpha(Config.Theme.colorAccent, 0.30)
                                       : Config.Theme.withAlpha(root.colBorder, 0.22)
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 4
                            anchors.rightMargin: 8
                            spacing: 8

                            Image {
                                Layout.preferredWidth: 38
                                Layout.preferredHeight: 22
                                Layout.alignment: Qt.AlignVCenter
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                opacity: possible ? 1.0 : 0.32
                                source: root.handIcon(cat)
                                sourceSize.width: Math.ceil(38 * Screen.devicePixelRatio)
                                sourceSize.height: Math.ceil(22 * Screen.devicePixelRatio)
                            }

                            AppText {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                text: root.handDefs[cat].name
                                // Eine Zeile pro Kategorie – kompakt; bei Platzmangel „…".
                                elide: Text.ElideRight
                                font.pixelSize: root.messageFontSize
                                color: possible ? root.colText : root.colTextMuted
                            }

                            AppText {
                                Layout.preferredWidth: 40
                                Layout.alignment: Qt.AlignVCenter
                                horizontalAlignment: Text.AlignRight
                                text: prob + "%"
                                font.pixelSize: root.messageFontSize
                                font.bold: possible && prob >= 50
                                color: possible ? root.colText : root.colTextMuted
                            }
                        }
                    }
                }
            }
        }
    }

    // ── Rechtsklick-Kontextmenü für den Verlauf (Kopieren / Alles auswählen) ──
    // Einheitlich gestylt, folgt den Tisch-Theme-Farben (wie die ChatBox).
    component CtxItem: MenuItem {
        height: visible ? implicitHeight : 0
        contentItem: Text {
            text: parent.text
            color: parent.enabled
                   ? (parent.highlighted ? root.colAccent : root.colText)
                   : root.colTextMuted
            font.family: Config.StaticData.loadedFont.font.family
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8
        }
        background: Rectangle {
            color: parent.highlighted ? root.colSurface : "transparent"
        }
    }

    Menu {
        id: logCtxMenu
        background: Rectangle {
            implicitWidth: 160
            color: root.colBackground
            border.width: 1
            border.color: root.colBorder
            radius: 6
        }

        CtxItem {
            text: qsTr("Kopieren")
            enabled: logText.selectedText.length > 0
            onTriggered: logText.copy()
        }
        CtxItem {
            text: qsTr("Alles auswählen")
            onTriggered: logText.selectAll()
        }
    }
}
