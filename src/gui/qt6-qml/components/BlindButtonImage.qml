import QtQuick

// Dealer/small/big blind puck. The source comes – if present – from the
// active table style (StyleProvider), otherwise it falls back to the bundled
// SVGs. button: 1=dealer, 2=small blind, 3=big blind, 0/other → empty.
// Positioning/visibility is set by the caller.
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
