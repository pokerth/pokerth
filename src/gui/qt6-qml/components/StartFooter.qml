import QtQuick
import QtQuick.Controls
import QtQuick.Effects

import "../config" as Config

// Fußzeile der Startseite – Vorbild ist der Connect-Screen des
// pokerth-web-client: eine Reihe kleiner Icon-Buttons (Discord, PokerTH.net,
// GitHub) über zwei dezenten Textzeilen mit Lizenz-, Versions- und
// Quellenangabe. Liegt als Overlay über dem Feuer-Hintergrund, deshalb sind
// die Farben – wie die Branding-Box – bewusst fest dunkel/gold gewählt und
// folgen nicht dem Hell/Dunkel-Theme.
Column {
    id: root

    readonly property string discordUrl: "https://discord.gg/QU3nu2MqvB"
    readonly property string websiteUrl: "https://www.pokerth.net"
    readonly property string sourceUrl:  "https://github.com/pokerth/pokerth"
    readonly property string privacyUrl: "https://www.pokerth.net/ucp.php?mode=privacy"

    // Gedämpftes Gold wie im Web-Client (--gold-dim), Links heben sich beim
    // Überfahren auf das volle Akzentgold an.
    readonly property color textColor: Config.Theme.colorAccentDim
    readonly property real  fontSize:  Config.Theme.compact ? 10 : 11

    spacing: 8

    // Öffnet einen Link im externen Browser. NICHT direkt Qt.openUrlExternally:
    // Im AppImage/Bundle erbt QDesktopServices das gebundelte LD_LIBRARY_PATH →
    // xdg-open crasht. Lobby.openExternalUrl startet die Host-Tools mit
    // bereinigter Umgebung (gleiche Begründung wie AboutPage/ChatBox).
    function openLink(link) {
        if (!link || link === "")
            return
        var opened = false
        if (typeof Lobby !== "undefined" && Lobby)
            opened = Lobby.openExternalUrl(link)
        if (!opened)
            opened = Qt.openUrlExternally(link)
        if (!opened)
            console.warn("StartFooter: konnte URL nicht öffnen:", link)
    }

    // ── Kleiner quadratischer Icon-Button (Web-Client: .btn-sm.btn-icon) ────
    component FooterIconButton: Rectangle {
        id: iconButton

        property url iconSource: ""
        // Leer = Icon in Originalfarbe rendern (PokerTH-Chip), sonst einfärben.
        property color iconColor: "transparent"
        property string tooltipText: ""
        signal clicked()

        width: 34; height: 34
        radius: 4
        color: iconButtonMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.10)
                                             : Qt.rgba(0, 0, 0, 0.35)
        border.width: 1
        border.color: iconButtonMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.35)
                                                    : Qt.rgba(1, 1, 1, 0.15)

        ToolTip.visible: iconButtonMouse.containsMouse && iconButton.tooltipText !== ""
                         && !Config.Responsive.isMobile && Config.Parameters.showTooltips
        ToolTip.delay: 600
        ToolTip.text: iconButton.tooltipText

        SvgIcon {
            anchors.centerIn: parent
            width: 18
            height: 18
            source: iconButton.iconSource
            layer.enabled: iconButton.iconColor.a > 0
            layer.effect: MultiEffect {
                colorization: 1.0
                colorizationColor: iconButton.iconColor
            }
        }

        MouseArea {
            id: iconButtonMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: iconButton.clicked()
        }
    }

    // ── Textzeile mit eingebetteten Links ──────────────────────────────────
    // onLinkActivated feuert innerhalb einer Flickable nicht zuverlässig,
    // deshalb – wie in AboutPage/ChatBox – TapHandler + linkAt().
    component FooterLine: AppText {
        id: footerLine

        // Breite folgt der (extern gesetzten) Footer-Breite, damit die Zeilen
        // auf schmalen Fenstern umbrechen statt seitlich herauszulaufen.
        width: root.width
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        textFormat: Text.RichText
        color: root.textColor
        font.pixelSize: root.fontSize
        linkColor: root.textColor

        HoverHandler {
            cursorShape: footerLine.hoveredLink !== "" ? Qt.PointingHandCursor
                                                       : Qt.ArrowCursor
        }
        TapHandler {
            id: footerLineTap
            acceptedButtons: Qt.LeftButton
            onTapped: {
                const link = footerLine.linkAt(footerLineTap.point.position.x,
                                               footerLineTap.point.position.y)
                if (link !== "")
                    root.openLink(link)
            }
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 6

        FooterIconButton {
            iconSource: "../resources/discord.svg"
            iconColor: "#5865F2"          // Discord-Blurple
            tooltipText: "Discord"
            onClicked: root.openLink(root.discordUrl)
        }

        FooterIconButton {
            iconSource: "../resources/pokerth.svg"
            tooltipText: "PokerTH.net"
            onClicked: root.openLink(root.websiteUrl)
        }

        FooterIconButton {
            iconSource: "../resources/github.svg"
            iconColor: "#e6e8ee"
            tooltipText: "GitHub"
            onClicked: root.openLink(root.sourceUrl)
        }
    }

    FooterLine {
        text: "♠ PokerTH " + SettingsManager.appVersion() + " · AGPL-3.0 · "
              + "<a href=\"" + root.privacyUrl + "\">" + qsTr("Privacy") + "</a>"
    }

    FooterLine {
        text: qsTr("Source:") + " <a href=\"" + root.sourceUrl
              + "\">github.com/pokerth/pokerth</a>"
    }
}
