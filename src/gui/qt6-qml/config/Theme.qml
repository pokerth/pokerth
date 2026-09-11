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
    // The mode reported by the system – likewise set by the ApplicationWindow
    // (source: SettingsManager.systemDark), see StaticData.systemDark.
    property bool systemDark: true

    // 0=light → false, 1=dark → true, 2=automatic → follow the system.
    readonly property bool isDark: darkMode === 2 ? systemDark : darkMode !== 0

    // Decorative effects (drop shadow, glow, blur) globally on/off. On
    // weak / passively cooled systems (or with software rendering without a
    // GPU) the many layered MultiEffect blur passes per frame force a high
    // CPU load and stuttering. If this switch is off, all decorative
    // `layer.enabled` effects are skipped (functional layers such as icon
    // colourising / folded greyscale stay untouched). Like darkMode it is
    // set externally by the ApplicationWindow (init) and GuiSettings (live toggle),
    // since a singleton cannot read the SettingsManager context property
    // directly. The persistent config key: "QmlReduceEffects" (0 = effects on).
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

    // ── Branding box (the start page & the login dialog share these values, so that
    //    the box height and the PokerTH icon look 1:1 identical while navigating) ────────
    readonly property real brandBoxWidth: 380
    // The height that the start page keeps free at the bottom for its footer (StartFooter) –
    // the icon row plus two text lines incl. the distance to the box. The value is the
    // height of the footer itself (StartFooter.implicitHeight binds to it), so that
    // there is only ONE source for this reservation. It applies only to the
    // start page: the login dialog has no footer and keeps the full
    // area, otherwise its card would be squeezed unnecessarily.
    readonly property real startFooterReserve: compact ? 104 : 116
    // A fixed target height; it only shrinks when the window is too low (short /
    // landscape windows). Both pages use exactly this value. The subtraction
    // reserves the top bar (38px) plus the outer margin, so that the box including its border
    // fits into the visible area (StackView) and stays centred.
    readonly property real brandBoxHeight: Math.max(380, Math.min(540, windowHeight - 96))
    // The icon size is coupled to the box height, capped at 126 (desktop) or
    // 100 (narrow phones), with a floor of 56 → it never overflows.
    readonly property real brandLogoSize:
        Math.round(Math.max(56, Math.min(compact ? 100 : 126, 0.4 * brandBoxHeight - 76)))
    // The lower limit down to which the logo may shrink before other elements
    // (the buttons) have to give way.
    readonly property real brandLogoSizeMin: 56

    // ── Geometry of the branding header (logo + card symbol row) ────────────
    // As functions in the tokens, so that a caller can derive the matching logo
    // size from its height budget WITHOUT reading the measured header height –
    // that would yield a binding loop (smaller logo → more room →
    // larger logo → …). BrandHeader itself uses the same functions, so there
    // is only one source for this geometry.
    function brandHeaderSpacing(logo)  { return Math.max(6, Math.round(logo * 0.07)) }
    function brandHeaderSuitSize(logo) { return Math.max(13, logo * 0.16) }
    // Line height of the card symbols ≈ font size × 1.45 (rounded with a reserve).
    function brandHeaderHeight(logo) {
        return logo + brandHeaderSpacing(logo)
               + Math.ceil(brandHeaderSuitSize(logo) * 1.45)
    }
    // The inversion of brandHeaderHeight(): the largest logo size whose header still
    // fits into budget. The three terms are the branches of the formula (the spacing or
    // the symbol size at its minimum, or proportional); the minimum is always safe.
    function brandHeaderLogoForHeight(budget) {
        return Math.floor(Math.min(budget - 25, (budget - 6) / 1.232, budget / 1.302))
    }

    // Card symbols (♠ ♥ ♦ ♣) on the dark branding box
    readonly property color colorSuitRed:   "#c0392b"   // ♥ ♦
    readonly property color colorSuitBlack: "#cdd3e0"   // ♠ ♣ (light on the dark box)

    // ── Overlay on the fire background (start page, login, PreLoader) ────────
    // The background there is the same dark photo in BOTH modes
    // (resources/startWindowBackground.png), which is why these colours – like the
    // branding box itself – are fixed and do NOT follow the light/dark theme. A value
    // coupled to isDark would yield dark text on a dark
    // fire image in light mode. The values correspond to --gold-dim / --text-hi of the
    // PokerTH palette of the pokerth-web-client.
    readonly property color colorOverlayText:      "#a0acc4"
    readonly property color colorOverlayTextHi:    "#eff1f5"
    // A darkening gradient behind overlay text (a scrim) – the opacity at the window edge
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

    // ── Surface roles: page background vs. content box ───────────────────────
    // The direction flips between the modes, so a common
    // Qt.darker(colorBackground, f) is not enough:
    //   Dark: the page is dark, the boxes lie a bit DEEPER still (darkened)
    //           – as before, the values are unchanged.
    //   Light: the page is grey, the boxes are WHITE (the classic card
    //           layout). Before it was the other way round (a white page, grey boxes) –
    //           which made the content look dirty instead of highlighted.
    // colorField (input fields, search lines) stays set apart in BOTH modes:
    // recessed, a field looks right on a white ground as well.
    readonly property color colorPanel:     isDark ? Qt.darker(colorBackground, 1.2) : "#ffffff"
    readonly property color colorPanelRow:  isDark ? Qt.darker(colorBackground, 1.1) : "#f5f7fb"
    readonly property color colorField:     isDark ? Qt.darker(colorBackground, 1.3) : "#eaeef6"
    // Surfaces that deliberately carry the page tone in dark mode (popups, dialogs,
    // cards, overlays, the top bar, buttons): in dark everything stays exactly as before,
    // in light they become white and thereby stand out from the grey page.
    readonly property color colorBox:       isDark ? colorBackground : "#ffffff"
    // The hover surface of transparent list rows INSIDE a box. Here the direction
    // flips as well: in dark it is brightened, in light (the row lies on white)
    // slightly darkened – brightening would be invisible on white.
    readonly property color colorHover:       isDark ? Qt.lighter(colorBackground, 1.2) : "#eef2f8"
    readonly property color colorHoverStrong: isDark ? Qt.lighter(colorBackground, 1.3) : "#e6ecf5"

    // Text / icon levels
    readonly property color colorTextPrimary:   isDark ? "#eff1f5" : "#1d222b"   // col100
    readonly property color colorTextSecondary: isDark ? "#cdd3e0" : "#394150"   // col200
    readonly property color colorTextMuted:     isDark ? "#a0acc4" : "#576378"   // col300

    // Accent (poker gold — used for active player, chips, highlights)
    readonly property color colorAccent:        "#E3C800"
    readonly property color colorAccentDim:     "#b09a00"

    // Action timeout progress bar: a slim bar with a contour + a shadow.
    // A blue fill (a bit brighter in the self box), a dark "empty" track.
    readonly property color colorTimeout:        "#4070D0"
    readonly property color colorTimeoutSelf:    "#6E9CEC"
    readonly property color colorTimeoutTrack:   "#0e1a30"

    // Semantic
    readonly property color colorDanger:        "#e05050"
    readonly property color colorSuccess:       "#50c878"

    // ── Action colours (fold / check-call / bet-raise / all-in) ──────────────
    // One source for the table actions: the action buttons use the light
    // top/bottom/edge gradient, the action badges on the player boxes use the
    // darker *badge* background + the same edge as the border → the button and the badge
    // always belong together in colour (the badge only a bit darker).
    readonly property color colorFoldTop:     "#d94040"
    readonly property color colorFoldBottom:  "#8b1a1a"
    readonly property color colorFoldEdge:    "#e87070"
    readonly property color colorFoldBadge:   "#5a1010"   // darker than FoldBottom

    readonly property color colorCallTop:     "#4080d8"
    readonly property color colorCallBottom:  "#1a3d8b"
    readonly property color colorCallEdge:    "#6aa0e8"
    readonly property color colorCallBadge:   "#122a55"   // darker than CallBottom

    readonly property color colorRaiseTop:    "#50b840"
    readonly property color colorRaiseBottom: "#1e6614"
    readonly property color colorRaiseEdge:   "#7ad06a"
    readonly property color colorRaiseBadge:  "#123f0b"   // darker than RaiseBottom

    readonly property color colorAllInTop:    "#9e2a2a"
    readonly property color colorAllInBottom: "#5c1111"
    readonly property color colorAllInEdge:   "#ef5350"
    readonly property color colorAllInBadge:  "#3c0a0a"   // darker than AllInBottom

    // Action-Code (1=Fold,2=Check,3=Call,4=Bet,5=Raise,6=All-In) → Badge-Farben.
    function actionBadgeColor(action) {
        switch (action) {
        case 1:  return colorFoldBadge   // Fold
        case 2:                          // Check  → like call (blue)
        case 3:  return colorCallBadge   // Call
        case 4:                          // Bet    → like raise (green)
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

    // The table admin (creator/host of a game) in the player lists of the lobby
    // and the waiting room. The widget client backs this entry in green – here
    // the same statement: a green badge + a subtly green tinted list row.
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

    // ── Elevation / shadow ────────────────────────────────────────────────────
    // A subtle drop shadow for panel cards (lobby columns, settings boxes).
    // Centrally, so that the depth of the whole app can be fine-tuned in one
    // place (see components/PanelShadow.qml). A bit stronger in light mode,
    // because a dark shadow needs more contrast there.
    readonly property color colorShadow:         "#000000"
    readonly property real  panelShadowOpacity:  isDark ? 0.36 : 0.22
    readonly property real  panelShadowBlur:     0.55
    readonly property real  panelShadowOffset:   2

    // ── Opacity helpers ──────────────────────────────────────────────────────
    readonly property real overlayOpacity: 0.80
    readonly property real dimmedOpacity:  0.40

    // Return the colour with the alpha set (for translucent sheet surfaces).
    function withAlpha(c, a) {
        return Qt.rgba(c.r, c.g, c.b, a)
    }
}
