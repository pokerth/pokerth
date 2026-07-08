import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Umschalter zwischen den Ranglisten-Quellen PokerTH / BBC / WEC – Buttonbox
// mit Aktiv-/Inaktiv-Zustand. Die einbettende Seite setzt `current` und
// reagiert auf selected(community): Player-Pages ersetzen sich damit selbst
// (StackView.replace), die Tisch-Übersicht lädt ihre Daten neu.
//
// Quell-Registry und Routing (Einträge, Basis-URL, Player-Page-URL+Props)
// liegen zentral in Config.Community, damit sie nicht in jeder aufrufenden
// Seite dupliziert werden müssen.
Rectangle {
    id: sw

    // "pokerth" | "bbc" | "wec"
    property string current: "pokerth"
    // Klick auf einen NICHT aktiven Eintrag (aktiver Eintrag löst nichts aus).
    signal selected(string community)

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
            model: Config.Community.entries

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
