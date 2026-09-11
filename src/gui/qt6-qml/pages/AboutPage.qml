import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// About page – ported from the Qt widgets about dialog (aboutpokerthimpl).
// Tabs as in the widget client: About / Project / Thanks to / License /
// third party libraries. (The "Translation" tab there was a placeholder that
// was never filled and is dropped.)
Rectangle {
    id: aboutPage
    objectName: "aboutPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700
    // The reading area gets the focus when opening, so that the page is
    // scrollable without a mouse click (Qt.callLater: during the stack animation
    // the focus does not take).
    StackView.onActivated: Qt.callLater(panelScroll.forceActiveFocus)

    // Opens a link in the external browser. NOT Qt.openUrlExternally directly:
    // in the AppImage/bundle QDesktopServices inherits the bundled LD_LIBRARY_PATH →
    // xdg-open crashes. Lobby.openExternalUrl starts the host tools with a
    // cleaned environment (same reasoning as ChatBox/LobbyStatsBar).
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

    // ChangeLog (plain text from <AppDataDir>misc/ChangeLog) as rich text:
    // the version lines ("2026-08-13 version 2.1.7:") are set in bold,
    // everything else stays line by line. Deliberately without a colour of its
    // own – the gold accent is not readable on the light panel background.
    // The file is mirrored by CMake from the ChangeLog in the project root
    // directory.
    function changelogHtml() {
        const raw = SettingsManager.changelogText()
        if (!raw)
            return "<i>" + qsTr("No changelog available.") + "</i>"

        const lines = raw.split(/\r?\n/)
        var html = ""
        for (var i = 0; i < lines.length; ++i) {
            const line = lines[i]
            if (line.trim() === "") {
                html += "<br>"
                continue
            }
            const escaped = line.replace(/&/g, "&amp;")
                                .replace(/</g, "&lt;")
                                .replace(/>/g, "&gt;")
            if (/^\d{4}-\d{2}-\d{2}\s+version\s/.test(line))
                html += "<b>" + escaped + "</b><br>"
            else
                html += escaped + "<br>"
        }
        return html
    }

    // Scrollable text panel in the style of the logs preview. Links are – if
    // enabled – opened via TapHandler + linkAt(): onLinkActivated does not fire
    // reliably inside a Flickable (for details see ChatBox).
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
            // Reading without a mouse: arrow up/down scrolls the Flickable
            // itself, page up/down and Home/End are unknown to it – those are
            // added here. Deliberately a single Keys.onPressed instead of several
            // individual handlers: as soon as an item has a special key handler,
            // its onPressed no longer sees the key in question.
            Keys.onPressed: (event) => {
                var f = panelScroll.contentItem
                if (!f)
                    return
                var maxY = Math.max(0, f.contentHeight - f.height)
                if (event.key === Qt.Key_PageDown) {
                    f.contentY = Math.min(maxY, f.contentY + f.height * 0.9)
                    event.accepted = true
                } else if (event.key === Qt.Key_PageUp) {
                    f.contentY = Math.max(0, f.contentY - f.height * 0.9)
                    event.accepted = true
                } else if (event.key === Qt.Key_Home) {
                    f.contentY = 0
                    event.accepted = true
                } else if (event.key === Qt.Key_End) {
                    f.contentY = maxY
                    event.accepted = true
                }
            }

            anchors.margins: 8
            clip: true
            contentWidth: availableWidth
            // The vertical scrollbar lies AS AN OVERLAY above the content and
            // is not included in availableWidth. Without subtracting it, it cuts
            // into the text in a narrow window (same reservation as in
            // StyleSettings).
            readonly property real scrollBarSpace: ScrollBar.vertical.visible ? 12 : 0

            TextEdit {
                id: panelText
                width: panelScroll.availableWidth - panelScroll.scrollBarSpace
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
            model: [qsTr("About"), qsTr("Changelog"), qsTr("Project"),
                    qsTr("Thanks to"), qsTr("License"), qsTr("Third party libs")]
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: aboutTabBar.currentIndex

            // Tab: About – logo, version, feature list, copyright, project link
            ScrollView {
                id: aboutTab
                clip: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                // Room for the overlaying scrollbar (see TextPanel),
                // otherwise it lies on the right-aligned project link.
                readonly property real scrollBarSpace: ScrollBar.vertical.visible ? 12 : 0

                ColumnLayout {
                    width: aboutTab.availableWidth - aboutTab.scrollBarSpace
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

            // Tab: Changelog – ChangeLog of the project
            TextPanel {
                text: aboutPage.changelogHtml()
            }

            // Tab: Project – project page and authors (as in the widget client)
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
                    t += ind + "Arnaud Obscur (<a href='mailto:narmod@pokerth.net'>narmod@pokerth.net</a>)<br>"
                    t += role + qsTr("web client development") + "<br>"
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

            // Tab: License – AGPL text from <AppDataDir>/misc/agpl.html
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
