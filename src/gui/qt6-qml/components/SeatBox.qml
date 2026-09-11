import QtQuick

import "../config" as Config

// Common basis of the two player boxes at the table: GamePlayerBox
// (opponents) and GamePlayerSelfBox (your own seat).
//
// Both derive the same values from GameTable.players[seatIndex] - your own
// seat is simply seat 0. Exactly these derivations used to stand word for word
// next to each other in BOTH files, which is why per-player features (e.g.
// the country flag) were regularly maintained in only one of the two.
//
// Only what applies to EVERY seat belongs in here. Everything that concerns
// only your own seat (ping display, anti-peek) or only foreign seats
// (context menu, ignore, player note) stays in the respective file -
// as does the complete look, which deliberately differs.
Item {
    id: seatBox

    // Seat whose data this box shows. Your own seat is always 0.
    property int seatIndex: 0
    // The box sits in the lower half of the table (influences the badge sides).
    property bool up: false
    // Effective table scaling of this box (oppScale × zoom), passed on to the
    // cards so that their SVG raster hits the real screen size.
    property real cardRenderScale: 1.0

    // Bet base in the seat style "inset" (Config.SeatStyle): the bet stands
    // IN the box instead of next to it, and the box grows by exactly this height. 0 in
    // the style "classic". Subtracted again from every derived size,
    // so that the box grows in HEIGHT only - the avatar, the cards and the box width
    // stay unchanged.
    readonly property bool betInset: Config.SeatStyle.betInset
    readonly property int betStripH: Config.SeatStyle.betStripExtra

    // Height of the box BODY (without the base). `height` is the height permanently
    // RESERVED at the table including the base: the body sits at the top of the
    // reserved area, the base unfolds downwards into the rest that is kept free.
    // That way the box only grows when there really is a bet - without
    // neighbouring boxes or the table scaling moving.
    readonly property int bodyH: height - betStripH
    // Only unfold the base when there is an actual bet.
    readonly property bool stripOpen: betInset && seatBox.bet > 0

    // ── Player data from GameTable ───────────────────────────────────────────
    readonly property var seatData: (typeof GameTable !== "undefined" && GameTable && GameTable.players.length > seatIndex)
        ? GameTable.players[seatIndex] : null

    readonly property int card0: seatData && seatData.card0 !== undefined ? seatData.card0 : -1
    readonly property int card1: seatData && seatData.card1 !== undefined ? seatData.card1 : -1
    // Showdown spotlight: dim an individual hole card of the winner if it
    // does not count towards the winning hand (set by the GameHandler).
    readonly property bool fade0: seatData && seatData.fade0 !== undefined ? seatData.fade0 : false
    readonly property bool fade1: seatData && seatData.fade1 !== undefined ? seatData.fade1 : false

    readonly property bool isMyTurn: seatData ? seatData.myTurn : false
    // Active player (to act): locally via seatData.myTurn (the engine sets
    // getMyTurn()), in a network game via the action timeout (timeoutSeatId) -
    // there myTurn is not set on the client side. Take both into account so that
    // the highlight frame appears in BOTH kinds of game.
    readonly property bool isAtTurn: seatBox.isMyTurn
        || ((typeof GameTable !== "undefined" && GameTable) ? GameTable.timeoutSeatId === seatBox.seatIndex : false)
    readonly property bool isWinner: typeof GameTable !== "undefined" && GameTable
        && GameTable.winnerSeatIds.indexOf(seatBox.seatIndex) !== -1

    readonly property int button: seatData && seatData.button !== undefined ? seatData.button : 0
    readonly property int bet: seatData && seatData.bet !== undefined ? seatData.bet : 0
    // Setting "show symbols for small/big blind" (config key
    // ShowBlindButtons). As in the Qt widgets client the dealer button (1) is
    // always shown, only the small blind (2) and the big blind (3) can be switched off.
    readonly property bool showBlindButtons:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("ShowBlindButtons") !== 0 : true
    readonly property bool buttonVisible:
        button === 1 || ((button === 2 || button === 3) && showBlindButtons)

    // The player has folded → the cards become translucent (as in the Qt widgets client)
    readonly property bool folded: seatData && seatData.folded !== undefined ? seatData.folded : false

    // Avatars of ignored players are hidden as in the Qt widgets client
    // (MyAvatarLabel). Only relevant for foreign seats - the opponent box
    // binds its ignore logic here, while your own seat stays at the
    // default.
    property bool hideIgnoredAvatar: false
    // The avatar that is set (a file:// URL) or "" → a placeholder.
    readonly property string avatarSource:
        (seatData && seatData.avatar !== undefined && !hideIgnoredAvatar) ? seatData.avatar : ""

    // The last action of this player (0=none,1=fold,2=check,3=call,4=bet,5=raise,6=all-in)
    readonly property int action: seatData && seatData.action !== undefined ? seatData.action : 0
    // Setting "do not translate international poker terms" (config key
    // DontTranslateInternationalPokerStringsFromStyle): action terms fixed to
    // English instead of localized. The qsTr() literals stay for the extraction.
    readonly property bool dontTranslatePokerTerms:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("DontTranslateInternationalPokerStringsFromStyle") !== 0 : false
    readonly property string actionText: Config.StaticData.pokerActionWord(seatBox.action, dontTranslatePokerTerms)

    // Country flag: directly from the seat data. In a network game the GameHandler
    // resolves it via the unique player id of the session (like the
    // Qt widgets client), not via the player list of the game - that one is
    // incomplete at the table as soon as somebody joins during the game.
    readonly property string countryCode:
        seatData && seatData.countryCode !== undefined ? seatData.countryCode : ""
}
