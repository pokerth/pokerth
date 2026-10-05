.pragma library

// ═══════════════════════════════════════════════════════════════════
// Ace's Help — the decision rules, without any QML item (the port of the web
// client's modules/guide/core.mjs and ranking-pick.mjs, docs/GUIDE.md of
// pokerth-web-client). GuideOverlay.qml describes where the player is in a
// `where` snapshot; the contexts are plain data; this file decides which
// context speaks, if any. The Ranking rules are the same as in the web client,
// so every client points the players at the SAME table.
//
// where = {
//   helpOn:  the player turned Ace's Help on
//   screen:  'connect' | 'lobby' | 'wait' | 'create' | 'game' | 'other'
//   playing: a game is running at the table
//   net:     the lobby is connected to pokerth.net (not a LAN server)
//   guest:   logged in as a guest on pokerth.net
//   ranked:  the table the player sits at is a Ranking game
//   windows: keys of the windows open right now (the overlay pages)
//   …        the extra fields the contexts read (see GuideOverlay.qml)
// }
//
// context = {
//   id, priority, screens | window, needs: { field: bool }, when(where),
//   manual (only when asked), repeat (may speak again after being seen),
//   live (re-rendered while it shows), fold (ms: unanswered → badge),
//   tour (a walk through a form), steps: [{ text, vars, target, buttons,
//   auto (ms → next step), when(where), optional (left out without target) }]
// }
// ═══════════════════════════════════════════════════════════════════

var TALK_SCREENS = ["connect", "lobby", "wait", "create"]

// Can the guide speak in this situation at all (never during play)?
function canSpeak(where) {
    if (!where || !where.helpOn || where.playing)
        return false
    return TALK_SCREENS.indexOf(where.screen) >= 0
}

// Does this context apply to this situation (ignoring the saved progress)?
function applies(ctx, where) {
    if (!ctx || !where)
        return false
    if (ctx.window) {
        if (!where.windows || where.windows.indexOf(ctx.window) < 0)
            return false
    } else if (!ctx.screens || ctx.screens.indexOf(where.screen) < 0) {
        return false
    }
    var needs = ctx.needs || {}
    for (var k in needs) {
        if (!!where[k] !== !!needs[k])
            return false
    }
    if (typeof ctx.when === "function") {
        try {
            if (!ctx.when(where))
                return false
        } catch (e) {
            return false
        }
    }
    return true
}

// The context to show now, or null. seen(id) / snoozed(id) read the progress.
function pickContext(where, contexts, seen, snoozed) {
    if (!canSpeak(where) || !contexts)
        return null
    var best = null, bestP = -Infinity
    for (var i = 0; i < contexts.length; ++i) {
        var ctx = contexts[i]
        if (!ctx || ctx.manual || !applies(ctx, where))
            continue
        if ((!ctx.repeat && seen && seen(ctx.id)) || (snoozed && snoozed(ctx.id)))
            continue
        var p = Number(ctx.priority) || 0
        if (p > bestP) {
            best = ctx
            bestP = p
        }
    }
    return best
}

// The context of this screen, seen or not (« This screen's tip »).
function replayContext(where, contexts) {
    return pickContext(where, contexts, null, null)
}

// The steps of a context that exist here: a step's own when(where) and the
// page's keep(step) (a tour step whose field is not on this page) filter.
function stepsFor(ctx, where, keep) {
    var out = []
    var steps = (ctx && ctx.steps) ? ctx.steps : []
    for (var i = 0; i < steps.length; ++i) {
        var s = steps[i]
        try {
            if (where && typeof s.when === "function" && !s.when(where))
                continue
            if (keep && !keep(s))
                continue
        } catch (e) {
            continue
        }
        out.push(s)
    }
    return out
}

// A value of a step: a plain value or a function of `where`.
function val(v, where) {
    return typeof v === "function" ? v(where) : v
}

// Fills {name} placeholders; unknown ones are left as they are.
function fill(text, vars) {
    if (!vars)
        return String(text)
    return String(text).replace(/\{(\w+)\}/g, function(m, k) {
        return vars[k] !== undefined && vars[k] !== null ? String(vars[k]) : m
    })
}

// ── Ranking rules (identical to the web client's ranking-pick.mjs) ─────
var RANKED_TYPE = 4   // NetGameInfo.netGameType of a Ranking game
var MODE_OPEN = 1     // game list mode: 1 open, 2 running, 3 closed

// The Ranking table to suggest: among Ranking tables that are open, not full
// and without a password, the FULLEST one; on a tie the oldest (lowest game
// id). Everyone converges on the same table, so it fills up and starts.
// games = [{ gameId, gameType, gameMode, playerCount, maxPlayers, isPrivate }]
function pickRankingTable(games, guest) {
    if (guest || !games)
        return null
    var best = null
    for (var i = 0; i < games.length; ++i) {
        var g = games[i]
        if (!g || g.gameType !== RANKED_TYPE || g.gameMode !== MODE_OPEN || g.isPrivate)
            continue
        var max = g.maxPlayers || 10, n = g.playerCount | 0
        if (n >= max)
            continue
        if (!best || n > (best.playerCount | 0)
                || (n === (best.playerCount | 0) && g.gameId < best.gameId))
            best = g
    }
    return best
}

// Points by finishing place (pokerth.net ranking): 15/9/6/4/3/2/1, then 0.
var POINTS = [15, 9, 6, 4, 3, 2, 1]
function pointsFor(place) {
    return place >= 1 && place <= POINTS.length ? POINTS[place - 1] : 0
}

// My finishing place from two snapshots of the stacks ({ seat: chips },
// null = unknown) taken at two moments of a game:
//  · I had chips before and none after → I am out: place = players still
//    holding chips + 1; if someone else went out in the same step the order
//    is unknown → { place: null, tied: true } (no guessing).
//  · I have chips and every other known stack is empty → I won: place 1.
// Returns null while nothing is decided.
function finishPlace(before, after, me) {
    if (!before || !after || me === undefined || me === null)
        return null
    function known(v) { return typeof v === "number" && isFinite(v) }
    var others = []
    for (var k in after) {
        if (String(k) !== String(me))
            others.push(k)
    }
    var mineB = before[me], mineA = after[me]
    if (known(mineB) && mineB > 0 && known(mineA) && mineA <= 0) {
        var alive = 0, tied = 0
        for (var i = 0; i < others.length; ++i) {
            var a = after[others[i]], b = before[others[i]]
            if (!known(a) || a > 0)
                alive++
            else if (known(b) && b > 0)
                tied++
        }
        return tied ? { place: null, tied: true } : { place: alive + 1 }
    }
    if (known(mineA) && mineA > 0 && others.length > 0) {
        for (var j = 0; j < others.length; ++j) {
            var v = after[others[j]]
            if (!known(v) || v > 0)
                return null
        }
        return { place: 1 }
    }
    return null
}
