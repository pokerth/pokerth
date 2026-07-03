import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

Rectangle {
    id: aboutPagePage
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    ColumnLayout {
        id: aboutPagePageRows
        anchors.fill: parent

        Rectangle {
            id: aboutPagePageContent
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            Layout.margins: 16
            color: Config.StaticData.palette.secondary.col700

            AppLabel {
                id: aboutPagePageLabel
                color: Config.StaticData.palette.secondary.col200
                text: qsTr("Über PokerTH")
                font.pointSize: 14
                font.bold: true
            }
        }
    }
}
