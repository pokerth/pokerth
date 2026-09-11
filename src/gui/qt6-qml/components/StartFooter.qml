import QtQuick
import QtQuick.Controls
import QtQuick.Effects

import "../config" as Config

// Footer of the start page – modelled on the connect screen of the
// pokerth-web-client: a row of small icon buttons (Discord, PokerTH.net,
// GitHub) above two subtle text lines with the license, version and source
// information.
//
// The row lies above the fire background, which is very bright in places
// (chips, flames). Two measures ensure readability – both taken from the
// web client:
//   • The text colour is the light grey-blue of the PokerTH palette (--gold-dim =
//     #a0acc4), NOT the dark accent gold. On fire, dark gold is
//     practically unreadable.
//   • A dark gradient (scrim) dims the background towards the lower edge,
//     so that the text and the icons always sit on a calm ground.
// Light/dark mode: the background is the same dark photo in both modes,
// which is why the footer stays light on dark in both modes – like the branding
// box. The colours live centrally in Theme (colorOverlayText*), see the
// reasoning there.
Item {
    id: root

    readonly property string discordUrl: "https://discord.gg/QU3nu2MqvB"
    readonly property string websiteUrl: "https://www.pokerth.net"
    readonly property string sourceUrl:  "https://github.com/pokerth/pokerth"
    readonly property string privacyUrl: "https://www.pokerth.net/ucp.php?mode=privacy"

    readonly property color textColor:      Config.Theme.colorOverlayText
    readonly property color textColorHover: Config.Theme.colorOverlayTextHi
    readonly property real  fontSize:       Config.Theme.compact ? 11 : 12
    readonly property real  bottomPadding:  Config.Theme.compact ? 10 : 14
    // Height of the transition over which the scrim runs from invisible to dark.
    // It deliberately does NOT count towards implicitHeight: in this area the gradient
    // is still almost invisible, it may overlap the lower edge of the branding
    // box (whose area is dark and opaque anyway).
    readonly property real  scrimFade:      32

    // The height comes from the token that the StartPage keeps free at the bottom
    // (Config.Theme.startFooterReserve) – one source for the reservation and the
    // footer. The content sits at the lower edge, the rest is the distance to the box.
    implicitHeight: Math.max(Config.Theme.startFooterReserve,
                             footerColumn.implicitHeight + bottomPadding)

    // Opens a link in the external browser. NOT Qt.openUrlExternally directly:
    // in the AppImage/bundle QDesktopServices inherits the bundled LD_LIBRARY_PATH →
    // xdg-open crashes. Lobby.openExternalUrl starts the host tools with a
    // cleaned environment (same reasoning as AboutPage/ChatBox).
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

    // ── Small square icon button (web client: .btn-sm.btn-icon) ────────────
    // The area stays the same on hover; as in the web client only the border
    // and the icon brighten.
    component FooterIconButton: Rectangle {
        id: iconButton

        property url iconSource: ""
        // "transparent" = render the icon in its original colour (PokerTH chip),
        // otherwise it is colourised in this colour.
        property color iconColor: "transparent"
        property string tooltipText: ""
        signal clicked()

        width: 34; height: 34
        radius: 4
        color: Qt.rgba(0, 0, 0, 0.55)
        border.width: 1
        border.color: iconButtonMouse.containsMouse ? Qt.rgba(0.63, 0.67, 0.77, 0.75)
                                                    : Qt.rgba(0.63, 0.67, 0.77, 0.30)

        ToolTip.visible: iconButtonMouse.containsMouse && iconButton.tooltipText !== ""
                         && !Config.Responsive.isMobile && Config.Parameters.showTooltips
        ToolTip.delay: 600
        ToolTip.text: iconButton.tooltipText

        SvgIcon {
            anchors.centerIn: parent
            width: 18
            height: 18
            source: iconButton.iconSource
            opacity: iconButtonMouse.containsMouse ? 1.0 : 0.85
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

    // ── Text line with embedded links ──────────────────────────────────────
    // onLinkActivated does not fire reliably inside a Flickable,
    // which is why – as in AboutPage/ChatBox – TapHandler + linkAt() is used.
    component FooterLine: AppText {
        id: footerLine

        // The width follows the (externally set) footer width, so that the lines
        // wrap in narrow windows instead of running out sideways.
        width: root.width
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        textFormat: Text.RichText
        color: root.textColor
        font.pixelSize: root.fontSize
        // Links carry the same colour as the body text (the standard blue would hardly
        // be readable on the background) and brighten under the cursor.
        linkColor: footerLine.hoveredLink !== "" ? root.textColorHover : root.textColor

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

    // ── Darkening gradient towards the lower window edge ───────────────────
    // It extends beyond the actual content upwards (scrimFade) and
    // downwards to the window edge, so that no hard edge appears.
    Rectangle {
        anchors {
            left: parent.left; right: parent.right; bottom: parent.bottom
            top: parent.top; topMargin: -root.scrimFade
        }
        gradient: Gradient {
            GradientStop { position: 0.0;  color: Qt.rgba(0, 0, 0, 0.0) }
            GradientStop { position: 0.55
                           color: Qt.rgba(0, 0, 0, Config.Theme.overlayScrimOpacity * 0.7) }
            GradientStop { position: 1.0
                           color: Qt.rgba(0, 0, 0, Config.Theme.overlayScrimOpacity) }
        }
    }

    Column {
        id: footerColumn
        anchors {
            left: parent.left; right: parent.right; bottom: parent.bottom
            bottomMargin: root.bottomPadding
        }
        spacing: 8

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
}
