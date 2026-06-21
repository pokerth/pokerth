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

    // Plattform: echte Mobilgeräte (Android/iOS) vs. Desktop. Wird gebraucht,
    // um bei gleicher Fenstergeometrie (z. B. breites Aspect-Ratio) zwischen
    // Touch-Layout (kompakte Action-Bar) und Desktop-Layout (große Buttons,
    // selbst auf Ultrawide/HiDPI) zu unterscheiden.
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
    // compact = „nutze mobile Layout-Variante" (Slide-in-Panels statt
    // 3-Spalten-Layout in Lobby/GameWait, schmälere Margins etc.).
    //   Mobile (Android/iOS): schmales Portrait ODER Phone-Landscape.
    //   Desktop: NUR wenn die Fensterbreite nicht für das 3-Spalten-Layout
    //   reicht (Spielerliste 200 + Spieleliste ~350 + Info/Chat 250 + Ränder).
    //   Die Geometrie-Heuristik landscapeCompact greift hier bewusst NICHT –
    //   breite Desktop-Fenster (Aspect > 1.85, z. B. HiDPI/Ultrawide) haben
    //   trotzdem locker Platz für drei Spalten.
    readonly property int  threeColumnMinWidth: 900
    readonly property bool compact:
        isMobile ? (windowWidth < 600 || landscapeCompact)
                 : windowWidth < threeColumnMinWidth
    readonly property bool tablet:         windowWidth >= 900  && windowWidth < 1400
    readonly property bool desktop:        windowWidth >= 1400

    // Landscape mit wenig vertikalem Platz: die Action-Bar + Status-Leiste würden
    // sonst 25–35 % der Höhe fressen → Boxen werden so groß, dass die Topreihe an
    // den oberen Rand stößt / Gegnerboxen überlappen.
    //   Mobile (Android/iOS): an der HÖHE festmachen (jedes Phone im Landscape),
    //   NICHT am Aspect. Sonst fallen 16:9-Phones wie das Galaxy A5 2017
    //   (1920×1080 → ~640×360 logisch, Ratio 1.78 < 1.85) durch und bekommen die
    //   Kompakt-Behandlung nicht.
    //   Desktop: an der Aspect-Heuristik (Ultrawide/HiDPI). Klassische
    //   16:9-Monitore (1.78) bleiben bewusst ausgeschlossen; windowHeight < 1300
    //   schließt große Ultrawide-Fenster aus.
    readonly property bool landscapeCompact:
        landscape
        && windowHeight > 0
        && (isMobile
            ? windowHeight < 600
            : ((windowWidth / windowHeight) > 1.85 && windowHeight < 1300))

    // Convenience: number of columns for a simple grid
    readonly property int columns: compact ? 1 : tablet ? 2 : 3
}
