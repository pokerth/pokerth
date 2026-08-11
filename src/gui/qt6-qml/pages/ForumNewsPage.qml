import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// Forum-Neuigkeiten – die letzten Beiträge von www.pokerth.net, wie im
// Web-Client (dort das Fenster „Forum news"). Ein Tippen öffnet den Beitrag
// IN der App (ForumPostPage), nicht im Browser.
//
// Daten, Entdopplung und Gelesen-Status liegen in Config.ForumNews.
Rectangle {
    id: forumPage
    objectName: "forumNewsPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    readonly property bool compact: Config.Responsive.compact
    readonly property bool listEmpty: Config.ForumNews.posts.length === 0

    // Beim Öffnen aktualisieren; die TTL im Singleton verhindert, dass jedes
    // Öffnen einen Abruf auslöst.
    Component.onCompleted: Config.ForumNews.refresh(false)

    function openPost(post) {
        if (post)
            mainStackView.push("ForumPostPage.qml", { post: post })
    }

    // Öffnet einen Link im externen Browser. NICHT direkt Qt.openUrlExternally:
    // im AppImage/Bundle erbt QDesktopServices das gebundelte LD_LIBRARY_PATH →
    // xdg-open stürzt ab (gleiche Begründung wie ChatBox/AboutPage).
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
                ScrollBar.vertical: ScrollBar {
                    policy: postList.contentHeight > postList.height + 4
                            ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                }

                delegate: Item {
                    id: postDelegate
                    required property int index
                    required property var modelData

                    // readRevision lesen, damit die Zeile nach „gelesen" neu
                    // ausgewertet wird (ein Funktionsaufruf allein erzeugt
                    // keine Bindungsabhängigkeit).
                    readonly property bool unread: {
                        var _rev = Config.ForumNews.readRevision
                        return Config.ForumNews.isUnread(modelData)
                    }

                    width: ListView.view.width
                    height: Math.max(forumPage.compact ? 58 : 50,
                                     textColumn.implicitHeight + 16)

                    Rectangle {
                        anchors.fill: parent
                        color: rowHover.hovered
                               ? Config.Theme.colorHover
                               : (postDelegate.index % 2 === 0
                                  ? Config.Theme.colorBox
                                  : Config.StaticData.palette.secondary.col600)

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

                        // Zustands-Punkt: gefüllt = ungelesen, leerer Ring = gelesen
                        // (wie .fn-dot / .fn-dot-read im Web-Client).
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

        // Fußzeile wie im Web-Client: alles als gelesen markieren + Forum öffnen.
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

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
