pragma Singleton
import QtQuick

// Community-„Suggest"-Feature (aus dem Legacy-bbcbot portiert): schlägt für ein
// BBC-Step- bzw. WEC-Invite-Spiel passende, gerade idle Spieler vor. Die
// Bewertungs-/Auswahllogik entspricht 1:1 dem Bot (bbcbotplayerdb):
//   • BBC Step N: Score (tickets<<11)+(games<<4)+rating aus der Spieler-DB
//     (minidb.txt), nur Score > 10, Top 12 nach Score.
//   • WEC: idle Spieler, die auf der WEC-Liste (weclist.txt) stehen, in
//     zufälliger Reihenfolge (wie der Bot), Top 10.
//
// Die Botfiles werden per XHR von bbc.pokerth.net geladen und 15 Minuten
// gecacht (danach beim nächsten Vorschlag frisch geholt). Der von Cloudflare
// erwartete User-Agent "PokerTH/2.0 (Qt Network)" wird global über die
// WebNetworkAccessManagerFactory injiziert (siehe pokerth.cpp) – QML-XHR darf
// den Header selbst nicht setzen, deshalb passiert das dort zentral.
QtObject {
    id: botSuggest

    readonly property string baseUrl: "https://bbc.pokerth.net/exp3/bbcbot/"
    readonly property int cacheTtlMs: 15 * 60 * 1000

    // Cache je Datei: data (geparst) + ts (Zeitpunkt des Ladens).
    property var _cache: ({ db: { data: null, ts: 0 }, wec: { data: null, ts: 0 },
                            gameslist: { data: null, ts: 0 } })
    // Wartende Callbacks, solange eine Datei gerade geladen wird.
    property var _queues: ({ db: [], wec: [], gameslist: [] })
    property var _inflight: ({ db: false, wec: false, gameslist: false })

    // ── Community-Suggest-Typ des eigenen Spiels ─────────────────────────────
    // Der Suggest-Typ wird NICHT (mehr) aus dem Spielnamen geraten – das war
    // fragil (Groß/Kleinschreibung, verschobener/ergänzter Prefix ⇒ „WEC" wurde
    // nur bei exakt unverändertem Namen erkannt). Stattdessen trägt jedes Preset
    // seinen Typ explizit; beim Erstellen setzt LobbyCreateGamePage diesen Wert,
    // der Warteraum (GameWaitPage) liest ihn. Der Suggest-Button erscheint ohnehin
    // nur für den Ersteller, der beim Anlegen das Preset kennt – der Spielname
    // darf also frei geändert werden.
    // Werte: "step1".."step4", "wec" (Suggest möglich) oder "" (kein Suggest –
    // Monthly Cup, WEC Monthly Final, Nicht-Community-Spiele).
    property string createdSuggestType: ""

    // Ist der gesetzte Typ ein gültiges Suggest-Ziel? (Button-Sichtbarkeit)
    function isSuggestType(type) {
        return type === "wec" || /^step[1-4]$/.test(type || "")
    }

    // Aktuellen „Game Title Prefix" eines Community-Spiels aus gameslist.txt.
    // Für die Monthly-Cup-Tische wird dieser Titel serverseitig monatlich
    // gepflegt (z. B. "July Cup" / "August Cup", command "mcup"/"mcupfinal").
    // onResult(title): leerer String, wenn nicht ermittelbar.
    function gameTitlePrefix(command, onResult) {
        _ensure("gameslist", function(ok) {
            var map = ok ? botSuggest._cache.gameslist.data : null
            onResult((map && map[command]) ? map[command] : "")
        })
    }

    // ── Vorschlag erzeugen ───────────────────────────────────────────────────
    // type:      Suggest-Typ des eigenen Spiels ("step1".."step4" | "wec")
    // idleNames: Namen der idle Lobby-Spieler (Lobby.idlePlayerNames())
    // onResult(success, message): message wird bei success (lokal) im Chat gezeigt.
    function suggestForType(type, idleNames, onResult) {
        var m = /^step([1-4])$/.exec(type || "")
        if (m) {
            var step = parseInt(m[1], 10)
            _ensure("db", function(ok) {
                onResult(ok, ok ? _suggestStep(step, idleNames) : "")
            })
            return
        }
        if (type === "wec") {
            _ensure("wec", function(ok) {
                onResult(ok, ok ? _suggestWec(idleNames) : "")
            })
            return
        }
        onResult(false, "")
    }

    // ── Laden + Cachen ───────────────────────────────────────────────────────
    function _fileName(kind) {
        if (kind === "wec") return "weclist.txt"
        if (kind === "gameslist") return "gameslist.txt"
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
            // Bei Netz-/Parsefehler auf (ggf. abgelaufene) Altdaten zurückfallen.
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
        if (kind === "wec") return _parseWec(text)
        if (kind === "gameslist") return _parseGameslist(text)
        return _parseDb(text)
    }

    // gameslist.txt: Zeilen "#command#permgroup#Game Title Prefix#" (mind. 4×'#';
    // Kommentare "//" und Zeilen mit weniger '#' werden ignoriert, wie im bbcbot).
    // → { command: titlePrefix }, z. B. { mcup: "July Cup", mcupfinal: "July Cup Final" }.
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

    // weclist.txt: ein Spielername pro Zeile → { lowercase: originalName }.
    function _parseWec(text) {
        var set = ({})
        var lines = text.split(/\r?\n/)
        for (var i = 0; i < lines.length; ++i) {
            var name = lines[i].trim()
            if (name.length === 0)
                continue
            set[name.toLowerCase()] = name
        }
        return set
    }

    // minidb.txt: Name<TAB>ts2<TAB>ts3<TAB>ts4<TAB>rating<TAB>games.
    // Name NICHT trimmen (kann führende/anhängende Zeichen enthalten, z. B.
    // "* ghoti *"); nur Zeilen mit rating > 0 übernehmen (wie der Bot).
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
            map[name.toLowerCase()] = {
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

    // ── Bewertung/Auswahl (identisch zu bbcbotplayerdb) ──────────────────────
    function _score2(rating, tickets, games) {
        if (tickets <= 0)
            return 0
        return (tickets << 11) + (games << 4) + rating
    }

    function _suggestStep(step, idleNames) {
        var db = _cache.db.data
        var scored = []
        for (var i = 0; i < idleNames.length; ++i) {
            var e = db[idleNames[i].toLowerCase()]
            if (!e)
                continue
            var tickets = step === 1 ? 1 : (step === 2 ? e.ts2 : (step === 3 ? e.ts3 : e.ts4))
            var s = _score2(e.rating, tickets, e.games)
            if (s <= 10)
                continue
            scored.push({ name: e.name, score: s })
        }
        scored.sort(function(a, b) { return b.score - a.score })
        if (scored.length === 0)
            return "Sorry, no player found to suggest"
        var names = []
        for (var j = 0; j < scored.length && j < 12; ++j)
            names.push(scored[j].name)
        return "I suggest the following players for step " + step + ": " + names.join(", ")
    }

    function _suggestWec(idleNames) {
        var set = _cache.wec.data
        var scored = []
        for (var i = 0; i < idleNames.length; ++i) {
            var orig = set[idleNames[i].toLowerCase()]
            if (orig === undefined)
                continue
            // Zufalls-Score wie im Legacy-Bot → zufällige Reihenfolge.
            scored.push({ name: orig, score: Math.random() })
        }
        scored.sort(function(a, b) { return b.score - a.score })
        if (scored.length === 0)
            return "Sorry, no wec player found to suggest"
        var names = []
        for (var j = 0; j < scored.length && j < 10; ++j)
            names.push(scored[j].name)
        return "I suggest the following players for wec: " + names.join(", ")
    }
}
