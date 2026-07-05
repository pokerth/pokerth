import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Umschalter zwischen den Ranglisten-Quellen PokerTH / BBC / WEC – Buttonbox
// mit Aktiv-/Inaktiv-Zustand. Die einbettende Seite setzt `current` und
// reagiert auf selected(community): Player-Pages ersetzen sich damit selbst
// (StackView.replace), die Tisch-Übersicht lädt ihre Daten neu.
//
// Die Helfer bündeln die Quell-Konfiguration (Basis-URL, Stat-Blöcke der
// CommunityPlayerView, Player-Page-URL+Props), damit sie nicht in jeder
// aufrufenden Seite dupliziert werden muss.
Rectangle {
    id: sw

    // "pokerth" | "bbc" | "wec"
    property string current: "pokerth"
    // Klick auf einen NICHT aktiven Eintrag (aktiver Eintrag löst nichts aus).
    signal selected(string community)

    readonly property var entries: [
        { label: "PokerTH", key: "pokerth" },
        { label: "BBC",     key: "bbc" },
        { label: "WEC",     key: "wec" }
    ]

    function baseUrlFor(community) {
        return community === "bbc" ? "https://bbc.pokerth.net"
                                   : "https://wec.pokerth.net"
    }

    // Stat-Blöcke der CommunityPlayerView je Quelle (Reihenfolge + Überschrift,
    // key referenziert den Block in deren stats-Objekt).
    function blocksFor(community) {
        if (community === "bbc")
            return [
                { label: qsTr("This season"), key: "season" },
                { label: qsTr("All-time"),    key: "alltime" }
            ]
        return [
            { label: qsTr("This month"), key: "month" },
            { label: qsTr("This year"),  key: "year" },
            { label: qsTr("All-time"),   key: "alltime" }
        ]
    }

    // Player-Page der Quelle: PokerTH hat eine eigene Seite, BBC/WEC teilen
    // sich die CommunityPlayerView.
    function playerPageUrl(community) {
        return community === "pokerth"
               ? "qrc:/pages/PokerthPlayerPage.qml"
               : "qrc:/components/CommunityPlayerView.qml"
    }
    function playerPageProps(community, nick) {
        if (community === "pokerth")
            return { username: nick }
        return { baseUrl: baseUrlFor(community), nickname: nick,
                 blocks: blocksFor(community) }
    }

    implicitWidth: segmentRow.implicitWidth + 2
    implicitHeight: 26
    radius: Config.Theme.radiusSmall
    color: Config.StaticData.palette.secondary.col600
    border.color: Config.StaticData.palette.secondary.col500
    border.width: 1

    Row {
        id: segmentRow
        anchors.fill: parent
        anchors.margins: 1

        Repeater {
            model: sw.entries

            Rectangle {
                id: segment
                required property var modelData
                readonly property bool active: sw.current === modelData.key

                width: segLabel.implicitWidth + 20
                height: segmentRow.height
                radius: 3
                color: active
                       ? Config.StaticData.palette.secondary.col500
                       : (segHover.hovered
                          ? Config.Theme.withAlpha(Config.StaticData.palette.secondary.col500, 0.5)
                          : "transparent")

                AppLabel {
                    id: segLabel
                    anchors.centerIn: parent
                    text: segment.modelData.label
                    color: segment.active || segHover.hovered
                           ? Config.StaticData.palette.secondary.col100
                           : Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                    font.bold: segment.active
                }

                HoverHandler {
                    id: segHover
                    cursorShape: segment.active ? Qt.ArrowCursor : Qt.PointingHandCursor
                }
                TapHandler {
                    onTapped: {
                        if (!segment.active)
                            sw.selected(segment.modelData.key)
                    }
                }
            }
        }
    }
}
