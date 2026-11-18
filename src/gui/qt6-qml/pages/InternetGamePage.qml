import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../pages"

Rectangle {
    id: internetGamePage
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    ColumnLayout {
        id: internetGamePageRows
        anchors.fill: parent

        Rectangle {
            id: internetGamePageContent
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: -38
            color: Config.StaticData.palette.secondary.col700

            // Label {
            //     id: internetGamePageLabel
            //     color: Config.StaticData.palette.secondary.col200
            //     text: qsTr("Internet Game")
            //     font.family: Config.StaticData.loadedFont.font.family
            //     font.pointSize: 14
            //     font.bold: true
            // }

            GamePage {}
        }
    }
}
