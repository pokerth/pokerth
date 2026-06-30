import QtQuick

// Dealer-/Small-/Big-Blind-Puck. Die Quelle kommt – falls vorhanden – aus dem
// aktiven Tisch-Style (StyleProvider), sonst Fallback auf die mitgelieferten
// SVGs. button: 1=Dealer, 2=Small Blind, 3=Big Blind, 0/sonst → leer.
// Positionierung/Sichtbarkeit setzt der Aufrufer.
Image {
    property int button: 0
    width: 32
    height: 32
    fillMode: Image.PreserveAspectFit
    source: button === 1 ? ((typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.dealerPuck !== "") ? StyleProvider.dealerPuck : "../resources/tableDealerPuck.svg")
          : button === 2 ? ((typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.smallBlindPuck !== "") ? StyleProvider.smallBlindPuck : "../resources/tableSmallBlind.svg")
          : button === 3 ? ((typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.bigBlindPuck !== "") ? StyleProvider.bigBlindPuck : "../resources/tableBigBlind.svg")
          : ""
}
