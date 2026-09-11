import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../components"
import "../config" as Config
import "seatlayout.js" as SL

Rectangle {
    id: gamePage
    objectName: "gamePage"
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    color: "transparent"

    // ── Table theme colours for the chat/log box ──────────────────────────────
    // Independent of the light/dark mode of the rest of the app: the source is solely the
    // table theme (StyleProvider.chatLog*, overridable via XML); the
    // fallback only applies if the context property is not set at some point.
    readonly property color tblChatBackground:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogBackground : "#1d222b"
    readonly property color tblChatSurface:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogSurface : "#394150"
    readonly property color tblChatBorder:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogBorder : "#576378"
    readonly property color tblChatText:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogText : "#eff1f5"
    readonly property color tblChatTextSecondary:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogTextSecondary : "#cdd3e0"
    readonly property color tblChatTextMuted:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogTextMuted : "#7787a3"
    readonly property color tblChatAccent:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogAccent : "#E3C800"
    readonly property color tblChatAccentText:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogAccentText : "#101010"
    readonly property color tblChatSend:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogSend : "#4ade80"

    // Switch the game mode – the actual logic (incl. the deferred
    // auto action) lives in the GameActionBar; here only as a forwarding for
    // the keyboard shortcuts.
    function applyPlayingMode(index) {
        if (actionBar)
            actionBar.applyPlayingMode(index)
    }

    // A spectator: neither a table chat nor a history/odds. The keyboard shortcuts
    // have to be off as well – otherwise Alt+C/Alt+L/Alt+I would open an overlay whose
    // close button (the toggle) is not there at all any more.
    readonly property bool spectating: tableZone ? tableZone.spectating : false

    // ── The info panel (history/odds/hand) ────────────────────────────────────
    // Desktop: permanently docked at the bottom RIGHT (mirrored to the chat at the bottom left),
    // floating ABOVE the table (it reserves no room). If the width is not enough
    // (or on mobile), it stays the floating overlay with a toggle. The toggle
    // folds the docked panel in/out (dockedInfoCollapsed).
    readonly property bool infoDocked: tableZone ? tableZone.dockedInfoFits : false
    // The visibility/active state of the toggle depending on the mode.
    readonly property bool infoPanelOpen:
        infoDocked ? !tableZone.dockedInfoCollapsed
                   : (tableZone ? tableZone.showInfo : false)

    // Open the panel or select a tab (shortcuts).
    function showInfoTab(idx) {
        if (gamePage.infoDocked) {
            tableZone.dockedInfoCollapsed = false
            infoPanelDock.currentIndex = idx
        } else {
            tableZone.showInfo = true
            infoPanelOverlay.currentIndex = idx
            if (!tableZone.wide)
                tableZone.showChat = false
        }
    }

    // The combined toggle: show/hide the panel (docked: fold it in).
    function toggleInfoOverlay() {
        if (gamePage.infoDocked) {
            tableZone.dockedInfoCollapsed = !tableZone.dockedInfoCollapsed
            return
        }
        tableZone.showInfo = !tableZone.showInfo
        if (tableZone.showInfo && !tableZone.wide)
            tableZone.showChat = false
    }

    // Alt+L (the history tab) – the name for compatibility with the previous assignment.
    // A toggle: if the history tab is already open, the shortcut closes the
    // panel again – docked (desktop) just as much as as an overlay.
    function toggleLogOverlay() {
        if (gamePage.infoDocked) {
            if (!tableZone.dockedInfoCollapsed && infoPanelDock.currentIndex === 0)
                tableZone.dockedInfoCollapsed = true
            else
                showInfoTab(0)
            return
        }
        if (tableZone.showInfo && infoPanelOverlay.currentIndex === 0)
            tableZone.showInfo = false
        else
            showInfoTab(0)
    }

    // The chat analogous to the info panel: desktop → permanently docked at the bottom left
    // (chatDocked), foldable in via a toggle; otherwise a floating overlay.
    readonly property bool chatDocked: tableZone ? tableZone.dockedChatFits : false
    readonly property bool chatPanelOpen:
        chatDocked ? !tableZone.dockedChatCollapsed
                   : (tableZone ? tableZone.showChat : false)

    function toggleChatOverlay() {
        if (!tableZone)
            return
        if (gamePage.chatDocked) {
            tableZone.dockedChatCollapsed = !tableZone.dockedChatCollapsed
            return
        }
        tableZone.showChat = !tableZone.showChat
        if (tableZone.showChat && !tableZone.wide)
            tableZone.showInfo = false
    }

    // ── Emoji reactions (a port from the web client) ─────────────────────────
    // Sending goes through the game chat with the web client convention
    // "/emoji 🎉"; received reactions are intercepted by GameHandler::appendChat and
    // reported via reactionReceived. Our own reactions are played locally right away
    // – the server echo is deduplicated by a time window.
    property string _lastOwnReactionEmoji: ""
    property double _lastOwnReactionTime: 0

    // Emoji reactions can be disabled in the settings
    // (the config key "DisableEmojiReactions"). Since readConfigInt() is not
    // reactive, the value is read again when the page appears – the
    // settings lie above it as a StackView page of their own, and on returning
    // activated() is triggered.
    // In addition, reactions only make sense in network games in which
    // human opponents play along – in a local game (computer opponents only)
    // the toggle is therefore hidden.
    property bool emojiReactionsEnabled: true
    function refreshEmojiReactionsEnabled() {
        var enabledInSettings = SettingsManager
            ? SettingsManager.readConfigInt("DisableEmojiReactions") === 0 : true
        var isLocalGame = (typeof GameTable !== "undefined" && GameTable
            && GameTable.isLocalGameRunning())
        emojiReactionsEnabled = enabledInSettings && !isLocalGame
    }
    Component.onCompleted: refreshEmojiReactionsEnabled()
    StackView.onActivated: refreshEmojiReactionsEnabled()

    function sendReaction(emoji) {
        tableZone.showReactions = false
        _lastOwnReactionEmoji = emoji
        _lastOwnReactionTime = Date.now()
        playReactionAtSeat(0, emoji)
        if (GameTable)
            GameTable.sendChat("/emoji " + emoji)
    }

    function playReactionAtSeat(seatIdx, emoji) {
        var px, py
        // Seat 0 sits in the self box – except as a spectator, then it is an
        // ordinary ring seat and is found via slotForSeat().
        if (seatIdx <= 0 && !tableZone.spectating) {
            px = selfBox.x + selfBox.width / 2
            py = selfBox.y + selfBox.height / 2
                 - (selfBox.height * tableZone.boxScale) / 2 - 6
        } else {
            var slot = tableZone.slotForSeat(seatIdx)
            if (!slot) return
            px = tableZone.width * slot.x + (slot.nudgeX || 0)
            py = tableZone.height * slot.y + slot.nudge
                 - (tableZone.oppBaseHeight * tableZone.boxScale) / 2 - 6
        }
        // The animation rises ~200 px – for seats close to the upper table
        // edge start lower, otherwise it is cut off at the top.
        py = Math.max(py, 205)
        reactionFx.play(emoji, px, py)
    }

    // All occupied seat names (for the Tab nickname completion in the chat).
    function gameNickList() {
        var nicks = []
        if (typeof GameTable !== "undefined" && GameTable)
            for (var i = 0; i < GameTable.players.length; i++)
                if (GameTable.players[i].name !== "")
                    nicks.push(GameTable.players[i].name)
        return nicks
    }

    Connections {
        target: GameTable
        function onReactionReceived(playerName, emoji) {
            if (!gamePage.emojiReactionsEnabled)
                return
            // console.log("[REACT] received from", playerName, "->", emoji)
            var players = GameTable.players
            var idx = -1
            for (var i = 0; i < players.length; i++)
                if (players[i].name !== "" && players[i].name === playerName) { idx = i; break }
            // Suppress the echo of our own reaction, which has already been played locally.
            if (idx <= 0
                && emoji === gamePage._lastOwnReactionEmoji
                && Date.now() - gamePage._lastOwnReactionTime < 3000)
                return
            if (idx < 0) {
                // The sender was not found at the table (e.g. a spectator):
                // play it above the centre of the table.
                reactionFx.play(emoji, tableZone.width / 2, tableZone.communityCenterY - 40)
                return
            }
            gamePage.playReactionAtSeat(idx, emoji)
        }
    }

    // ── The F key assignment of the game table actions (1:1 from the Qt widgets client) ──
    // F1–F4 trigger fold/call-check/bet-raise/all-in; the order reverses
    // with AlternateFKeysUserActionMode (the setting "reverse the F keys"):
    //   normal:    F1 fold · F2 call/check · F3 bet/raise · F4 all-in
    //   alternate: F1 all-in · F2 bet/raise · F3 call/check · F4 fold
    // F5 reveals your own cards, F6/F7/F8 switch the game mode.
    readonly property bool fKeysAlternate:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
        ? SettingsManager.readConfigInt("AlternateFKeysUserActionMode") !== 0 : false

    function fKeyAction(which) {
        if (actionBar)
            actionBar.clickAction(which)
    }

    // This GamePage is the topmost one in the stack – even when ANOTHER page
    // lies above it (the table/player statistics, the settings, the community pages).
    // That is exactly what the property is for: `visible` is false in that case, but the
    // turn timer of the server keeps running. The action keys have to stay
    // reachable then, otherwise they are – for no reason from the player's point of view –
    // mute until they happen to navigate back to the table.
    // It is re-evaluated on every navigation via depth/currentItem (the pattern of
    // mainWindow.inLobbySession); find() searches from the top and thus delivers the
    // GamePage that was pushed last. That way only ONE set of shortcuts is ever
    // active even with a second GamePage in the stack (which the push guards
    // actually prevent) – two active shortcuts with the same sequence do not fire
    // at all in Qt any more, only activatedAmbiguously.
    readonly property bool topGamePage: {
        var view = gamePage.StackView.view
        if (!view)
            return false
        var _d = view.depth
        var _c = view.currentItem
        return view.find(function(it) { return it && it.objectName === "gamePage" }) === gamePage
    }

    // Diagnostics for exactly this case: if the shortcut fires ambiguously, the
    // action is discarded silently. Without a log the player could not distinguish that from a
    // "broken" key.
    function ambiguousShortcut(seq) {
        console.warn("[SHORTCUT] '" + seq + "' ist mehrdeutig (zwei aktive Shortcuts "
                     + "mit derselben Sequenz) – Aktion NICHT ausgeführt.")
    }

    Shortcut {
        sequence: "Alt+L"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.toggleLogOverlay()
    }
    Shortcut {
        sequence: "Alt+C"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.toggleChatOverlay()
    }
    Shortcut {
        sequence: "Alt+I"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.toggleInfoOverlay()
    }
    Shortcut {
        sequence: "Alt+F"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.applyPlayingMode(2)
    }
    Shortcut {
        sequence: "Alt+M"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.applyPlayingMode(0)
    }
    Shortcut {
        sequence: "Alt+K"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.applyPlayingMode(1)
    }
    // F11 (fullscreen) deliberately does NOT sit here but on the ApplicationWindow
    // (pokerth.qml) – it should work on every page, not only at the table.

    // ── Game table actions: the F keys (see fKeysAlternate above) ──
    // Off for spectators: the action bar is hidden then (actionBar.visible),
    // so its controls would be reachable by keyboard but not by mouse.
    // F11 (fullscreen) deliberately stays active for spectators as well.
    //
    // The condition is topGamePage (NOT visible): F1–F5 intervene in the running game
    // and have to work even when another page lies above
    // the table – the turn timer keeps running there. The panel toggles and the
    // game mode keys below, by contrast, stay on visible, they only operate the
    // user interface of the table.
    Shortcut {
        sequence: "F1"
        context: Qt.ApplicationShortcut
        enabled: gamePage.topGamePage && !gamePage.spectating
        onActivated: gamePage.fKeyAction(gamePage.fKeysAlternate ? "allin" : "fold")
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F1")
    }
    Shortcut {
        sequence: "F2"
        context: Qt.ApplicationShortcut
        enabled: gamePage.topGamePage && !gamePage.spectating
        onActivated: gamePage.fKeyAction(gamePage.fKeysAlternate ? "raise" : "call")
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F2")
    }
    Shortcut {
        sequence: "F3"
        context: Qt.ApplicationShortcut
        enabled: gamePage.topGamePage && !gamePage.spectating
        onActivated: gamePage.fKeyAction(gamePage.fKeysAlternate ? "call" : "raise")
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F3")
    }
    Shortcut {
        sequence: "F4"
        context: Qt.ApplicationShortcut
        enabled: gamePage.topGamePage && !gamePage.spectating
        onActivated: gamePage.fKeyAction(gamePage.fKeysAlternate ? "fold" : "allin")
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F4")
    }
    Shortcut {
        sequence: "F5"
        context: Qt.ApplicationShortcut
        enabled: gamePage.topGamePage && !gamePage.spectating
        onActivated: if (GameTable && GameTable.canShowCards) GameTable.showMyCards()
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F5")
    }
    Shortcut {
        sequence: "F6"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.applyPlayingMode(0)   // Manuell
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F6")
    }
    Shortcut {
        sequence: "F7"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.applyPlayingMode(2)   // Auto Check/Fold
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F7")
    }
    Shortcut {
        sequence: "F8"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.applyPlayingMode(1)   // Auto Check/Call
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F8")
    }

    // gameBackground (the diamond pattern) was removed – it is not needed any more.


    // ── The table layout (portrait & landscape) ───────────────────────────────
    // A uniform structure for all window sizes:
    //   the status bar → the large table (all players overlaid) → the action bar.
    // The player slots rearrange themselves automatically depending on the table aspect
    // ratio (tall/wide) – no separate desktop layout any more.
    ColumnLayout {
        id: portraitLayout
        anchors.fill: parent
        spacing: 0

        // 1. The status bar: the game phase | the pot | the hand number
        // Tighter in landscapeCompact (28 instead of 40) — it creates ~12 px more
        // tableZone height for the necklace ellipse.
        // z above the tableZone (the next ColumnLayout element, which would otherwise be
        // drawn above it): the top centre player box (the TC slot) sticks into the
        // strip of the bar and would otherwise intercept the click on the
        // (clickable) table name with its MouseArea.
        GameStatusBar {
            z: 1
            Layout.fillWidth: true
            Layout.preferredHeight: Config.Responsive.landscapeCompact ? 28 : 40
        }

        // 2. The table zone: the green table graphic fills the whole room, all players overlaid
        Item {
            id: tableZone
            Layout.fillWidth: true
            Layout.fillHeight: true

            // The green table graphic fills the whole zone. At the bottom of the image lies the
            // wooden table edge → align the crop at the lower edge, so that it
            // stays visible in wide landscape as well (in portrait it does anyway).
            // Landscape: it reaches behind the shrunk action box down to the
            // lower screen edge, so that no dark strip stays there.
            // The clip container for the table background image.
            //   • Clip at the TOP at the tableZone edge → the image scaled up to
            //     cover (in the center mode) can NOT cover the status/navigation bar
            //     beyond the upper edge.
            //   • Extend it at the BOTTOM to the window edge: the action box is the third
            //     ColumnLayout element and takes room away at the bottom, so that the tableZone
            //     ends at its upper edge. The container therefore reaches further down by the
            //     height of the action box (the tableZone does not clip itself),
            //     so that the table graphic stays visible BEHIND the action box down to the lower
            //     window edge (no dark strip).
            // The player boxes/badges stay direct children of the tableZone and may
            // still stick out beyond the edge.
            Item {
                id: tableBgClip
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: tableZone.height + (tableZone.wide ? actionBar.height : 0)
                clip: true

                Image {
                    id: tableBackgroundImage
                    // We use the image source as the implicit size (implicitWidth/Height)
                    source: (typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.tableBackground !== "")
                            ? StyleProvider.tableBackground : "../resources/tableGreen.png"
                    fillMode: Image.PreserveAspectCrop
                    smooth: true

                    // ── The center mode (e.g. danuxi) ────────────────────────────
                    // The image fills the WHOLE game table edge to edge (down to the lower
                    // window edge behind the action box) and the table sits in the
                    // centre of the player box ellipse (communityCenterY). So that the
                    // image covers the complete zone even with an off-centre ellipse,
                    // it is scaled up far enough that – centred on (the zone centre,
                    // the ellipse centre) – it covers ALL four edges (a stronger,
                    // symmetric cropping of the outer edge = "more crop").
                    // Without the center style: a classic cover.
                    // The reference size is deliberately the tableZone (NOT parent/tableBgClip –
                    // whose height is larger by the height of the action box).
                    readonly property bool centerMode:
                        typeof StyleProvider !== "undefined" && StyleProvider
                        && StyleProvider.tableBackgroundAlignment === "center"
                    readonly property real srcW: Math.max(1, implicitWidth  || tableZone.width)
                    readonly property real srcH: Math.max(1, implicitHeight || tableZone.height)
                    // Additional height for the area BEHIND the action bar. If it is
                    // hidden (a spectator), there is nothing to cover – otherwise the
                    // surcharge would push the table graphic down out of the centre.
                    readonly property real coverExtra:
                        (tableZone.wide && actionBar.visible) ? actionBar.height : 0
                    // The vertical centre of the player box ellipse (zone coordinates).
                    readonly property real ellipseCenterY: tableZone.communityCenterY
                    // The crop/zoom factor from the style (>= 1.0, default 1.0): larger =
                    // a larger table / more cropping of the outer edge. Adjustable via the data
                    // (the XML <TableBackgroundZoom>) – no rebuild needed.
                    readonly property real centerZoom:
                        centerMode && StyleProvider.tableBackgroundZoom > 0
                        ? Math.max(1.0, StyleProvider.tableBackgroundZoom) : 1.0
                    // The scaling that – centred on (the zone centre, ellipseCenterY) –
                    // covers the whole zone (incl. the area behind the action box),
                    // multiplied by the zoom/crop factor.
                    readonly property real fillScale: {
                        var reqH = 2 * Math.max(ellipseCenterY,
                                                tableZone.height + coverExtra - ellipseCenterY)
                        return Math.max(tableZone.width / srcW, reqH / srcH) * centerZoom
                    }

                    width:  centerMode ? Math.round(srcW * fillScale) : tableZone.width
                    height: centerMode ? Math.round(srcH * fillScale)
                                       : tableZone.height + coverExtra
                    // Centre it horizontally on the zone centre, vertically on the ellipse centre
                    // → the table lies in the centre of the image = the centre of the ellipse.
                    x: centerMode ? Math.round(tableZone.width / 2 - width / 2) : 0
                    y: centerMode ? Math.round(ellipseCenterY - height / 2) : 0
                }
            }

            // The placeholder mode: seats of players who have left the table
            // (a disconnect/kick/leaving/being knocked out – marked by the GameHandler with
            // `reserved`) stay part of the ring. They are merely not
            // drawn, but still occupy their slot → the ellipse is NOT
            // redistributed and the remaining boxes stay where they are.
            readonly property bool keepEmptySeats: Config.Parameters.keepEmptySeats

            // The only criterion for whether a seat claims a ring slot.
            // It has to apply everywhere where slots are counted/assigned (seatCount,
            // slotForSeat, oppOrder in the delegate) – otherwise the order and the
            // slot count drift apart.
            function seatOnRing(p) {
                if (!p) return false
                return p.name !== "" || (keepEmptySeats && p.reserved === true)
            }

            // The number of seats that claim a place in the ring
            readonly property int seatCount: {
                if (typeof GameTable === "undefined" || !GameTable) return 1
                var c = 0
                for (var i = 0; i < GameTable.players.length; i++)
                    if (seatOnRing(GameTable.players[i])) c++
                return Math.max(c, 1)
            }

            // The maximum number of players since the start of the game – it is used for boxScale,
            // so that players who drop out do NOT change the box size.
            // It is only adjusted upwards (when new players join) or
            // reset (when the game ends, i.e. seatCount falls to 1).
            property int _peakSeatCount: 1
            property bool _gameWasActive: false
            onSeatCountChanged: {
                if (seatCount > 1) {
                    if (seatCount > _peakSeatCount) {
                        _peakSeatCount = seatCount
                    }
                    _gameWasActive = true
                } else if (_gameWasActive) {
                    // The game is finished: reset the peak, so that the next game
                    // scales with its own number of players.
                    _peakSeatCount = 1
                    _gameWasActive = false
                }
            }

            // A wide table (landscape) vs. a tall table (portrait) – the
            // player slots rearrange themselves automatically depending on the aspect ratio.
            readonly property bool wide: width >= height

            // The spectator mode: no seat of your own. The place at the bottom centre
            // where the (larger) self box otherwise sticks becomes a perfectly
            // normal ring seat for seat 0 – all boxes are the same size and
            // distributed evenly over the ellipse.
            readonly property bool spectating:
                (typeof GameTable !== "undefined" && GameTable) ? GameTable.spectating : false

            // The opponent and the self box grow together in landscape. The reference is
            // not only the absolute width but how much additional width
            // arises at the same height: that way the boxes react visibly
            // faster when dragging from portrait to wide.
            // The base dimensions: in portrait the opponent box is LOWER (71 instead of
            // 84), because otherwise the square avatar (= topRow.height) gets too
            // wide and the cards stick out of the cardsLane
            // horizontally. With 71 topRow becomes ≈ 45 → 2 cards + the avatar fit
            // comfortably next to each other. In landscape (84) the 2 line
            // footer is 44 px, topRow = 40 → the avatar/cards are visibly larger.
            //
            // The seat style "inset" (Config.SeatStyle): the bet stands in the base
            // INSIDE the box – the box gets higher by betStripH for that. This
            // additional height deliberately sits in oppBaseHeight/selfBaseHeight, so that
            // the complete space bisection below (boxScale, the ellipse radii,
            // the community scale, the chat/info dock) automatically computes with the larger box
            // and no new overlaps arise. The dimensions DERIVED
            // from it (the box width, the card/avatar height) subtract it
            // again – the box grows in height only.
            readonly property int betStripH: Config.SeatStyle.betStripExtra
            readonly property int oppBaseHeight: (wide ? 84 : 71) + betStripH
            // A dynamic width: 2×hMargin + AvatarCardRow.implicitWidth.
            // AvatarCardRow: avatarH + gap(4) + 2·cardW + cardSpacing(4)
            //   Landscape: topRow=40, cardW=29 → 2×4 + 40 + 4 + 2×29 + 4 = 114
            //   Portrait : topRow=43, cardW=31 → 2×4 + 43 + 4 + 2×31 + 4 = 121
            readonly property int oppBaseWidth: {
                var rowH = oppBaseHeight - (wide ? 44 : 28) - betStripH
                var cw   = Math.round(rowH * 120 / 168)
                return 2 * 4 + rowH + 4 + 2 * cw + 4
            }
            // The self box is deliberately LARGER than the opponent boxes (your own box is prominent) —
            // in EVERY mode: desktop wide 96, compact landscape 94, portrait 82
            // (the opponent boxes: 84 or 71). The bisection derives selfVisualH/bottomY/
            // selfClearX/selfVisualTopY from selfBaseHeight → it automatically reserves
            // more room (the opponents' bottom seats move up), hence no overlaps.
            // As a spectator an ordinary seat sits at the bottom: the same dimensions as the
            // opponent boxes, so that the ring looks even.
            readonly property int selfBaseHeight:
                spectating ? oppBaseHeight
                           : ((!wide ? 82 : (Config.Responsive.landscapeCompact ? 94 : 96))
                              + betStripH)
            // The self box width is dynamic: the same spacings as the opponent boxes.
            //   Compact  : cardsH=46, cardW=33, avW=46 → 2×4 + 46 + 4 + 2×33 + 4 = 128
            //   Landscape: cardsH=40, cardW=29, avW=40 → 2×4 + 40 + 4 + 2×29 + 4 = 114
            //   Portrait : cardsH=41, cardW=29, avW=41 → 2×4 + 41 + 4 + 2×29 + 4 = 115
            readonly property int selfBaseWidth: {
                if (spectating) return oppBaseWidth
                var cH  = selfBaseHeight - betStripH - 12 - (Config.Responsive.landscape ? 32 : 18)
                var cW  = Math.round(cH * 120 / 168)
                var avS = Math.min(cH, 60)
                return 2 * 4 + avS + 4 + cW * 2 + 4
            }
            readonly property real opponentGapBase: 10
            readonly property real opponentHorizontalGapBase: opponentGapBase * 2.8
            readonly property real selfGapBase: opponentGapBase * 2
            // A vertical safety padding between the bottom seats and the self box.
            readonly property real selfBadgeGapBase: 8
            // The room that the bet/puck needs NEXT TO a box. On mobile devices it is
            // derived from the seat style (Config.SeatStyle.betSideOutset): in the
            // style "inset" only the dealer puck stands there (40 instead of 68) →
            // the bisection may make the boxes correspondingly larger; in the style
            // "classic" the room for the bet chip is honestly
            // reserved for it. The desktop stays at the previous flat value.
            readonly property real sideBadgeGapBase:
                Config.Responsive.isMobile ? Config.SeatStyle.betSideOutset : 48
            readonly property int landscapeRowCount: seatCount <= 4 ? 1
                : seatCount <= 6 ? 2
                : seatCount <= 8 ? 3
                : 4
            // The inputs of the layout computation (seatlayout.js). Deliberately a single
            // object: that way it is visible in one place what the box size and the seat
            // positions depend on - and the mathematics stays free of QML objects.
            readonly property var layoutEnv: ({
                width: width, height: height, wide: wide, spectating: spectating,
                seatCount: seatCount, ringCount: ringCount,
                peakSeatCount: _peakSeatCount,
                oppBaseWidth: oppBaseWidth, oppBaseHeight: oppBaseHeight,
                selfBaseWidth: selfBaseWidth, selfBaseHeight: selfBaseHeight,
                opponentGapBase: opponentGapBase,
                opponentHorizontalGapBase: opponentHorizontalGapBase,
                selfGapBase: selfGapBase, selfBadgeGapBase: selfBadgeGapBase,
                sideBadgeGapBase: sideBadgeGapBase,
                landscapeRowCount: landscapeRowCount,
                Config: { Responsive: { landscapeCompact: Config.Responsive.landscapeCompact,
                                        isMobile: Config.Responsive.isMobile },
                          SeatStyle: { betSideOutset: Config.SeatStyle.betSideOutset } }
            })

            // The largest scale at which all boxes lie without overlapping (bisection).
            readonly property real boxScale: SL.boxScale(layoutEnv)
            readonly property real oppScale: boxScale
            // The community card scale:
            //   – landscape (desktop + Android compact): they fill the free
            //     table centre (gap based, see below), the floor is the bisection reserve.
            //   – portrait: it fills the reserved middle band, limited by the
            //     side columns (horizontally) and the top/bottom row (vertically).
            readonly property real communityScale: {
                if (wide) {
                    // Landscape (desktop AND Android compact): the community cards
                    // FILL the free table centre instead of only growing along
                    // with boxScale. We measure the vertical gap around the drop point
                    // communityCenterY and scale by it. The divisor 84 (> the pure
                    // half height 64) leaves some air.
                    //   – topB: the lower edge of the topmost box + its bet badge
                    //     pointing downwards (compact 39, otherwise 26 · oppScale).
                    //   – selfTop: the upper edge of the (scaled) self box.
                    // It depends ONLY on boxScale → no circularity. The floor =
                    // the bisection reserve (compact 1.1, otherwise 0.72 · boxScale) →
                    // a guaranteed minimum gap, no regression risk.
                    var isCmp     = Config.Responsive.landscapeCompact
                    var topB      = topOpponentBottomY + (isCmp ? 39 : 26) * oppScale
                    var halfAbove = communityCenterY - topB - 6
                    var halfBelow = selfVisualTopY - communityCenterY - 6
                    var avail     = Math.min(halfAbove, halfBelow)
                    // Filled compactly (82), but deliberately not as aggressively as
                    // before (66): on small phone landscape resolutions (the iPhone
                    // mini) the community cards otherwise looked disproportionately large
                    // compared to the (small) opponent boxes. The desktop stays at 84.
                    var gapFill   = avail > 0 ? avail / (isCmp ? 82 : 84) : 0
                    // Horizontal safety: the card row (~264 base width) stays
                    // within 70 % of the window width (the middle between the side boxes).
                    var capW      = (0.70 * width) / 264
                    var cap       = Math.min(isCmp ? 2.1 : 1.8, boxScale * 2.0, capW)
                    // The floor (the minimum size): lowered to 0.85·boxScale in compact (it was
                    // 1.1) – that way the cards stay proportional to the boxes instead of
                    // towering over them. The desktop is unchanged (0.72).
                    var floor     = boxScale * (isCmp ? 0.85 : 0.72)
                    return Math.max(0.55, Math.min(cap, Math.max(floor, gapFill)))
                }
                // Portrait: the community sits vertically BETWEEN the upper (~0.345)
                // and the lower (~0.65) row – at its height (the table centre) there are
                // NO side boxes. It may therefore become wide horizontally; the
                // only overlap limit is the vertical distance to the next
                // row (~0.15·height minus half the box height). Horizontally it is only limited by the
                // screen width. → considerably larger than the earlier
                // side column cap (which wrongly assumed boxes at community height).
                // Mobile: the middle band is no longer a fixed 0.305·height but
                // exactly the rest between the upper and the lower seat group
                // (buildPortraitSlots) – the cards fill it out.
                var vHalf = Config.Responsive.isMobile
                    ? (portraitBand[1] - portraitBand[0]) / 2 - 6
                    : 0.15 * height - oppBaseHeight * boxScale / 2 - 6
                var maxScaleV = vHalf > 0 ? vHalf / 62 : 0.55
                var maxScaleScreen = Math.max(0, width - 16) / 264
                return Math.max(0.55, Math.min(1.8, maxScaleV, maxScaleScreen))
            }

            // ── The magnifier: zoom + pan of the opponent zone (compact only) ──────────────────
            property bool  zoomActive: false
            readonly property real zoomFactor: 2.0
            // The discrete zoom factor (the target, NOT the animated scale value) for the
            // card rasterization: that way it is rasterized once in the larger
            // resolution on the zoom toggle instead of per animation frame (see CardImage.renderScale).
            readonly property real zoomRenderMul: zoomActive ? zoomFactor : 1.0
            property real  _zoomPanX: 0
            property real  _zoomPanY: 0
            // It remembers whether the zoom was active before the showdown. In the showdown it zooms
            // out to the table overview; for the next hand a zoom that was active
            // before is switched on again automatically.
            property bool  _zoomSuspendedByShowdown: false
            // Deferred following of the active player (the compact zoom):
            // when a player is to act, the excerpt does NOT jump to them immediately
            // but only when they act (refreshActionTriggered) OR 1/4 of their
            // thinking time has passed. That way the table/community area stays visible
            // longer after a new community card.
            property int   _pendingFollowSeat: -1   // the planned pan seat (the timer is running)
            property int   _followedSeat: -1        // the active seat that has already been panned to
            // The pan animation on releasing/resetting; disabled during
            // an active drag, so that the finger is followed without a delay.
            Behavior on _zoomPanX {
                enabled: !zoomPanner.active
                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
            }
            Behavior on _zoomPanY {
                enabled: !zoomPanner.active
                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
            }

            // Fixed slot positions (the centre of the box as a fraction 0..1 of the zone).
            // Portrait: 3 at the top, the rest at the sides going down.
            // A vertical arrangement with identical inner gaps at the top/bottom
            // (TL↔L_upper = L_lower↔L_bottom = 0.135). The middle between
            // L_upper and L_lower (0.305) deliberately stays larger and is
            // reserved for the community card area. That way player
            // 1↔2 and player 3↔4 (or 8↔9 and 6↔7) are each spaced
            // identically (a user wish for symmetry).
            // The column x at 0.14 (instead of 0.15), so that the boxes sit a bit
            // further out at medium portrait sizes.
            // "BC" (bottom center) is only used in the spectator mode: there an
            // ordinary ring seat (seat 0) stands there instead of the self box.
            // Portrait on MOBILE DEVICES: the seat rows are derived from the actual
            // box size – as in landscape (buildLandscapeSlots) – instead of being
            // hard-wired. The static fractions above were
            // tailored to the box that used to be 20 px lower WITHOUT a bet base:
            // their row spacing of 0.135·height capped the
            // bisection (the boxes got noticeably smaller in the style "inset"), while
            // room lay fallow in the middle band (0.305·height, reserved for the community).
            // Distributed dynamically, the boxes get exactly what
            // the community row and the self box leave over.
            // Desktop portrait (a narrow window) keeps the fixed values.
            readonly property var slotPosPortrait:
                Config.Responsive.isMobile
                    ? SL.buildPortraitSlots(layoutEnv, boxScale, ringCount)
                    : SL.slotPosPortraitFixed


            // The middle band of the current distribution (only in mobile portrait).
            readonly property var portraitBand:
                (!wide && Config.Responsive.isMobile)
                    ? SL.portraitBandAt(layoutEnv, boxScale, slotPosPortrait,
                                        slotSeq[ringCount] || [])
                    : [0, 0]
            // Landscape: the slot distances are computed from the visual box size,
            // the number of players and the self distance instead of being hard-wired
            // as an open ellipse. The horizontal and the vertical opponent distance are limited separately;
            // towards the self box deliberately more air is kept.
            readonly property var slotPosLandscape:
                SL.buildLandscapeSlots(layoutEnv, boxScale, ringCount).slots
            readonly property var slotPos: wide ? slotPosLandscape : slotPosPortrait


            // The slot order of the current distribution (the tables in seatlayout.js).
            readonly property var slotSeq: SL.slotSeq(layoutEnv)
            // The number of seats distributed on the ring: as a spectator all of them,
            // otherwise all except your own (which sits in the self box at the bottom).
            readonly property int ringCount: spectating ? seatCount : seatCount - 1

            // zoomContent.transformOrigin == TopLeft, x=(1−sc)·w/2 + panX
            // → the screen centre on the content point (cx,cy): panX = w − cx·sc
            function _panToPoint(cx, cy) {
                var sc = zoomFactor
                var maxX = (sc - 1) * width  / 2
                var maxY = (sc - 1) * height / 2
                _zoomPanX = Math.max(-maxX, Math.min(maxX, width  - cx * sc))
                _zoomPanY = Math.max(-maxY, Math.min(maxY, height - cy * sc))
            }
            function _panToSeat(seatIdx) {
                var slot = slotForSeat(seatIdx)
                if (!slot) return
                _panToPoint(width * slot.x + (slot.nudgeX || 0), height * slot.y + slot.nudge)
            }

            // It plans a deferred pan to the seat that is currently active. The
            // pan only happens when the player acts or 1/4 of their
            // thinking time (timeoutSec) has passed – triggered via followTimer
            // or refreshActionTriggered.
            function _scheduleFollow(seatId, sec) {
                if (!zoomActive || !GameTable || zoomPanner.active) return
                // -1 = none. Seat 0 is me (the myTurn path) – except as a spectator,
                // where seat 0 is a perfectly normal ring seat.
                if (seatId < 0 || (seatId === 0 && !spectating)) return
                if (seatId === _followedSeat) return          // already there
                if (seatId === _pendingFollowSeat && followTimer.running) return  // already planned
                _pendingFollowSeat = seatId
                followTimer.interval = Math.max(800, (sec > 0 ? sec : 8) * 250)
                followTimer.restart()
            }

            // It executes the planned pan immediately (the timer expired or an action).
            function _doFollow() {
                followTimer.stop()
                if (!zoomActive || !GameTable || zoomPanner.active) return
                if (_pendingFollowSeat < 0 || (_pendingFollowSeat === 0 && !spectating)) return
                _panToSeat(_pendingFollowSeat)
                _followedSeat = _pendingFollowSeat
                _pendingFollowSeat = -1
            }

            Timer {
                id: followTimer
                repeat: false
                onTriggered: tableZone._doFollow()
            }

            function slotForSeat(seatIdx) {
                // Seat 0 only has a ring slot as a spectator; otherwise the
                // self box sits there, which is not panned to.
                if (!GameTable || seatIdx < 0 || (seatIdx === 0 && !spectating)) return null
                var players = GameTable.players
                var oppOrder = 0
                for (var i = spectating ? 0 : 1; i <= seatIdx && i < players.length; i++)
                    if (seatOnRing(players[i])) oppOrder++
                if (oppOrder < 1) return null
                var seatCount = 0
                for (var j = 0; j < players.length; j++)
                    if (seatOnRing(players[j])) seatCount++
                var seq = slotSeq[spectating ? seatCount : seatCount - 1]
                if (!seq || oppOrder > seq.length) return null
                var name = seq[oppOrder - 1]
                var pos = slotPos[name]
                if (!pos) return null
                var nudge
                var nudgeX = 0
                // Spiegelt seatNudge/seatNudgeX im Repeater-Delegate.
                var flankWide = wide && !Config.Responsive.landscapeCompact && !spectating
                                && (name === "opp1" || name === "opp" + (seatCount - 1))
                                && pos[1] > 0.5
                if (flankWide) {
                    nudge = oppBaseHeight * boxScale * 0.6
                    var dir = pos[0] < 0.5 ? -1 : 1
                    var wantCenter = width / 2 + dir *
                        (selfBaseWidth * boxScale / 2 + 40 * boxScale
                         + oppBaseWidth * oppScale / 2 + 18)
                    var d = wantCenter - width * pos[0]
                    nudgeX = dir < 0 ? Math.min(0, d) : Math.max(0, d)
                } else if (wide) {
                    nudge = 0
                } else if (Config.Responsive.isMobile) {
                    // Mobile: dynamic rows without an offset (see seatNudge).
                    nudge = 0
                } else {
                    // "TC": half the base height downwards (see seatNudge in the
                    // repeater delegate) – otherwise the emoji reaction floated
                    // above the position at which the box would sit WITHOUT a base.
                    nudge = (name === "L_lower" || name === "L_bottom"
                             || name === "R_lower" || name === "R_bottom") ? 14
                          : (name === "L_upper" || name === "TL"
                             || name === "R_upper" || name === "TR") ? -4
                          : (name === "TC") ? betStripH * oppScale / 2
                          : 0
                }
                return { x: pos[0], y: pos[1], nudge: nudge, nudgeX: nudgeX }
            }

            readonly property real topOpponentBottomY: {
                var seq = slotSeq[ringCount] || []
                // 0.13 caps how deep the "upper edge of the upper row" may
                // count – necessary as long as the community is fitted between the upper seats and the
                // self box. As a spectator it is placed
                // centrally into the free ring instead; there the REAL topmost
                // seat is authoritative (with few players it lies well below 0.13).
                var topCenter = spectating ? Infinity : 0.13
                for (var i = 0; i < seq.length; ++i) {
                    var p = slotPos[seq[i]]
                    if (p && p[1] < topCenter) topCenter = p[1]
                }
                if (!isFinite(topCenter)) topCenter = 0.13
                return topCenter * height + oppBaseHeight * oppScale / 2
            }
            // The upper edge of the lowest box. Without the spectator mode that is the
            // self box; as a spectator the lowest ring seat (slot opp0 / BC).
            readonly property real selfVisualTopY: {
                if (spectating) {
                    var p = slotPos[wide ? "opp0" : "BC"]
                    var cy = (p ? p[1] : 0.9) * height
                    return cy - oppBaseHeight * oppScale / 2
                }
                // The upper edge of the visually scaled self box - resolved algebraically from its
                // own bindings instead of read from selfBox.y:
                //   selfBox.y = height - selfBaseHeight - m - selfBaseHeight·(boxScale-1)/2
                //   top       = selfBox.y + selfBaseHeight/2 - selfBaseHeight·boxScale/2
                //             = height - m - selfBaseHeight·boxScale
                // That keeps the layout computation free of a feedback onto a
                // UI element that is itself derived from boxScale (m = bottomMargin
                // without the scale compensation, see selfBox.anchors.bottomMargin).
                return height - (wide ? 12 : 4) - selfBaseHeight * boxScale
            }
            // The Y position of the community cards:
            //   – Regular wide: the vertical CENTRE OF GRAVITY of all boxes (the opponents +
            //     the self). The middle that was used before (topOpp+self)/2 is independent of the
            //     number of players (~the table centre) and let the cards float ABOVE the
            //     low sitting side boxes with few players (e.g. with
            //     4 players: a lonely top player at the top, two side players
            //     further down). The centre of gravity counts these lower boxes in →
            //     the cards move into the crowd and look centred.
            //   – landscapeCompact / portrait: still (topOpp+self)/2
            //     (a separate layout, tuned on its own).
            readonly property real communityCenterY: {
                // The centre of gravity trick below exists to pull the cards into the crowd
                // of the opponents AND the self box. Without a self box (a spectator)
                // the ring lies symmetrically around the zone centre – then the
                // cards simply belong in the middle of the free inner space.
                // Portrait/mobile: the middle of the free middle band (the rows
                // no longer sit symmetrically around the zone centre there).
                if (!wide && Config.Responsive.isMobile)
                    return (portraitBand[0] + portraitBand[1]) / 2
                if (!wide || Config.Responsive.landscapeCompact || spectating)
                    return (topOpponentBottomY + selfVisualTopY) / 2
                var sumY = height - 12 - selfBaseHeight * boxScale / 2   // Self-Box-Mitte
                var n = 1
                var seq = slotSeq[ringCount] || []
                for (var i = 0; i < seq.length; ++i) {
                    var p = slotPos[seq[i]]
                    if (p) { sumY += p[1] * height; n++ }
                }
                return sumY / n
            }

            // ── The zoom layer: the opponents + the community – scalable + pannable ─────────
            // actionBar and gameBackground lie OUTSIDE and stay fixed.
            // selfBox is INSIDE the zoomable layer now.
            Item {
                id: zoomLayer
                anchors.fill: parent
                // Only clip when the zoom is active – without the zoom badge overlays
                // (e.g. the winner badge at player 5 at the top) should be able to stick out beyond
                // the tableZone edge. The tableZone comes after the status bar in the document,
                // so the overflow renders on it.
                clip: tableZone.zoomActive

                Item {
                    id: zoomContent
                    // The full tableZone height (not zoomLayer.height), so that all
                    // slot positions (tableZone.height * slot[1]) and the
                    // communityArea verticalCenter anchor stay correct.
                    width:  tableZone.width
                    height: tableZone.height
                    transformOrigin: Item.TopLeft
                    scale: tableZone.zoomActive ? tableZone.zoomFactor : 1.0
                    // It centres the zoom pivot on the middle of the visible
                    // opponent zone (zoomLayer.height/2); the (1−scale) term
                    // compensates automatically when zooming out.
                    x: (1.0 - scale) * (zoomLayer.width  / 2)
                       + (tableZone.zoomActive ? tableZone._zoomPanX : 0)
                    y: (1.0 - scale) * (zoomLayer.height / 2)
                       + (tableZone.zoomActive ? tableZone._zoomPanY : 0)

                    Behavior on scale {
                        NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                    }

            // ── The community cards + the pot – in the upper table area ───────────────
            // The position: in portrait centrally between the upper/lower side boxes,
            // in wide screen at the centre of the necklace ellipse. The size = only the
            // card row, so that the winning hand badge does not disturb the centring.
            CommunityCards {
                id: communityArea
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                // Portrait/desktop: just above the zone centre (the static slots
                // lie symmetrically around 0.4975). Mobile or landscape: at the
                // computed community height (the middle band or the ellipse gap).
                anchors.verticalCenterOffset:
                    (tableZone.wide || Config.Responsive.isMobile)
                    ? tableZone.communityCenterY - tableZone.height / 2
                    : -tableZone.height * 0.0025 + 5
                z: 0
                wide: tableZone.wide
                // It scales more subtly than the opponent boxes; the scaling is around the centre.
                scale: tableZone.communityScale
                // Rasterize the cards in the real screen size (scale × zoom).
                cardRenderScale: tableZone.communityScale * tableZone.zoomRenderMul
            }

            // The winning hand (e.g. "full house") – only during the showdown.
            // Deliberately standalone (NOT inside the community cards), so that it
            // always lies ABOVE the player boxes independently of their z/scale.
            WinningHandBadge {
                community: communityArea
                wide: tableZone.wide
                communityScale: tableZone.communityScale
            }

            // ── The opponent boxes: distributed over symmetric slots ────────────────────
            // Seat 0 (the human) sits at the bottom in the middle; the other occupied seats
            // are distributed left/right in a balanced way according to slotSeq.
            Repeater {
                model: 10
                delegate: Item {
                    id: seatSlot
                    required property int index
                    // Bring the winner seat to the front, so that its WINNER badge
                    // (and the golden frame) ALWAYS lies above the neighbouring
                    // opponent boxes on an overlap – a higher z in the badge
                    // itself only applies inside its own box, not
                    // between the equally ranked (z:1) sibling slots.
                    z: (typeof GameTable !== "undefined" && GameTable
                        && GameTable.winnerSeatIds.indexOf(index) !== -1) ? 5 : 1

                    readonly property var pdata: (typeof GameTable !== "undefined" && GameTable && GameTable.players.length > index)
                        ? GameTable.players[index] : null
                    // A real player sits here → the box is drawn.
                    readonly property bool occupied: pdata !== null && pdata.name !== ""
                    // The seat claims a ring slot – as a reserved,
                    // invisible placeholder of a player who left as well.
                    readonly property bool onRing: tableZone.seatOnRing(pdata)

                    // The position of this seat on the ring (1 based). Without the
                    // spectator mode seat 0 sits in the self box and does not count
                    // along; as a spectator it is the first ring seat.
                    readonly property int oppOrder: {
                        if (typeof GameTable === "undefined" || !GameTable) return 0
                        var c = 0
                        for (var i = tableZone.spectating ? 0 : 1;
                             i <= index && i < GameTable.players.length; i++)
                            if (tableZone.seatOnRing(GameTable.players[i])) c++
                        return c
                    }

                    readonly property int oppCount: tableZone.ringCount
                    readonly property var seq: tableZone.slotSeq[oppCount] || []
                    readonly property string slotName:
                        (onRing && oppOrder >= 1 && oppOrder <= seq.length) ? seq[oppOrder - 1] : ""
                    // The lowest ring seat (only occupied in the spectator mode).
                    readonly property bool bottomCenter:
                        slotName === "opp0" || slotName === "BC"
                    // Always deliver a valid [x,y] pair. During an
                    // orientation change (or before the first layout, when
                    // width/height are still 0) slotSeq and slotPos can briefly
                    // come from different sets → a fallback to the middle,
                    // so that slot[0]/slot[1] never access undefined.
                    readonly property var slot: {
                        if (slotName === "") return [0.5, 0.5]
                        var p = tableZone.slotPos[slotName]
                        return (p === undefined || p === null) ? [0.5, 0.5] : p
                    }

                    visible: occupied && (tableZone.spectating || index !== 0) && slotName !== ""

                    // The content fills the box without excess margins; the cards in the
                    // original aspect ratio (2×31+3=65)
                    // (4 + avatar 44 + 4 + cards 65 + 4 + 4 = 125)
                    width: tableZone.oppBaseWidth
                    height: tableZone.oppBaseHeight
                    // The boxes scale with the resolution (max = the height of the self box);
                    // around the slot centre, so that the position is kept.
                    transformOrigin: Item.Center
                    scale: tableZone.oppScale
                    // Portrait: spread the side boxes vertically as a group,
                    // to give the table centre more air. The lower ones (players 1/2/8/9 →
                    // L_lower/L_bottom/R_lower/R_bottom) 14px downwards, the upper ones
                    // (L_upper/TL/R_upper/TR) 4px upwards. TC (top centre) stays.
                    // The bottom seats flanking the self box (opp1 / oppN = players 1 & 9) in
                    // desktop wide: lower + horizontally OUTWARDS, so that they do not touch /
                    // constrict the (large) self box. Ultrawide
                    // (landscapeCompact) has its own corner sink layout for that.
                    // Push the seats to the left/right of the self box outwards. As a
                    // spectator there is no self box around which room would be
                    // needed – the ring is already even.
                    readonly property bool flankWide:
                        tableZone.wide && !Config.Responsive.landscapeCompact
                        && !tableZone.spectating
                        && (slotName === "opp1" || slotName === "opp" + oppCount)
                        && slot[1] > 0.5
                    // The horizontal outward offset: the box centre at least
                    // (half the self width + half the opponent width + 18) away from the table centre.
                    // Only push outwards (never pull inwards).
                    readonly property real seatNudgeX: {
                        if (!flankWide) return 0
                        var dir = slot[0] < 0.5 ? -1 : 1
                        var wantCenter = tableZone.width / 2 + dir *
                            (tableZone.selfBaseWidth * tableZone.boxScale / 2
                             + 40 * tableZone.boxScale   // The dealer/blind puck to the right of the self box (6 + 32 + some air)
                             + tableZone.oppBaseWidth * tableZone.oppScale / 2 + 18)
                        var d = wantCenter - tableZone.width * slot[0]
                        return dir < 0 ? Math.min(0, d) : Math.max(0, d)
                    }
                    readonly property real seatNudge: {
                        if (tableZone.wide)
                            return flankWide
                                ? tableZone.oppBaseHeight * tableZone.boxScale * 0.6 : 0
                        // Portrait/mobile: the rows come dynamically from the
                        // box size (buildPortraitSlots) – the spreading and the
                        // base compensation are already in the slot values.
                        if (Config.Responsive.isMobile)
                            return 0
                        // The seat style "inset": the bet base makes the box
                        // higher, and since it is centred around its slot centre,
                        // half the additional height would grow UPWARDS. In landscape
                        // that is harmless (topY in buildLandscapeSlots is derived
                        // from visualH, the upper edge stays at 12 px) –
                        // in portrait, by contrast, the slots are static, and the
                        // topmost centre box sticks close below the status bar with y=0.075.
                        // Put it down by half the (scaled) base height:
                        // the base then grows
                        // exclusively downwards and the upper edge lies exactly
                        // where it lies in the style "classic".
                        //
                        // It is necessary because the feasibility probe of the bisection
                        // (visualH > 2*(0.075*height - 4)) cannot apply any more at the
                        // scale floor of 0.55: without this offset the box stuck into the bar
                        // from a tableZone height < 387 px on
                        // (with a base), instead of only < 314 px as before.
                        if (slotName === "TC")
                            return tableZone.betStripH * tableZone.oppScale / 2
                        if (slotName === "L_lower" || slotName === "L_bottom"
                            || slotName === "R_lower" || slotName === "R_bottom") return 14
                        if (slotName === "L_upper" || slotName === "TL"
                            || slotName === "R_upper" || slotName === "TR") return -4
                        return 0
                    }
                    x: tableZone.width * slot[0] - width / 2 + seatNudgeX
                    y: tableZone.height * slot[1] - height / 2 + seatNudge

                    GamePlayerBox {
                        anchors.fill: parent
                        seatIndex: seatSlot.index
                        // Rasterize the cards in the real screen size (oppScale × zoom).
                        cardRenderScale: tableZone.oppScale * tableZone.zoomRenderMul
                        // Only the topmost box (player 5, the TC slot) shows the
                        // winner badge below it in portrait – everywhere else above.
                        winnerBelow: !tableZone.wide && seatSlot.slotName === "TC"
                        // Let the bet/button point towards the table centre:
                        // the left seats to the right, the right seats to the left, the top/bottom centre downwards.
                        // In the wide (landscape) layout the upper boxes
                        // (players 4–6) sit tightly in the arc → show the bet/icon below the box,
                        // so that the lateral area does not overlap with the
                        // neighbouring boxes.
                        betSide: tableZone.wide
                               ? (seatSlot.slot[0] < 0.45 ? "left"
                                  : seatSlot.slot[0] > 0.55 ? "right"
                                  : "bottom")
                               : seatSlot.slot[0] < 0.45 ? "right"
                               : seatSlot.slot[0] > 0.55 ? "left"
                               : "bottom"
                        // The topmost centre box (player 5): the badge below it would collide with
                        // the pot badge → the button on the LEFT, the bet on the RIGHT
                        // of the box. It applies in the whole landscape (desktop wide
                        // as well as ultrawide), not only in landscapeCompact.
                        //
                        // The lowest ring seat of the spectator (slot opp0/BC) needs
                        // the same split, and that in EVERY orientation: below
                        // it there is no room, and next to each other INSIDE the box width
                        // the bet (centred) and the blind button (on the right) overlap.
                        betSplit: seatSlot.bottomCenter
                                  || (tableZone.wide
                                      && seatSlot.slot[0] >= 0.45 && seatSlot.slot[0] <= 0.55)
                    }
                }
            }

            // ── Your own box: it scales with the zoom layer now ────────────────────
            GamePlayerSelfBox {
                id: selfBox
                z: 1
                // As a spectator I have no seat – seat 0 is drawn as an
                // ordinary ring seat instead (slot opp0/BC).
                visible: !tableZone.spectating
                anchors.bottom: parent.bottom
                anchors.bottomMargin: tableZone.wide
                    ? 12 + tableZone.selfBaseHeight * (tableZone.boxScale - 1) / 2
                    :  4 + tableZone.selfBaseHeight * (tableZone.boxScale - 1) / 2
                anchors.horizontalCenter: parent.horizontalCenter
                width: tableZone.selfBaseWidth
                height: tableZone.selfBaseHeight
                transformOrigin: Item.Center
                scale: tableZone.boxScale
                maxAvatarSize: tableZone.wide ? 60 : 54
                // Rasterize the cards in the real screen size (boxScale × zoom).
                cardRenderScale: tableZone.boxScale * tableZone.zoomRenderMul
            }

            // The emoji reaction animations – in the zoom layer, so that they scale
            // along with the player boxes when the zoom is active.
            GameReactionFx {
                id: reactionFx
                anchors.fill: parent
                z: 60
            }

                } // zoomContent

                // Drag to pan: transfer the finger delta directly onto _zoomPanX/Y.
                // It is only allowed in compact mode and with the zoom active, so that
                // normal table interactions keep working unchanged.
                DragHandler {
                    id: zoomPanner
                    target: null
                    enabled: Qt.platform.os === "android" && tableZone.zoomActive && Config.Responsive.compact

                    property point _startPt
                    property real  _startX
                    property real  _startY

                    onActiveChanged: {
                        if (active) {
                            _startPt = centroid.position
                            _startX  = tableZone._zoomPanX
                            _startY  = tableZone._zoomPanY
                        }
                    }
                    onCentroidChanged: {
                        if (!active) return
                        var dx   = centroid.position.x - _startPt.x
                        var dy   = centroid.position.y - _startPt.y
                        // The pan limits: the content edge may just reach the
                        // screen edge → max = (zoom−1)·halfSize
                        var maxX = (tableZone.zoomFactor - 1) * zoomLayer.width  / 2
                        var maxY = (tableZone.zoomFactor - 1) * zoomLayer.height / 2
                        tableZone._zoomPanX = Math.max(-maxX, Math.min(maxX, _startX + dx))
                        tableZone._zoomPanY = Math.max(-maxY, Math.min(maxY, _startY + dy))
                    }
                }

                // Auto centring: when the player is to act and the zoom is
                // active, it pans to the self box zone automatically, so that
                // the hand cards and the action area are visible right away.
                Connections {
                    target: (typeof GameTable !== "undefined") ? GameTable : null
                    function onMyTurnChanged() {
                        if (!tableZone.zoomActive || !GameTable || !GameTable.myTurn)
                            return
                        // Your own turn: abort a planned opponent pan and pan to the
                        // self box zone immediately.
                        followTimer.stop()
                        tableZone._pendingFollowSeat = -1
                        tableZone._followedSeat = 0
                        tableZone._zoomPanY = -(tableZone.zoomFactor - 1) * zoomLayer.height / 2
                        tableZone._zoomPanX = 0
                    }
                    function onTimeoutChanged() {
                        // A new active seat → plan a deferred following
                        // (it does not jump there immediately). seatId <= 0 (−1 = briefly
                        // between two players, 0 = me) is ignored, so that
                        // a pan to the acting player that is still pending
                        // stays.
                        tableZone._scheduleFollow(GameTable ? GameTable.timeoutSeatId : -1,
                                                  GameTable ? GameTable.timeoutSec : 0)
                    }
                    function onRefreshActionTriggered() {
                        // The planned player has acted → pan there
                        // immediately (it shows the action), without waiting for the
                        // 1/4 interval.
                        tableZone._doFollow()
                    }
                    function onPlayersChanged() {
                        // A safety net: follow the active opponent in a deferred way
                        // as well, in case timeoutChanged fails to appear. _scheduleFollow
                        // is idempotent (no timer thrash under continuous fire).
                        if (!tableZone.zoomActive || !GameTable || zoomPanner.active) return
                        var players = GameTable.players
                        for (var i = 1; i < players.length; i++) {
                            if (players[i].name !== "" && players[i].myTurn) {
                                tableZone._scheduleFollow(i, GameTable.timeoutSec)
                                return
                            }
                        }
                    }
                    function onBoardCardsChanged() {
                        if (!tableZone.zoomActive || !GameTable) return
                        // A new betting round / a new hand: reset the "already panned to"
                        // mark, so that the first player of the round is followed
                        // in a deferred way again.
                        followTimer.stop()
                        tableZone._pendingFollowSeat = -1
                        tableZone._followedSeat = -1
                        if (zoomPanner.active) return
                        if (GameTable.boardCardCount <= 0) return
                        // New card(s): pan to the community area.
                        tableZone._panToPoint(tableZone.width / 2, tableZone.communityCenterY)
                    }
                    function onWinningHandTextChanged() {
                        if (!tableZone.zoomActive || !GameTable || zoomPanner.active) return
                        if (!GameTable.winningHandText) return
                        // The midpoint between the centre of the community cards and the centre of the self box:
                        // both areas are visible at the same time with a ~20px margin.
                        var selfCY = tableZone.selfVisualTopY
                                     + tableZone.selfBaseHeight * tableZone.boxScale / 2
                        var cy = (tableZone.communityCenterY + selfCY) / 2
                        tableZone._panToPoint(tableZone.width / 2, cy)
                    }
                    function onShowdownActiveChanged() {
                        if (!GameTable) return
                        if (GameTable.showdownActive) {
                            // The showdown begins: zoom out automatically, so that the
                            // whole table with all players (and their revealed
                            // cards) is visible again. Remember the previous zoom state
                            // and abort a planned player pan.
                            tableZone._zoomSuspendedByShowdown = tableZone.zoomActive
                            followTimer.stop()
                            tableZone._pendingFollowSeat = -1
                            tableZone._followedSeat = -1
                            tableZone.zoomActive = false
                            tableZone._zoomPanX = 0
                            tableZone._zoomPanY = 0
                        } else if (tableZone._zoomSuspendedByShowdown) {
                            // The next hand: switch a zoom that was active before the showdown
                            // on again.
                            tableZone._zoomSuspendedByShowdown = false
                            tableZone.zoomActive = true
                        }
                    }
                }
            } // zoomLayer

            // ── Spielverlauf (Log) + Chat – Umschalt-Icons + Overlays ──────────
            property bool showChat: false
            // The info panel overlay (history/odds/hand) – only in the NOT docked
            // mode (portrait/mobile). In desktop landscape it is docked.
            property bool showInfo: false
            // The emoji reaction picker (a panel below the toggle next to the chat icon)
            property bool showReactions: false

            // ── The permanent game chat at the bottom left (desktop only, never Android) ───
            // The chat is docked permanently to the left of the action box (the same
            // height, the same vertical position) – provided there is enough free width
            // there. If the room is not enough, it stays the overlay chat (the chat icon).
            readonly property real dockedChatW: {
                if (Config.Responsive.isMobile) return 0
                if (spectating) return 0
                if (typeof GameTable === "undefined" || !GameTable || !GameTable.hasHumanOpponents) return 0
                return Math.min(280, (width - actionBar.panelWidth) / 2 - 24)
            }
            readonly property bool dockedChatFits: dockedChatW >= 170
            // If the chat is docked, the overlay is superfluous.
            onDockedChatFitsChanged: if (dockedChatFits) showChat = false

            // The slot names behind which there really is a VISIBLE player box.
            // Reserved placeholders (keepEmptySeats) do keep their slot in the
            // ring but draw nothing – the chat and the info panel may therefore be pulled up
            // across them to the next visible box. Without
            // this distinction an invisible placeholder would limit the two
            // boxes just like a real opponent.
            readonly property var visibleSlotNames: {
                var set = {}
                if (typeof GameTable === "undefined" || !GameTable) return set
                var seq = slotSeq[ringCount] || []
                var players = GameTable.players
                var order = 0
                for (var i = spectating ? 0 : 1; i < players.length; i++) {
                    if (!seatOnRing(players[i])) continue
                    order++
                    if (order <= seq.length && players[i].name !== "")
                        set[seq[order - 1]] = true
                }
                return set
            }

            // The minimum height of the docked chat (= the action bar height minus the outer margin).
            readonly property real dockedChatMinH: actionBar.height - 8
            // The maximum height: it can be pulled up until the lower edge
            // of the lowest opponent box that overlaps the chat horizontally
            // (+ 8 px of distance) is reached – no overlap guaranteed.
            readonly property real dockedChatMaxH: {
                if (!wide || !dockedChatFits) return dockedChatMinH
                var s = oppScale
                var visualW = oppBaseWidth  * s
                var visualH = oppBaseHeight * s
                // The horizontal area of the chat in tableZone coordinates
                // (the chat is anchored on the left with an 8 px distance, the width = dockedChatW).
                var chatLeft  = 8
                var chatRight = 8 + dockedChatW
                // The bet/puck badge dimensions (base pixels, they scale with s) – see
                // GamePlayerBox.betGroup. The edge seats carry the bet + the dealer/
                // blind button LATERALLY towards the screen edge (= into the chat
                // area), the central seats below the box. So that the
                // bet of an opponent is never covered by the chat, the full
                // box width PLUS the bet width counts towards the horizontal overlap.
                var betSideExt   = (8 + 52) * s
                var betBottomExt = 39 * s
                // Search through all landscape slots: which boxes overlap horizontally?
                var slots = slotPosLandscape
                var maxH = height + actionBar.height - 8   // no limit → full
                for (var name in slots) {
                    // An invisible placeholder of a seat that was left → it does not limit
                    // the chat; it may go up to the next visible box.
                    if (!visibleSlotNames[name]) continue
                    var pos     = slots[name]
                    // Mirror seatNudge/seatNudgeX from the repeater delegate: the
                    // two bottom seats flanking the self box (opp1/oppN)
                    // are moved lower AND further outwards (towards the screen edge = into
                    // the chat area) than their raw slot position. Without
                    // this correction the check underestimates how deep/far out
                    // they reach → maxH too large → an overlap with the chat.
                    var nudgeY  = 0, nudgeX = 0
                    if (!Config.Responsive.landscapeCompact && !spectating
                            && (name === "opp1" || name === "opp" + (seatCount - 1))
                            && pos[1] > 0.5) {
                        nudgeY = oppBaseHeight * boxScale * 0.6
                        var dir = pos[0] < 0.5 ? -1 : 1
                        var wantCX = width / 2 + dir *
                            (selfBaseWidth * boxScale / 2 + 40 * boxScale
                             + oppBaseWidth * oppScale / 2 + 18)
                        var dCX = wantCX - width * pos[0]
                        nudgeX = dir < 0 ? Math.min(0, dCX) : Math.max(0, dCX)
                    }
                    var boxCX   = width  * pos[0] + nudgeX
                    var boxCY   = height * pos[1] + nudgeY
                    var boxL    = boxCX - visualW / 2
                    var boxR    = boxCX + visualW / 2
                    var boxBot  = boxCY + visualH / 2
                    // Count in the bet/puck area depending on betSide (cf. the repeater
                    // delegate): the left seats to the left, the right ones to the right, the central ones
                    // downwards; in landscapeCompact the central box points split
                    // to both sides.
                    if (Config.Responsive.landscapeCompact && pos[0] >= 0.45 && pos[0] <= 0.55) {
                        boxL -= betSideExt
                        boxR += betSideExt
                    } else if (pos[0] < 0.45) {
                        boxL -= betSideExt
                    } else if (pos[0] > 0.55) {
                        boxR += betSideExt
                    } else {
                        boxBot += betBottomExt
                    }
                    // Only check the overlap when the box (incl. the bet) lies in the chat area.
                    if (boxR <= chatLeft || boxL >= chatRight) continue
                    // The lower edge of the box/the bet + an 8 px safety distance:
                    var boxBottom = boxBot + 8
                    // the chat may reach at most to the lower edge of this box.
                    var limit = height - boxBottom + actionBar.height - 8
                    if (limit < maxH) maxH = limit
                }
                return Math.max(dockedChatMinH, maxH)
            }
            // The height set by the user via the drag handle; -1 = the default.
            property real dockedChatUserH: -1
            // Folded in (hidden via the toggle)? The default: folded in (the toggle off).
            property bool dockedChatCollapsed: true

            // ── The permanent info panel at the bottom RIGHT – the mirror image of the docked chat ──
            // The same layout as the chat (at the bottom left): it floats ABOVE the table,
            // can be pulled up and reserves no room. Only on the desktop with enough
            // free width; otherwise an overlay + a toggle.
            readonly property real dockedInfoW: {
                if (Config.Responsive.isMobile) return 0
                if (spectating) return 0
                return Math.min(280, (width - actionBar.panelWidth) / 2 - 24)
            }
            readonly property bool dockedInfoFits: dockedInfoW >= 170
            // If the panel is docked, the overlay is superfluous.
            onDockedInfoFitsChanged: if (dockedInfoFits) showInfo = false
            // Folded in (hidden via the toggle)? The default: folded in (the toggle off).
            property bool dockedInfoCollapsed: true

            readonly property real dockedInfoMinH: actionBar.height - 8
            // The maximum height: up to the lower edge of the lowest opponent box that
            // overlaps the panel (the right edge) horizontally – as with the chat.
            readonly property real dockedInfoMaxH: {
                if (!wide || !dockedInfoFits) return dockedInfoMinH
                var s = oppScale
                var visualW = oppBaseWidth  * s
                var visualH = oppBaseHeight * s
                var infoRight = width - 8
                var infoLeft  = width - 8 - dockedInfoW
                // The bet/puck badge dimensions as with the chat (see dockedChatMaxH): the
                // right edge seats carry the bet laterally to the right (into the
                // info area) – the full box PLUS the bet width count.
                var betSideExt   = (8 + 52) * s
                var betBottomExt = 39 * s
                var slots = slotPosLandscape
                var maxH = height + actionBar.height - 8
                for (var name in slots) {
                    // A placeholder slot (no visible player) → no limitation.
                    if (!visibleSlotNames[name]) continue
                    var pos   = slots[name]
                    // Mirror seatNudge/seatNudgeX from the repeater delegate (cf.
                    // dockedChatMaxH): the bottom seats flanking the self box
                    // (opp1/oppN) sit lower and further out (here: the right
                    // seat into the info area) than their raw slot position.
                    var nudgeY = 0, nudgeX = 0
                    if (!Config.Responsive.landscapeCompact && !spectating
                            && (name === "opp1" || name === "opp" + (seatCount - 1))
                            && pos[1] > 0.5) {
                        nudgeY = oppBaseHeight * boxScale * 0.6
                        var dir = pos[0] < 0.5 ? -1 : 1
                        var wantCX = width / 2 + dir *
                            (selfBaseWidth * boxScale / 2 + 40 * boxScale
                             + oppBaseWidth * oppScale / 2 + 18)
                        var dCX = wantCX - width * pos[0]
                        nudgeX = dir < 0 ? Math.min(0, dCX) : Math.max(0, dCX)
                    }
                    var boxCX = width  * pos[0] + nudgeX
                    var boxCY = height * pos[1] + nudgeY
                    var boxL  = boxCX - visualW / 2
                    var boxR  = boxCX + visualW / 2
                    var boxBot = boxCY + visualH / 2
                    if (Config.Responsive.landscapeCompact && pos[0] >= 0.45 && pos[0] <= 0.55) {
                        boxL -= betSideExt
                        boxR += betSideExt
                    } else if (pos[0] < 0.45) {
                        boxL -= betSideExt
                    } else if (pos[0] > 0.55) {
                        boxR += betSideExt
                    } else {
                        boxBot += betBottomExt
                    }
                    if (boxR <= infoLeft || boxL >= infoRight) continue
                    var boxBottom = boxBot + 8
                    var limit = height - boxBottom + actionBar.height - 8
                    if (limit < maxH) maxH = limit
                }
                return Math.max(dockedInfoMinH, maxH)
            }
            // The height set by the user via the drag handle; -1 = the default.
            property real dockedInfoUserH: -1

            // Unread chat messages: everything above chatReadCount counts as
            // unread. It is marked as read as soon as the chat has been open for 2 s
            // (chatReadTimer); after that further messages count as read
            // immediately while the chat is open.
            property int chatReadCount: 0
            readonly property int chatUnread: {
                var n = (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatLog.length : 0
                return Math.max(0, n - chatReadCount)
            }
            onShowChatChanged: {
                if (showChat) chatReadTimer.restart()
                else chatReadTimer.stop()
            }
            Timer {
                id: chatReadTimer
                interval: 2000
                onTriggered: tableZone.chatReadCount =
                    (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatLog.length : 0
            }
            Connections {
                target: (typeof GameTable !== "undefined") ? GameTable : null
                // With an open chat that has already been read (the 2s timer has expired)
                // new messages count as read immediately.
                function onChatLogChanged() {
                    // The chat was cleared (a new game) → update the counter.
                    if (GameTable.chatLog.length < tableZone.chatReadCount)
                        tableZone.chatReadCount = GameTable.chatLog.length
                    if (tableZone.showChat && !chatReadTimer.running)
                        tableZone.chatReadCount = GameTable.chatLog.length
                }
            }

            // ── The info panel overlay (history/odds/hand) ────────────────────
            // Only in the NOT docked mode (portrait/mobile). In desktop
            // landscape the panel lies docked permanently on the right (infoDock,
            // a direct child of gamePage).
            GameSidePanel {
                id: infoOverlay
                z: 150
                edge: Qt.RightEdge
                wide: tableZone.wide
                // No heading – the tab bar (history/odds) is enough;
                // it is closed via the toggle button at the top right.
                showHeader: false
                visible: tableZone.showInfo && !gamePage.infoDocked
                onCloseRequested: gamePage.toggleInfoOverlay()

                GameInfoPanel {
                    id: infoPanelOverlay
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            // The combined toggle (history/odds/hand). As a child of the tableZone
            // it sits at its right edge automatically: docked to the left of
            // the panel, otherwise at the right screen edge. In desktop landscape
            // it folds the docked panel in/out, otherwise the overlay.
            GameRoundIconButton {
                id: infoToggle
                z: 200
                // A spectator: no history/odds panel (the odds presuppose a
                // hand of your own, which does not exist here).
                visible: !tableZone.spectating
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 8
                iconSource: "../resources/gameLog.svg"
                active: gamePage.infoPanelOpen
                tooltipText: qsTr("Verlauf & Chancen")
                onClicked: gamePage.toggleInfoOverlay()
            }

            // ── The chat overlay (only with human fellow players) ────────────────
            GameSidePanel {
                id: chatOverlay
                z: 150
                edge: Qt.LeftEdge
                wide: tableZone.wide
                // No heading – it is closed via the toggle button at the top.
                showHeader: false
                visible: tableZone.showChat
                onCloseRequested: gamePage.toggleChatOverlay()
                // The chat was closed → close the emoji picker along with it.
                onVisibleChanged: if (!visible) overlayChat.closeEmojiPicker()

                ChatBox {
                    id: overlayChat
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    // The colours of the table theme (independent of the app mode).
                    colText: gamePage.tblChatText
                    colTextSecondary: gamePage.tblChatTextSecondary
                    colTextMuted: gamePage.tblChatTextMuted
                    colBorder: gamePage.tblChatBorder
                    colSurface: gamePage.tblChatSurface
                    colBackground: gamePage.tblChatBackground
                    colAccent: gamePage.tblChatAccent
                    colAccentText: gamePage.tblChatAccentText
                    colSend: gamePage.tblChatSend
                    chatModel: (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatLog : []
                    chatTranslator: (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatTranslator : null
                    nickList: gamePage.gameNickList()
                    onSendRequested: (text) => {
                        if (typeof GameTable !== "undefined" && GameTable)
                            GameTable.sendChat(text)
                    }
                }
            }

            GameRoundIconButton {
                id: chatToggle
                z: 200
                // Always visible (provided there are human fellow players) – even when the
                // chat is docked permanently, so that it can be hidden.
                // Spectators do not take part in the table chat.
                visible: !tableZone.spectating
                         && ((typeof GameTable !== "undefined" && GameTable) ? GameTable.hasHumanOpponents : false)
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 8
                iconSource: "../resources/gameChat.svg"
                active: gamePage.chatPanelOpen
                unread: tableZone.chatUnread
                tooltipText: qsTr("Chat")
                onClicked: gamePage.toggleChatOverlay()
            }

            // ── The emoji reaction picker: the toggle to the right of the chat icon ──────
            GameRoundIconButton {
                id: reactionToggle
                z: 200
                // Spectators have no seat above which a reaction of their own
                // could rise. Reactions of OTHERS stay visible.
                visible: gamePage.emojiReactionsEnabled && !tableZone.spectating
                anchors.top: parent.top
                anchors.left: chatToggle.visible ? chatToggle.right : parent.left
                anchors.leftMargin: chatToggle.visible ? 6 : 8
                anchors.topMargin: 8
                iconSource: "../resources/addReaction.svg"
                active: tableZone.showReactions
                tooltipText: qsTr("Emoji-Reaktionen")
                onClicked: tableZone.showReactions = !tableZone.showReactions
            }

            // The panel with the reaction emojis (three pages of 30, 6 columns –
            // like the reaction picker of the web client).
            ReactionPicker {
                visible: tableZone.showReactions && gamePage.emojiReactionsEnabled
                z: 210
                anchors.top: reactionToggle.bottom
                anchors.topMargin: 6
                anchors.left: parent.left
                anchors.leftMargin: 8
                onPicked: (emoji) => gamePage.sendReaction(emoji)
            }

        }

        // 3. The action bar: the raise controls + fold / call / raise
        // Spectators do not intervene in the game → no action bar. The
        // room that is freed benefits the table (Layout.fillHeight above).
        GameActionBar {
            id: actionBar
            visible: !tableZone.spectating
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            wide: tableZone.wide
            communityVisualWidth: communityArea.width * communityArea.scale
            // While new community cards are being revealed, no action.
            boardDealing: communityArea.dealing
        }
    }

    // ── The permanent game chat: to the left of the action box ───────────────────
    // A direct child of gamePage (above the ColumnLayout), so that it can be pulled up
    // beyond the action bar.
    Rectangle {
        id: dockedChat
        visible: tableZone.dockedChatFits && !tableZone.dockedChatCollapsed
        z: 20
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        width: tableZone.dockedChatW
        height: {
            var h = tableZone.dockedChatUserH >= 0
                    ? tableZone.dockedChatUserH : tableZone.dockedChatMinH
            return Math.max(tableZone.dockedChatMinH,
                            Math.min(tableZone.dockedChatMaxH, h))
        }
        radius: 10
        // Deliberately more transparent than the chat overlay – the table stays visible
        // behind the permanent chat.
        color: Config.Theme.withAlpha(gamePage.tblChatBackground, 0.7)
        border.color: gamePage.tblChatBorder
        border.width: 1

        onVisibleChanged: {
            if (visible && typeof GameTable !== "undefined" && GameTable)
                tableZone.chatReadCount = GameTable.chatLog.length
            if (!visible)
                dockedChatBox.closeEmojiPicker()
        }

        // Permanently visible → new messages count as read immediately.
        Connections {
            target: GameTable
            function onChatLogChanged() {
                if (dockedChat.visible)
                    tableZone.chatReadCount = GameTable.chatLog.length
            }
        }

        // ── The resize handle (drag upwards) ────────────────────────────────
        Item {
            id: chatResizeHandle
            anchors.top: parent.top
            width: parent.width
            height: 10
            z: 10

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: 32
                height: 3
                radius: 2
                color: resizeDragArea.containsMouse || resizeDragArea.pressed
                       ? gamePage.tblChatAccent
                       : Config.Theme.withAlpha(gamePage.tblChatTextMuted, 0.55)
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            MouseArea {
                id: resizeDragArea
                anchors.fill: parent
                cursorShape: Qt.SizeVerCursor
                hoverEnabled: true
                property real pressGlobalY: 0
                property real pressH: 0
                onPressed: (mouse) => {
                    pressGlobalY = mapToItem(gamePage, mouse.x, mouse.y).y
                    pressH = dockedChat.height
                }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    var curY = mapToItem(gamePage, mouse.x, mouse.y).y
                    var delta = pressGlobalY - curY   // upwards = positive
                    var newH = Math.max(tableZone.dockedChatMinH,
                                       Math.min(tableZone.dockedChatMaxH,
                                                pressH + delta))
                    tableZone.dockedChatUserH = newH
                }
            }
        }

        ChatBox {
            id: dockedChatBox
            anchors.fill: parent
            anchors.margins: 6
            anchors.topMargin: 12   // Room for the resize handle
            // The colours of the table theme (independent of the app mode).
            colText: gamePage.tblChatText
            colTextSecondary: gamePage.tblChatTextSecondary
            colTextMuted: gamePage.tblChatTextMuted
            colBorder: gamePage.tblChatBorder
            colSurface: gamePage.tblChatSurface
            colBackground: gamePage.tblChatBackground
            colAccent: gamePage.tblChatAccent
            colAccentText: gamePage.tblChatAccentText
            colSend: gamePage.tblChatSend
            chatModel: (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatLog : []
            chatTranslator: (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatTranslator : null
            nickList: gamePage.gameNickList()
            messageFontSize: 13
            inputHeight: 28
            // Little room → the picker as a popup above the box.
            emojiPickerAsPopup: true
            onSendRequested: (text) => {
                if (typeof GameTable !== "undefined" && GameTable)
                    GameTable.sendChat(text)
            }
        }
    }

    // ── The permanent info panel: at the bottom right (the mirror image of the docked chat) ──────
    // A direct child of gamePage (above the ColumnLayout), so that it can be pulled up beyond
    // the action bar. It floats ABOVE the table.
    Rectangle {
        id: infoDock
        visible: tableZone.dockedInfoFits && !tableZone.dockedInfoCollapsed
        z: 20
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        width: tableZone.dockedInfoW
        height: {
            // Initially = the height of the action button box (like the docked chat),
            // it can be pulled up via the handle. It is always clamped to the available room.
            var h = tableZone.dockedInfoUserH >= 0
                    ? tableZone.dockedInfoUserH : tableZone.dockedInfoMinH
            return Math.max(tableZone.dockedInfoMinH,
                            Math.min(tableZone.dockedInfoMaxH, h))
        }
        radius: 10
        // Deliberately more transparent (like the docked chat) – the table stays visible.
        color: Config.Theme.withAlpha(gamePage.tblChatBackground, 0.7)
        border.color: gamePage.tblChatBorder
        border.width: 1

        GameInfoPanel {
            id: infoPanelDock
            anchors.fill: parent
            anchors.margins: 8
            anchors.topMargin: 12   // Room for the resize handle
            // The same font size as the docked chat.
            messageFontSize: 11
        }

        // ── The resize handle (drag upwards) – as with the chat ─────────
        Item {
            id: infoResizeHandle
            anchors.top: parent.top
            width: parent.width
            height: 10
            z: 10

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: 32
                height: 3
                radius: 2
                color: infoResizeDrag.containsMouse || infoResizeDrag.pressed
                       ? gamePage.tblChatAccent
                       : Config.Theme.withAlpha(gamePage.tblChatTextMuted, 0.55)
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            MouseArea {
                id: infoResizeDrag
                anchors.fill: parent
                cursorShape: Qt.SizeVerCursor
                hoverEnabled: true
                property real pressGlobalY: 0
                property real pressH: 0
                onPressed: (mouse) => {
                    pressGlobalY = mapToItem(gamePage, mouse.x, mouse.y).y
                    pressH = infoDock.height
                }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    var curY = mapToItem(gamePage, mouse.x, mouse.y).y
                    var delta = pressGlobalY - curY   // upwards = positive
                    tableZone.dockedInfoUserH = Math.max(
                        tableZone.dockedInfoMinH,
                        Math.min(tableZone.dockedInfoMaxH, pressH + delta))
                }
            }
        }
    }

    // ── The magnifier button ──────────────────────────────────────────────────────────
    // A direct child of gamePage (not of the tableZone), so that the button appears at the
    // lower screen edge in landscape mode – i.e. next to the
    // action box, not above it. z:200 puts it above all ColumnLayout elements.
    // No layer.enabled/shadow on the Rectangle – it avoids interference
    // between nested MultiEffects, which breaks the icon colourising.
    Rectangle {
        id: zoomToggle
        visible: Qt.platform.os === "android" && Config.Responsive.compact && Config.Parameters.tableZoomEnabled
        z: 200
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.bottom: parent.bottom
        // Portrait: 8 px above the lower edge of the action bar (= 8 px above tableZone.bottom)
        // Landscape: 8 px from the real screen edge (= next to the narrow action box)
        anchors.bottomMargin: tableZone.wide ? 8 : (8 + actionBar.height)
        width: 36; height: 36; radius: 18
        color: tableZone.zoomActive ? Config.Theme.colorAccent : Qt.rgba(0, 0, 0, 0.50)

        onVisibleChanged: {
            if (!visible) {
                tableZone.zoomActive = false
                tableZone._zoomPanX = 0
                tableZone._zoomPanY = 0
            }
        }

        SvgIcon {
            anchors.centerIn: parent
            width: 22; height: 22
            source: tableZone.zoomActive ? "../resources/zoomOut.svg" : "../resources/zoomIn.svg"
            layer.enabled: true
            layer.effect: MultiEffect {
                colorization: 1.0
                colorizationColor: tableZone.zoomActive ? "#101010" : "#FFFFFF"
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                tableZone.zoomActive = !tableZone.zoomActive
                if (!tableZone.zoomActive) {
                    tableZone._zoomPanX = 0
                    tableZone._zoomPanY = 0
                }
            }
        }
    }

    // ── The end of the game in a local game ───────────────────────────────────────────
    // The GameHandler reports the end of the tournament (only one player with chips left)
    // and starts no further hand. Here the winner gets their
    // message – with the choice of starting anew with the same settings or
    // returning to the menu (the counterpart to the "start" button of the widget client).
    Connections {
        target: GameTable
        function onLocalGameFinished(winnerName, winnerSeatId) {
            gameOverPopup.winnerName = winnerName
            gameOverPopup.humanWon = (winnerSeatId === 0)
            gameOverPopup.open()
        }
    }

    Popup {
        id: gameOverPopup
        // It appears at the end of the game by itself. The initial focus is on "new game" – the
        // action that stays on the page; "back to the menu" is one Tab
        // away, Escape closes.
        focus: true
        onOpened: newGameButton.forceActiveFocus()
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 20
        width: Math.min((parent ? parent.width : 380) * 0.85, 380)
        closePolicy: Popup.CloseOnEscape

        property string winnerName: ""
        property bool humanWon: false

        background: Rectangle {
            color: Config.Theme.colorBox
            border.color: Config.StaticData.palette.secondary.col400
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            spacing: 12
            width: gameOverPopup.availableWidth

            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Game Over")
                color: Config.StaticData.palette.secondary.col100
                font.pixelSize: 15
                font.bold: true
            }
            AppLabel {
                Layout.fillWidth: true
                text: gameOverPopup.humanWon
                      ? qsTr("Congratulations, you won the game!")
                      : qsTr("%1 wins the game!").arg(gameOverPopup.winnerName)
                color: Config.StaticData.palette.secondary.col200
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                CustomButton {
                    Layout.fillWidth: true
                    text: qsTr("Back to Menu")
                    onClicked: {
                        gameOverPopup.close()
                        mainWindow.performLeaveGame()
                    }
                }
                CustomButton {
                    id: newGameButton
                    Layout.fillWidth: true
                    text: qsTr("New Game")
                    onClicked: {
                        // A new game with the same settings – the page
                        // stays, GameHandler::setGame() resets the table,
                        // the history and the chat.
                        gameOverPopup.close()
                        GameTable.startLocalGame()
                    }
                }
            }
        }
    }
}
