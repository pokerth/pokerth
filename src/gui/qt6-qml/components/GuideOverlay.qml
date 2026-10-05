pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config
import "guidecore.js" as Core

// ═══════════════════════════════════════════════════════════════════
// Ace's Help — the contextual assistant of the web client (narmod), ported
// to the QML client without the animated mascot: a small A♠ button in the
// header (pokerth.qml) opens the menu, a bubble explains what matters where
// the player is — on the real screens, never with fake data.
//
//  · Offered ONCE on the start page; off until the player says yes. On/off in
//    the side menu and in the interface settings.
//  · Each screen / window is explained once; « Later » puts a tip off for this
//    session (red dot on the A♠ button, a tap brings it back).
//  · Silent while a game runs at the table.
//  · The rules (which tip, which Ranking table, which place) live in
//    guidecore.js, the same as in the web client (docs/GUIDE.md there).
//  · A page lends its controls with guideTarget(key) (and guideLocked(key)
//    for a greyed-out form field); the bubble outlines them in gold.
//
// The overlay covers the whole window but only the bubble takes input.
// ═══════════════════════════════════════════════════════════════════
Item {
    id: guide

    // Set by pokerth.qml.
    property StackView stack: null
    // The lobby page lies in the stack (the connection to a server is up).
    property bool inLobby: false

    readonly property bool helpOn: Config.Parameters.guideOn
    // A tip put off with « Later » still applies here → red dot on the A♠ button.
    property bool badge: false

    // ── Session state ───────────────────────────────────────────────
    // showing = { kind: "ctx" | "note" | "offer", ctx, steps, index }
    property var showing: null
    property var snoozed: ({})
    property var resumeAt: ({})
    // { gid, place, tied } of my last Ranking game, until it has been told.
    property var result: null
    // While I play a game on pokerth.net: { gid, ranked, prev, last, done }.
    property var tracker: null
    property double lobbySince: 0
    property var lastWait: null
    property bool offerDone: false

    // ── What the bubble shows ───────────────────────────────────────
    property string bubbleText: ""
    property string bubbleCount: ""
    property var bubbleButtons: []
    property var buttonHandler: null
    property var target: null
    property bool tourMode: false
    property bool bubbleAtTop: true
    // The area of the pages (below the header), in overlay coordinates.
    property rect area: Qt.rect(0, 0, width, height)
    property rect targetRect: Qt.rect(0, 0, 0, 0)
    property bool targetShown: false

    // ── Texts ───────────────────────────────────────────────────────
    // The keys and the English texts follow the web client's catalogue
    // (modules/guide/lang/en.mjs); « **…** » marks bold.
    function t(key) {
        switch (key) {
        case "name": return qsTr("Ace’s Help")
        case "aceLabel": return qsTr("Ace’s Help — tap for my menu")
        case "offer": return qsTr("New here? I can show you around as you go.")
        case "offerYes": return qsTr("Yes, please")
        case "offerNo": return qsTr("No thanks")
        case "gotIt": return qsTr("Got it")
        case "later": return qsTr("Later")
        case "next": return qsTr("Next")
        case "prev": return qsTr("Back")
        case "close": return qsTr("Close")
        case "turnOff": return qsTr("Turn off")
        case "resetTips": return qsTr("Show all tips again")
        case "resetDone": return qsTr("Done — every tip will show again.")
        case "turnedOff": return qsTr("Ace’s Help is off. You can turn it back on from the menu at any time.")
        case "join": return qsTr("Join")
        case "createRanking": return qsTr("Create a Ranking table")
        case "signup": return qsTr("Create an account")
        case "seeRanking": return qsTr("See the ranking")
        case "replayTip": return qsTr("This screen’s tip")
        case "c1Join": return qsTr("A ranked game is waiting for you: **{n}/{max}** players. It starts as soon as it’s full!")
        case "c1None": return qsTr("No ranked game open right now. Create one — any player with an account can! It starts by itself as soon as 10 players have joined.")
        case "c1Guest": return qsTr("Ranked games need a (free) pokerth.net account. As a guest you can play Normal games.")
        case "c2Wait": return qsTr("Ranked game: **{n}/{max}** players. It starts by itself as soon as the table is full — meanwhile, here is how the ranking works.")
        case "c2Points": return qsTr("Each ranked game hands out points by finishing place: **15, 9, 6, 4, 3, 2, 1** from 1st to 7th, nothing from 8th to 10th — 40 points per table.")
        case "c2Score": return qsTr("Your **Score** is not the sum of your points but your average per game, tempered by how many games you have played: playing regularly matters.")
        case "c2Seasons": return qsTr("The ranking runs in **quarterly seasons**: at each new season the counters are archived and start again from zero.")
        case "c2Why55": return qsTr("Why is **5/5** everyone’s favourite? 5 seconds to act, 5 seconds between hands, 10,000 chips and blinds doubling every 11 hands: fast and the same for everyone, so games stay short and comparable.")
        case "oneMore": return qsTr("Just one more player!")
        case "c2Result": return qsTr("Game over — you finished in place **{place}**: **+{points}** points. See your ranking?")
        case "c2ResultTie": return qsTr("Game over! Several players went out in the same hand, so your exact place is on the ranking page. See your ranking?")
        case "cfIntro": return qsTr("Let’s go through this form together, one field at a time — I’ll bring each one into view. **Next** moves on, **Back** goes back, **Later** stops the tour.")
        case "cfName": return qsTr("**Game name**: what the other players see in the list of tables. Say what to expect — « Fast game », « Beginners welcome »…")
        case "cfNameGuest": return qsTr("**Game name**: as a guest, the name is chosen for you.")
        case "c5CreateGuest": return qsTr("As a guest you can create **Normal** games. Ranking tables and registered-only games need a (free) pokerth.net account.")
        case "cfPassword": return qsTr("**Password**: switch it on to keep the table private — only players who know the password can sit down. Not available for a **Ranking game** or an **Invited players only** game.")
        case "cfSpectators": return qsTr("**Spectators**: lets other players watch the game without playing.")
        case "cfPlayers": return qsTr("**Max players**: from 2 to 10 seats. The host starts the game from the waiting room, even with empty seats — a **Ranking game** starts by itself once all 10 seats are taken.")
        case "cfStack": return qsTr("**Starting stack**: the chips each player gets. What counts is the stack compared with the blinds: 3000 chips with a small blind of 10 is 150 big blinds, a comfortable game. Fewer big blinds = a faster game, with more all-ins.")
        case "cfBlind": return qsTr("**First small blind**: the small blind when the game starts. The big blind is always twice the small blind.")
        case "cfInterval": return qsTr("**Blind increase interval**: the blinds go up every so many **hands** or **minutes**. The shorter the interval, the shorter the game.")
        case "cfTimeout": return qsTr("**Time per action**: how many seconds each player has to act on their turn (5 to 60).")
        case "cfDelay": return qsTr("**Pause between hands**: the seconds to see how a hand ended before the next one is dealt (5 to 20).")
        case "cfLocked": return qsTr("Greyed out here: the chosen game type (or community template) sets this value.")
        case "welcome": return qsTr("Great! I’ll pop up whenever there is something useful to explain — each screen and window once. The **A♠** button at the top opens my menu: this screen’s tip again, all tips again, or turn me off.")
        case "menuOn": return qsTr("Ace’s Help is on: I explain each screen and window the first time you open it. **Later** puts a tip off — the red dot on the **A♠** button then brings it back.")
        case "startModes": return qsTr("Four ways to play: **Internet Game** on pokerth.net, with the official rankings; **Start Local Game** against computer players, even offline; **Create Network Game** opens a server for your local network, and **Join Network Game** connects to one.")
        case "loginAccount": return qsTr("On pokerth.net, play with your free account — **Login as User** — or **Continue as Guest**. Guests can only play Normal games: no ranked games and no chat. **Register** creates a free account in a minute.")
        case "c2WhereQml": return qsTr("To see where you stand: the **trophy** button at the top — and at the table, tap the **table name** to see the season ranking of the players you sit with.")
        case "c4HostQml": return qsTr("This table is yours: press **Start Game** when everyone is here — or tick **Fill up with computer players** to fill the empty seats.")
        case "c4GuestQml": return qsTr("The host of the table starts the game — you only have to wait until everyone is here.")
        case "cfTypeQml": return qsTr("Four game types: **Normal** (open to all), **Registered players only**, **Invited players only** and **Ranking game**. Any player with an account can create a Ranking table: 10 players, no password, it starts by itself when full.")
        case "cfPresetQml": return qsTr("**Community template** (invited players only): the exact settings of a BBC, Monthly Cup or WEC game in one tap, for the admins who open these games. The fields it sets are then locked.")
        case "cfActionsNet": return qsTr("Last step: **Create Game** opens your table and its waiting room; **Cancel** goes back without creating anything.")
        case "lanIntro": return qsTr("A game on your local network: this device becomes the server, and the others connect with **Join Network Game**. Let’s go through the settings one at a time — **Next** moves on, **Back** goes back, **Later** stops the tour.")
        case "lanDouble": return qsTr("**Always double blinds**: the small blind doubles at each raise. Switched off, the blinds follow your manual blinds order from the network game settings.")
        case "cfActionsLan": return qsTr("Last step: **Create Game** starts the server on this device and opens its lobby; **Cancel** goes back.")
        case "localIntro": return qsTr("Training table: choose the number of players, the starting stack, the blinds and the game speed, then start the game. Nothing here counts towards a ranking.")
        case "localPlayers": return qsTr("**Number of players**: you and up to 9 computer players.")
        case "localBlinds": return qsTr("**Blinds**: keep the saved settings, or change them here — the first small blind, how often it goes up (every so many hands or minutes) and how: always doubled, or a manual blinds order.")
        case "localSpeed": return qsTr("**Game speed**: how fast the computer players act and the cards are dealt — from 1 (slow) to 11 (fast).")
        case "localActions": return qsTr("Last step: **Start game** opens the table straight away; **Cancel** goes back.")
        case "wRankingHub": return qsTr("The official **PokerTH** ranking and the community ones (**BBC**, **WEC**). Pick one: search a player, choose a season, and tap a name to open the profile.")
        case "wRanking": return qsTr("The official ranking: the current season, or an earlier one under **Season**. Search a player by name and tap a row for the profile. At the bottom: how the ranking is calculated.")
        case "wForum": return qsTr("The latest posts of the pokerth.net forum, newest first. Tap a post to read it here — it is then marked as read. A filled dot means not read yet; the badge on the newspaper button counts them, and **Mark all as read** clears them all. **BBC games** lists the upcoming BBC games.")
        case "wSettings": return qsTr("Every setting, by section: interface, style, sound, local, network and internet games, nicknames and avatars, log messages, and back to the defaults. Ace’s Help is switched on and off in the interface settings.")
        case "wLogs": return qsTr("Your logs: every game played on this device is recorded here. Pick a game for a preview, export it as HTML or text, or analyse it for a review of your play.")
        case "wProfile": return qsTr("A player’s card: the profile and the statistics — the current season, the last games and the results so far.")
        }
        return key
    }

    // « **bold** » → StyledText; everything else stays plain text.
    function styled(s) {
        return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
                        .replace(/\*\*(.+?)\*\*/g, "<b>$1</b>").replace(/\n/g, "<br>")
    }

    // ── The contexts (plain data, see guidecore.js) ─────────────────
    function windowTip(id, key, text) {
        return { id: id, window: key, priority: 50, steps: [{ text: text }] }
    }
    readonly property var contexts: [
        windowTip("w-ranking-hub", "rankingHub", "wRankingHub"),
        windowTip("w-ranking", "ranking", "wRanking"),
        windowTip("w-forum", "forum", "wForum"),
        windowTip("w-settings", "settings", "wSettings"),
        windowTip("w-logs", "logs", "wLogs"),
        windowTip("w-profile", "profile", "wProfile"),
        // Back in the lobby after a Ranking game: the place and its points.
        { id: "ranked-result", priority: 40, repeat: true, screens: ["lobby"],
          needs: { net: true }, when: function(w) { return !!w.result },
          steps: [{ text: function(w) { return w.result.place ? "c2Result" : "c2ResultTie" },
                    vars: function(w) {
                        return w.result.place
                            ? { place: w.result.place, points: Core.pointsFor(w.result.place) } : null
                    },
                    buttons: ["later", "gotIt", "seeRanking"] }] },
        // The forms: a tour, one field per step, in the order of the page.
        { id: "create-game", priority: 20, screens: ["create"], tour: true,
          when: function(w) { return w.form === "net" },
          steps: [
              { text: "cfIntro" },
              { text: function(w) { return w.guest ? "cfNameGuest" : "cfName" }, target: "name", optional: true },
              { text: function(w) { return w.guest ? "c5CreateGuest" : "cfTypeQml" }, target: "type", optional: true },
              { text: "cfPresetQml", target: "preset", optional: true },
              { text: "cfPassword", target: "password", optional: true },
              { text: "cfSpectators", target: "spectators", optional: true },
              { text: "cfPlayers", target: "players", optional: true },
              { text: "cfStack", target: "stack", optional: true },
              { text: "cfBlind", target: "blind", optional: true },
              { text: "cfInterval", target: "interval", optional: true },
              { text: "cfTimeout", target: "timeout", optional: true },
              { text: "cfDelay", target: "delay", optional: true },
              { text: "cfActionsNet", target: "actions", optional: true }
          ] },
        { id: "create-lan", priority: 20, screens: ["create"], tour: true,
          when: function(w) { return w.form === "lan" },
          steps: [
              { text: "lanIntro" },
              { text: "cfPlayers", target: "players", optional: true },
              { text: "cfStack", target: "stack", optional: true },
              { text: "cfBlind", target: "blind", optional: true },
              { text: "cfInterval", target: "interval", optional: true },
              { text: "lanDouble", target: "double", optional: true },
              { text: "cfTimeout", target: "timeout", optional: true },
              { text: "cfDelay", target: "delay", optional: true },
              { text: "cfActionsLan", target: "actions", optional: true }
          ] },
        { id: "create-local", priority: 20, screens: ["create"], tour: true,
          when: function(w) { return w.form === "local" },
          steps: [
              { text: "localIntro" },
              { text: "localPlayers", target: "players", optional: true },
              { text: "cfStack", target: "stack", optional: true },
              { text: "localBlinds", target: "blinds", optional: true },
              { text: "localSpeed", target: "speed", optional: true },
              { text: "localActions", target: "actions", optional: true }
          ] },
        // pokerth.net lobby: point everyone at the same Ranking table.
        { id: "lobby-ranking", priority: 30, screens: ["lobby"], live: true, fold: 25000,
          needs: { net: true, guest: false }, when: function(w) { return !!w.rankPick },
          steps: [{ text: "c1Join",
                    vars: function(w) { return { n: w.rankPick.playerCount, max: w.rankPick.maxPlayers || 10 } },
                    target: function(w) { return "game:" + w.rankPick.gameId },
                    buttons: ["later", "gotIt", "join"] }] },
        { id: "lobby-ranking-create", priority: 29, screens: ["lobby"], live: true, fold: 25000,
          needs: { net: true, guest: false, gamesLoaded: true },
          when: function(w) { return !w.rankPick },
          steps: [{ text: "c1None", target: "createGame",
                    buttons: ["later", "gotIt", "createRanking"] }] },
        { id: "lobby-guest", priority: 30, screens: ["lobby"], needs: { net: true, guest: true },
          steps: [{ text: "c1Guest", buttons: ["later", "gotIt", "signup"] }] },
        // A Ranking table waiting for players: one fact every ~20 s.
        { id: "wait-ranking", priority: 30, screens: ["wait"], live: true,
          needs: { net: true, ranked: true },
          steps: [
              { text: "c2Wait", auto: 20000, target: "players",
                vars: function(w) { return { n: w.waitCount, max: w.waitMax } } },
              { text: "c2Points", auto: 20000 },
              { text: "c2Score", auto: 20000 },
              { text: "c2Seasons", auto: 20000 },
              { text: "c2Why55", auto: 20000 },
              { text: "c2WhereQml" }
          ] },
        { id: "wait-normal", priority: 20, screens: ["wait"], needs: { ranked: false, spectator: false },
          steps: [{ text: function(w) { return w.host ? "c4HostQml" : "c4GuestQml" },
                    target: function(w) { return w.host ? "start" : "" } }] },
        { id: "start", priority: 20, screens: ["connect"],
          when: function(w) { return w.loginStep === "start" },
          steps: [{ text: "startModes", target: "buttons" }] },
        { id: "login", priority: 20, screens: ["connect"],
          when: function(w) { return w.loginStep === "login" },
          steps: [{ text: "loginAccount", target: "choices" }] }
    ]
    // Buttons that do something (primary), besides later / next / gotIt.
    readonly property var actionIds: ["join", "createRanking", "signup", "seeRanking"]

    // ── Saved progress (Config.Parameters) ──────────────────────────
    function seenMap() {
        try {
            var m = JSON.parse(Config.Parameters.guideSeen || "{}")
            return (m && typeof m === "object") ? m : {}
        } catch (e) {
            return {}
        }
    }
    function isSeen(id) { return seenMap()[id] !== undefined }
    function markSeen(id) {
        var m = seenMap()
        m[id] = Date.now()
        Config.Parameters.guideSeen = JSON.stringify(m)
    }
    function isSnoozed(id) { return snoozed[id] === true }

    // ── Where is the player? ────────────────────────────────────────
    // Pages that lie over a screen (the header icons, sub-pages): they never
    // count as the screen itself.
    readonly property var overlayPages: [
        "settingsPage", "communityRankingPage", "rankingPage", "bbcRankingPage",
        "wecRankingPage", "pokerthPlayerPage", "communityPlayerPage", "forumNewsPage",
        "forumPostPage", "logsPage", "aboutPage", "gameTableStatsPage"
    ]
    function windowKey(name) {
        switch (name) {
        case "communityRankingPage": return "rankingHub"
        case "rankingPage": return "ranking"
        case "forumNewsPage": return "forum"
        case "settingsPage": return "settings"
        case "logsPage": return "logs"
        case "pokerthPlayerPage":
        case "communityPlayerPage": return "profile"
        }
        return ""
    }
    function basePage() {
        if (!stack)
            return null
        for (var i = stack.depth - 1; i >= 0; --i) {
            var it = stack.get(i, StackView.DontLoad)
            if (it && overlayPages.indexOf(it.objectName) < 0)
                return it
        }
        return null
    }
    function lobby() { return (typeof Lobby !== "undefined" && Lobby) ? Lobby : null }

    function gameList() {
        var l = lobby()
        var out = []
        if (!l || !l.gameListModel)
            return out
        var n = l.gameListModel.rowCount()
        for (var i = 0; i < n; ++i)
            out.push(l.gameListModel.get(i))
        return out
    }

    // The snapshot read by guidecore.js.
    function where() {
        var l = lobby()
        var top = stack ? stack.currentItem : null
        var base = basePage()
        var bn = base ? base.objectName : ""
        var screen = "other", form = "", loginStep = ""
        switch (bn) {
        case "startPage": screen = "connect"; loginStep = "start"; break
        case "serverConnectionPage":
            screen = "connect"
            loginStep = base.guideStep === 0 ? "login" : ""
            break
        case "lobbyPage": screen = "lobby"; break
        case "gameWaitPage": screen = "wait"; break
        case "lobbyCreateGamePage": screen = "create"; form = "net"; break
        case "networkGameCreatePage": screen = "create"; form = "lan"; break
        case "localGamePage": screen = "create"; form = "local"; break
        case "gamePage": screen = "game"; break
        }
        var net = !!(inLobby && l && l.isInternetSession())
        var guest = !!(net && l.isMyPlayerGuest)
        var w = {
            helpOn: helpOn,
            screen: screen,
            playing: screen === "game",
            net: net,
            guest: (screen === "create" && form === "net") ? !!(l && l.isMyPlayerGuest) : guest,
            ranked: screen === "wait" ? !!base.isRanking : false,
            spectator: !!(l && l.isSpectating),
            host: screen === "wait" ? !!base.isAdmin : false,
            form: form,
            loginStep: loginStep,
            gamesLoaded: screen === "lobby" && lobbySince > 0 && Date.now() - lobbySince > 3000,
            rankPick: null,
            waitCount: 0,
            waitMax: 10,
            gid: l ? l.currentGameId : 0,
            result: result,
            windows: [],
            // A window (settings, ranking…) lies over the screen: only its own tip speaks.
            covered: !!(top && base && top !== base)
        }
        if (screen === "lobby" && net && !guest && l && !l.isInGame)
            w.rankPick = Core.pickRankingTable(gameList(), false)
        if (screen === "wait") {
            w.waitCount = base.players ? base.players.length : 0
            w.waitMax = (base.info && base.info.maxPlayers) ? base.info.maxPlayers : 10
        }
        if (top && top !== base && windowKey(top.objectName) !== "")
            w.windows = [windowKey(top.objectName)]
        return w
    }

    // Does this context fit here? Over a window only the window's own tip.
    function fits(ctx, w) {
        return (!w.covered || !!ctx.window) && Core.applies(ctx, w)
    }
    function candidates(w) {
        if (!w.covered)
            return contexts
        var out = []
        for (var i = 0; i < contexts.length; ++i)
            if (contexts[i].window)
                out.push(contexts[i])
        return out
    }

    // ── Targets ─────────────────────────────────────────────────────
    function contextPage(ctx) {
        return ctx.window ? (stack ? stack.currentItem : null) : basePage()
    }
    function itemsOf(t) {
        if (!t)
            return []
        return Array.isArray(t) ? t : [t]
    }
    function shownItems(t) {
        var out = []
        var list = itemsOf(t)
        for (var i = 0; i < list.length; ++i) {
            var it = list[i]
            try {
                if (it && it.visible && it.width > 0 && it.height > 0)
                    out.push(it)
            } catch (e) {}
        }
        return out
    }
    function stepTargetKey(step, w) {
        try { return Core.val(step.target, w) || "" } catch (e) { return "" }
    }
    function findTarget(ctx, step, w) {
        var key = stepTargetKey(step, w)
        if (key === "")
            return null
        var page = contextPage(ctx)
        if (!page || typeof page.guideTarget !== "function")
            return null
        var items = shownItems(page.guideTarget(key))
        return items.length ? items : null
    }
    // The nearest Flickable (ScrollView, ListView…) above an item.
    function flickableOf(it) {
        for (var p = it ? it.parent : null; p; p = p.parent) {
            if (p.contentY !== undefined && p.flickableDirection !== undefined)
                return p
        }
        return null
    }
    // The outlined area in overlay coordinates, clipped by the scrolling lists.
    function updateTargetRect() {
        var items = shownItems(target)
        var x1 = Infinity, y1 = Infinity, x2 = -Infinity, y2 = -Infinity
        for (var i = 0; i < items.length; ++i) {
            var it = items[i]
            var p = it.mapToItem(guide, 0, 0)
            var rx1 = p.x, ry1 = p.y, rx2 = p.x + it.width, ry2 = p.y + it.height
            for (var f = flickableOf(it); f; f = flickableOf(f)) {
                var fp = f.mapToItem(guide, 0, 0)
                rx1 = Math.max(rx1, fp.x); ry1 = Math.max(ry1, fp.y)
                rx2 = Math.min(rx2, fp.x + f.width); ry2 = Math.min(ry2, fp.y + f.height)
            }
            if (rx2 - rx1 < 2 || ry2 - ry1 < 2)
                continue
            x1 = Math.min(x1, rx1); y1 = Math.min(y1, ry1)
            x2 = Math.max(x2, rx2); y2 = Math.max(y2, ry2)
        }
        targetShown = x2 > x1 && y2 > y1
        if (targetShown)
            targetRect = Qt.rect(x1, y1, x2 - x1, y2 - y1)
    }
    // Tour: the field goes to the top of its scrolling list, so that the bubble
    // at the bottom does not hide it.
    function reveal() {
        var items = shownItems(target)
        if (!items.length)
            return
        var it = items[0]
        var f = flickableOf(it)
        if (!f || !f.contentItem)
            return
        var a = stackArea()
        var p = it.mapToItem(guide, 0, 0)
        var bubbleTop = a.y + a.height - bubble.height - 12
        if (p.y >= a.y + 8 && p.y + it.height <= bubbleTop - 8)
            return
        var c = it.mapToItem(f.contentItem, 0, 0)
        var maxY = Math.max(0, f.contentHeight - f.height)
        f.contentY = Math.max(0, Math.min(maxY, c.y - 12))
    }

    // ── Bubble placement ────────────────────────────────────────────
    function stackArea() {
        if (!stack)
            return Qt.rect(0, 0, width, height)
        var p = stack.mapToItem(guide, 0, 0)
        return Qt.rect(p.x, p.y, stack.width, stack.height)
    }
    function overlapArea(a, b) {
        var w = Math.min(a.x + a.width, b.x + b.width) - Math.max(a.x, b.x)
        var h = Math.min(a.y + a.height, b.y + b.height) - Math.max(a.y, b.y)
        return (w > 0 && h > 0) ? w * h : 0
    }
    function place() {
        updateTargetRect()
        var a = stackArea()
        area = a
        var bw = bubble.width, bh = bubble.height
        var top = Qt.rect(a.x + a.width - bw - 12, a.y + 8, bw, bh)
        var bottom = Qt.rect(top.x, a.y + a.height - bh - 12, bw, bh)
        var first = tourMode ? false : true
        if (targetShown) {
            var oFirst = overlapArea(targetRect, first ? top : bottom)
            var oOther = overlapArea(targetRect, first ? bottom : top)
            if (oFirst > 0 && oOther < oFirst)
                first = !first
        }
        bubbleAtTop = first
    }

    // ── Showing ─────────────────────────────────────────────────────
    function btn(id, primary) { return { id: id, label: t(id), primary: !!primary } }

    function present(text, count, buttons, tgt, tour, handler) {
        area = stackArea()
        bubbleText = styled(text)
        bubbleCount = count
        // The primary button sits rightmost.
        bubbleButtons = buttons
        buttonHandler = handler
        target = tgt
        tourMode = tour
        Qt.callLater(function() {
            if (guide.tourMode)
                guide.reveal()
            guide.place()
        })
    }

    function clearTimers() {
        stepTimer.stop()
        foldTimer.stop()
        noteTimer.stop()
    }

    function closeBubble() {
        clearTimers()
        showing = null
        bubbleText = ""
        bubbleButtons = []
        buttonHandler = null
        target = null
        targetShown = false
    }

    function showContext(ctx) {
        var w = where()
        var keep = ctx.tour
            ? function(s) { return !s.target || !s.optional || !!guide.findTarget(ctx, s, w) }
            : null
        var steps = Core.stepsFor(ctx, w, keep)
        if (!steps.length)
            return
        var idx = 0
        if (ctx.tour && resumeAt[ctx.id] !== undefined) {
            idx = Math.min(resumeAt[ctx.id], steps.length - 1)
            delete resumeAt[ctx.id]
        }
        showing = { kind: "ctx", ctx: ctx, steps: steps, index: idx }
        renderStep(false)
    }

    // Shows the current step (again, after a live update: keep = true leaves
    // the timers and the scroll position alone).
    function renderStep(keep) {
        var s = showing
        if (!s || s.kind !== "ctx")
            return
        var step = s.steps[s.index]
        var w = where()
        var last = s.index >= s.steps.length - 1
        var list = []
        if (step.buttons) {
            var hasAction = false
            for (var i = 0; i < step.buttons.length; ++i)
                if (actionIds.indexOf(step.buttons[i]) >= 0)
                    hasAction = true
            for (var j = 0; j < step.buttons.length; ++j) {
                var id = step.buttons[j]
                list.push(btn(id, actionIds.indexOf(id) >= 0
                                  || (!hasAction && (id === "gotIt" || id === "next"))))
            }
        } else {
            list.push(btn("later"))
            if (s.ctx.tour && s.index > 0)
                list.push(btn("prev"))
            list.push(last ? btn("gotIt", true) : btn("next", true))
        }
        var text, vars
        try {
            text = Core.val(step.text, w)
            vars = Core.val(step.vars, w)
        } catch (e) {
            closeBubble()
            return
        }
        var say = Core.fill(t(text), vars)
        var tgt = findTarget(s.ctx, step, w)
        var page = contextPage(s.ctx)
        if (s.ctx.tour && tgt && page && typeof page.guideLocked === "function"
                && page.guideLocked(stepTargetKey(step, w)))
            say += "\n" + t("cfLocked")
        var count = (s.ctx.tour && s.steps.length > 1) ? (s.index + 1) + "/" + s.steps.length : ""
        if (keep) {
            // A live update: only what changed (no new scroll, no new placement).
            var html = styled(say)
            if (html !== bubbleText)
                bubbleText = html
            target = tgt
            return
        }
        present(say, count, list, tgt, !!s.ctx.tour, onCtxButton)
        stepTimer.stop()
        if (step.auto && !last) {
            stepTimer.interval = step.auto
            stepTimer.start()
        }
        foldTimer.stop()
        if (s.ctx.fold) {
            foldTimer.interval = s.ctx.fold
            foldTimer.start()
        }
    }

    function onCtxButton(id) {
        var s = showing
        if (!s || s.kind !== "ctx") {
            closeBubble()
            return
        }
        var cid = s.ctx.id
        if (id === "next") {
            if (s.index < s.steps.length - 1) {
                s.index++
                renderStep(false)
            } else {
                finish(cid)
            }
            return
        }
        if (id === "prev") {
            if (s.index > 0)
                s.index--
            renderStep(false)
            return
        }
        if (id === "gotIt") {
            finish(cid)
            return
        }
        if (actionIds.indexOf(id) >= 0) {
            var w = where()
            finish(cid)
            doAction(id, w)
            return
        }
        // « Later », Escape, no answer: put off for this session (a tour
        // starts again at the same step), red dot on the A♠ button.
        var sn = snoozed
        sn[cid] = true
        snoozed = sn
        if (s.ctx.tour)
            resumeAt[cid] = s.index
        closeBubble()
        badge = true
    }

    function finish(cid) {
        markSeen(cid)
        var sn = snoozed
        delete sn[cid]
        snoozed = sn
        delete resumeAt[cid]
        if (cid === "ranked-result")
            result = null
        closeBubble()
        evalTimer.restart()
    }

    function doAction(id, w) {
        var l = lobby()
        if (id === "join" && w.rankPick && l) {
            l.joinGame(w.rankPick.gameId, "")
        } else if (id === "createRanking" && stack) {
            stack.push(Qt.resolvedUrl("../pages/LobbyCreateGamePage.qml"), { initialGameType: 3 })
        } else if (id === "signup") {
            if (typeof ServerConnection !== "undefined" && ServerConnection)
                ServerConnection.openExternalUrl(ServerConnection.registerUrl)
        } else if (id === "seeRanking" && stack) {
            stack.push(Qt.resolvedUrl("../pages/RankingPage.qml"))
        }
    }

    // A short message with its own buttons (menu, offer, notes).
    function note(text, buttons, handler, kind) {
        clearTimers()
        showing = { kind: kind || "note" }
        present(text, "", buttons, null, false, handler)
    }

    function showMenu() {
        var w = where()
        var tip = Core.canSpeak(w) ? Core.replayContext(w, candidates(w)) : null
        var list = []
        if (tip)
            list.push(btn("replayTip"))
        list.push(btn("resetTips"), btn("turnOff"), btn("close", true))
        note(t("menuOn"), list, function(id) {
            if (id === "replayTip" && tip) {
                var sn = guide.snoozed
                delete sn[tip.id]
                guide.snoozed = sn
                guide.showContext(tip)
            } else if (id === "resetTips") {
                Config.Parameters.guideSeen = "{}"
                guide.snoozed = ({})
                guide.resumeAt = ({})
                guide.note(guide.t("resetDone"), [guide.btn("close", true)],
                           function() { guide.closeBubble(); evalTimer.restart() })
            } else if (id === "turnOff") {
                guide.turnOff()
            } else {
                guide.closeBubble()
                evalTimer.restart()
            }
        })
    }

    function turnOn() {
        Config.Parameters.guideOffered = true
        Config.Parameters.guideOn = true
        note(t("welcome"), [btn("gotIt", true)], function() {
            guide.closeBubble()
            evalTimer.restart()
        })
    }

    function turnOff() {
        Config.Parameters.guideOn = false
        snoozed = ({})
        badge = false
        note(t("turnedOff"), [btn("close", true)], function() { guide.closeBubble() })
    }

    // ── Entry points (pokerth.qml, the side menu, the settings) ─────
    function chipTapped() {
        if (showing && showing.kind === "ctx") {
            onCtxButton("later")
            return
        }
        if (showing) {
            closeBubble()
            evalTimer.restart()
            return
        }
        // A tip put off with « Later » that applies here comes first.
        var w = where()
        if (Core.canSpeak(w)) {
            for (var i = 0; i < contexts.length; ++i) {
                var c = contexts[i]
                if (isSnoozed(c.id) && fits(c, w) && (c.repeat || !isSeen(c.id))) {
                    var sn = snoozed
                    delete sn[c.id]
                    snoozed = sn
                    showContext(c)
                    updateBadge(w)
                    return
                }
            }
        }
        showMenu()
    }

    function menuEntry() {
        if (!helpOn)
            turnOn()
        else
            showMenu()
    }

    // Escape: « Later » for a tip, closes any other bubble.
    function handleEscape() {
        if (!showing)
            return false
        if (showing.kind === "ctx")
            onCtxButton("later")
        else if (showing.kind === "offer")
            offerAnswer(false)
        else
            closeBubble()
        return true
    }

    // ── The first-launch offer ──────────────────────────────────────
    function offerAnswer(yes) {
        offerDone = true
        if (yes) {
            turnOn()
            return
        }
        Config.Parameters.guideOffered = true
        closeBubble()
    }
    function showOffer() {
        var w = where()
        if (showing || offerDone || helpOn || Config.Parameters.guideOffered
                || w.loginStep !== "start" || w.covered)
            return
        note(t("offer"), [btn("offerNo"), btn("offerYes", true)],
             function(id) { guide.offerAnswer(id === "offerYes") }, "offer")
    }

    // ── Watching the game (the finishing place of a Ranking game) ───
    function stacksSnapshot() {
        if (typeof GameTable === "undefined" || !GameTable)
            return null
        var ps = GameTable.players
        if (!ps || !ps.length || !ps[0] || !ps[0].name)
            return null
        var out = {}
        for (var i = 0; i < ps.length; ++i) {
            var p = ps[i]
            if (!p || (!p.name && !p.reserved))
                continue
            // Seat 0 is always my own; a player who left counts as unknown.
            out[p.seatId] = p.reserved ? null : (p.stack | 0) + (p.bet | 0)
        }
        return out
    }
    function settleResult(r) {
        if (!r || !tracker)
            return
        result = { gid: tracker.gid, place: r.place, tied: !!r.tied }
        tracker.done = true
    }
    function track(w) {
        var l = lobby()
        if (w.screen === "lobby") {
            if (lobbySince === 0)
                lobbySince = Date.now()
        } else {
            lobbySince = 0
        }
        // The finishing place, only on pokerth.net and not as a spectator.
        if (w.screen === "game" && w.net && !w.spectator && l && l.currentGameId) {
            if (!tracker || tracker.gid !== l.currentGameId) {
                var info = l.currentGameInfo()
                tracker = { gid: l.currentGameId, ranked: (info.gameType || 0) === Core.RANKED_TYPE,
                            prev: null, last: null, done: false }
            }
        } else if (tracker && w.screen !== "game") {
            if (tracker.ranked && !tracker.done)
                settleResult(Core.finishPlace(tracker.prev, tracker.last, 0))
            tracker = null
        }
        // « Just one more player! » when a Ranking table reaches max − 1.
        if (w.screen === "wait" && w.ranked && w.net) {
            if (lastWait && lastWait.gid === w.gid && w.waitCount > lastWait.n
                    && w.waitCount === w.waitMax - 1 && helpOn)
                flash(t("oneMore"))
            lastWait = { gid: w.gid, n: w.waitCount }
        } else {
            lastWait = null
        }
    }
    function flash(text) {
        if (showing && !(showing.kind === "ctx" && showing.ctx.id === "wait-ranking"))
            return
        if (showing) {
            bubbleText = styled(text)
            noteTimer.restart()
        } else {
            note(text, [btn("close", true)], function() { guide.closeBubble() })
            noteTimer.restart()
        }
    }

    Connections {
        target: (typeof GameTable !== "undefined") ? GameTable : null
        function onHandNumberChanged() {
            if (guide.tracker && guide.tracker.ranked && !guide.tracker.done)
                handSnapTimer.restart()
        }
        function onPlayersChanged() {
            if (!guide.tracker || !guide.tracker.ranked)
                return
            var s = guide.stacksSnapshot()
            if (s)
                guide.tracker.last = s
        }
        function onMyTurnChanged() {
            // My turn: an open menu or note gives the table free.
            if (GameTable.myTurn && guide.showing && guide.showing.kind !== "ctx")
                guide.closeBubble()
        }
    }
    // Read the stacks a moment after the new hand began (blinds posted, the
    // seats refreshed); stack + bet, so an all-in blind does not count as out.
    Timer {
        id: handSnapTimer
        interval: 500
        onTriggered: {
            var tr = guide.tracker
            if (!tr || !tr.ranked || tr.done)
                return
            var s = guide.stacksSnapshot()
            if (!s)
                return
            if (tr.prev)
                guide.settleResult(Core.finishPlace(tr.prev, s, 0))
            tr.prev = s
            tr.last = s
        }
    }

    // ── The evaluation loop ─────────────────────────────────────────
    function updateBadge(w) {
        var on = false
        if (w.helpOn && Core.canSpeak(w)) {
            for (var i = 0; i < contexts.length; ++i) {
                var c = contexts[i]
                if (isSnoozed(c.id) && fits(c, w) && (c.repeat || !isSeen(c.id))) {
                    on = true
                    break
                }
            }
        }
        badge = on
    }

    function evaluate() {
        if (!stack)
            return
        var w = where()
        track(w)
        if (!w.helpOn) {
            if (showing && showing.kind === "ctx")
                closeBubble()
            badge = false
            if (!showing && !offerDone && !Config.Parameters.guideOffered
                    && w.loginStep === "start" && !w.covered) {
                if (!offerTimer.running)
                    offerTimer.start()
            }
            return
        }
        if (showing) {
            if (showing.kind === "ctx") {
                if (!Core.canSpeak(w) || !fits(showing.ctx, w))
                    closeBubble()
                else if (showing.ctx.live && !noteTimer.running)
                    renderStep(true)
            }
            updateBadge(w)
            if (showing)
                return
        }
        var ctx = Core.pickContext(w, candidates(w), isSeen, isSnoozed)
        if (ctx)
            showContext(ctx)
        updateBadge(w)
    }

    Timer {
        id: evalTimer
        interval: 350
        onTriggered: guide.evaluate()
    }
    // The situation also changes without a signal (a page's own state, the
    // game list arriving): look again every second.
    Timer {
        interval: 1000
        repeat: true
        running: guide.stack !== null
        onTriggered: guide.evaluate()
    }
    Timer {
        id: offerTimer
        interval: 2500
        onTriggered: guide.showOffer()
    }
    Timer {
        id: stepTimer
        onTriggered: {
            var s = guide.showing
            if (s && s.kind === "ctx" && s.index < s.steps.length - 1) {
                s.index++
                guide.renderStep(false)
            }
        }
    }
    Timer {
        id: foldTimer
        onTriggered: {
            if (guide.showing && guide.showing.kind === "ctx")
                guide.onCtxButton("later")
        }
    }
    // « Just one more player! »: back to the tip after a few seconds.
    Timer {
        id: noteTimer
        interval: 4000
        onTriggered: {
            if (guide.showing && guide.showing.kind === "ctx")
                guide.renderStep(true)
            else if (guide.showing)
                guide.closeBubble()
        }
    }
    // The outline follows its control (scrolling, layout changes).
    Timer {
        interval: 200
        repeat: true
        running: guide.target !== null && guide.bubbleText !== ""
        onTriggered: guide.updateTargetRect()
    }

    Connections {
        target: guide.stack
        function onCurrentItemChanged() { evalTimer.restart() }
    }
    Connections {
        target: Config.Parameters
        function onGuideOnChanged() { evalTimer.restart() }
    }
    Connections {
        target: guide.lobby()
        function onGameListRevisionChanged() { evalTimer.restart() }
        function onIsInGameChanged() { evalTimer.restart() }
    }
    onWidthChanged: if (bubbleText !== "") Qt.callLater(place)
    onHeightChanged: if (bubbleText !== "") Qt.callLater(place)

    // ── The outline ─────────────────────────────────────────────────
    Rectangle {
        id: outline
        visible: guide.targetShown && guide.bubbleText !== ""
        x: guide.targetRect.x - 4
        y: guide.targetRect.y - 4
        width: guide.targetRect.width + 8
        height: guide.targetRect.height + 8
        radius: 8
        color: "transparent"
        border.color: Config.Theme.colorAccent
        border.width: 3
        onVisibleChanged: opacity = 1

        SequentialAnimation on opacity {
            running: outline.visible && Config.Theme.effectsEnabled
            loops: Animation.Infinite
            NumberAnimation { to: 0.35; duration: 700; easing.type: Easing.InOutSine }
            NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutSine }
        }
    }

    // ── The bubble ──────────────────────────────────────────────────
    Rectangle {
        id: bubble
        readonly property real maxTextHeight: Math.max(80, guide.area.height * 0.6)
        visible: guide.bubbleText !== ""
        width: Math.min(380, guide.width - 24)
        height: bubbleColumn.implicitHeight + 24
        x: guide.width - width - 12
        y: guide.bubbleAtTop ? guide.area.y + 8 : guide.area.y + guide.area.height - height - 12
        radius: Config.Theme.radiusMedium
        color: Config.Theme.colorPanel
        border.color: Config.Theme.colorAccent
        border.width: 1

        // The bubble takes every click (nothing goes through to the page below).
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onWheel: function(wheel) { wheel.accepted = true }
        }

        ColumnLayout {
            id: bubbleColumn
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                AppText {
                    text: "A♠"
                    font.bold: true
                    font.pixelSize: 13
                    color: Config.Theme.colorAccent
                }
                AppText {
                    Layout.fillWidth: true
                    text: guide.t("name")
                    font.bold: true
                    font.pixelSize: 13
                    color: Config.Theme.colorTextSecondary
                    elide: Text.ElideRight
                }
                AppText {
                    visible: guide.bubbleCount !== ""
                    text: guide.bubbleCount
                    font.pixelSize: 12
                    color: Config.Theme.colorTextMuted
                }
            }

            Flickable {
                id: textFlick
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(bubbleTextItem.implicitHeight, bubble.maxTextHeight)
                contentWidth: width
                contentHeight: bubbleTextItem.implicitHeight
                clip: true
                interactive: contentHeight > height
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: textFlick.interactive ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                }

                AppText {
                    id: bubbleTextItem
                    width: textFlick.width
                    textFormat: Text.StyledText
                    text: guide.bubbleText
                    wrapMode: Text.WordWrap
                    font.pixelSize: Config.Theme.fontSizeBody - 1
                    lineHeight: 1.15
                    color: Config.Theme.colorTextPrimary
                    onTextChanged: textFlick.contentY = 0
                }
            }

            // Right-aligned while the buttons fit on one line, wrapping in reading
            // order otherwise; the primary button comes last.
            Flow {
                id: buttonFlow
                readonly property real rowWidth: {
                    var w = 0
                    for (var i = 0; i < buttonRepeater.count; ++i) {
                        var it = buttonRepeater.itemAt(i)
                        if (it)
                            w += it.implicitWidth + (i > 0 ? spacing : 0)
                    }
                    return w
                }
                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: Math.min(bubbleColumn.width, rowWidth)
                spacing: 6

                Repeater {
                    id: buttonRepeater
                    model: guide.bubbleButtons
                    delegate: AbstractButton {
                        id: bubbleButton
                        required property var modelData
                        text: modelData.label
                        implicitWidth: buttonLabel.implicitWidth + 24
                        implicitHeight: Config.Responsive.isMobile ? 40 : 32
                        Keys.onReturnPressed: clicked()
                        Keys.onEnterPressed: clicked()
                        onClicked: {
                            var h = guide.buttonHandler
                            if (h)
                                h(bubbleButton.modelData.id)
                        }
                        background: Rectangle {
                            radius: Config.Theme.radiusSmall
                            color: bubbleButton.modelData.primary
                                   ? (bubbleButton.pressed ? Config.Theme.colorAccentDim : Config.Theme.colorAccent)
                                   : (bubbleButton.hovered ? Config.Theme.colorHover : Config.Theme.colorBox)
                            border.width: 1
                            border.color: bubbleButton.visualFocus
                                          ? Config.Theme.colorTextPrimary
                                          : bubbleButton.modelData.primary
                                            ? Config.Theme.colorAccentDim
                                            : Config.Theme.colorSurfaceMid
                        }
                        contentItem: AppText {
                            id: buttonLabel
                            text: bubbleButton.text
                            font.pixelSize: 13
                            font.bold: bubbleButton.modelData.primary
                            color: bubbleButton.modelData.primary ? "#1d222b" : Config.Theme.colorTextPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        HoverHandler { cursorShape: Qt.PointingHandCursor }
                    }
                }
            }
        }
    }
}
