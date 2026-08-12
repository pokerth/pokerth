import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config
import "../components"

// Einzelner Forenbeitrag – im Web-Client öffnet der Link den Browser, hier wird
// der Beitrag in der App gezeigt: der Atom-Feed liefert den kompletten Text
// gleich mit, Config.ForumNews bereitet ihn für Qt-RichText auf (Bilder als
// eigene Blöcke, damit sie auf die Spaltenbreite passen).
//
// Oben rechts (neben Titel und Autor) sitzt das Globus-Symbol: es übersetzt den
// Beitrag in die eingestellte Sprache – derselbe Dienst und derselbe Schalter
// wie bei der Chat-Übersetzung (siehe Translator/TextTranslator). Erneutes
// Antippen zeigt wieder das Original.
Rectangle {
    id: postPage
    objectName: "forumPostPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    // Beitrag aus der Liste (ForumNewsPage) – siehe Config.ForumNews.posts.
    property var post: null

    // Aufbereitete Blöcke; hängt an Theme.isDark, damit ein Themenwechsel die
    // Farben des Beitrags neu anpasst.
    readonly property var blocks: {
        var _dark = Config.Theme.isDark
        if (!post)
            return []
        return Config.ForumNews.postBlocks(post, {
            dark: _dark, basePx: Config.Theme.fontSizeBody })
    }

    // ── Übersetzung ──────────────────────────────────────────────────────────
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
        // Der Dienst bekommt nur den reinen Text (ohne HTML) und nur so viel,
        // wie in eine Anfrage passt.
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
        // Angesehen = gelesen (im Web-Client zählt das Öffnen im Browser).
        if (post)
            Config.ForumNews.markRead(post)
    }

    // Öffnet einen Link im externen Browser (Begründung siehe ForumNewsPage).
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

            // Globus: Beitrag übersetzen / Original wieder anzeigen.
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
                anchors.margins: 10
                clip: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                // Die vertikale Scrollleiste liegt als Overlay über dem Inhalt
                // und ist in availableWidth nicht enthalten - ohne Abzug
                // schneidet sie bei schmalem Fenster in den Text.
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

                                // Links per TapHandler + linkAt: onLinkActivated
                                // feuert innerhalb einer Flickable nicht
                                // zuverlässig (Details siehe ChatBox/AboutPage).
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

                            // Bilder liegen als eigener Block vor: so lassen sie
                            // sich auf die Spaltenbreite begrenzen (Qt-RichText
                            // kennt kein max-width) – kleine Bilder bleiben in
                            // Originalgröße, große werden proportional verkleinert.
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

                    // Eingeblendete Übersetzung ersetzt den Beitragstext
                    // (gleiches Umschalten wie im Chat), kursiv als Kennzeichnung.
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

        // ── Fußzeile: Forum-Link, Übersetzen ─────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            CustomButton {
                Layout.preferredWidth: Config.Responsive.compact ? 150 : 190
                Layout.preferredHeight: Config.Theme.buttonHeight
                text: qsTr("Open in the forum")
                onClicked: postPage.openExternal(postPage.post ? postPage.post.link : "")
            }

            // Meldung, wenn die Übersetzung nicht geklappt hat.
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
