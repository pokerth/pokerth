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

    // ── Zweispaltiger Button-Modus ────────────────────────────────────────
    // Passt die Box mit einspaltigen Buttons (inkl. Außenabstand) nicht in den
    // sichtbaren Bereich (Android-Querformat, flache Desktop-Fenster), werden
    // die Buttons zweispaltig angeordnet, statt vertikal zu scrollen –
    // vorausgesetzt, die breitere Box passt horizontal. Die Vergleichshöhe
    // wird aus den Design-Tokens berechnet statt aus der gemessenen Box-Höhe,
    // sonst entstünde eine Binding-Schleife (zweispaltig → Box passt →
    // wieder einspaltig → …).
    readonly property int buttonCount: Config.Parameters.showCommunityContent ? 6 : 5
    readonly property real singleColumnBoxHeight:
        vPad * 2 + brandHeader.implicitHeight + startBoxContent.spacing
        + buttonCount * Config.Theme.touchTarget
        + (buttonCount - 1) * innerSpacing
    readonly property real twoColumnBoxWidth: 620
    readonly property bool twoColumns:
        singleColumnBoxHeight + Config.Theme.margin * 2 > height
        && width >= twoColumnBoxWidth + Config.Theme.margin * 2

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
                width: Math.min(startContent.width - Config.Theme.margin * 2,
                                startPage.twoColumns ? startPage.twoColumnBoxWidth
                                                     : Config.Theme.brandBoxWidth)
                // Höhe folgt dem Inhalt (Logo + Buttons) inkl. oben/unten gleichem
                // Innenabstand – so bleibt die Box bei beliebig vielen Buttons
                // vertikal zentriert, statt unten herauszulaufen.
                height: startBoxContent.implicitHeight + startPage.vPad * 2
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
                        id: brandHeader
                        anchors.horizontalCenter: parent.horizontalCenter
                        logoSize: Config.Theme.brandLogoSize
                    }

                    // ── Navigations-Buttons ───────────────────────────────────
                    // Gleiche preferredWidth auf allen Buttons → im zweispaltigen
                    // Modus bekommen beide Spalten exakt dieselbe Breite.
                    GridLayout {
                        id: startPageMainButtons
                        width: parent.width
                        columns: startPage.twoColumns ? 2 : 1
                        columnSpacing: startPage.innerSpacing
                        rowSpacing: startPage.innerSpacing

                        CustomButton {
                            text: qsTr("Internetspiel")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: Config.Theme.touchTarget
                            onClicked: mainStackView.push("ServerConnectionDialog.qml")
                        }

                        CustomButton {
                            text: qsTr("Lokales Spiel starten")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: Config.Theme.touchTarget
                            onClicked: mainStackView.push("LocalGamePage.qml")
                        }

                        CustomButton {
                            text: qsTr("Netzwerkspiel erstellen")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: Config.Theme.touchTarget
                            onClicked: mainStackView.push("NetworkGameCreatePage.qml")
                        }

                        CustomButton {
                            text: qsTr("Netzwerkspiel beitreten")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: Config.Theme.touchTarget
                            onClicked: mainStackView.push("NetworkGameEnterPage.qml")
                        }

                        CustomButton {
                            text: qsTr("Community / Ranking")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: Config.Theme.touchTarget
                            visible: Config.Parameters.showCommunityContent
                            // Über denselben Toggle wie der Globus → gemerkter
                            // Ranking-Stand wird wiederhergestellt.
                            onClicked: mainWindow.toggleTopBarSection(
                                "pages/CommunityRankingPage.qml",
                                mainWindow.rankingSectionPages, true)
                        }

                        CustomButton {
                            text: qsTr("Logs")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: Config.Theme.touchTarget
                            onClicked: mainStackView.push("LogsPage.qml")
                        }
                    }
                }
            }
        }
    }

    // ── Fußzeile: Community-Links + Lizenz/Quelle ─────────────────────────
    // Liegt als Overlay über der Flickable (wie im pokerth-web-client), damit
    // die Branding-Box exakt so zentriert bleibt wie im Login-Dialog. Sie wird
    // nur eingeblendet, wenn unter der Box genug Platz bleibt – auf niedrigen /
    // Querformat-Fenstern entfällt sie (Vorbild: Media-Query des Web-Clients).
    StartFooter {
        id: startFooter
        anchors {
            left: parent.left; right: parent.right; bottom: parent.bottom
            bottomMargin: Config.Theme.compact ? 8 : 12
        }
        visible: startPage.height - startPageMainButtonsBox.height
                 >= (startFooter.implicitHeight + Config.Theme.spacing) * 2
    }
}
