import QtQuick

import "../config" as Config

// PokerTH-Logo + Kartensymbol-Reihe (♠ ♥ ♦ ♣).
// Gemeinsam genutzt von StartPage und Login-Dialog, damit Icon-Größe und
// Branding über die Seiten hinweg identisch sind (Vorbild: pokerth-web-client).
Column {
    id: root

    property real logoSize: Config.Theme.brandLogoSize
    readonly property real suitSize: Math.max(13, logoSize * 0.16)

    spacing: Math.max(6, Math.round(logoSize * 0.07))

    SvgIcon {
        anchors.horizontalCenter: parent.horizontalCenter
        width:  root.logoSize
        height: root.logoSize
        source: "../resources/pokerth.svg"
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: root.suitSize * 0.6

        Repeater {
            model: [
                { glyph: "♠", red: false },  // ♠ Pik
                { glyph: "♥", red: true  },  // ♥ Herz
                { glyph: "♦", red: true  },  // ♦ Karo
                { glyph: "♣", red: false }   // ♣ Kreuz
            ]
            delegate: Text {
                text: modelData.glyph
                font.pixelSize: root.suitSize
                color: modelData.red ? Config.Theme.colorSuitRed
                                     : Config.Theme.colorSuitBlack
            }
        }
    }
}
