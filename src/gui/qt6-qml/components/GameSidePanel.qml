import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

// Schwebendes Seiten-Panel für Spielverlauf & Chat: abgerundetes Sheet mit
// Header (Titel + Schließen) und freiem Inhaltsbereich darunter.
//   – Querformat/Vollbild: Sidebar (~1/3 Breite) an einer Seite.
//   – Hochformat: volles Overlay über den Tisch.
// Der Inhalt (ListView, ChatBox …) wird als Default-Kind übergeben und landet
// unterhalb der Kopfzeile im Body-Layout.
Item {
    id: root

    property string title: ""
    // Kopfzeile (Titel + Schließen + Trennlinie) ein-/ausblenden. Das Info-Panel
    // braucht keine Überschrift – dort genügt die Tab-Leiste, und geschlossen wird
    // über den Umschalt-Button oben. Der Chat nutzt weiterhin die Kopfzeile.
    property bool showHeader: true
    property int edge: Qt.LeftEdge          // Qt.LeftEdge | Qt.RightEdge
    property bool wide: false
    signal closeRequested()

    // Tisch-Theme-Farben (fest/dunkel, unabhängig vom Hell/Dunkel-Modus der App).
    // StyleProvider liefert immer gültige Werte; der Fallback deckt nur den Fall
    // ab, dass die Context-Property mal nicht gesetzt ist (z. B. Vorschau).
    readonly property color colBackground:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogBackground : "#1d222b"
    readonly property color colBorder:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogBorder : "#576378"
    readonly property color colTextSecondary:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogTextSecondary : "#cdd3e0"

    // Default-Inhalt landet unter Header + Trennlinie im Body-Layout.
    default property alias content: bodyLayout.data

    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.left:  edge === Qt.LeftEdge  ? parent.left  : undefined
    anchors.right: edge === Qt.RightEdge ? parent.right : undefined
    width: wide ? Math.max(parent.width / 3, 300) : parent.width

    // Chrome (Sheet-Hintergrund, Klick-Fänger, Header-Layout) EXPLIZIT als
    // children zuweisen – sonst würden diese über die Default-Property
    // (content → bodyLayout.data) ins Inhalts-Layout umgeleitet. So fließt nur
    // der vom Aufrufer deklarierte Inhalt in bodyLayout.
    children: [
        // Schwebendes Sheet: eingerückt, abgerundet, mit Elevation.
        Rectangle {
            id: panel
            anchors.fill: parent
            anchors.topMargin: 50   // Abstand zum Umschalt-Icon oben
            anchors.bottomMargin: 10
            anchors.leftMargin: root.wide ? 10 : 8
            anchors.rightMargin: root.wide ? 10 : 8
            radius: 16
            color: Config.Theme.withAlpha(root.colBackground, 0.95)
            border.color: root.colBorder
            border.width: 1

            layer.enabled: Config.Theme.effectsEnabled
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#000000"
                shadowOpacity: 0.55
                shadowBlur: 0.9
                shadowVerticalOffset: 3
                shadowHorizontalOffset: 0
            }
        },

        // Klicks innerhalb des Sheets abfangen (Tisch daneben bleibt nutzbar)
        MouseArea { anchors.fill: panel },

        ColumnLayout {
            id: bodyLayout
            anchors.fill: panel
            anchors.margins: 12
            spacing: 8

            // Header: Titel + Schließen
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                visible: root.showHeader
                AppText {
                    Layout.fillWidth: true
                    text: root.title
                    color: Config.Theme.colorAccent
                    font.pixelSize: 15
                    font.bold: true
                    font.letterSpacing: 0.4
                }
                Rectangle {
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 26
                    radius: 13
                    color: closeArea.containsMouse
                           ? Config.Theme.withAlpha(root.colBorder, 0.7)
                           : "transparent"
                    SvgIcon {
                        anchors.centerIn: parent
                        width: 14; height: 14
                        source: "../resources/close.svg"
                        layer.enabled: true
                        layer.effect: MultiEffect {
                            colorization: 1.0
                            colorizationColor: root.colTextSecondary
                        }
                    }
                    MouseArea {
                        id: closeArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.closeRequested()
                    }
                }
            }

            // Trennlinie
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Config.Theme.withAlpha(root.colBorder, 0.5)
                visible: root.showHeader
            }
        }
    ]
}
