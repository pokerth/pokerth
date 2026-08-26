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
// Die Botfiles (minidb.txt, weclist.txt, gameslist.txt, bbcadmins.txt) werden
// per XHR von bbc.pokerth.net geladen und 15 Minuten gecacht (danach beim nächsten Vorschlag frisch geholt). Der von Cloudflare
// erwartete User-Agent "PokerTH/2.0 (Qt Network)" wird global über die
// WebNetworkAccessManagerFactory injiziert (siehe pokerth.cpp) – QML-XHR darf
// den Header selbst nicht setzen, deshalb passiert das dort zentral.
QtObject {
    id: botSuggest

    readonly property string baseUrl: "https://bbc.pokerth.net/exp3/bbcbot/"
    readonly property int cacheTtlMs: 15 * 60 * 1000

    // Cache je Datei: data (geparst) + ts (Zeitpunkt des Ladens).
    property var _cache: ({ db: { data: null, ts: 0 }, wec: { data: null, ts: 0 },
                            gameslist: { data: null, ts: 0 }, bbcadmins: { data: null, ts: 0 } })
    // Wartende Callbacks, solange eine Datei gerade geladen wird.
    property var _queues: ({ db: [], wec: [], gameslist: [], bbcadmins: [] })
    property var _inflight: ({ db: false, wec: false, gameslist: false, bbcadmins: false })

    // ── Community-Suggest-Typ des eigenen Spiels ─────────────────────────────
    // Der Suggest-Typ wird NICHT (mehr) aus dem Spielnamen geraten – das war
    // fragil (Groß/Kleinschreibung, verschobener/ergänzter Prefix ⇒ „WEC" wurde
    // nur bei exakt unverändertem Namen erkannt). Stattdessen trägt jedes Preset
    // seinen Typ explizit; beim Erstellen setzt LobbyCreateGamePage diesen Wert,
    // der Warteraum (GameWaitPage) liest ihn. Der Spielname darf also frei
    // geändert werden.
    // Gilt nur für den ERSTELLER. Ein beitretender BBC-Admin kennt diesen Wert
    // nicht und leitet den Typ aus den Tischeinstellungen ab – ebenfalls ohne
    // den Namen, siehe suggestTypeForGameInfo().
    // Werte: "step1".."step4", "wec" (Suggest möglich) oder "" (kein Suggest –
    // Monthly Cup, WEC Monthly Final, Nicht-Community-Spiele).
    property string createdSuggestType: ""

    // Ist der gesetzte Typ ein gültiges Suggest-Ziel? (Button-Sichtbarkeit)
    function isSuggestType(type) {
        return type === "wec" || /^step[1-4]$/.test(type || "")
    }

    // ── Community-Vorlagen ───────────────────────────────────────────────────
    // Die Turniervorlagen liegen hier (nicht mehr in der Create-Page), weil sie
    // zwei Aufgaben haben: die Formularfelder beim Erstellen füllen UND als
    // Fingerprint dienen, um den Typ eines FREMDEN Tisches zu erkennen (siehe
    // suggestTypeForGameInfo). Beides muss aus derselben Tabelle kommen.
    readonly property var presets: [
        // suggestType: expliziter Community-Suggest-Typ (statt Namens-Regex).
        // Wird beim Erstellen an Config.BotSuggest.createdSuggestType übergeben;
        // fehlt er, gibt es keinen Spielervorschlag (Monthly Cup, WEC Monthly
        // Final). Siehe [[Config.BotSuggest]].
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
        // Monthly Cup: der Tischname wird serverseitig monatlich gepflegt
        // (gameslist.txt, command "mcup"/"mcupfinal" → z. B. "July Cup",
        // "August Cup"). titleCommand triggert das Ziehen des aktuellen Titels.
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

    // ── Typ-Erkennung fremder Tische ─────────────────────────────────────────
    // Ein beitretender Spieler kennt createdSuggestType nicht (der steckt nur im
    // Client des Erstellers) und das Protokoll überträgt keinen Vorlagen-Typ.
    // Der Tischname taugt NICHT als Quelle – er ist frei editierbar. Stattdessen
    // werden die tatsächlichen Spieleinstellungen gegen die Vorlagen geprüft:
    // Startgeld + erster Small Blind + die vollständige manuelle Blindreihenfolge
    // identifizieren einen BBC-Step eindeutig.
    //
    // Vorlagen OHNE feste Blindliste (Monthly Cup, WEC) werden bewusst
    // übersprungen: „Blinds verdoppeln" mit 10000/50 ist keine Signatur, das
    // träfe auch beliebige fremde Tische. Für die geht es weiterhin nur über den
    // expliziten createdSuggestType des Erstellers.
    //
    // info: Lobby.currentGameInfo() (Felder startMoney, firstSmallBlind,
    // manualBlinds). Rückgabe: "step1".."step4" oder "" (nicht erkannt).
    function suggestTypeForGameInfo(info) {
        if (!info)
            return ""
        var blinds = info.manualBlinds || []
        if (blinds.length === 0)
            return ""
        for (var i = 0; i < presets.length; ++i) {
            var p = presets[i]
            if (!p.suggestType || !p.blinds || p.blinds.length === 0)
                continue
            if (p.startCash !== info.startMoney || p.firstSmallBlind !== info.firstSmallBlind)
                continue
            if (p.blinds.length !== blinds.length)
                continue
            var same = true
            for (var b = 0; b < p.blinds.length; ++b) {
                if (p.blinds[b] !== blinds[b]) {
                    same = false
                    break
                }
            }
            if (same)
                return p.suggestType
        }
        return ""
    }

    // ── BBC-Admin-Abgleich ───────────────────────────────────────────────────
    // bbcadmins.txt (Format wie weclist.txt) entscheidet, ob der eigene Spieler
    // an einem fremden BBC-Step-Tisch vorschlagen darf. Erst aufrufen, wenn der
    // lokale Fingerprint bereits „BBC-Step" sagt – dann kostet das Feature für
    // alle anderen Tische keinen einzigen Request.
    // onResult(isAdmin): false auch, wenn die Datei (noch) nicht abrufbar ist.
    property real _bbcAdminLastTry: 0

    function isBbcAdmin(nick, onResult) {
        if (!nick || nick.length === 0) {
            onResult(false)
            return
        }
        // Fehlschläge drosseln: die Anfrage hängt an der Button-Sichtbarkeit.
        // Ohne diese Sperre liefe bei fehlender/unerreichbarer Datei ein
        // Download pro Betreten eines Step-Tisches. Ein gefüllter Cache
        // beantwortet die Frage ohnehin ohne Netz (_ensure).
        var fresh = _cache.bbcadmins.data !== null
                    && (Date.now() - _cache.bbcadmins.ts) < cacheTtlMs
        if (!fresh && (Date.now() - _bbcAdminLastTry) < cacheTtlMs) {
            onResult(false)
            return
        }
        if (!fresh)
            _bbcAdminLastTry = Date.now()
        _ensure("bbcadmins", function(ok) {
            var set = ok ? botSuggest._cache.bbcadmins.data : null
            onResult(!!(set && set[botSuggest._key(nick)] !== undefined))
        })
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

    // Nachschlage-Schlüssel für den Abgleich Lobby-Nick ⇔ Botfile. Server-Nicks
    // dürfen führende/anhängende Leerzeichen enthalten (der registrierte Account
    // "tammnt " z. B.), die Botfiles führen denselben Spieler getrimmt – und
    // umgekehrt steht in der minidb auch "silver skies- " mit Leerzeichen. Ohne
    // diese Normalisierung fällt so ein Spieler stillschweigend aus jedem
    // Vorschlag heraus. Nur der Schlüssel wird getrimmt, der ausgegebene Name
    // bleibt unverändert (Namen dürfen Zierzeichen tragen, z. B. "* ghoti *").
    function _key(name) {
        return (name || "").trim().toLowerCase()
    }

    // ── Laden + Cachen ───────────────────────────────────────────────────────
    function _fileName(kind) {
        if (kind === "wec") return "weclist.txt"
        if (kind === "gameslist") return "gameslist.txt"
        if (kind === "bbcadmins") return "bbcadmins.txt"
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
        if (kind === "wec" || kind === "bbcadmins") return _parseNameList(text)
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

    // weclist.txt / bbcadmins.txt: ein Spielername pro Zeile → { lowercase:
    // originalName }. Beide Botfiles teilen dieses Format.
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

    // minidb.txt: Name<TAB>ts2<TAB>ts3<TAB>ts4<TAB>rating<TAB>games. Der
    // ausgegebene Name wird NICHT getrimmt (kann führende/anhängende Zeichen
    // enthalten, z. B. "* ghoti *"), nur der Schlüssel (_key); nur Zeilen mit
    // rating > 0 übernehmen (wie der Bot).
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

    // ── Bewertung/Auswahl (identisch zu bbcbotplayerdb) ──────────────────────
    function _score2(rating, tickets, games) {
        if (tickets <= 0)
            return 0
        return (tickets << 11) + (games << 4) + rating
    }

    // Namensliste (idle Spieler) → Kandidatenobjekte { name } ohne Tischbezug.
    function _asCandidates(names) {
        var out = []
        for (var i = 0; i < names.length; ++i)
            out.push({ name: names[i] })
        return out
    }

    // BBC-Step: Kandidaten { name, game? } bewerten, nach Score absteigend.
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

    // WEC: Kandidaten { name, game? } auf der WEC-Liste, Zufalls-Score (wie Bot).
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

    // Baut die Vorschlagszeile: zuerst idle Spieler, dann – an letzter Stelle –
    // die gerade spielenden, je mit „(playing in game …)" annotiert. Beide
    // Gruppen auf `limit` begrenzt; emptyText, falls beide leer.
    function _buildMessage(headline, idleScored, busyScored, limit, emptyText) {
        if (idleScored.length === 0 && busyScored.length === 0)
            return emptyText
        var parts = []
        for (var i = 0; i < idleScored.length && i < limit; ++i)
            parts.push(idleScored[i].dbName)
        for (var j = 0; j < busyScored.length && j < limit; ++j)
            parts.push(busyScored[j].dbName + " (playing in game " + busyScored[j].game + ")")
        return headline + parts.join(", ")
    }

    function _suggestStep(step, idleNames, playingPlayers) {
        // Step 1 rechnet mit festem Ticket=1 → praktisch jeder DB-Spieler
        // qualifiziert sich. Die gerade spielenden dann NICHT mit vorschlagen,
        // sonst wird die Liste zu lang (erst ab Step 2 einblenden).
        var busy = step === 1 ? [] : _scoreStep(playingPlayers, step)
        return _buildMessage(
            "I suggest the following players for step " + step + ": ",
            _scoreStep(_asCandidates(idleNames), step),
            busy,
            12,
            "Sorry, no player found to suggest")
    }

    function _suggestWec(idleNames, playingPlayers) {
        return _buildMessage(
            "I suggest the following players for wec: ",
            _scoreWec(_asCandidates(idleNames)),
            _scoreWec(playingPlayers),
            10,
            "Sorry, no wec player found to suggest")
    }
}
