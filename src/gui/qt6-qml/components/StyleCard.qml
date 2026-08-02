import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Eine Stil-Karte für die Stil-Auswahl (Spieltisch / Kartenstapel):
// Vorschaubild links, Beschreibung rechts, Auswahl-Markierung per Klick.
Rectangle {
    id: card

    // Style-Eintrag (Map vom SettingsManager): { name, description, maintainer,
    // preview, previewPortrait, dir, xml }.
    property var styleEntry: ({})
    property bool selected: false
    // Wenn true, IMMER das Querformat-Vorschaubild verwenden (z. B. Kartenstapel
    // und Kartenrückseite – dort gibt es bewusst kein Portrait-Preview).
    property bool forceLandscape: false
    // Importierte Stile (userStyle) lassen sich wieder entfernen; mitgelieferte
    // nicht – für sie bleibt der Entfernen-Button ausgeblendet.
    readonly property bool removable: styleEntry.userStyle === true
    signal clicked()
    signal removeRequested()
    signal exportRequested()

    // Auf echten Mobilgeräten das Portrait-Vorschaubild, auf dem Desktop das
    // Querformat. Fehlt die jeweilige Orientierung, die andere als Ersatz nutzen.
    readonly property bool usePortrait: !forceLandscape && Config.Responsive.isMobile
    readonly property string previewSource:
        usePortrait ? (styleEntry.previewPortrait || styleEntry.preview || "")
                    : (styleEntry.preview || styleEntry.previewPortrait || "")

    Layout.fillWidth: true
    Layout.preferredHeight: (usePortrait ? 150 : 100) + 16
    radius: 6
    color: selected ? Config.StaticData.palette.secondary.col600 : "transparent"
    border.width: selected ? 2 : 1
    border.color: selected ? Config.StaticData.chartColors[0]
                           : Config.StaticData.palette.secondary.col600

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 12

        // Vorschau-Rahmen
        Rectangle {
            Layout.preferredWidth: card.usePortrait ? 96 : 150
            Layout.preferredHeight: card.usePortrait ? 150 : 100
            Layout.alignment: Qt.AlignVCenter
            radius: 4
            clip: true
            color: Config.Theme.colorBox

            Image {
                id: previewImg
                anchors.fill: parent
                anchors.margins: 1
                source: card.previewSource
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                cache: true
                sourceSize.width: 320 // begrenzt den Speicherbedarf großer PNGs
            }

            Label {
                anchors.centerIn: parent
                visible: card.previewSource === "" || previewImg.status === Image.Error
                text: qsTr("Keine\nVorschau")
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 8
                color: Config.StaticData.palette.secondary.col400
            }
        }

        // Beschreibung
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: card.styleEntry.description || card.styleEntry.name || ""
                font.bold: true
                wrapMode: Text.WordWrap
                color: Config.StaticData.palette.secondary.col100
            }

            Label {
                Layout.fillWidth: true
                visible: !!card.styleEntry.maintainer && card.styleEntry.maintainer.length > 0
                text: qsTr("von %1").arg(card.styleEntry.maintainer || "")
                font.pointSize: 9
                wrapMode: Text.WordWrap
                color: Config.StaticData.palette.secondary.col400
            }

            Label {
                Layout.fillWidth: true
                visible: card.selected
                text: qsTr("✓ Ausgewählt")
                font.pointSize: 9
                font.bold: true
                color: Config.StaticData.chartColors[0]
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: card.clicked()
    }

    // Entfernen-Button (nur importierte Stile), oberhalb der Karten-MouseArea,
    // damit der Klick nicht gleichzeitig den Stil auswählt.
    Rectangle {
        visible: card.removable
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 8
        width: 28; height: 28; radius: 14
        color: removeArea.containsMouse ? Config.StaticData.palette.secondary.col500
                                        : Config.Theme.colorBox

        SvgIcon {
            anchors.centerIn: parent
            width: 16
            height: 16
            source: "../resources/close.svg"
        }

        MouseArea {
            id: removeArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: card.removeRequested()
        }

        ToolTip.visible: removeArea.containsMouse && !Config.Responsive.isMobile
                         && Config.Parameters.showTooltips
        ToolTip.delay: 600
        ToolTip.text: qsTr("Stil entfernen")
    }

    // Export-Button (als .zip teilen) – für ALLE Stile, auch mitgelieferte.
    // Liegt links neben dem Entfernen-Button, sofern dieser sichtbar ist.
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.rightMargin: 8 + (card.removable ? 34 : 0)
        width: 28; height: 28; radius: 14
        color: exportArea.containsMouse ? Config.StaticData.palette.secondary.col500
                                        : Config.Theme.colorBox

        SvgIcon {
            anchors.centerIn: parent
            width: 12
            height: 12
            source: "../resources/download.svg"
        }

        MouseArea {
            id: exportArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: card.exportRequested()
        }

        ToolTip.visible: exportArea.containsMouse && !Config.Responsive.isMobile
                         && Config.Parameters.showTooltips
        ToolTip.delay: 600
        ToolTip.text: qsTr("Stil als ZIP exportieren")
    }
}
