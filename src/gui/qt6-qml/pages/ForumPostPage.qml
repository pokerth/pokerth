import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config
import "../components"

// A single forum post – in the web client the link opens the browser, here the
// post is shown in the app: the Atom feed delivers the complete text right
// away, Config.ForumNews prepares it for Qt rich text (images as separate
// blocks so that they fit the column width).
//
// At the top right (next to the title and the author) sits the globe symbol: it
// translates the post into the configured language – the same service and the same
// switch as the chat translation (see Translator/TextTranslator). Tapping it
// again shows the original.
Rectangle {
    id: postPage
    objectName: "forumPostPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700
    // The reading area gets the focus when opening, so that the post is
    // scrollable without a mouse click.
    StackView.onActivated: Qt.callLater(postScroll.forceActiveFocus)

    // Post from the list (ForumNewsPage) – see Config.ForumNews.posts.
    property var post: null

    // Prepared blocks; depends on Theme.isDark so that a theme change adjusts
    // the colours of the post again.
    readonly property var blocks: {
        var _dark = Config.Theme.isDark
        if (!post)
            return []
        return Config.ForumNews.postBlocks(post, {
            dark: _dark, basePx: Config.Theme.fontSizeBody })
    }

    // ── Translation ──────────────────────────────────────────────────────────
    property bool translationShown: false
    property bool translating: false
    property string translatedText: ""
    property string translateError: ""
    property int translateRequestId: -1

    readonly property bool translateAvailable:
        typeof Translator !== "undefined" && Translator && Translator.enabled

    function toggleTranslation() {
        if (translationShown) {
            translationShown = false
            return
        }
        if (translatedText !== "") {
            translationShown = true
            return
        }
        if (translating || !translateAvailable || !post)
            return
        // The service only gets the plain text (without HTML) and only as much
        // as fits into one request.
        var source = Config.ForumNews.plainText(post.html, 1800)
        if (source === "")
            return
        translateError = ""
        translateRequestId = Translator.translate(source)
        translating = translateRequestId >= 0
    }

    Connections {
        target: (typeof Translator !== "undefined") ? Translator : null

        function onTranslated(requestId, text, ok) {
            if (requestId !== postPage.translateRequestId)
                return
            postPage.translating = false
            if (!ok || text === "") {
                postPage.translateError = qsTr("Translation failed.")
                return
            }
            postPage.translatedText = text
            postPage.translationShown = true
        }
    }

    Component.onCompleted: {
        // Viewed = read (in the web client opening it in the browser counts).
        if (post)
            Config.ForumNews.markRead(post)
    }

    // Opens a link in the external browser (for the reasoning see ForumNewsPage).
    function openExternal(link) {
        if (!link || link === "")
            return
        var opened = false
        if (typeof Lobby !== "undefined" && Lobby)
            opened = Lobby.openExternalUrl(link)
        if (!opened)
            opened = Qt.openUrlExternally(link)
        if (!opened)
            console.warn("ForumPostPage: konnte URL nicht öffnen:", link)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // ── Kopf: Forum, Titel, Autor/Datum ──────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ForumBadge {
                forum: postPage.post ? (postPage.post.forum || "") : ""
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 2
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                AppText {
                    Layout.fillWidth: true
                    text: postPage.post ? (postPage.post.title || "") : ""
                    wrapMode: Text.WordWrap
                    color: Config.StaticData.palette.secondary.col100
                    font.pixelSize: Config.Theme.fontSizeBody + 2
                    font.bold: true
                }

                AppText {
                    Layout.fillWidth: true
                    text: {
                        if (!postPage.post)
                            return ""
                        var a = postPage.post.author || ""
                        var d = Config.ForumNews.formatDate(postPage.post.ts)
                        return a !== "" && d !== "" ? a + " · " + d : a + d
                    }
                    elide: Text.ElideRight
                    color: Config.StaticData.palette.secondary.col400
                    font.pixelSize: Config.Theme.fontSizeCaption
                }
            }

            BusyIndicator {
                Layout.alignment: Qt.AlignTop
                running: postPage.translating
                visible: running
                implicitWidth: 26
                implicitHeight: 26
            }

            // Globe: translate the post / show the original again.
            Item {
                id: translateButton
                visible: postPage.translateAvailable && !postPage.translating
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: Config.Theme.iconSize + 8
                Layout.preferredHeight: Config.Theme.iconSize + 8

                ToolTip.visible: translateHover.hovered && !Config.Responsive.isMobile
                                 && Config.Parameters.showTooltips
                ToolTip.delay: 600
                ToolTip.text: postPage.translationShown
                              ? qsTr("Show the original post")
                              : qsTr("Translate the post")

                SvgIcon {
                    id: translateIcon
                    anchors.centerIn: parent
                    width: Config.Theme.iconSize
                    height: Config.Theme.iconSize
                    source: "../resources/globe.svg"
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        colorization: 1.0
                        colorizationColor: postPage.translationShown
                            ? Config.Theme.colorAccent
                            : translateHover.hovered
                                ? Config.StaticData.palette.secondary.col100
                                : Config.StaticData.palette.secondary.col200
                    }
                }

                HoverHandler {
                    id: translateHover
                    cursorShape: Qt.PointingHandCursor
                }
                TapHandler {
                    onTapped: postPage.toggleTranslation()
                }
            }
        }

        // ── Beitragstext ─────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Config.StaticData.palette.secondary.col600
            border.color: Config.StaticData.palette.secondary.col500
            border.width: 1
            radius: 4

            ScrollView {
                id: postScroll
                anchors.fill: parent
                // Reading without a mouse: arrow up/down scrolls the Flickable
                // itself, page up/down and Home/End are unknown to it – those are
                // added here. Deliberately a single Keys.onPressed instead of several
                // individual handlers: as soon as an item has a special key handler,
                // its onPressed no longer sees the key in question.
                Keys.onPressed: (event) => {
                    var f = postScroll.contentItem
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

                anchors.margins: 10
                clip: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                // The vertical scrollbar lies as an overlay above the content
                // and is not included in availableWidth - without subtracting it,
                // it cuts into the text in a narrow window.
                readonly property real scrollBarSpace: ScrollBar.vertical.visible ? 12 : 0

                Column {
                    id: contentColumn
                    width: postScroll.availableWidth - postScroll.scrollBarSpace
                    spacing: 10

                    Repeater {
                        model: postPage.translationShown ? [] : postPage.blocks

                        delegate: Item {
                            id: blockItem
                            required property var modelData
                            readonly property bool isImage: modelData.type === "image"

                            width: contentColumn.width
                            height: isImage ? blockImage.height : blockText.height

                            TextEdit {
                                id: blockText
                                width: parent.width
                                visible: !blockItem.isImage
                                text: blockItem.isImage ? "" : blockItem.modelData.value
                                readOnly: true
                                selectByMouse: true
                                textFormat: TextEdit.RichText
                                wrapMode: TextEdit.WordWrap
                                color: Config.StaticData.palette.secondary.col100
                                selectionColor: Config.Theme.colorAccent
                                selectedTextColor: "#101010"
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: Config.Theme.fontSizeBody

                                // Links via TapHandler + linkAt: onLinkActivated
                                // does not fire reliably inside a Flickable
                                // (for details see ChatBox/AboutPage).
                                HoverHandler {
                                    cursorShape: blockText.hoveredLink !== ""
                                                 ? Qt.PointingHandCursor : Qt.IBeamCursor
                                }
                                TapHandler {
                                    id: blockLinkTap
                                    acceptedButtons: Qt.LeftButton
                                    onTapped: {
                                        var link = blockText.linkAt(blockLinkTap.point.position.x,
                                                                    blockLinkTap.point.position.y)
                                        if (link !== "")
                                            postPage.openExternal(link)
                                    }
                                }
                            }

                            // Images are present as a separate block: that way they can
                            // be limited to the column width (Qt rich text knows no
                            // max-width) – small images keep their original size, large
                            // ones are scaled down proportionally.
                            Image {
                                id: blockImage
                                visible: blockItem.isImage
                                source: blockItem.isImage ? blockItem.modelData.value : ""
                                asynchronous: true
                                fillMode: Image.PreserveAspectFit
                                readonly property real natW: implicitWidth > 0 ? implicitWidth : 0
                                readonly property real natH: implicitHeight > 0 ? implicitHeight : 0
                                width: natW > 0 ? Math.min(natW, parent.width) : 0
                                height: natW > 0 ? width * natH / natW : 0
                            }
                        }
                    }

                    // The inserted translation replaces the post text
                    // (the same toggling as in the chat), in italics as a marker.
                    TextEdit {
                        width: contentColumn.width
                        visible: postPage.translationShown
                        text: postPage.translatedText
                        readOnly: true
                        selectByMouse: true
                        textFormat: TextEdit.PlainText
                        wrapMode: TextEdit.WordWrap
                        color: Config.StaticData.palette.secondary.col100
                        selectionColor: Config.Theme.colorAccent
                        selectedTextColor: "#101010"
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: Config.Theme.fontSizeBody
                        font.italic: true
                    }
                }
            }
        }

        // ── Footer: forum link, translate ────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            CustomButton {
                Layout.preferredWidth: Config.Responsive.compact ? 150 : 190
                Layout.preferredHeight: Config.Theme.buttonHeight
                text: qsTr("Open in the forum")
                onClicked: postPage.openExternal(postPage.post ? postPage.post.link : "")
            }

            // Message if the translation did not work.
            AppText {
                Layout.fillWidth: true
                text: postPage.translateError
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                color: Config.Theme.colorDanger
                font.pixelSize: Config.Theme.fontSizeCaption
            }
        }
    }
}
