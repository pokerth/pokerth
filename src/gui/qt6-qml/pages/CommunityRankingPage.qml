import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// Auswahl-Zwischenseite für die verschiedenen Ranglisten:
//   • PokerTH Ranking  (offizielle Rangliste, pokerth.net)
//   • BBC Ranking      (Best Brainies Cup, bbc.pokerth.net)
//   • WEC Ranking      (World Elite Cup, wec.pokerth.net)
Rectangle {
    id: communityPage
    objectName: "communityRankingPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: content.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Item {
            id: content
            width: parent.width
            implicitHeight: Math.max(communityPage.height, box.height + Config.Theme.margin * 2)

            ColumnLayout {
                id: box
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(content.width - Config.Theme.margin * 2, 420)
                spacing: Config.Theme.spacing

                Label {
                    text: qsTr("Community / Ranking")
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: Config.Theme.spacing
                    color: Config.StaticData.palette.secondary.col200
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pointSize: 16
                    font.bold: true
                }

                CustomButton {
                    text: qsTr("PokerTH Ranking")
                    Layout.fillWidth: true
                    Layout.preferredHeight: Config.Theme.touchTarget
                    onClicked: mainStackView.push("RankingPage.qml")
                }

                CustomButton {
                    text: qsTr("BBC Ranking")
                    Layout.fillWidth: true
                    Layout.preferredHeight: Config.Theme.touchTarget
                    onClicked: mainStackView.push("BbcRankingPage.qml")
                }

                CustomButton {
                    text: qsTr("WEC Ranking")
                    Layout.fillWidth: true
                    Layout.preferredHeight: Config.Theme.touchTarget
                    onClicked: mainStackView.push("WecRankingPage.qml")
                }
            }
        }
    }
}
