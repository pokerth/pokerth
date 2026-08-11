pragma Singleton
import QtQuick

// Adaptive design tokens — all values react to window dimensions.
// NOTE: We cannot `import Config` here (same-module circular dependency in Qt 6).
// windowWidth / windowHeight must be kept in sync by ApplicationWindow
// alongside Config.Responsive (see pokerth.qml onWidthChanged / onHeightChanged).
QtObject {

    // Set by ApplicationWindow — mirrors Responsive.windowWidth/windowHeight
    property real windowWidth:  900
    property real windowHeight: 600

    // Set by ApplicationWindow and GuiSettings — same semantics as StaticData.darkMode
    // 0 = Hell (Light), 1 = Dunkel (Dark), 2 = Automatisch
    property int darkMode: 1
    // Vom System gemeldeter Modus – ebenfalls von der ApplicationWindow gesetzt
    // (Quelle: SettingsManager.systemDark), siehe StaticData.systemDark.
    property bool systemDark: true

    // 0=Hell → false, 1=Dunkel → true, 2=Automatisch → dem System folgen.
    readonly property bool isDark: darkMode === 2 ? systemDark : darkMode !== 0

    // Dekorative Effekte (Schlagschatten, Glow, Blur) global an/aus. Auf
    // schwachen / passiv gekühlten Systemen (oder bei Software-Rendering ohne
    // GPU) erzwingen die vielen gelayerten MultiEffect-Blur-Pässe pro Frame hohe
    // CPU-Last und Ruckeln. Ist dieser Schalter aus, werden alle dekorativen
    // `layer.enabled`-Effekte übersprungen (funktionale Layer wie Icon-
    // Kolorierung / Folded-Graustufen bleiben unberührt). Wird – wie darkMode –
    // extern von der ApplicationWindow (Init) und GuiSettings (Live-Toggle)
    // gesetzt, da ein Singleton die SettingsManager-Context-Property nicht direkt
    // lesen kann. Persistenter Config-Key: "QmlReduceEffects" (0 = Effekte an).
    property bool effectsEnabled: true

    readonly property bool compact: windowWidth < 600
    readonly property bool tablet:  windowWidth >= 900 && windowWidth < 1400

    // ── Spacing & Layout ────────────────────────────────────────────────────
    readonly property real margin:  compact ? 12 : tablet ? 20 : 28
    readonly property real spacing: compact ?  8 : tablet ? 12 : 16

    // ── Touch Targets ────────────────────────────────────────────────────────
    // Apple HIG / Material: minimum interactive area 44–48 dp
    readonly property real touchTarget:   compact ? 48 : 44
    readonly property real buttonHeight:  compact ? 48 : 40
    readonly property real buttonWidth:   compact ? -1 : 180   // -1 = fillWidth
    readonly property real iconSize:      compact ? 28 : 24
    readonly property real smallIconSize: compact ? 22 : 18

    // ── Branding-Box (Startseite & Login-Dialog teilen sich diese Werte, damit
    //    Box-Höhe und PokerTH-Icon beim Navigieren 1:1 identisch wirken) ────────
    readonly property real brandBoxWidth: 380
    // Höhe, die Startseite und Login-Dialog unten für die Fußzeile (StartFooter)
    // freihalten – Icon-Reihe plus zwei Textzeilen inkl. Abstand zur Box. Der
    // Wert ist die Höhe der Fußzeile selbst (StartFooter.implicitHeight bindet
    // daran), damit es nur EINE Quelle für diese Reservierung gibt.
    readonly property real startFooterReserve: compact ? 104 : 116
    // Feste Zielhöhe; schrumpft nur, wenn das Fenster zu niedrig ist (kurze /
    // Querformat-Fenster). Beide Seiten nutzen exakt diesen Wert. Der Abzug
    // reserviert die Topbar (38px) plus Außenabstand sowie die Fußzeile, damit
    // die Box samt Rand in den sichtbaren Bereich (StackView) passt und
    // zentriert bleibt.
    readonly property real brandBoxHeight:
        Math.max(380, Math.min(540, windowHeight - 96 - startFooterReserve))
    // Icon-Größe an die Box-Höhe gekoppelt, gedeckelt auf 126 (Desktop) bzw.
    // 100 (schmale Phones), Boden 56 → läuft nie über.
    readonly property real brandLogoSize:
        Math.round(Math.max(56, Math.min(compact ? 100 : 126, 0.4 * brandBoxHeight - 76)))
    // Untergrenze, bis zu der das Logo schrumpfen darf, bevor andere Elemente
    // (Buttons) nachgeben müssen.
    readonly property real brandLogoSizeMin: 56

    // ── Geometrie des Branding-Headers (Logo + Kartensymbol-Reihe) ──────────
    // Als Funktionen in den Tokens, damit ein Aufrufer aus seinem Höhenbudget
    // die passende Logo-Größe ableiten kann, OHNE die gemessene Header-Höhe zu
    // lesen – das ergäbe eine Bindungsschleife (kleineres Logo → mehr Platz →
    // größeres Logo → …). BrandHeader selbst benutzt dieselben Funktionen, es
    // gibt also nur eine Quelle für diese Geometrie.
    function brandHeaderSpacing(logo)  { return Math.max(6, Math.round(logo * 0.07)) }
    function brandHeaderSuitSize(logo) { return Math.max(13, logo * 0.16) }
    // Zeilenhöhe der Kartensymbole ≈ Schriftgröße × 1.45 (mit Reserve gerundet).
    function brandHeaderHeight(logo) {
        return logo + brandHeaderSpacing(logo)
               + Math.ceil(brandHeaderSuitSize(logo) * 1.45)
    }
    // Umkehrung von brandHeaderHeight(): größte Logo-Größe, deren Header noch in
    // budget passt. Die drei Terme sind die Äste der Formel (Abstand bzw.
    // Symbolgröße am Minimum oder proportional); das Minimum ist immer sicher.
    function brandHeaderLogoForHeight(budget) {
        return Math.floor(Math.min(budget - 25, (budget - 6) / 1.232, budget / 1.302))
    }

    // Kartensymbole (♠ ♥ ♦ ♣) auf der dunklen Branding-Box
    readonly property color colorSuitRed:   "#c0392b"   // ♥ ♦
    readonly property color colorSuitBlack: "#cdd3e0"   // ♠ ♣ (hell auf dunkler Box)

    // ── Overlay auf dem Feuer-Hintergrund (Startseite, Login, PreLoader) ─────
    // Hintergrund ist dort in BEIDEN Modi dasselbe dunkle Foto
    // (resources/startWindowBackground.png), deshalb sind diese Farben – wie die
    // Branding-Box selbst – fest und folgen NICHT dem Hell/Dunkel-Theme. Ein an
    // isDark gekoppelter Wert würde im Hellmodus dunkle Schrift auf dunklem
    // Feuerbild ergeben. Werte entsprechen --gold-dim / --text-hi der
    // PokerTH-Palette des pokerth-web-client.
    readonly property color colorOverlayText:      "#a0acc4"
    readonly property color colorOverlayTextHi:    "#eff1f5"
    // Abdunkelnder Verlauf hinter Overlay-Text (Scrim) – Deckkraft am Fensterrand
    readonly property real  overlayScrimOpacity:   0.80

    // ── Border Radius ────────────────────────────────────────────────────────
    readonly property real radiusSmall:  4
    readonly property real radiusMedium: 8
    readonly property real radiusLarge:  16

    // ── Typography ───────────────────────────────────────────────────────────
    readonly property real fontSizeCaption: compact ? 11 : 12
    readonly property real fontSizeBody:    compact ? 14 : 15
    readonly property real fontSizeLabel:   compact ? 14 : 14
    readonly property real fontSizeTitle:   compact ? 20 : 24
    readonly property real fontSizeHeader:  compact ? 26 : 32

    // ── Colors (mirrors StaticData.palette for use without Config prefix) ────
    // Background levels
    readonly property color colorBackground:    isDark ? "#1d222b" : "#e3e8f0"   // col700
    readonly property color colorSurface:       isDark ? "#394150" : "#dce2ec"   // col600
    readonly property color colorSurfaceMid:    isDark ? "#576378" : "#a0acc4"   // col500
    readonly property color colorSurfaceLight:  isDark ? "#7787a3" : "#7787a3"   // col400

    // ── Flächen-Rollen: Seitenhintergrund vs. Inhalts-Box ────────────────────
    // Die Richtung kippt zwischen den Modi, deshalb genügt ein gemeinsames
    // Qt.darker(colorBackground, f) nicht:
    //   Dunkel: die Seite ist dunkel, Boxen liegen noch etwas TIEFER (abgedunkelt)
    //           – so wie bisher, die Werte sind unverändert.
    //   Hell:   die Seite ist grau, die Boxen sind WEISS (klassisches Karten-
    //           Layout). Vorher war es umgekehrt (weiße Seite, graue Boxen) –
    //           die Inhalte wirkten dadurch schmutzig statt hervorgehoben.
    // colorField (Eingabefelder, Suchzeilen) bleibt in BEIDEN Modi abgesetzt:
    // eingelassen wirkt ein Feld auch auf weißem Grund richtig.
    readonly property color colorPanel:     isDark ? Qt.darker(colorBackground, 1.2) : "#ffffff"
    readonly property color colorPanelRow:  isDark ? Qt.darker(colorBackground, 1.1) : "#f5f7fb"
    readonly property color colorField:     isDark ? Qt.darker(colorBackground, 1.3) : "#eaeef6"
    // Flächen, die im Dunkelmodus bewusst den Seitenton tragen (Popups, Dialoge,
    // Karten, Overlays, Topbar, Buttons): dunkel bleibt alles exakt wie bisher,
    // hell werden sie weiß und heben sich damit von der grauen Seite ab.
    readonly property color colorBox:       isDark ? colorBackground : "#ffffff"
    // Hover-Fläche transparenter Listenzeilen INNERHALB einer Box. Auch hier
    // kippt die Richtung: dunkel wird aufgehellt, hell (Zeile liegt auf Weiß)
    // leicht abgedunkelt – ein Aufhellen wäre auf Weiß unsichtbar.
    readonly property color colorHover:       isDark ? Qt.lighter(colorBackground, 1.2) : "#eef2f8"
    readonly property color colorHoverStrong: isDark ? Qt.lighter(colorBackground, 1.3) : "#e6ecf5"

    // Text / icon levels
    readonly property color colorTextPrimary:   isDark ? "#eff1f5" : "#1d222b"   // col100
    readonly property color colorTextSecondary: isDark ? "#cdd3e0" : "#394150"   // col200
    readonly property color colorTextMuted:     isDark ? "#a0acc4" : "#576378"   // col300

    // Accent (poker gold — used for active player, chips, highlights)
    readonly property color colorAccent:        "#E3C800"
    readonly property color colorAccentDim:     "#b09a00"

    // Action-Timeout-Fortschrittsbalken: schlanker Balken mit Kontur + Schatten.
    // Blaue Füllung (Self-Box etwas heller), dunkler "leerer" Track.
    readonly property color colorTimeout:        "#4070D0"
    readonly property color colorTimeoutSelf:    "#6E9CEC"
    readonly property color colorTimeoutTrack:   "#0e1a30"

    // Semantic
    readonly property color colorDanger:        "#e05050"
    readonly property color colorSuccess:       "#50c878"

    // ── Action colors (Fold / Check-Call / Bet-Raise / All-In) ───────────────
    // Eine Quelle für die Tisch-Aktionen: die Action-Buttons nutzen den hellen
    // Top/Bottom/Edge-Verlauf, die Action-Badges auf den Spielerboxen nutzen den
    // dunkleren *Badge*-Hintergrund + denselben Edge als Rand → Button und Badge
    // gehören farblich immer zusammen (Badge nur etwas dunkler).
    readonly property color colorFoldTop:     "#d94040"
    readonly property color colorFoldBottom:  "#8b1a1a"
    readonly property color colorFoldEdge:    "#e87070"
    readonly property color colorFoldBadge:   "#5a1010"   // dunkler als FoldBottom

    readonly property color colorCallTop:     "#4080d8"
    readonly property color colorCallBottom:  "#1a3d8b"
    readonly property color colorCallEdge:    "#6aa0e8"
    readonly property color colorCallBadge:   "#122a55"   // dunkler als CallBottom

    readonly property color colorRaiseTop:    "#50b840"
    readonly property color colorRaiseBottom: "#1e6614"
    readonly property color colorRaiseEdge:   "#7ad06a"
    readonly property color colorRaiseBadge:  "#123f0b"   // dunkler als RaiseBottom

    readonly property color colorAllInTop:    "#9e2a2a"
    readonly property color colorAllInBottom: "#5c1111"
    readonly property color colorAllInEdge:   "#ef5350"
    readonly property color colorAllInBadge:  "#3c0a0a"   // dunkler als AllInBottom

    // Action-Code (1=Fold,2=Check,3=Call,4=Bet,5=Raise,6=All-In) → Badge-Farben.
    function actionBadgeColor(action) {
        switch (action) {
        case 1:  return colorFoldBadge   // Fold
        case 2:                          // Check  → wie Call (blau)
        case 3:  return colorCallBadge   // Call
        case 4:                          // Bet    → wie Raise (grün)
        case 5:  return colorRaiseBadge  // Raise
        case 6:  return colorAllInBadge  // All-In
        default: return colorCallBadge
        }
    }
    function actionBadgeBorder(action) {
        switch (action) {
        case 1:  return colorFoldEdge
        case 2:
        case 3:  return colorCallEdge
        case 4:
        case 5:  return colorRaiseEdge
        case 6:  return colorAllInEdge
        default: return colorCallEdge
        }
    }

    // Chat send action (spectral green, readable on both themes)
    readonly property color colorChatSend:      isDark ? "#4ade80" : "#16a34a"

    // Game status (game list)
    readonly property color colorStatusRunning: isDark ? "#FF6D00" : "#BF360C"
    readonly property color colorStatusClosed:  isDark ? "#EF5350" : "#C62828"
    readonly property color colorStatusOpen:    isDark ? "#4CAF50" : "#2E7D32"
    readonly property color colorStatusFull:    isDark ? "#FFC107" : "#E65100"

    // Tisch-Admin (Ersteller/Host eines Spiels) in den Spielerlisten von Lobby
    // und Warteraum. Der Widget-Client hinterlegt diesen Eintrag grün – hier
    // dieselbe Aussage: grünes Badge + dezent grün getönte Listenzeile.
    readonly property color colorGameAdmin: colorStatusOpen
    readonly property color colorGameAdminRow: Qt.tint(colorPanelRow,
        Qt.rgba(colorGameAdmin.r, colorGameAdmin.g, colorGameAdmin.b, isDark ? 0.20 : 0.16))

    // Error / feedback text
    readonly property color colorError:          isDark ? "#FF5252" : "#C62828"
    readonly property color colorSuccessMessage: isDark ? "#2ecc71" : "#27ae60"

    // Danger button states (destructive actions e.g. reset settings)
    readonly property color colorButtonDangerNormal:      isDark ? "#922b21" : "#c0392b"
    readonly property color colorButtonDangerHover:       isDark ? "#c0392b" : "#e74c3c"
    readonly property color colorButtonDangerPress:       isDark ? "#7f1010" : "#922b21"
    readonly property color colorButtonDangerBorder:      isDark ? "#e74c3c" : "#ff6b6b"
    readonly property color colorButtonDangerBorderHover: isDark ? "#ff6b6b" : "#c0392b"

    // Text / icon on a colored (accent / chart) background — always light
    readonly property color colorOnAccent: "#ffffff"

    // ── Elevation / Schatten ──────────────────────────────────────────────────
    // Dezenter Schlagschatten für Panel-Karten (Lobby-Spalten, Settings-Boxen).
    // Zentral, damit sich die Tiefe der ganzen App an einer Stelle feinjustieren
    // lässt (siehe components/PanelShadow.qml). Im Light-Mode etwas kräftiger,
    // weil ein dunkler Schatten dort mehr Kontrast braucht.
    readonly property color colorShadow:         "#000000"
    readonly property real  panelShadowOpacity:  isDark ? 0.36 : 0.22
    readonly property real  panelShadowBlur:     0.55
    readonly property real  panelShadowOffset:   2

    // ── Opacity helpers ──────────────────────────────────────────────────────
    readonly property real overlayOpacity: 0.80
    readonly property real dimmedOpacity:  0.40

    // Farbe mit gesetztem Alpha zurückgeben (für transluzente Sheet-Flächen).
    function withAlpha(c, a) {
        return Qt.rgba(c.r, c.g, c.b, a)
    }
}
