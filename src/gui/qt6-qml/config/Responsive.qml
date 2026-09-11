pragma Singleton
import QtQuick

// Responsive singleton: bind width/height from ApplicationWindow.
// Usage in pokerth.qml:
//   onWidthChanged:  Config.Responsive.windowWidth  = width
//   onHeightChanged: Config.Responsive.windowHeight = height
QtObject {
    id: root

    // Set by ApplicationWindow
    property real windowWidth:  900
    property real windowHeight: 600

    // Platform: real mobile devices (Android/iOS) vs. desktop. It is needed to
    // distinguish, at the same window geometry (e.g. a wide aspect ratio),
    // between the touch layout (compact action bar) and the desktop layout
    // (large buttons, even on ultrawide/HiDPI).
    readonly property bool isMobile: Qt.platform.os === "android" || Qt.platform.os === "ios"

    // Orientation
    readonly property bool portrait:  windowHeight > windowWidth
    readonly property bool landscape: windowWidth >= windowHeight

    // Breakpoints  (logical pixels / dp)
    // phonePortrait  < 600 wide
    // phoneLandscape >= 600 and < 900 wide
    // tablet         >= 900 and < 1400 wide
    // desktop        >= 1400 wide
    readonly property bool phonePortrait:  portrait  && windowWidth  < 600
    readonly property bool phoneLandscape: landscape && windowHeight < 600
    // compact = "use the mobile layout variant" (slide-in panels instead of the
    // 3 column layout in lobby/game wait, narrower margins etc.).
    //   Mobile (Android/iOS): narrow portrait OR phone landscape.
    //   Desktop: ONLY if the window width is not enough for the 3 column layout
    //   (player list 200 + game list ~350 + info/chat 250 + margins).
    //   The geometry heuristic landscapeCompact deliberately does NOT apply here –
    //   wide desktop windows (aspect > 1.85, e.g. HiDPI/ultrawide) still have
    //   plenty of room for three columns.
    readonly property int  threeColumnMinWidth: 900
    readonly property bool compact:
        isMobile ? (windowWidth < 600 || landscapeCompact)
                 : windowWidth < threeColumnMinWidth
    readonly property bool tablet:         windowWidth >= 900  && windowWidth < 1400
    readonly property bool desktop:        windowWidth >= 1400

    // Landscape with little vertical room: the action bar + status bar would
    // otherwise eat 25–35 % of the height → the boxes become so large that the top
    // row bumps into the upper edge / opponent boxes overlap.
    //   Mobile (Android/iOS): tie it to the HEIGHT (every phone in landscape),
    //   NOT to the aspect. Otherwise 16:9 phones such as the Galaxy A5 2017
    //   (1920×1080 → ~640×360 logical, ratio 1.78 < 1.85) fall through and do not
    //   get the compact treatment.
    //   Desktop: tie it to the aspect heuristic (ONLY real ultrawide). Threshold
    //   2.1, so that a MAXIMIZED 16:9 window (1920×1006 with a taskbar → ratio
    //   ~1.91) gets the full desktop layout instead of the phone compact layout
    //   (topmost box flush at the top). Real 21:9 (≈2.33) stays compact;
    //   windowHeight < 1300 excludes large ultrawide windows.
    readonly property bool landscapeCompact:
        landscape
        && windowHeight > 0
        && (isMobile
            ? windowHeight < 600
            : ((windowWidth / windowHeight) > 2.1 && windowHeight < 1300))

    // Convenience: number of columns for a simple grid
    readonly property int columns: compact ? 1 : tablet ? 2 : 3
}
