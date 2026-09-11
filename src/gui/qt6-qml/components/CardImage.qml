import QtQuick 6.5
import QtQuick.Window

import "../config" as Config

// Responsive card element.
// cardIndex: engine encoding 0-51 (-1 = backside)
//   0-12 = diamonds (♦), 13-25 = hearts (♥), 26-38 = spades (♠), 39-51 = clubs (♣)
//   Rank: 0=2, 1=3, …, 8=10, 9=J, 10=Q, 11=K, 12=A
//
// The cards are rendered via the slim 'cards-simple' SVG set: a large
// centred rank + one centred suit symbol. This variant was extracted from the
// responsive-cards (rank and suit glyphs) and is optimally readable at small
// sizes as well and can be scaled up arbitrarily. Rendered via 'Image'
// (the Qt SVG rasterizer), which evaluates the viewBox cleanly.
Item {
    id: root

    property int cardIndex: -1
    // Optional start delay of the flip animation in ms (e.g. for staggered
    // dealing: the second hole card gets flipDelay: 80).
    property int flipDelay: 0

    // Effective scaling factor with which a PARENT transform scales this card up
    // on the table (boxScale/oppScale/communityScale × zoom). Since the
    // SVGs are rasterized to their LOGICAL size via the image rasterizer, a
    // scale>1 would blow the finished texture up on the GPU side → blurry. By
    // multiplying sourceSize with renderScale as well, we rasterize directly in
    // the actual screen pixel size → sharp. Deliberately coupled to the DISCRETE
    // layout/zoom factors (not to the animated scale value), so that
    // it is not rasterized anew per animation frame.
    property real renderScale: 1.0
    readonly property real _rs: Math.max(1.0, renderScale)

    readonly property bool isBack: !(Number.isInteger(cardIndex) && cardIndex >= 0 && cardIndex <= 51)

    // The reveal animation can be switched off via the setting "animated cards"
    // (config key ShowFlipCardsAnimation) – as in the Qt widgets client. If it is off,
    // the cards appear directly in their final size without a flip (_flipScale = 1).
    readonly property bool flipAnimationEnabled:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("ShowFlipCardsAnimation") !== 0 : true

    // ── Flip animation when revealing (showdown / deal) ───────────────────────
    // Triggered when isBack changes from true → false (the card is being
    // revealed). The horizontal scale melts to 0 (the turning point – at
    // that moment the front side is already active, since QML switches the property
    // atomically), then it springs back with a slight overshoot.
    property real _flipScale: 1.0
    transform: Scale {
        xScale: root._flipScale
        origin.x: root.width / 2
        origin.y: root.height / 2
    }

    onIsBackChanged: {
        if (!isBack && flipAnimationEnabled) {
            flipAnim.restart()
        }
    }

    SequentialAnimation {
        id: flipAnim
        PauseAnimation { duration: root.flipDelay }
        // Phase 1: backside → the zero line (as in the widget client: the card "turns" away)
        NumberAnimation {
            target: root
            property: "_flipScale"
            from: 1.0; to: 0.0
            duration: 170
            easing.type: Easing.InQuad
        }
        // Phase 2: the front side grows back – a small overshoot for liveliness
        NumberAnimation {
            target: root
            property: "_flipScale"
            from: 0.0; to: 1.0
            duration: 300
            easing.type: Easing.OutBack
            easing.overshoot: 1.15
        }
    }

    // ── "Show" flip (manual confirmation) ────────────────────────────────────
    // Unlike the deal flip above, your own cards are already open here
    // (isBack == false). In phase 1 we briefly show the backside and
    // then turn it to the front side – exactly like the widget client on a click
    // on "show cards" (gameTableImpl::showHoleCards → startFlipCards).
    property bool _showFlipBack: false

    function playShowFlip() {
        if (isBack || !flipAnimationEnabled)
            return
        showFlipAnim.restart()
    }

    SequentialAnimation {
        id: showFlipAnim
        PauseAnimation { duration: root.flipDelay }
        // Phase 1: show the backside and shrink to the zero line
        PropertyAction { target: root; property: "_showFlipBack"; value: true }
        NumberAnimation {
            target: root
            property: "_flipScale"
            from: 1.0; to: 0.0
            duration: 170
            easing.type: Easing.InQuad
        }
        // Turning point: switch back to the front side
        PropertyAction { target: root; property: "_showFlipBack"; value: false }
        // Phase 2: the front side grows back with a slight overshoot
        NumberAnimation {
            target: root
            property: "_flipScale"
            from: 0.0; to: 1.0
            duration: 300
            easing.type: Easing.OutBack
            easing.overshoot: 1.15
        }
    }

    // Is a style card deck active? StyleProvider.cardDeckDir is only set if the
    // chosen style actually contains card SVGs (0.svg..51.svg). If loading a
    // style card fails, _styledFrontFailed falls back to the bundled
    // 'cards-simple' set.
    readonly property bool _styledDeck:
        (typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.cardDeckDir !== "")
    property bool _styledFrontFailed: false

    // Style change at runtime: reset the per-card fallback flag so that
    // a new (working) deck does not wrongly stay at the default.
    Connections {
        target: (typeof StyleProvider !== "undefined") ? StyleProvider : null
        function onChanged() { root._styledFrontFailed = false }
    }

    // Compute the front side source in ONE binding (depending only on cardIndex),
    // so that no invalid intermediate paths like "-1s.svg" arise when switching.
    readonly property string frontSource: {
        if (isBack)
            return ""
        // Style deck: the cards are named after the engine index 0.svg..51.svg.
        if (_styledDeck && !_styledFrontFailed)
            return StyleProvider.cardDeckDir + "/" + cardIndex + ".svg"
        var suits = ["d", "h", "s", "c"]
        var ranks = [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 1]
        return "qrc:resources/cards-simple/" + ranks[cardIndex % 13] + suits[Math.floor(cardIndex / 13)] + ".svg"
    }

    // Backside source: the style flipside, otherwise the bundled backside.
    readonly property string backSource:
        (typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.cardBack !== "")
            ? StyleProvider.cardBack : "qrc:resources/cardBackground.svg"

    // ── Card back ──────────────────────────────────────────────────────────────
    Image {
        visible: root.isBack || root._showFlipBack
        anchors.fill: parent
        fillMode: Image.Stretch
        smooth: true
        // Rasterize the SVG sharply to the actual display size × devicePixelRatio ×
        // table scaling (instead of a fixed 100×140), otherwise it is scaled up blurrily.
        sourceSize.width: width > 0 ? Math.ceil(width * Screen.devicePixelRatio * root._rs) : 100
        sourceSize.height: height > 0 ? Math.ceil(height * Screen.devicePixelRatio * root._rs) : 140
        source: (root.isBack || root._showFlipBack) ? root.backSource : ""
    }

    // ── Front side (style SVG or the bundled cards-simple, via the image rasterizer) ─
    Image {
        visible: !root.isBack && !root._showFlipBack
        anchors.fill: parent
        fillMode: Image.Stretch
        smooth: true
        // Rasterize the SVG sharply to the display size × devicePixelRatio × table scaling
        // (instead of a fixed 120×168).
        sourceSize.width: width > 0 ? Math.ceil(width * Screen.devicePixelRatio * root._rs) : 120
        sourceSize.height: height > 0 ? Math.ceil(height * Screen.devicePixelRatio * root._rs) : 168
        source: root.frontSource
        onStatusChanged: {
            // If an individual style card is missing, fall back to the bundled set.
            if (status === Image.Error && root._styledDeck && !root._styledFrontFailed)
                root._styledFrontFailed = true
        }
    }
}
