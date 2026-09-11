import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// Forum news – the latest posts from www.pokerth.net, as in the
// web client (the "Forum news" window there). A tap opens the post
// IN the app (ForumPostPage), not in the browser.
//
// Data, deduplication and read status live in Config.ForumNews.
Rectangle {
    id: forumPage
    objectName: "forumNewsPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    readonly property bool compact: Config.Responsive.compact
    readonly property bool listEmpty: Config.ForumNews.posts.length === 0

    // Refresh when opening; the TTL in the singleton prevents every
    // open from triggering a fetch.
    Component.onCompleted: Config.ForumNews.refresh(false)

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
                text: qsTr("Forum news")
                Layout.fillWidth: true
                color: Config.StaticData.palette.secondary.col200
                font.pointSize: 14
                font.bold: true
            }

            BusyIndicator {
                running: Config.ForumNews.loading
                visible: running
                implicitWidth: 22
                implicitHeight: 22
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
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

        // Footer as in the web client: mark everything as read + open the forum.
        // In portrait below each other – next to each other the width is not
        // enough for "mark all as read" (CustomButton does not elide).
        GridLayout {
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
    }
}
