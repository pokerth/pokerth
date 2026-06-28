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

    // Aktiver Tab von außen steuerbar (Shortcuts/Toggle): 0 Verlauf · 1 Chancen
    property alias currentIndex: tabs.currentIndex

    // Schriftgröße für Verlauf-/Chancen-Texte – analog zur ChatBox
    // (messageFontSize), damit Log/Chancen so groß wie der Chat sind.
    // Overlay: 12 (Default), gedockt: 11 (vom Aufrufer gesetzt).
    property int messageFontSize: 12

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
        ListView {
            id: logList
            clip: true
            model: (typeof GameTable !== "undefined" && GameTable) ? GameTable.gameLog : []
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {
                id: logScrollBar
                policy: logList.contentHeight > logList.height + 4
                        ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }
            // Auto-Scroll folgt neuen Einträgen, solange der Nutzer unten ist.
            property bool autoScroll: true
            property real savedContentY: 0
            Timer {
                id: logAutoScrollTimer
                interval: 15000
                onTriggered: { logList.autoScroll = true; logList.positionViewAtEnd() }
            }
            function restoreScroll() {
                contentY = Math.min(savedContentY, Math.max(0, contentHeight - height))
            }
            // Nur benutzergetriebene Bewegungen werten (Flick/Wheel sowie
            // Scrollbar-Ziehen) – NICHT das programmatische Positionieren oder
            // den Sprung beim Model-Ersetzen (dort ist moving==false).
            onContentYChanged: {
                if (!moving && !logScrollBar.pressed) return
                savedContentY = contentY
                // Mit Toleranz prüfen statt exaktem atYEnd: knapp am Ende reicht
                // (Subpixel/async wachsende RichText-Zeilen), damit der
                // Auto-Scroll am unteren Rand zuverlässig wieder anspringt.
                if (contentY >= contentHeight - height - 4) {
                    autoScroll = true; logAutoScrollTimer.stop()
                } else {
                    autoScroll = false; logAutoScrollTimer.restart()
                }
            }
            onCountChanged: {
                if (autoScroll) positionViewAtEnd()
                else Qt.callLater(restoreScroll)
            }
            // Spieleranzahl ändert sich → Boxen werden resized. Ohne dies bliebe
            // contentY stehen und die View „hinge" über der letzten Zeile bzw.
            // die gemerkte Position liefe aus dem Rahmen.
            onHeightChanged: {
                if (autoScroll) Qt.callLater(positionViewAtEnd)
                else            Qt.callLater(restoreScroll)
            }
            delegate: AppText {
                required property string line
                width: ListView.view.width - root.scrollGutter
                text: line
                textFormat: Text.RichText
                wrapMode: Text.WordWrap
                font.pixelSize: root.messageFontSize
                lineHeight: 1.15
                bottomPadding: 4
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
                spacing: 4

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
                        height: 40

                        // Wahrscheinlichkeits-Balken als Zeilen-Hintergrund – kostet
                        // keine horizontale Breite, sodass der Name den vollen Platz
                        // bekommt (statt früh mit „…" abgeschnitten zu werden).
                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            height: parent.height - 6
                            radius: 6
                            color: Config.Theme.withAlpha(root.colBorder, 0.18)
                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                radius: parent.radius
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
                                Layout.preferredWidth: 52
                                Layout.preferredHeight: 34
                                Layout.alignment: Qt.AlignVCenter
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                opacity: possible ? 1.0 : 0.32
                                source: root.handIcon(cat)
                                sourceSize.width: Math.ceil(52 * Screen.devicePixelRatio)
                                sourceSize.height: Math.ceil(34 * Screen.devicePixelRatio)
                            }

                            AppText {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                text: root.handDefs[cat].name
                                // Lieber umbrechen als früh kürzen; „…" erst nach 2 Zeilen.
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
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
}
