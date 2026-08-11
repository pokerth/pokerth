import QtQuick
import QtQuick.Controls
import QtQuick.Effects

import "../config" as Config

// Fußzeile der Startseite – Vorbild ist der Connect-Screen des
// pokerth-web-client: eine Reihe kleiner Icon-Buttons (Discord, PokerTH.net,
// GitHub) über zwei dezenten Textzeilen mit Lizenz-, Versions- und
// Quellenangabe.
//
// Die Zeile liegt über dem Feuer-Hintergrund, der stellenweise sehr hell ist
// (Chips, Flammen). Zwei Maßnahmen sichern die Lesbarkeit – beide aus dem
// Web-Client übernommen:
//   • Textfarbe ist das helle Grau-Blau der PokerTH-Palette (--gold-dim =
//     #a0acc4), NICHT das dunkle Akzentgold. Auf Feuer ist dunkles Gold
//     praktisch unlesbar.
//   • Ein dunkler Verlauf (Scrim) blendet den Hintergrund zum unteren Rand
//     hin ab, damit Text und Icons immer auf ruhigem Grund sitzen.
// Hell-/Dunkelmodus: Der Hintergrund ist in beiden Modi dasselbe dunkle Foto,
// deshalb bleibt die Fußzeile – wie die Branding-Box – in beiden Modi hell auf
// dunkel. Die Farben liegen zentral in Theme (colorOverlayText*), siehe die
// Begründung dort.
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
    // Höhe des Übergangs, über den der Scrim von unsichtbar nach dunkel läuft.
    // Zählt bewusst NICHT zur implicitHeight: In diesem Bereich ist der Verlauf
    // noch nahezu unsichtbar, er darf sich mit dem unteren Rand der Branding-Box
    // überlappen (deren Fläche ist ohnehin dunkel und deckend).
    readonly property real  scrimFade:      32

    // Höhe kommt aus dem Token, das StartPage/Login-Dialog unten freihalten
    // (Config.Theme.startFooterReserve) – eine Quelle für beide Seiten. Der
    // Inhalt sitzt am unteren Rand, der Rest ist Abstand zur Box darüber.
    implicitHeight: Math.max(Config.Theme.startFooterReserve,
                             footerColumn.implicitHeight + bottomPadding)

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
    // Fläche bleibt beim Überfahren gleich; wie im Web-Client hellen sich nur
    // Rand und Icon auf.
    component FooterIconButton: Rectangle {
        id: iconButton

        property url iconSource: ""
        // "transparent" = Icon in Originalfarbe rendern (PokerTH-Chip),
        // sonst wird es in dieser Farbe eingefärbt.
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
        // Links tragen dieselbe Farbe wie der Fließtext (Standard-Blau wäre auf
        // dem Hintergrund kaum lesbar) und hellen unter dem Zeiger auf.
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

    // ── Abdunkelnder Verlauf zum unteren Fensterrand ───────────────────────
    // Reicht über den eigentlichen Inhalt hinaus nach oben (scrimFade) und
    // unten bis an die Fensterkante, damit kein harter Rand entsteht.
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
