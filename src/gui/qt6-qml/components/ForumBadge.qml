import QtQuick

import "../config" as Config

// Forum badge of the news list (BBC, WEC, BUGS, …) – counterpart to
// .fn-forum in the web client: pill shape in the stable colour of the forum
// (Config.ForumNews.forumColor). Long names wrap instead of being elided.
Rectangle {
    id: badge

    property string forum: ""
    property real maxWidth: 78

    readonly property color accent: Config.ForumNews.forumColor(forum)

    // Width via TextMetrics instead of via the label: the label gets its width
    // from the badge, a binding on label.implicitWidth would be a binding
    // loop.
    TextMetrics {
        id: metrics
        font: label.font
        text: label.text
    }

    implicitWidth: Math.min(metrics.width + 14, maxWidth)
    implicitHeight: label.implicitHeight + 6
    radius: 7
    color: Qt.rgba(accent.r, accent.g, accent.b, 0.14)
    border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.45)
    border.width: 1
    visible: forum !== ""

    AppText {
        id: label
        anchors.centerIn: parent
        width: badge.width - 12
        text: badge.forum.toUpperCase()
        color: badge.accent
        font.pixelSize: 9
        font.bold: true
        font.letterSpacing: 0.8
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        lineHeight: 1.15
    }
}
