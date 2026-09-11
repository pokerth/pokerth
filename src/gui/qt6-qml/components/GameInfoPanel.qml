import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import "../config" as Config

// Content of the right info panel in the game: the tab bar "history" / "odds".
//   • History: the game history (log).
//   • Odds: 10 poker hand categories with a probability + a bar
//     (a port of the CardsChanceMonitor from the Qt widgets client).
// The data source is GameTable (GameHandler): gameLog, cardsChance,
// cardsChanceFolded. It is used both in the floating overlay (GameSidePanel) and
// in the permanently docked desktop mode.
ColumnLayout {
    id: root
    spacing: 8

    // Table theme colours (independent of the light/dark mode of the app – only
    // the table theme is authoritative). The game history text follows them as well: its
    // lines only carry colour roles, which the GameHandler fills with exactly these
    // values (chatcolors.h).
    readonly property color colText:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogText : "#eff1f5"
    readonly property color colTextMuted:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogTextMuted : "#7787a3"
    readonly property color colBorder:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogBorder : "#576378"
    readonly property color colSurface:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogSurface : "#394150"
    readonly property color colBackground:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogBackground : "#1d222b"
    readonly property color colAccent:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogAccent : "#E3C800"
    // Text ON the accent (highlighted text) – with light themes the accent is
    // dark, "#101010" would be unreadable there.
    readonly property color colAccentText:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogAccentText : "#101010"
    // Restrained but still clearly readable text for the impossible
    // odds lines. colTextMuted is the colour for placeholders/side matters –
    // applied to ten lines it made the whole list look greyed out.
    // Halfway between the main and the muted text: it dims without swallowing.
    readonly property color colTextDim: Qt.tint(root.colTextMuted,
                                                Config.Theme.withAlpha(root.colText, 0.45))

    // The active tab is controllable from outside (shortcuts/toggle): 0 history · 1 odds
    property alias currentIndex: tabs.currentIndex

    // Font size of the odds texts (category + percentage).
    // Overlay: 12 (the default), docked: 11 (set by the caller).
    property int messageFontSize: 12

    // The game history (log) runs as continuous text like the chat and is set
    // larger than the compact odds list – analogous to the ChatBox
    // (messageFontSize there). +2 keeps the log and the chat at the same size:
    // overlay 14, docked 13.
    property int logFontSize: messageFontSize + 2

    // ── Datenanbindung ────────────────────────────────────────────────────────
    readonly property var chance:
        (typeof GameTable !== "undefined" && GameTable) ? GameTable.cardsChance : []
    readonly property bool folded:
        (typeof GameTable !== "undefined" && GameTable) ? GameTable.cardsChanceFolded : false

    // Category index (0 = high card … 9 = royal flush) → name + SVG icon.
    // The order is identical to the cardsChance indexing in the GameHandler.
    readonly property var handDefs: [
        { name: qsTr("Höchste Karte"),  icon: "highcard" },
        { name: qsTr("Paar"),           icon: "onepair" },
        { name: qsTr("Zwei Paare"),     icon: "twopair" },
        { name: qsTr("Drilling"),       icon: "threeofakind" },
        { name: qsTr("Straße"),         icon: "straight" },
        { name: qsTr("Flush"),          icon: "flush" },
        { name: qsTr("Full House"),     icon: "fullhouse" },
        { name: qsTr("Vierling"),       icon: "fourofakind" },
        { name: qsTr("Straight Flush"), icon: "straightflush" },
        { name: qsTr("Royal Flush"),    icon: "royalflush" }
    ]

    function handIcon(cat) {
        return (cat >= 0 && cat < handDefs.length)
            ? "qrc:resources/hands/" + handDefs[cat].icon + ".svg" : ""
    }

    // Free space on the right for the vertical scrollbar, so that it never
    // overlaps the content (in the overlay style scrollbars otherwise lie above the text).
    readonly property int scrollGutter: 14

    // ── Tab-Leiste ────────────────────────────────────────────────────────────
    CustomTabBar {
        id: tabs
        // "History" and "odds" are always both reachable – the
        // card odds monitor is no longer switched off separately in the settings
        // but shown/hidden via the info panel toggle.
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        tabHeight: 18
        tabFontPointSize: 8
        // The bar sits on the panel surface of the table theme – its colours
        // have to come from there, not from the app palette.
        colTextActive: root.colText
        colTextIdle:   root.colTextMuted
        colTabActive:  root.colSurface
        colTabIdle:    Config.Theme.withAlpha(root.colBorder, 0.20)
        model: [qsTr("Verlauf"), qsTr("Chancen")]
    }

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: tabs.currentIndex

        // ── Tab "history" (game history / log) ────────────────────────────────
        // ONE contiguous rich text document (like the ChatBox), NOT a
        // ListView: with variably tall rich text delegates its contentHeight is
        // only estimated and fluctuates while scrolling (delegate
        // recycling) – that made the auto-scroll fire again and again and jam
        // at the bottom. With a Flickable over a deterministic contentHeight
        // (= TextEdit.implicitHeight) this is stable; in addition it gives
        // continuous mouse selection + copy/select all.
        Flickable {
            id: logFlick
            clip: true
            contentWidth: width
            contentHeight: logText.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick
            ScrollBar.vertical: ScrollBar {
                id: logScrollBar
                policy: logFlick.contentHeight > logFlick.height + 4
                        ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                // Dragging the handle sets contentY directly and produces NO
                // movementStarted/Ended signals – attach them by hand.
                onPressedChanged: pressed ? logFlick.userScrollStarted()
                                          : logFlick.userScrollEnded()
            }
            // Auto-scroll: it pauses when scrolling up, the position is kept on new
            // lines, and after 15 s of inactivity it goes back to the end.
            property bool autoScroll: true
            property real savedContentY: 0
            Timer {
                id: logAutoScrollTimer
                interval: 15000
                onTriggered: { logFlick.autoScroll = true; logFlick.scrollToBottom() }
            }
            // Stick to the end. pinBottom() checks autoScroll itself, so that a
            // deferred (Qt.callLater) call does nothing if the user has
            // scrolled away meanwhile.
            function pinBottom() {
                if (autoScroll) contentY = Math.max(0, contentHeight - height)
            }
            function scrollToBottom() { autoScroll = true; pinBottom() }
            function restoreScroll() {
                contentY = Math.min(savedContentY, Math.max(0, contentHeight - height))
            }
            // With auto-scroll, pull to the end TWICE: immediately (contentHeight is
            // already the new value in the change handler) AND once via Qt.callLater.
            // QQuickTextEdit only updates its implicitHeight in the polish
            // phase and QQuickFlickable its internal scroll limit there as well;
            // depending on the order the still old limit clamps the immediate setting
            // downwards – then the callLater after the polish takes effect. One of the
            // two always lands correctly, setting it twice has no consequence.
            function followBottom() {
                // Do not touch anything at all during a running user gesture.
                if (moving || logScrollBar.pressed)
                    return
                if (autoScroll) { pinBottom(); Qt.callLater(pinBottom) }
                // Paused: keep the remembered position while the text is replaced
                // completely.
                else restoreScroll()
            }
            // Hang off contentHeight: it fires on EVERY height change – a new line,
            // rich text lines wrapping asynchronously and a complete replacement of the text.
            onContentHeightChanged: followBottom()
            // A resize (e.g. a changed number of players) – treat it the same way.
            onHeightChanged: followBottom()

            // Derive the auto-scroll state ONLY from real user gestures (see the
            // detailed reasoning in ChatBox.qml): `moving` also applies to
            // the position correction that the Flickable performs itself on every
            // height change – its intermediate values must not switch off the
            // auto-scroll, otherwise the last line stays cut off.
            // angeschnitten stehen.
            function userScrollStarted() {
                autoScroll = false
                logAutoScrollTimer.stop()
            }
            function userScrollEnded() {
                savedContentY = contentY
                // Check with a tolerance instead of an exact atYEnd: close to the end is enough
                // (subpixels/rich text lines growing asynchronously), so that the
                // auto-scroll reliably kicks in again at the lower edge.
                autoScroll = contentY >= contentHeight - height - 4
                if (autoScroll) { logAutoScrollTimer.stop(); pinBottom() }
                else logAutoScrollTimer.restart()
            }
            onMovementStarted: userScrollStarted()
            onMovementEnded: userScrollEnded()

            // A read-only TextEdit holds the whole history as ONE HTML document
            // (GameLogModel.html – the lines concatenated with <br>). The lines are
            // already coloured brightly on the server side (GameHandler::formatLogLine).
            TextEdit {
                id: logText
                width: logFlick.width - root.scrollGutter
                text: (typeof GameTable !== "undefined" && GameTable)
                      ? GameTable.gameLog.html : ""
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.Wrap
                readOnly: true
                selectByMouse: true
                persistentSelection: true
                color: root.colText
                selectionColor: root.colAccent
                selectedTextColor: root.colAccentText
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: root.logFontSize
                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: logCtxMenu.popup()
                }
            }
        }

        // ── Tab „Chancen" ─────────────────────────────────────────────────────
        Flickable {
            id: chanceFlick
            contentHeight: chanceCol.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {
                policy: chanceFlick.contentHeight > chanceFlick.height + 2
                        ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }

            Column {
                id: chanceCol
                width: chanceFlick.width - root.scrollGutter
                spacing: 0

                // Royal flush at the top, high card at the bottom (as in the widgets client).
                Repeater {
                    model: 10
                    delegate: Item {
                        required property int index
                        // 9 → royal flush … 0 → high card
                        readonly property int cat: 9 - index
                        readonly property var entry:
                            (root.chance && root.chance.length > cat) ? root.chance[cat] : null
                        readonly property int prob: entry ? entry.prob : 0
                        readonly property bool possible: entry ? entry.possible : false

                        width: chanceCol.width
                        height: 26

                        // The probability bar as the row background – it costs
                        // no horizontal width, so that the name gets the full room
                        // (instead of being cut off early with "…").
                        // As in the chat: no bubbles/margins – the lines fill the
                        // full height without spacing and thereby form a continuous,
                        // uniform surface; only the fill bar stands out.
                        Rectangle {
                            anchors.fill: parent
                            color: Config.Theme.withAlpha(root.colBorder, 0.22)
                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: parent.width * Math.max(0, Math.min(100, prob)) / 100
                                color: possible
                                       ? Config.Theme.withAlpha(root.colAccent, 0.42)
                                       : Config.Theme.withAlpha(root.colBorder, 0.34)
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 4
                            anchors.rightMargin: 8
                            spacing: 8

                            Image {
                                Layout.preferredWidth: 38
                                Layout.preferredHeight: 22
                                Layout.alignment: Qt.AlignVCenter
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                                // Impossible categories stay restrained but
                                // readable – at 0.32 they almost vanished completely on light
                                // table themes.
                                opacity: possible ? 1.0 : 0.5
                                source: root.handIcon(cat)
                                sourceSize.width: Math.ceil(38 * Screen.devicePixelRatio)
                                sourceSize.height: Math.ceil(22 * Screen.devicePixelRatio)
                            }

                            AppText {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                text: root.handDefs[cat].name
                                // One line per category – compact; with too little room "…".
                                elide: Text.ElideRight
                                font.pixelSize: root.messageFontSize
                                color: possible ? root.colText : root.colTextDim
                            }

                            AppText {
                                Layout.preferredWidth: 40
                                Layout.alignment: Qt.AlignVCenter
                                horizontalAlignment: Text.AlignRight
                                text: prob + "%"
                                font.pixelSize: root.messageFontSize
                                font.bold: possible && prob >= 50
                                color: possible ? root.colText : root.colTextDim
                            }
                        }
                    }
                }
            }
        }
    }

    // ── Right click context menu for the history (copy / select all) ──
    // Styled uniformly, it follows the table theme colours (like the ChatBox).
    component CtxItem: MenuItem {
        height: visible ? implicitHeight : 0
        contentItem: Text {
            text: parent.text
            color: parent.enabled
                   ? (parent.highlighted ? root.colAccent : root.colText)
                   : root.colTextMuted
            font.family: Config.StaticData.loadedFont.font.family
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8
        }
        background: Rectangle {
            color: parent.highlighted ? root.colSurface : "transparent"
        }
    }

    Menu {
        id: logCtxMenu
        background: Rectangle {
            implicitWidth: 160
            color: root.colBackground
            border.width: 1
            border.color: root.colBorder
            radius: 6
        }

        CtxItem {
            text: qsTr("Kopieren")
            enabled: logText.selectedText.length > 0
            onTriggered: logText.copy()
        }
        CtxItem {
            text: qsTr("Alles auswählen")
            onTriggered: logText.selectAll()
        }
    }
}
