import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

Rectangle {
    id: startPage
    // Bind to the visible area (StackView below the top bar), not to the whole
    // window – otherwise the box is not centred vertically.
    width: mainStackView.width
    height: mainStackView.height
    color: "transparent"

    // Initial focus on the main action – as when opening a dialog. Qt.callLater,
    // because the focus would otherwise fizzle out during the stack animation.
    // Without a focus reason there is no focus frame; it appears on the first Tab.
    StackView.onActivated: Qt.callLater(internetGameButton.forceActiveFocus)

    Image {
        id: preLoaderBackground
        anchors.fill: parent
        source: "../resources/startWindowBackground.png"
        fillMode: Image.PreserveAspectCrop
    }

    // Inner margins of the box – identical to the login dialog (Config.Theme.margin).
    readonly property real hPad: Config.Theme.margin
    readonly property real vPad: Config.Theme.margin
    readonly property real innerSpacing: Config.Theme.spacing
    // Distance between the logo block and the button grid (column spacing).
    readonly property real contentSpacing: 20

    // ── Height budget: the footer always stays visible ────────────────────
    // At the bottom the room for the footer is reserved firmly; the box gives way
    // in this order: smaller logo → flatter buttons → two columns.
    // All comparison heights are computed from tokens and NOT from the
    // measured heights of the header/box – otherwise a binding loop would arise
    // (smaller logo → more room → larger logo → …).
    readonly property real footerReserve: Config.Theme.startFooterReserve
    readonly property real minButtonHeight: 36
    readonly property real boxBudget: height - Config.Theme.margin * 2 - footerReserve

    readonly property int buttonCount: Config.Parameters.showCommunityContent ? 6 : 5

    // The non-shrinking part of the box: inner margins + row spacings.
    function chromeHeight(rows) {
        return vPad * 2 + contentSpacing + (rows - 1) * innerSpacing
    }
    // Smallest possible box height (logo and buttons at their limit) – the basis for
    // the switching point to two columns and for the visibility of the footer.
    function minBoxHeight(rows) {
        return chromeHeight(rows)
               + Config.Theme.brandHeaderHeight(Config.Theme.brandLogoSizeMin)
               + rows * minButtonHeight
    }

    // ── Two column button mode ────────────────────────────────────────────
    // If the room is not enough even with the smallest logo and flat buttons, the
    // buttons are arranged in two columns instead of scrolling vertically –
    // provided the wider box fits horizontally.
    readonly property real twoColumnBoxWidth: 620
    readonly property bool twoColumns:
        minBoxHeight(buttonCount) > boxBudget
        && width >= twoColumnBoxWidth + Config.Theme.margin * 2
    readonly property int buttonRows: twoColumns ? Math.ceil(buttonCount / 2) : buttonCount

    // The buttons stay at touch size as long as the logo can compensate for the
    // room; only when that is at its minimum do they get flatter.
    readonly property real buttonHeight:
        Math.max(minButtonHeight,
                 Math.min(Config.Theme.touchTarget,
                          (boxBudget - chromeHeight(buttonRows)
                           - Config.Theme.brandHeaderHeight(Config.Theme.brandLogoSizeMin))
                          / buttonRows))
    // The logo gets what is left after the buttons and the spacings – capped at
    // the regular size (Config.Theme.brandLogoSize, as in the login dialog).
    readonly property real logoSize:
        Math.max(Config.Theme.brandLogoSizeMin,
                 Math.min(Config.Theme.brandLogoSize,
                          Config.Theme.brandHeaderLogoForHeight(
                              boxBudget - chromeHeight(buttonRows)
                              - buttonRows * buttonHeight)))

    Flickable {
        id: startScroll
        anchors.fill: parent
        // The lower edge stays free for the footer – the box centres itself in the
        // remaining area instead of covering the footer.
        anchors.bottomMargin: startPage.footerReserve
        contentWidth: width
        contentHeight: startContent.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Item {
            id: startContent
            width: startScroll.width
            // Minimum height = viewport → the box stays vertically centred as long as it
            // fits; otherwise it can be scrolled.
            implicitHeight: Math.max(startScroll.height,
                                     startPageMainButtonsBox.height + Config.Theme.margin * 2)

            // ── Overlay box: contains the logo + navigation buttons ──────────
            Rectangle {
                id: startPageMainButtonsBox
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(startContent.width - Config.Theme.margin * 2,
                                startPage.twoColumns ? startPage.twoColumnBoxWidth
                                                     : Config.Theme.brandBoxWidth)
                // The height follows the content (logo + buttons) incl. the same inner
                // margin at the top and the bottom – that way the box stays vertically
                // centred with any number of buttons instead of running out at the bottom.
                height: startBoxContent.implicitHeight + startPage.vPad * 2
                color: "transparent"

                // Dark background – always dark so that the contrast to the fire
                // background is right, independently of the light/dark theme.
                Rectangle {
                    anchors.fill: parent
                    color: "#1d222b"
                    opacity: 0.88
                    radius: 5
                }

                Column {
                    id: startBoxContent
                    // The icon is positioned fixed at the upper edge of the box (Config.Theme.margin)
                    // – identical to the login dialog. The box itself is centred vertically
                    // in the window.
                    anchors {
                        left: parent.left; right: parent.right; top: parent.top
                        leftMargin: startPage.hPad
                        rightMargin: startPage.hPad
                        topMargin: startPage.vPad
                    }
                    spacing: startPage.contentSpacing

                    // ── PokerTH-Logo + Kartensymbole ─────────────────────────
                    BrandHeader {
                        id: brandHeader
                        anchors.horizontalCenter: parent.horizontalCenter
                        logoSize: startPage.logoSize
                    }

                    // ── Navigation buttons ────────────────────────────────────
                    // The same preferredWidth on all buttons → in two column
                    // mode both columns get exactly the same width.
                    GridLayout {
                        id: startPageMainButtons
                        width: parent.width
                        columns: startPage.twoColumns ? 2 : 1
                        columnSpacing: startPage.innerSpacing
                        rowSpacing: startPage.innerSpacing

                        CustomButton {
                            id: internetGameButton
                            text: qsTr("Internetspiel")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: startPage.buttonHeight
                            onClicked: mainStackView.push("ServerConnectionDialog.qml")
                        }

                        CustomButton {
                            text: qsTr("Lokales Spiel starten")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: startPage.buttonHeight
                            onClicked: mainStackView.push("LocalGamePage.qml")
                        }

                        CustomButton {
                            text: qsTr("Netzwerkspiel erstellen")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: startPage.buttonHeight
                            onClicked: mainStackView.push("NetworkGameCreatePage.qml")
                        }

                        CustomButton {
                            text: qsTr("Netzwerkspiel beitreten")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: startPage.buttonHeight
                            onClicked: mainStackView.push("NetworkGameEnterPage.qml")
                        }

                        CustomButton {
                            text: qsTr("Community / Ranking")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: startPage.buttonHeight
                            visible: Config.Parameters.showCommunityContent
                            // Via the same toggle as the globe → the remembered
                            // ranking state is restored.
                            onClicked: mainWindow.toggleTopBarSection(
                                "pages/CommunityRankingPage.qml",
                                mainWindow.rankingSectionPages, true)
                        }

                        CustomButton {
                            text: qsTr("Logs")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: startPage.buttonHeight
                            onClicked: mainStackView.push("LogsPage.qml")
                        }
                    }
                }
            }
        }
    }

    // ── Footer: community links + license/source ──────────────────────────
    // It sits in the strip reserved at the bottom (footerReserve) that the Flickable
    // leaves free – so it never covers the box. It is only hidden
    // when the box itself does not fit any more even with the smallest logo and flat
    // buttons: then it scrolls and the strip is needed as well.
    StartFooter {
        id: startFooter
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: implicitHeight
        visible: startPage.boxBudget >= startPage.minBoxHeight(startPage.buttonRows)
    }
}
