import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// Über-Seite – portiert aus dem Qt-Widgets About-Dialog (aboutpokerthimpl).
// Tabs wie im Widget-Client: Über / Projekt / Dank an / Lizenz /
// Drittanbieter-Bibliotheken. (Der dortige "Translation"-Tab war ein nie
// gefüllter Platzhalter und entfällt.)
Rectangle {
    id: aboutPage
    objectName: "aboutPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    // Öffnet einen Link im externen Browser. NICHT direkt Qt.openUrlExternally:
    // Im AppImage/Bundle erbt QDesktopServices das gebundelte LD_LIBRARY_PATH →
    // xdg-open crasht. Lobby.openExternalUrl startet die Host-Tools mit
    // bereinigter Umgebung (gleiche Begründung wie ChatBox/LobbyStatsBar).
    function openLink(link) {
        if (!link || link === "")
            return
        var opened = false
        if (typeof Lobby !== "undefined" && Lobby)
            opened = Lobby.openExternalUrl(link)
        if (!opened)
            opened = Qt.openUrlExternally(link)
        if (!opened)
            console.warn("AboutPage: konnte URL nicht öffnen:", link)
    }

    // Scrollbares Text-Panel im Stil der Logs-Vorschau. Links werden – wenn
    // aktiviert – über TapHandler + linkAt() geöffnet: onLinkActivated feuert
    // innerhalb einer Flickable nicht zuverlässig (Details siehe ChatBox).
    component TextPanel: Rectangle {
        id: panel

        property alias text: panelText.text
        property alias textFormat: panelText.textFormat
        property bool linksEnabled: false

        Layout.fillWidth: true
        Layout.fillHeight: true
        color: Config.StaticData.palette.secondary.col600
        border.color: Config.StaticData.palette.secondary.col500
        border.width: 1
        radius: 4

        ScrollView {
            id: panelScroll
            anchors.fill: parent
            anchors.margins: 8
            clip: true
            contentWidth: availableWidth

            TextEdit {
                id: panelText
                width: panelScroll.availableWidth
                readOnly: true
                selectByMouse: true
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.WordWrap
                color: Config.StaticData.palette.secondary.col100
                selectionColor: Config.Theme.colorAccent
                selectedTextColor: "#101010"
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 13

                HoverHandler {
                    enabled: panel.linksEnabled
                    cursorShape: panelText.hoveredLink !== ""
                                 ? Qt.PointingHandCursor : Qt.IBeamCursor
                }
                TapHandler {
                    id: panelLinkTap
                    enabled: panel.linksEnabled
                    acceptedButtons: Qt.LeftButton
                    onTapped: {
                        const link = panelText.linkAt(panelLinkTap.point.position.x,
                                                      panelLinkTap.point.position.y)
                        if (link !== "")
                            aboutPage.openLink(link)
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        AppLabel {
            text: qsTr("About PokerTH")
            color: Config.StaticData.palette.secondary.col200
            font.pointSize: 14
            font.bold: true
        }

        CustomTabBar {
            id: aboutTabBar
            model: [qsTr("About"), qsTr("Project"), qsTr("Thanks to"),
                    qsTr("License"), qsTr("Third party libs")]
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: aboutTabBar.currentIndex

            // Tab: Über – Logo, Version, Feature-Liste, Copyright, Projektlink
            ScrollView {
                id: aboutTab
                clip: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    width: aboutTab.availableWidth
                    spacing: 12

                    BrandHeader {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 12
                    }

                    AppText {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("PokerTH %1").arg(SettingsManager.appVersion())
                        color: Config.StaticData.palette.secondary.col100
                        font.pointSize: 14
                        font.bold: true
                    }

                    AppText {
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        wrapMode: Text.WordWrap
                        color: Config.StaticData.palette.secondary.col100
                        font.pixelSize: Config.Theme.fontSizeBody
                        text: [
                            qsTr("- Poker engine for the popular Texas Hold'em Poker"),
                            qsTr("- Singleplayer games with up to 9 computer-opponents"),
                            qsTr("- Multiplayer network games"),
                            qsTr("- Internet online games"),
                            qsTr("- Changeable gui with online style gallery"),
                            qsTr("- Online ranking website with result tables")
                        ].join("\n")
                    }

                    AppText {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: Config.StaticData.palette.secondary.col200
                        font.pixelSize: Config.Theme.fontSizeBody
                        text: "(c)2006-" + new Date().getFullYear()
                              + ", Kai Philipp, Felix Hammer, Florian Thauer, Lothar May"
                    }

                    AppText {
                        id: projectLink
                        Layout.alignment: Qt.AlignRight
                        Layout.bottomMargin: 12
                        textFormat: Text.RichText
                        text: "<a href='https://www.pokerth.net'>https://www.pokerth.net</a>"
                        font.pixelSize: Config.Theme.fontSizeBody

                        HoverHandler {
                            cursorShape: projectLink.hoveredLink !== ""
                                         ? Qt.PointingHandCursor : Qt.ArrowCursor
                        }
                        TapHandler {
                            onTapped: aboutPage.openLink("https://www.pokerth.net")
                        }
                    }
                }
            }

            // Tab: Projekt – Projektseite und Autoren (wie Widget-Client)
            TextPanel {
                linksEnabled: true
                text: {
                    var ind = "&nbsp;&nbsp;&nbsp;&nbsp;"
                    var role = ind + ind + "- "
                    var t = "<b>" + qsTr("Project page:") + "</b><br>"
                    t += ind + "<a href='https://www.pokerth.net'>https://www.pokerth.net</a><br>"
                    t += "<b>" + qsTr("Authors:") + "</b><br>"
                    t += ind + "Felix Hammer (<a href='mailto:doitux@pokerth.net'>doitux@pokerth.net</a>)<br>"
                    t += role + qsTr("initial idea, basic architecture, gui implementation, gui graphics editing, linux package") + "<br>"
                    t += ind + "Florian Thauer (<a href='mailto:floty@pokerth.net'>floty@pokerth.net</a>)<br>"
                    t += role + qsTr("initial idea, basic architecture, engine development") + "<br>"
                    t += ind + "Lothar May (<a href='mailto:lotodore@pokerth.net'>lotodore@pokerth.net</a>)<br>"
                    t += role + qsTr("basic architecture, network development, windows package, MacOS package") + "<br>"
                    t += ind + "Oskar Lindqvist (<a href='mailto:tranberry@pokerth.net'>tranberry@pokerth.net</a>)<br>"
                    t += role + qsTr("initial gui graphics design") + "<br>"
                    t += ind + "Kai Philipp (<a href='mailto:kphilipp@inquies.de'>kphilipp@inquies.de</a>)<br>"
                    t += role + qsTr("code modernization, QML layout") + "<br>"
                    return t
                }
            }

            // Tab: Dank an
            TextPanel {
                text: [
                    qsTr("- Wikimedia Commons: for different popular avatar picture resources"),
                    qsTr("- Benedikt, Erhard, Felix, Florian, Linus, Lothar, Steffi, Caro: for people avatar pictures"),
                    qsTr("- ZeiZei: for misc avatar pictures"),
                    qsTr("- kde-look.org: for different gpl licensed sounds"),
                    qsTr("- doc_dos: for self recorded chip sounds"),
                    qsTr("- thiger, dunkanx, BerndA, coldz, drull: for different patches"),
                    qsTr("- kraut: for internet-game-server hosting and administration"),
                    qsTr("- danuxi: for startwindow background gfx and danuxi1 table background"),
                    qsTr("- heyn: for moderating forum and organise bugtracker and feature requests"),
                    qsTr("- texas_outlaw: for new table sounds")
                ].join("<br>")
            }

            // Tab: Lizenz – AGPL-Text aus <AppDataDir>/misc/agpl.html
            TextPanel {
                text: SettingsManager.licenseHtml()
            }

            // Tab: Drittanbieter-Bibliotheken – <AppDataDir>/misc/third_party_libs.txt
            TextPanel {
                textFormat: TextEdit.PlainText
                text: SettingsManager.thirdPartyLibsText()
            }
        }
    }
}
