import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Effects
import QtQuick.Layouts

import "../config" as Config
import "../components"

// Forum news – the latest posts from www.pokerth.net, as in the
// web client (the "Forum news" window there). A tap opens the post
// IN the app (ForumPostPage), not in the browser.
//
// Second tab (switch top right): the BBC game dates of the next days with
// the number of registered players; a row with registrations expands to the
// list of nicknames. Registering itself happens on the BBC site.
//
// Data, deduplication and read status live in Config.ForumNews, the game
// dates in Config.BbcGameDates.
Rectangle {
    id: forumPage
    objectName: "forumNewsPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    readonly property bool compact: Config.Responsive.compact
    readonly property bool listEmpty: Config.ForumNews.posts.length === 0

    // "forum" | "bbc"
    property string tab: "forum"
    readonly property bool bbcTab: tab === "bbc"

    // Expanded BBC games by id – kept here, not in the delegate, so that the
    // state survives a refresh of the list (the delegates are recreated).
    property var expandedGames: ({})

    function toggleGame(game) {
        if (!game || game.num <= 0)
            return
        var m = Object.assign({}, expandedGames)
        if (m[game.id])
            delete m[game.id]
        else
            m[game.id] = true
        expandedGames = m
    }

    // Refresh when opening; the TTL in the singleton prevents every
    // open from triggering a fetch.
    Component.onCompleted: Config.ForumNews.refresh(false)
    onTabChanged: {
        if (tab === "bbc")
            Config.BbcGameDates.refresh(false)
    }

    // Step colours as on the BBC calendar (info/primary/success/warning/danger).
    function stepColor(step) {
        switch (step) {
        case 1: return Config.Theme.isDark ? Config.Theme.colorAccent : Config.Theme.colorAccentDim
        case 2: return Config.Theme.colorSuccess
        case 3: return "#e89a30"
        case 4: return Config.Theme.colorDanger
        default: return Config.Theme.colorTimeoutSelf
        }
    }

    function openPost(post) {
        if (post)
            mainStackView.push("ForumPostPage.qml", { post: post })
    }

    // Opens a link in the external browser. NOT Qt.openUrlExternally directly:
    // in the AppImage/bundle QDesktopServices inherits the bundled LD_LIBRARY_PATH →
    // xdg-open crashes (same reasoning as ChatBox/AboutPage).
    function openExternal(link) {
        if (!link || link === "")
            return
        var opened = false
        if (typeof Lobby !== "undefined" && Lobby)
            opened = Lobby.openExternalUrl(link)
        if (!opened)
            opened = Qt.openUrlExternally(link)
        if (!opened)
            console.warn("ForumNewsPage: konnte URL nicht öffnen:", link)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            AppLabel {
                text: forumPage.bbcTab ? qsTr("BBC games") : qsTr("Forum news")
                Layout.fillWidth: true
                elide: Text.ElideRight
                color: Config.StaticData.palette.secondary.col200
                font.pointSize: 14
                font.bold: true
            }

            BusyIndicator {
                running: forumPage.bbcTab ? Config.BbcGameDates.loading
                                          : Config.ForumNews.loading
                visible: running
                implicitWidth: 22
                implicitHeight: 22
            }

            SegmentedSwitch {
                objectName: "forumNewsTabSwitch"
                Layout.alignment: Qt.AlignVCenter
                model: [
                    { key: "forum", label: qsTr("Forum") },
                    { key: "bbc",   label: qsTr("BBC games") }
                ]
                current: forumPage.tab
                onSelected: function(key) { forumPage.tab = key }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: forumPage.bbcTab ? 1 : 0

            Rectangle {
                color: Config.StaticData.palette.secondary.col600
                border.color: Config.StaticData.palette.secondary.col500
                border.width: 1
                radius: 4

                ListView {
                    id: postList
                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true
                    model: Config.ForumNews.posts
                    boundsBehavior: Flickable.StopAtBounds

                    // Keyboard operation: Tab leads into the list, the arrows change the
                    // row, Enter opens the post (like a click).
                    activeFocusOnTab: true
                    keyNavigationEnabled: true
                    // Directly from the model instead of via currentItem: a ListView
                    // only creates the visible delegates, so currentItem can be
                    // null.
                    function openCurrent() {
                        var post = Config.ForumNews.posts[postList.currentIndex]
                        if (post)
                            forumPage.openPost(post)
                    }
                    Keys.onReturnPressed: postList.openCurrent()
                    Keys.onEnterPressed: postList.openCurrent()
                    ScrollBar.vertical: ScrollBar {
                        policy: postList.contentHeight > postList.height + 4
                                ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                    }

                    delegate: Item {
                        id: postDelegate
                        required property int index
                        required property var modelData

                        // Read readRevision so that the row is re-evaluated after
                        // "read" (a function call alone creates no binding
                        // dependency).
                        readonly property bool unread: {
                            var _rev = Config.ForumNews.readRevision
                            return Config.ForumNews.isUnread(modelData)
                        }

                        width: ListView.view.width
                        height: Math.max(forumPage.compact ? 58 : 50,
                                         textColumn.implicitHeight + 16)

                        // Current row of the keyboard navigation – only while the
                        // list has the focus, otherwise the mouse user would see a
                        // highlight they never touched.
                        readonly property bool keyboardCurrent: ListView.isCurrentItem
                                                                && postList.activeFocus

                        Rectangle {
                            anchors.fill: parent
                            color: rowHover.hovered || postDelegate.keyboardCurrent
                                   ? Config.Theme.colorHover
                                   : (postDelegate.index % 2 === 0
                                      ? Config.Theme.colorBox
                                      : Config.StaticData.palette.secondary.col600)

                            // The accent stripe on the left marks the keyboard selection.
                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: 3
                                visible: postDelegate.keyboardCurrent
                                color: Config.Theme.colorAccent
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 1
                                color: Config.StaticData.palette.secondary.col500
                                opacity: 0.5
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: postList.contentHeight > postList.height + 4 ? 16 : 10
                            anchors.topMargin: 6
                            anchors.bottomMargin: 6
                            spacing: 10

                            ForumBadge {
                                forum: postDelegate.modelData.forum || ""
                                Layout.alignment: Qt.AlignVCenter
                            }

                            ColumnLayout {
                                id: textColumn
                                Layout.fillWidth: true
                                spacing: 1

                                AppText {
                                    Layout.fillWidth: true
                                    text: postDelegate.modelData.title || ""
                                    elide: Text.ElideRight
                                    color: postDelegate.unread
                                           ? Config.StaticData.palette.secondary.col100
                                           : Config.StaticData.palette.secondary.col200
                                    font.pixelSize: Config.Theme.fontSizeBody
                                    font.bold: postDelegate.unread
                                }

                                AppText {
                                    Layout.fillWidth: true
                                    text: {
                                        var a = postDelegate.modelData.author || ""
                                        var d = Config.ForumNews.formatDate(postDelegate.modelData.ts)
                                        return a !== "" && d !== "" ? a + " · " + d : a + d
                                    }
                                    elide: Text.ElideRight
                                    color: Config.StaticData.palette.secondary.col400
                                    font.pixelSize: Config.Theme.fontSizeCaption
                                }
                            }

                            // State dot: filled = unread, empty ring = read
                            // (like .fn-dot / .fn-dot-read in the web client).
                            Rectangle {
                                Layout.alignment: Qt.AlignVCenter
                                implicitWidth: 9
                                implicitHeight: 9
                                radius: 4.5
                                color: postDelegate.unread ? Config.Theme.colorAccent : "transparent"
                                border.color: Config.Theme.colorAccentDim
                                border.width: postDelegate.unread ? 0 : 1.5
                                opacity: postDelegate.unread ? 1 : 0.55
                            }
                        }

                        HoverHandler {
                            id: rowHover
                            cursorShape: Qt.PointingHandCursor
                        }
                        TapHandler {
                            onTapped: forumPage.openPost(postDelegate.modelData)
                        }
                    }
                }

                AppLabel {
                    anchors.centerIn: parent
                    width: parent.width - 32
                    visible: forumPage.listEmpty && !Config.ForumNews.loading
                    text: Config.ForumNews.errorText !== ""
                          ? Config.ForumNews.errorText : qsTr("No entries.")
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    color: Config.ForumNews.errorText !== ""
                           ? Config.Theme.colorDanger
                           : Config.StaticData.palette.secondary.col300
                    font.pixelSize: Config.Theme.fontSizeBody
                }
            }

            // BBC game dates, grouped by (local) day.
            Rectangle {
                color: Config.StaticData.palette.secondary.col600
                border.color: Config.StaticData.palette.secondary.col500
                border.width: 1
                radius: 4

                ListView {
                    id: gameList
                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true
                    model: Config.BbcGameDates.games
                    boundsBehavior: Flickable.StopAtBounds
                    activeFocusOnTab: true
                    keyNavigationEnabled: true
                    Keys.onReturnPressed: forumPage.toggleGame(Config.BbcGameDates.games[gameList.currentIndex])
                    Keys.onEnterPressed: forumPage.toggleGame(Config.BbcGameDates.games[gameList.currentIndex])
                    ScrollBar.vertical: ScrollBar {
                        policy: gameList.contentHeight > gameList.height + 4
                                ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                    }

                    delegate: Column {
                        id: gameDelegate
                        required property int index
                        required property var modelData

                        readonly property string day: modelData.day
                        readonly property bool firstOfDay: {
                            if (index === 0)
                                return true
                            var prev = Config.BbcGameDates.games[index - 1]
                            return !prev || prev.day !== day
                        }
                        readonly property bool full: modelData.num >= Config.BbcGameDates.maxPlayers
                        readonly property bool keyboardCurrent: ListView.isCurrentItem
                                                                && gameList.activeFocus
                        readonly property bool expandable: modelData.num > 0
                        readonly property bool expanded: expandable
                                                         && forumPage.expandedGames[modelData.id] === true
                        readonly property var regs: {
                            var _rev = Config.BbcGameDates.regsRevision
                            return Config.BbcGameDates.regsFor(modelData.id)
                        }

                        // Load on expanding – also when a refreshed list recreates
                        // an expanded row (the player count may have changed).
                        onExpandedChanged: {
                            if (expanded)
                                Config.BbcGameDates.loadRegs(modelData)
                        }
                        Component.onCompleted: {
                            if (expanded)
                                Config.BbcGameDates.loadRegs(modelData)
                        }

                        width: ListView.view.width

                        // Day header
                        Rectangle {
                            visible: gameDelegate.firstOfDay
                            width: parent.width
                            height: visible ? dayLabel.implicitHeight + 10 : 0
                            color: Config.StaticData.palette.secondary.col500

                            AppText {
                                id: dayLabel
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                text: Config.BbcGameDates.dayLabel(gameDelegate.day)
                                elide: Text.ElideRight
                                color: Config.StaticData.palette.secondary.col100
                                font.pixelSize: Config.Theme.fontSizeCaption
                                font.bold: true
                            }
                        }

                        Item {
                            width: parent.width
                            height: forumPage.compact ? 44 : 38

                            Rectangle {
                                anchors.fill: parent
                                color: gameHover.hovered || gameDelegate.keyboardCurrent
                                       ? Config.Theme.colorHover
                                       : (gameDelegate.index % 2 === 0
                                          ? Config.Theme.colorBox
                                          : Config.StaticData.palette.secondary.col600)

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    width: 3
                                    visible: gameDelegate.keyboardCurrent
                                    color: Config.Theme.colorAccent
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: gameList.contentHeight > gameList.height + 4 ? 16 : 10
                                spacing: 10

                                AppText {
                                    Layout.preferredWidth: Math.max(46, implicitWidth)
                                    text: Config.BbcGameDates.timeText(gameDelegate.modelData.ts)
                                    color: Config.StaticData.palette.secondary.col100
                                    font.pixelSize: Config.Theme.fontSizeBody
                                    font.bold: true
                                }

                                ForumBadge {
                                    Layout.alignment: Qt.AlignVCenter
                                    maxWidth: 110
                                    forum: Config.BbcGameDates.gameTitle(gameDelegate.modelData)
                                    accent: forumPage.stepColor(gameDelegate.modelData.step)
                                }

                                AppText {
                                    Layout.fillWidth: true
                                    text: "(" + Config.BbcGameDates.playersText(gameDelegate.modelData.num) + ")"
                                    elide: Text.ElideRight
                                    color: gameDelegate.full
                                           ? Config.Theme.colorSuccess
                                           : (gameDelegate.modelData.num > 0
                                              ? Config.StaticData.palette.secondary.col200
                                              : Config.StaticData.palette.secondary.col400)
                                    font.pixelSize: Config.Theme.fontSizeCaption
                                    font.bold: gameDelegate.full
                                }

                                // Expand / collapse chevron (as in the lobby game list);
                                // rows without registrations keep the space empty.
                                Item {
                                    Layout.preferredWidth: 12
                                    Layout.preferredHeight: 12

                                    Image {
                                        anchors.fill: parent
                                        visible: gameDelegate.expandable
                                        source: "../resources/caretLeft.svg"
                                        sourceSize: Qt.size(24, 24)
                                        rotation: gameDelegate.expanded ? 90 : -90
                                        smooth: true
                                        antialiasing: true
                                        layer.enabled: true
                                        layer.effect: MultiEffect {
                                            colorization: 1.0
                                            colorizationColor: Config.StaticData.palette.secondary.col400
                                        }
                                        Behavior on rotation {
                                            NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
                                        }
                                    }
                                }
                            }

                            HoverHandler {
                                id: gameHover
                                enabled: gameDelegate.expandable
                                cursorShape: Qt.PointingHandCursor
                            }
                            TapHandler {
                                enabled: gameDelegate.expandable
                                onTapped: forumPage.toggleGame(gameDelegate.modelData)
                            }
                        }

                        // Registered players
                        Rectangle {
                            visible: gameDelegate.expanded
                            width: parent.width
                            height: visible ? regsContent.implicitHeight + 16 : 0
                            color: Config.Theme.colorPanelRow

                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: 3
                                color: forumPage.stepColor(gameDelegate.modelData.step)
                                opacity: 0.6
                            }

                            Item {
                                id: regsContent
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.leftMargin: 20
                                anchors.rightMargin: 10
                                anchors.topMargin: 8
                                implicitHeight: chips.visible ? chips.implicitHeight : regsStatus.implicitHeight

                                readonly property var players: gameDelegate.regs ? gameDelegate.regs.players : []

                                Flow {
                                    id: chips
                                    width: parent.width
                                    spacing: 6
                                    visible: regsContent.players.length > 0

                                    Repeater {
                                        model: regsContent.players

                                        // BBC admins get a gold outline and an "Admin" tag
                                        // (gold, not the green of the table admin badge).
                                        Rectangle {
                                            id: chip
                                            required property var modelData
                                            readonly property bool admin: modelData.admin === true
                                            readonly property color adminColor: Config.Theme.isDark
                                                                                ? Config.Theme.colorAccent
                                                                                : Config.Theme.colorAccentDim

                                            implicitWidth: chipRow.implicitWidth + 16
                                            implicitHeight: chipRow.implicitHeight + 8
                                            radius: height / 2
                                            color: admin ? Qt.rgba(adminColor.r, adminColor.g, adminColor.b, 0.14)
                                                         : Config.StaticData.palette.secondary.col600
                                            border.color: admin ? Qt.rgba(adminColor.r, adminColor.g, adminColor.b, 0.6)
                                                                : Config.StaticData.palette.secondary.col500
                                            border.width: 1

                                            Row {
                                                id: chipRow
                                                anchors.centerIn: parent
                                                spacing: 5

                                                AppText {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    text: chip.modelData.nick
                                                    color: Config.StaticData.palette.secondary.col100
                                                    font.pixelSize: Config.Theme.fontSizeCaption
                                                }
                                                AppText {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    visible: chip.admin
                                                    text: qsTr("Admin")
                                                    color: chip.adminColor
                                                    font.pixelSize: 9
                                                    font.bold: true
                                                    font.letterSpacing: 0.5
                                                }
                                            }
                                        }
                                    }
                                }

                                AppText {
                                    id: regsStatus
                                    width: parent.width
                                    visible: !chips.visible
                                    wrapMode: Text.WordWrap
                                    text: gameDelegate.regs && gameDelegate.regs.error
                                          ? qsTr("The registrations could not be loaded.")
                                          : qsTr("Loading registrations…")
                                    color: gameDelegate.regs && gameDelegate.regs.error
                                           ? Config.Theme.colorDanger
                                           : Config.StaticData.palette.secondary.col300
                                    font.pixelSize: Config.Theme.fontSizeCaption
                                }
                            }
                        }
                    }
                }

                AppLabel {
                    anchors.centerIn: parent
                    width: parent.width - 32
                    visible: Config.BbcGameDates.games.length === 0 && !Config.BbcGameDates.loading
                    text: Config.BbcGameDates.errorText !== ""
                          ? Config.BbcGameDates.errorText
                          : qsTr("No BBC games in the next %1 days.").arg(Config.BbcGameDates.daysAhead)
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    color: Config.BbcGameDates.errorText !== ""
                           ? Config.Theme.colorDanger
                           : Config.StaticData.palette.secondary.col300
                    font.pixelSize: Config.Theme.fontSizeBody
                }
            }
        }

        // Footer as in the web client: mark everything as read + open the forum.
        // In portrait below each other – next to each other the width is not
        // enough for "mark all as read" (CustomButton does not elide).
        GridLayout {
            visible: !forumPage.bbcTab
            Layout.fillWidth: true
            columns: forumPage.compact ? 1 : 2
            columnSpacing: 8
            rowSpacing: 8

            CustomButton {
                Layout.fillWidth: true
                Layout.preferredHeight: Config.Theme.buttonHeight
                text: qsTr("Mark all as read")
                enabled: Config.ForumNews.unreadCount > 0
                opacity: enabled ? 1 : 0.5
                onClicked: Config.ForumNews.markAllRead()
            }

            CustomButton {
                Layout.fillWidth: true
                Layout.preferredHeight: Config.Theme.buttonHeight
                text: qsTr("Open the forum")
                onClicked: forumPage.openExternal(Config.ForumNews.forumUrl)
            }
        }

        // Registering needs a login on the BBC site → external browser.
        CustomButton {
            objectName: "bbcRegisterButton"
            visible: forumPage.bbcTab
            Layout.fillWidth: true
            Layout.preferredHeight: Config.Theme.buttonHeight
            text: qsTr("Register")
            onClicked: forumPage.openExternal(Config.BbcGameDates.registrationUrl)
        }
    }
}
