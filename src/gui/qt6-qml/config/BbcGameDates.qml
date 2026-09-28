pragma Singleton
import QtQuick

// BBC (Best Brainies Cup) game dates of the next days – the second tab of
// the forum news page.
//
// bbc.pokerth.net has no JSON list endpoint: the registration page embeds all
// game dates of the running season as a Vue prop
//   <registration-component :gamedates="[{id, step, date, title, num}, …]">
// which is fetched here via XHR and parsed (same approach as
// BbcRankingPage/CommunityRankingView with :results).
//   date   local time of the BBC site (Europe/Berlin), "YYYY-MM-DD HH:MM:SS"
//   step   1..4 = cup step, 0 = special game with its own title
//   num    number of registered players (a table holds 10)
//
// NO reference to other config singletons (circular `import Config`, see
// ForumNews.qml).
QtObject {
    id: bbc

    readonly property string siteUrl:         "https://bbc.pokerth.net"
    readonly property string registrationUrl: "https://bbc.pokerth.net/registration"

    readonly property int cacheTtlMs: 5 * 60 * 1000
    // Shown: games that started at most graceMs ago up to daysAhead days ahead.
    readonly property int daysAhead: 3
    readonly property int graceMs: 30 * 60 * 1000
    readonly property int maxPlayers: 10

    // [{ id, step, title, ts (UTC ms), day, num }] sorted by ts, already filtered.
    property var games: []
    property bool loading: false
    property string errorText: ""
    property real lastFetchMs: 0

    // Registered players per game, loaded lazily when a row is expanded:
    //   regs[id] = { players: [{ nick, admin }, …], loading, error, fetchedMs }
    // Only nickname and the BBC admin flag are kept – the endpoint also
    // delivers the IP address and browser fingerprint of every registration,
    // which are dropped here.
    // regsRevision is incremented on every change (bindings read it, the
    // object itself is mutated in place).
    property var regs: ({})
    property int regsRevision: 0
    readonly property int regsTtlMs: 2 * 60 * 1000

    function regsFor(id) {
        return regs[id] || null
    }

    // Fetches the registrations of a game unless a fresh list whose length
    // matches the current player count is cached.
    function loadRegs(game) {
        if (!game || game.num <= 0)
            return
        var id = game.id
        var old = regs[id]
        if (old && (old.loading
                    || (!old.error && old.players.length === game.num
                        && Date.now() - old.fetchedMs < regsTtlMs)))
            return

        _setRegs(id, { players: old ? old.players : [], loading: true, error: false,
                       fetchedMs: old ? old.fetchedMs : 0 })
        var xhr = new XMLHttpRequest()
        xhr.open("GET", siteUrl + "/registration/date/get/" + id)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            var players = null
            if (xhr.status === 200) {
                try {
                    var data = JSON.parse(xhr.responseText)
                    if (data && data.success === true && data.date && Array.isArray(data.date.regs)) {
                        players = []
                        for (var i = 0; i < data.date.regs.length; ++i) {
                            var p = data.date.regs[i].player
                            if (p && p.nickname)
                                players.push({ nick: String(p.nickname), admin: p.admin === true })
                        }
                    }
                } catch (e) {
                    console.warn("BbcGameDates: registrations not readable:", e)
                }
            } else {
                console.warn("BbcGameDates: registrations fetch failed, status", xhr.status)
            }
            var prev = bbc.regs[id]
            if (players === null)
                bbc._setRegs(id, { players: prev ? prev.players : [], loading: false, error: true,
                                   fetchedMs: prev ? prev.fetchedMs : 0 })
            else
                bbc._setRegs(id, { players: players, loading: false, error: false,
                                   fetchedMs: Date.now() })
        }
        xhr.send()
    }

    function _setRegs(id, entry) {
        regs[id] = entry
        ++regsRevision
    }

    // force = bypass the TTL. On errors the games fetched last stay.
    function refresh(force) {
        if (loading)
            return
        if (!force && lastFetchMs > 0 && (Date.now() - lastFetchMs) < cacheTtlMs) {
            // Re-apply the time window: the cached list may contain games that
            // have started since.
            games = _window(games)
            return
        }

        loading = true
        var xhr = new XMLHttpRequest()
        xhr.open("GET", registrationUrl)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            bbc.loading = false
            var list = null
            if (xhr.status === 200) {
                try {
                    list = bbc._parse(xhr.responseText)
                } catch (e) {
                    console.warn("BbcGameDates: page not readable:", e)
                }
            } else {
                console.warn("BbcGameDates: fetch failed, status", xhr.status)
            }
            if (list === null) {
                if (bbc.games.length === 0)
                    bbc.errorText = qsTr("The BBC game dates could not be loaded.")
                return
            }
            bbc.games = bbc._window(list)
            bbc.lastFetchMs = Date.now()
            bbc.errorText = ""
        }
        xhr.send()
    }

    // null = the prop is missing (page layout changed / error page).
    function _parse(html) {
        var m = html.match(/:gamedates="([^"]*)"/)
        if (!m)
            return null
        var raw = JSON.parse(m[1].replace(/&quot;/g, "\"").replace(/&#39;/g, "'")
                                 .replace(/&lt;/g, "<").replace(/&gt;/g, ">")
                                 .replace(/&amp;/g, "&"))
        if (!Array.isArray(raw))
            return null
        var out = []
        for (var i = 0; i < raw.length; ++i) {
            var g = raw[i]
            var ts = _berlinToUtc(g.date)
            if (isNaN(ts))
                continue
            out.push({
                id: g.id,
                step: Number(g.step) || 0,
                title: g.title || "",
                ts: ts,
                day: _gameDay(g.date),
                num: Number(g.num) || 0
            })
        }
        out.sort(function(a, b) { return a.ts - b.ts })
        return out
    }

    function _window(list) {
        var from = Date.now() - graceMs
        var to = Date.now() + daysAhead * 24 * 3600 * 1000
        return list.filter(function(g) { return g.ts >= from && g.ts <= to })
    }

    // "YYYY-MM-DD HH:MM[:SS]" in Europe/Berlin → UTC ms. EU rule: summer time
    // (UTC+2) from the last Sunday of March to the last Sunday of October,
    // each at 01:00 UTC; otherwise UTC+1.
    function _berlinToUtc(s) {
        var m = /^(\d{4})-(\d{2})-(\d{2})[ T](\d{2}):(\d{2})/.exec(s || "")
        if (!m)
            return NaN
        var y = +m[1]
        var wall = Date.UTC(y, +m[2] - 1, +m[3], +m[4], +m[5])
        var dstStart = _lastSundayUtc(y, 2) + 3600 * 1000
        var dstEnd = _lastSundayUtc(y, 9) + 3600 * 1000
        var summer = wall - 2 * 3600 * 1000
        return summer >= dstStart && summer < dstEnd ? summer : wall - 3600 * 1000
    }

    function _lastSundayUtc(year, month) {
        var d = new Date(Date.UTC(year, month + 1, 0))
        return Date.UTC(year, month, d.getUTCDate() - d.getUTCDay())
    }

    // Section key of the list, "YYYY-MM-DD": the game day as on the BBC
    // calendar – games before 14:00 Berlin time (the 01:00 game) belong to the
    // evening of the previous day.
    function _gameDay(s) {
        var m = /^(\d{4})-(\d{2})-(\d{2})[ T](\d{2})/.exec(s || "")
        var d = new Date(Date.UTC(+m[1], +m[2] - 1, +m[3]))
        if (+m[4] < 14)
            d.setUTCDate(d.getUTCDate() - 1)
        return d.toISOString().slice(0, 10)
    }

    function dayLabel(key) {
        var d = new Date(key + "T00:00:00")
        var today = new Date()
        today.setHours(0, 0, 0, 0)
        var diff = Math.round((d.getTime() - today.getTime()) / (24 * 3600 * 1000))
        var loc = Qt.locale()
        var name = loc.toString(d, "dddd") + ", " + d.toLocaleDateString(loc, Locale.ShortFormat)
        if (diff === 0)
            return qsTr("Today") + " · " + name
        if (diff === 1)
            return qsTr("Tomorrow") + " · " + name
        return name
    }

    function timeText(ts) {
        return new Date(ts).toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
    }

    function gameTitle(game) {
        if (!game)
            return ""
        return game.step > 0 ? qsTr("Step %1").arg(game.step)
                             : (game.title !== "" ? game.title : qsTr("Special game"))
    }

    function playersText(num) {
        return num === 1 ? qsTr("1 player registered")
                         : qsTr("%1 players registered").arg(num)
    }
}
