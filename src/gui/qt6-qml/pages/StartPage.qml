import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

Rectangle {
    id: startPage
    // An den sichtbaren Bereich (StackView unterhalb der Topbar) binden, nicht
    // an das ganze Fenster – sonst ist die Box vertikal nicht zentriert.
    width: mainStackView.width
    height: mainStackView.height
    color: "transparent"

    Image {
        id: preLoaderBackground
        anchors.fill: parent
        source: "../resources/startWindowBackground.png"
        fillMode: Image.PreserveAspectCrop
    }

    // Innenabstände der Box – identisch zum Login-Dialog (Config.Theme.margin).
    readonly property real hPad: Config.Theme.margin
    readonly property real vPad: Config.Theme.margin
    readonly property real innerSpacing: Config.Theme.spacing

    Flickable {
        id: startScroll
        anchors.fill: parent
        contentWidth: width
        contentHeight: startContent.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Item {
            id: startContent
            width: startScroll.width
            // Mindesthöhe = Viewport → Box bleibt vertikal zentriert, solange sie
            // passt; sonst kann gescrollt werden.
            implicitHeight: Math.max(startScroll.height,
                                     startPageMainButtonsBox.height + Config.Theme.margin * 2)

            // ── Overlay-Box: enthält Logo + Navigations-Buttons ──────────────
            Rectangle {
                id: startPageMainButtonsBox
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(startContent.width - Config.Theme.margin * 2, Config.Theme.brandBoxWidth)
                height: Config.Theme.brandBoxHeight
                color: "transparent"

                // Dunkler Hintergrund – immer dunkel damit der Kontrast zum Feuer-
                // Hintergrund stimmt, unabhängig vom Hell/Dunkel-Theme.
                Rectangle {
                    anchors.fill: parent
                    color: "#1d222b"
                    opacity: 0.88
                    radius: 5
                }

                Column {
                    id: startBoxContent
                    // Icon fix am oberen Rand der Box positioniert (Config.Theme.margin)
                    // – identisch zum Login-Dialog. Die Box selbst ist im Fenster
                    // vertikal zentriert.
                    anchors {
                        left: parent.left; right: parent.right; top: parent.top
                        leftMargin: startPage.hPad
                        rightMargin: startPage.hPad
                        topMargin: startPage.vPad
                    }
                    spacing: 20

                    // ── PokerTH-Logo + Kartensymbole ─────────────────────────
                    BrandHeader {
                        anchors.horizontalCenter: parent.horizontalCenter
                        logoSize: Config.Theme.brandLogoSize
                    }

                    // ── Navigations-Buttons ───────────────────────────────────
                    ColumnLayout {
                        id: startPageMainButtons
                        width: parent.width
                        spacing: startPage.innerSpacing

                        CustomButton {
                            text: qsTr("Internetspiel")
                            Layout.fillWidth: true
                            Layout.preferredHeight: Config.Theme.touchTarget
                            onClicked: mainStackView.push("ServerConnectionDialog.qml")
                        }

                        CustomButton {
                            text: qsTr("Lokales Spiel starten")
                            Layout.fillWidth: true
                            Layout.preferredHeight: Config.Theme.touchTarget
                            onClicked: mainStackView.push("LocalGamePage.qml")
                        }

                        CustomButton {
                            text: qsTr("Netzwerkspiel erstellen")
                            Layout.fillWidth: true
                            Layout.preferredHeight: Config.Theme.touchTarget
                            onClicked: mainStackView.push("NetworkGameCreatePage.qml")
                        }

                        CustomButton {
                            text: qsTr("Netzwerkspiel beitreten")
                            Layout.fillWidth: true
                            Layout.preferredHeight: Config.Theme.touchTarget
                            onClicked: mainStackView.push("NetworkGameEnterPage.qml")
                        }

                        CustomButton {
                            text: qsTr("Logs")
                            Layout.fillWidth: true
                            Layout.preferredHeight: Config.Theme.touchTarget
                            onClicked: mainStackView.push("LogsPage.qml")
                        }
                    }
                }
            }
        }
    }
}
