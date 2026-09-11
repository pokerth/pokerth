import QtQuick

import "../config" as Config

// PokerTH logo + card symbol row (♠ ♥ ♦ ♣).
// Shared by the StartPage and the login dialog so that icon size and
// branding are identical across the pages (model: pokerth-web-client).
Column {
    id: root

    property real logoSize: Config.Theme.brandLogoSize
    // Spacings/symbol size come from the theme functions so that callers can
    // precompute the header height (Config.Theme.brandHeaderHeight) –
    // see StartPage, which derives its logo budget from it.
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
