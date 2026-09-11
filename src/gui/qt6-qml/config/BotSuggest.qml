pragma Singleton
import QtQuick

// The community "suggest" feature (ported from the legacy bbcbot): it suggests
// matching, currently idle players for a BBC step or WEC invite game. The
// rating/selection logic corresponds 1:1 to the bot (bbcbotplayerdb):
//   • BBC step N: the score (tickets<<11)+(games<<4)+rating from the player DB
//     (minidb.txt), only a score > 10, the top 12 by score.
//   • WEC: idle players who are on the WEC list (weclist.txt), in
//     random order (like the bot), the top 10.
//
// The botfiles (minidb.txt, weclist.txt, gameslist.txt, bbcadmins.txt,
// wecadmins.txt) are
// loaded via XHR from bbc.pokerth.net and cached for 15 minutes (afterwards fetched freshly on the next suggestion). The user agent
// "PokerTH/2.0 (Qt Network)" expected by Cloudflare is injected globally via the
// WebNetworkAccessManagerFactory (see pokerth.cpp) – QML XHR must not
// set the header itself, which is why it happens centrally there.
QtObject {
    id: botSuggest

    readonly property string baseUrl: "https://bbc.pokerth.net/exp3/bbcbot/"
    readonly property int cacheTtlMs: 15 * 60 * 1000

    // The cache per file: data (parsed) + ts (the time it was loaded).
    property var _cache: ({ db: { data: null, ts: 0 }, wec: { data: null, ts: 0 },
                            gameslist: { data: null, ts: 0 }, bbcadmins: { data: null, ts: 0 },
                            wecadmins: { data: null, ts: 0 } })
    // Waiting callbacks while a file is being loaded.
    property var _queues: ({ db: [], wec: [], gameslist: [], bbcadmins: [], wecadmins: [] })
    property var _inflight: ({ db: false, wec: false, gameslist: false, bbcadmins: false,
                              wecadmins: false })

    // ── Community suggest type of your own game ──────────────────────────────
    // The suggest type is NOT (any more) guessed from the game name – that was
    // fragile (upper/lower case, a shifted/extended prefix ⇒ "WEC" was
    // only recognised with an exactly unchanged name). Instead every preset carries
    // its type explicitly; when creating, LobbyCreateGamePage sets this value,
    // and the waiting room (GameWaitPage) reads it. So the game name may be
    // changed freely.
    // It applies only to the CREATOR. A joining BBC admin does not know this value
    // and derives the type from the table settings – likewise without
    // the name, see suggestTypeForGameInfo().
    // The values: "step1".."step4", "wec" (a suggestion is possible) or "" (no suggestion –
    // monthly cup, WEC monthly final, non-community games).
    property string createdSuggestType: ""

    // Is the type that is set a valid suggest target? (the button visibility)
    function isSuggestType(type) {
        return type === "wec" || /^step[1-4]$/.test(type || "")
    }

    // ── Community templates ──────────────────────────────────────────────────
    // The tournament templates live here (no longer in the create page), because they
    // have two jobs: filling the form fields when creating AND serving as a
    // fingerprint to recognise the type of a FOREIGN table (see
    // suggestTypeForGameInfo). Both have to come from the same table.
    readonly property var presets: [
        // suggestType: the explicit community suggest type (instead of a name regex).
        // It is passed to Config.BotSuggest.createdSuggestType when creating;
        // if it is missing, there is no player suggestion (monthly cup, WEC monthly
        // final). See [[Config.BotSuggest]].
        { name: "BBC Step 1", suggestType: "step1", startCash: 3000, firstSmallBlind: 15,
          raiseOnHands: false, raiseEveryHands: 11, raiseEveryMinutes: 5, playerActionTimeout: 10,
          blinds: [20, 25, 30, 40, 50, 60, 80, 100, 120, 150, 200, 250, 300, 400, 500,
                   600, 800, 1000, 1200, 1500, 2000, 2500, 3000, 4000, 5000, 6000, 8000,
                   10000, 12000, 15000] },
        { name: "BBC Step 2", suggestType: "step2", startCash: 4000, firstSmallBlind: 20,
          raiseOnHands: false, raiseEveryHands: 11, raiseEveryMinutes: 5, playerActionTimeout: 10,
          blinds: [25, 30, 40, 50, 60, 80, 100, 120, 150, 200, 250, 300, 400, 500, 600,
                   800, 1000, 1200, 1500, 2000, 2500, 3000, 4000, 5000, 6000, 8000, 10000,
                   12000, 15000, 20000] },
        { name: "BBC Step 3", suggestType: "step3", startCash: 5000, firstSmallBlind: 25,
          raiseOnHands: false, raiseEveryHands: 11, raiseEveryMinutes: 5, playerActionTimeout: 10,
          blinds: [30, 40, 50, 60, 80, 100, 120, 150, 200, 250, 300, 400, 500, 600, 800,
                   1000, 1200, 1500, 2000, 2500, 3000, 4000, 5000, 6000, 8000, 10000,
                   12000, 15000, 20000, 25000] },
        { name: "BBC Step 4", suggestType: "step4", startCash: 10000, firstSmallBlind: 50,
          raiseOnHands: false, raiseEveryHands: 11, raiseEveryMinutes: 5, playerActionTimeout: 10,
          blinds: [60, 80, 100, 120, 150, 200, 250, 300, 400, 500, 600, 800, 1000, 1200,
                   1500, 2000, 2500, 3000, 4000, 5000, 6000, 8000, 10000, 12000, 15000,
                   20000, 25000, 30000, 40000, 50000] },
        // Monthly cup: the table name is maintained monthly on the server side
        // (gameslist.txt, the command "mcup"/"mcupfinal" → e.g. "July Cup",
        // "August Cup"). titleCommand triggers pulling the current title.
        { name: "Monthly Cup", titleCommand: "mcup", startCash: 10000, firstSmallBlind: 50,
          raiseOnHands: true, raiseEveryHands: 16, raiseEveryMinutes: 5, playerActionTimeout: 10,
          blinds: [] },
        { name: "Monthly Cup Final", titleCommand: "mcupfinal", startCash: 10000, firstSmallBlind: 50,
          raiseOnHands: true, raiseEveryHands: 22, raiseEveryMinutes: 5, playerActionTimeout: 12,
          blinds: [] },
        { name: "WEC", suggestType: "wec", startCash: 10000, firstSmallBlind: 50,
          raiseOnHands: true, raiseEveryHands: 22, raiseEveryMinutes: 5, playerActionTimeout: 12,
          blinds: [] },
        { name: "WEC Monthly Final", startCash: 10000, firstSmallBlind: 50,
          raiseOnHands: true, raiseEveryHands: 25, raiseEveryMinutes: 5, playerActionTimeout: 15,
          blinds: [] },
        { name: "WEC Grand Final", suggestType: "wec", startCash: 10000, firstSmallBlind: 50,
          raiseOnHands: true, raiseEveryHands: 35, raiseEveryMinutes: 5, playerActionTimeout: 25,
          blinds: [] }
    ]

    // ── Type detection of foreign tables ─────────────────────────────────────
    // A joining player does not know createdSuggestType (that only sits in the
    // client of the creator) and the protocol transmits no template type.
    // The table name is NOT suitable as a source – it is freely editable. Instead
    // the actual game settings are checked against the templates:
    // the starting money + the first small blind + the complete manual blind order
    // identify a BBC step unambiguously.
    //
    // The WEC templates double the blinds, so they have no blind list as a
    // fingerprint. The starting money + the first small blind alone are NO
    // signature for them (10000/50 matches arbitrary foreign tables), which is why the
    // raise interval (mode + value) and the action timeout have to match there in addition –
    // together with the invite-only filter of the caller that is tight enough.
    // A known fuzziness: "Monthly Cup Final" has exactly the same settings
    // as "WEC" (10000/50, every 22 hands, 12 s) – the two cannot be separated by their
    // settings, so a WEC admin sees the button at the
    // monthly cup final table as well. The suggestion only lands locally at whoever clicks,
    // so this is accepted (the table name stays excluded as a distinction:
    // it is freely editable).
    //
    // info: Lobby.currentGameInfo() (the fields startMoney, firstSmallBlind,
    // manualBlinds, raiseIntervalMode, raiseEveryHands, raiseEveryMinutes,
    // playerActionTimeoutSec).
    // The return value: "step1".."step4", "wec" or "" (not recognised).
    function suggestTypeForGameInfo(info) {
        if (!info)
            return ""
        var blinds = info.manualBlinds || []
        for (var i = 0; i < presets.length; ++i) {
            var p = presets[i]
            if (!p.suggestType)
                continue
            if (p.startCash !== info.startMoney || p.firstSmallBlind !== info.firstSmallBlind)
                continue
            var pb = p.blinds || []
            if (pb.length !== blinds.length)
                continue
            if (pb.length > 0) {
                var same = true
                for (var b = 0; b < pb.length; ++b) {
                    if (pb[b] !== blinds[b]) {
                        same = false
                        break
                    }
                }
                if (!same)
                    continue
            } else {
                // RAISE_ON_HANDNUMBER = 1, RAISE_ON_MINUTES = 2 (gamedata.h)
                if (p.raiseOnHands !== (info.raiseIntervalMode === 1))
                    continue
                if (p.raiseOnHands ? (p.raiseEveryHands !== info.raiseEveryHands)
                                   : (p.raiseEveryMinutes !== info.raiseEveryMinutes))
                    continue
                if (p.playerActionTimeout !== info.playerActionTimeoutSec)
                    continue
            }
            return p.suggestType
        }
        return ""
    }

    // ── Community admin match ────────────────────────────────────────────────
    // One admin list per community in the format of weclist.txt: bbcadmins.txt for
    // the BBC steps, wecadmins.txt for the WEC tables. It decides whether your
    // own player may suggest at a FOREIGN table of this community.
    // Only call it once the local fingerprint already delivers a suggest type –
    // then the feature costs no request at all other tables.
    // onResult(isAdmin): false as well if the file is not (yet) retrievable.
    // The time of the last (also failed) attempt per admin list.
    property var _adminLastTry: ({ bbcadmins: 0, wecadmins: 0 })

    // Suggest type → the responsible admin list ("" = none, i.e. no foreign table suggestion).
    function _adminKind(type) {
        if (/^step[1-4]$/.test(type || "")) return "bbcadmins"
        if (type === "wec") return "wecadmins"
        return ""
    }

    function isCommunityAdmin(type, nick, onResult) {
        var kind = _adminKind(type)
        if (kind.length === 0 || !nick || nick.length === 0) {
            onResult(false)
            return
        }
        // Throttle the failures: the request hangs off the button visibility.
        // Without this lock a missing/unreachable file would cause one
        // download per entering of a community table. A filled cache
        // answers the question without the network anyway (_ensure).
        var fresh = _cache[kind].data !== null
                    && (Date.now() - _cache[kind].ts) < cacheTtlMs
        if (!fresh && (Date.now() - _adminLastTry[kind]) < cacheTtlMs) {
            onResult(false)
            return
        }
        if (!fresh)
            _adminLastTry[kind] = Date.now()
        _ensure(kind, function(ok) {
            var set = ok ? botSuggest._cache[kind].data : null
            onResult(!!(set && set[botSuggest._key(nick)] !== undefined))
        })
    }

    // Fetch gameslist.txt into the cache in advance. Without that the file is only loaded when
    // switching to a monthly cup template – the title then arrives
    // asynchronously, and whoever clicks "create game" immediately sends the
    // template fallback name ("Monthly Cup Final" instead of "August Cup Final").
    // It is called when opening the create page; the file is ~1 kB.
    function prefetchGameTitles() {
        _ensure("gameslist", function(ok) {})
    }

    // The current "game title prefix" of a community game from gameslist.txt.
    // For the monthly cup tables this title is maintained monthly on the
    // server side (e.g. "July Cup" / "August Cup", the command "mcup"/"mcupfinal").
    // onResult(title): an empty string if it cannot be determined.
    function gameTitlePrefix(command, onResult) {
        _ensure("gameslist", function(ok) {
            var map = ok ? botSuggest._cache.gameslist.data : null
            onResult((map && map[command]) ? map[command] : "")
        })
    }

    // ── Create the suggestion ────────────────────────────────────────────────
    // type:      the suggest type of your own game ("step1".."step4" | "wec")
    // idleNames: the names of the idle lobby players (Lobby.idlePlayerNames())
    // onResult(success, message): on success the message is shown (locally) in the chat.
    function suggestForType(type, idleNames, playingPlayers, onResult) {
        var m = /^step([1-4])$/.exec(type || "")
        if (m) {
            var step = parseInt(m[1], 10)
            _ensure("db", function(ok) {
                onResult(ok, ok ? _suggestStep(step, idleNames, playingPlayers) : "")
            })
            return
        }
        if (type === "wec") {
            _ensure("wec", function(ok) {
                onResult(ok, ok ? _suggestWec(idleNames, playingPlayers) : "")
            })
            return
        }
        onResult(false, "")
    }

    // The lookup key for matching the lobby nick ⇔ the botfile. Server nicks
    // may contain leading/trailing spaces (the registered account
    // "tammnt " for instance), the botfiles carry the same player trimmed – and
    // the other way round the minidb contains "silver skies- " with a space. Without
    // this normalization such a player silently drops out of every
    // suggestion. Only the key is trimmed, the name that is output
    // stays unchanged (names may carry decorative characters, e.g. "* ghoti *").
    function _key(name) {
        return (name || "").trim().toLowerCase()
    }

    // ── Laden + Cachen ───────────────────────────────────────────────────────
    function _fileName(kind) {
        if (kind === "wec") return "weclist.txt"
        if (kind === "gameslist") return "gameslist.txt"
        if (kind === "bbcadmins") return "bbcadmins.txt"
        if (kind === "wecadmins") return "wecadmins.txt"
        return "minidb.txt"
    }

    function _ensure(kind, done) {
        var c = _cache[kind]
        if (c.data !== null && (Date.now() - c.ts) < cacheTtlMs) {
            done(true)
            return
        }
        _queues[kind].push(done)
        if (_inflight[kind])
            return
        _inflight[kind] = true

        var xhr = new XMLHttpRequest()
        xhr.open("GET", baseUrl + _fileName(kind))
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            var ok = false
            if (xhr.status === 200 && xhr.responseText.length > 0) {
                try {
                    botSuggest._cache[kind] = { data: botSuggest._parse(kind, xhr.responseText), ts: Date.now() }
                    ok = true
                } catch (e) {
                    console.warn("BotSuggest: parse error for", botSuggest._fileName(kind), e)
                }
            } else {
                console.warn("BotSuggest: fetch failed for", botSuggest._fileName(kind), "status", xhr.status)
            }
            // On a network/parse error fall back to the (possibly expired) old data.
            if (!ok && botSuggest._cache[kind].data !== null)
                ok = true
            botSuggest._inflight[kind] = false
            var q = botSuggest._queues[kind]
            botSuggest._queues[kind] = []
            for (var i = 0; i < q.length; ++i)
                q[i](ok)
        }
        xhr.send()
    }

    function _parse(kind, text) {
        if (kind === "wec" || kind === "bbcadmins" || kind === "wecadmins")
            return _parseNameList(text)
        if (kind === "gameslist") return _parseGameslist(text)
        return _parseDb(text)
    }

    // gameslist.txt: lines "#command#permgroup#Game Title Prefix#" (at least 4×'#';
    // comments "//" and lines with fewer '#' are ignored, as in the bbcbot).
    // → { command: titlePrefix }, e.g. { mcup: "July Cup", mcupfinal: "July Cup Final" }.
    function _parseGameslist(text) {
        var map = ({})
        var lines = text.split(/\r?\n/)
        for (var i = 0; i < lines.length; ++i) {
            var line = lines[i].trim()
            if (line.length === 0 || line.indexOf("//") === 0)
                continue
            var parts = line.split("#")
            if (parts.length < 5)   // "" + command + perm + title + "" ⇒ ≥ 4 '#'
                continue
            var cmd = parts[1].trim()
            var title = parts[3].trim()
            if (cmd.length === 0 || title.length === 0)
                continue
            map[cmd] = title
        }
        return map
    }

    // weclist.txt / bbcadmins.txt / wecadmins.txt: one player name per line →
    // { lowercase: originalName }. All three botfiles share this format.
    function _parseNameList(text) {
        var set = ({})
        var lines = text.split(/\r?\n/)
        for (var i = 0; i < lines.length; ++i) {
            var name = lines[i].trim()
            if (name.length === 0)
                continue
            set[_key(name)] = name
        }
        return set
    }

    // minidb.txt: name<TAB>ts2<TAB>ts3<TAB>ts4<TAB>rating<TAB>games. The
    // name that is output is NOT trimmed (it may contain leading/trailing characters,
    // e.g. "* ghoti *"), only the key (_key); only take over lines with
    // rating > 0 (like the bot).
    function _parseDb(text) {
        var map = ({})
        var lines = text.split(/\r?\n/)
        for (var i = 0; i < lines.length; ++i) {
            var line = lines[i]
            if (line.length === 0)
                continue
            var f = line.split("\t")
            if (f.length < 6)
                continue
            var rating = parseInt(f[4], 10)
            if (!(rating > 0))
                continue
            var name = f[0]
            map[_key(name)] = {
                name: name,
                ts2: parseInt(f[1], 10) || 0,
                ts3: parseInt(f[2], 10) || 0,
                ts4: parseInt(f[3], 10) || 0,
                rating: rating,
                games: parseInt(f[5], 10) || 0
            }
        }
        return map
    }

    // ── Rating/selection (identical to bbcbotplayerdb) ───────────────────────
    function _score2(rating, tickets, games) {
        if (tickets <= 0)
            return 0
        return (tickets << 11) + (games << 4) + rating
    }

    // A list of names (idle players) → candidate objects { name } without a table reference.
    function _asCandidates(names) {
        var out = []
        for (var i = 0; i < names.length; ++i)
            out.push({ name: names[i] })
        return out
    }

    // BBC step: rate the candidates { name, game? }, descending by score.
    function _scoreStep(candidates, step) {
        var db = _cache.db.data
        var out = []
        for (var i = 0; i < candidates.length; ++i) {
            var e = db[_key(candidates[i].name)]
            if (!e)
                continue
            var tickets = step === 1 ? 1 : (step === 2 ? e.ts2 : (step === 3 ? e.ts3 : e.ts4))
            var s = _score2(e.rating, tickets, e.games)
            if (s <= 10)
                continue
            out.push({ dbName: e.name, score: s, game: candidates[i].game })
        }
        out.sort(function(a, b) { return b.score - a.score })
        return out
    }

    // WEC: candidates { name, game? } on the WEC list, a random score (as in the bot).
    function _scoreWec(candidates) {
        var set = _cache.wec.data
        var out = []
        for (var i = 0; i < candidates.length; ++i) {
            var orig = set[_key(candidates[i].name)]
            if (orig === undefined)
                continue
            out.push({ dbName: orig, score: Math.random(), game: candidates[i].game })
        }
        out.sort(function(a, b) { return b.score - a.score })
        return out
    }

    // Builds the suggestion text: a header line, then ONE player per line – first
    // the idle players, then in last place those currently playing, each annotated
    // with "(playing in game …)". Both groups are limited to `limit`;
    // emptyText if both are empty.
    // The line break is a real "\n"; the chat display converts it into <br>
    // (Lobby.postLocalChatNote), because the chat is a rich text document.
    function _buildMessage(headline, idleScored, busyScored, limit, emptyText) {
        if (idleScored.length === 0 && busyScored.length === 0)
            return emptyText
        var parts = []
        for (var i = 0; i < idleScored.length && i < limit; ++i)
            parts.push(idleScored[i].dbName)
        for (var j = 0; j < busyScored.length && j < limit; ++j)
            parts.push(busyScored[j].dbName + " (playing in game " + busyScored[j].game + ")")
        return headline + "\n" + parts.join("\n")
    }

    function _suggestStep(step, idleNames, playingPlayers) {
        // Step 1 computes with a fixed ticket=1 → practically every DB player
        // qualifies. So do NOT suggest those currently playing as well,
        // otherwise the list gets too long (only show them from step 2 on).
        var busy = step === 1 ? [] : _scoreStep(playingPlayers, step)
        return _buildMessage(
            "I suggest the following players for step " + step + ":",
            _scoreStep(_asCandidates(idleNames), step),
            busy,
            12,
            "Sorry, no player found to suggest")
    }

    function _suggestWec(idleNames, playingPlayers) {
        return _buildMessage(
            "I suggest the following players for wec:",
            _scoreWec(_asCandidates(idleNames)),
            _scoreWec(playingPlayers),
            10,
            "Sorry, no wec player found to suggest")
    }
}
