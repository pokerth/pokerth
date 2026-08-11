import QtQuick

import "../config" as Config

// Forums-Plakette der Neuigkeiten-Liste (BBC, WEC, BUGS, …) – Pendant zu
// .fn-forum im Web-Client: Pillenform in der stabilen Farbe des Forums
// (Config.ForumNews.forumColor). Lange Namen brechen um statt zu kürzen.
Rectangle {
    id: badge

    property string forum: ""
    property real maxWidth: 78

    readonly property color accent: Config.ForumNews.forumColor(forum)

    // Breite über TextMetrics statt über das Label: das Label bekommt seine
    // Breite von der Plakette, eine Bindung auf label.implicitWidth wäre eine
    // Bindungsschleife.
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
