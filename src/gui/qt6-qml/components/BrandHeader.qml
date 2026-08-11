import QtQuick

import "../config" as Config

// PokerTH-Logo + Kartensymbol-Reihe (♠ ♥ ♦ ♣).
// Gemeinsam genutzt von StartPage und Login-Dialog, damit Icon-Größe und
// Branding über die Seiten hinweg identisch sind (Vorbild: pokerth-web-client).
Column {
    id: root

    property real logoSize: Config.Theme.brandLogoSize
    // Abstände/Symbolgröße kommen aus den Theme-Funktionen, damit Aufrufer die
    // Header-Höhe vorab berechnen können (Config.Theme.brandHeaderHeight) –
    // siehe StartPage, die daraus ihr Logo-Budget ableitet.
    readonly property real suitSize: Config.Theme.brandHeaderSuitSize(logoSize)

    spacing: Config.Theme.brandHeaderSpacing(logoSize)

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
