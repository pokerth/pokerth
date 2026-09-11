pragma Singleton
import QtQuick

// Seat style of the player boxes at the table – the QML counterpart to the "seat
// packs" of the web client (html[data-seat="…"] there: classic/plate/card/pokerth). For
// now the style determines ONLY where a player's bet is shown; further
// variants can dock on here later without rebuilding the boxes again.
//
//   "classic" – bet chip OUTSIDE the box (left/right/above/below the
//               box, depending on the seat position). The state until 08/2026.
//   "inset"   – bet chip in the base INSIDE the box; the box grows by
//               betStripHeight in height for that.
//
// The dealer/blind puck stays outside the box in BOTH variants.
//
// Both player boxes (GamePlayerBox, GamePlayerSelfBox) AND the space
// calculation of the tableZone (GamePage) read these values exclusively. A
// later settings switch (a selection as with the table styles) therefore only has
// to write `variant` – nothing is left to do at the boxes then.
QtObject {
    id: root

    // The default as long as the user has not chosen anything else – the same on
    // all platforms: the base inside the box needs no room around the
    // boxes and comes off better especially on small tables.
    readonly property string defaultVariant: "inset"

    // Active seat style. Like Theme.darkMode/effectsEnabled it is set externally by
    // the ApplicationWindow (init from the config key "QmlSeatStyle") and by
    // the style settings (live switching); a singleton cannot read the
    // SettingsManager context property itself. An empty config value
    // means "default" and leaves this default in place.
    property string variant: defaultVariant

    readonly property bool betInset: variant === "inset"

    // Height of the bet base in base pixels (before boxScale): chip icon 15 +
    // air. Deliberately kept tight – every pixel here shrinks the whole table
    // via the bisection in GamePage.boxScale.
    readonly property int betStripHeight: 20

    // Room that the bet + dealer/blind puck need NEXT TO the box (base
    // pixels, cf. GamePlayerBox.betGroup: 8 px spacing + group width).
    //   "classic" – chip icon 20 + amount (~40) → 8 + 60 = 68.
    //   "inset"   – only the puck is left there (32) → 8 + 32 = 40.
    // In style "classic" the space bisection thus honestly reserves the room
    // the bet really needs next to the box (a flat 48 before), and
    // frees it in style "inset" for larger boxes.
    readonly property int betSideOutset: betInset ? 40 : 68

    // Additional height a player box needs for the base (0 for "classic").
    // ONLY this size goes into the base dimensions of the box; all derived
    // dimensions (avatar/card row, box width) subtract it again, so that the box
    // grows in HEIGHT only.
    readonly property int betStripExtra: betInset ? betStripHeight : 0
}
