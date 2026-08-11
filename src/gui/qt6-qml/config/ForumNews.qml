pragma Singleton
import QtQuick
import QtCore

// Forum-Neuigkeiten – Portierung des Web-Client-Features (dort
// public/modules/ui/forumnews.mjs + proxy.js /api/forumfeed) auf den
// QML-Client: die letzten Forenbeiträge von www.pokerth.net mit Zähler
// ungelesener Beiträge in der Topbar.
//
// Datenquelle ist der phpBB-Atom-Feed. Der Web-Client braucht dafür einen
// Relay (der Feed schickt keinen CORS-Header) – der QML-Client holt ihn direkt
// per XHR. Den von Cloudflare erwarteten User-Agent "PokerTH/2.0 (Qt Network)"
// setzt die WebNetworkAccessManagerFactory global (siehe pokerth.cpp), XHR aus
// QML darf den Header selbst nicht setzen.
//
// Unterschied zum Web-Client: der Beitrag wird NICHT im externen Browser
// geöffnet. Der Feed liefert den kompletten Beitrags-HTML gleich mit; der wird
// hier für Qt-RichText aufbereitet (Farben ans Theme, Prozent-Schriftgrößen,
// absolute URLs) und in der App angezeigt (ForumPostPage).
//
// Gelesen-Status bleibt lokal (eigene Settings-Kategorie, kein Server):
//   readBase  Wasserzeichen von „alles als gelesen markieren" (ms)
//   readIds   einzeln gelesene Beiträge, begrenzt auf maxReadIds
//
// KEIN Verweis auf andere Config-Singletons: innerhalb des Moduls Config ist
// `import Config` in Qt 6 eine Zirkelabhängigkeit (siehe Theme.qml). Was von
// außen kommt, wird übergeben: `enabled` per Binding aus pokerth.qml, Theme-
// abhängige Werte als opts-Objekt an postBlocks().
QtObject {
    id: forumNews

    readonly property string feedUrl:  "https://www.pokerth.net/app.php/feed"
    readonly property string forumUrl: "https://www.pokerth.net/"
    readonly property string siteBase: "https://www.pokerth.net"

    // Feed-Cache: erneutes Öffnen der Seite lädt nicht jedes Mal neu.
    readonly property int cacheTtlMs: 5 * 60 * 1000
    // Hintergrund-Aktualisierung des Zählers (der Feed ist ~40 kB).
    readonly property int refreshIntervalMs: 15 * 60 * 1000
    readonly property int maxPosts: 40
    readonly property int maxReadIds: 120

    // Von pokerth.qml an Parameters.showForumNews gebunden – aus = kein Abruf.
    property bool enabled: true

    // Beiträge, nach Thema entdoppelt (siehe _dedup).
    property var posts: []
    property bool loading: false
    property string errorText: ""
    property real lastFetchMs: 0

    // Hochgezählt bei jeder Änderung des Gelesen-Status: Bindings, die
    // isUnread() aufrufen, lesen diese Property und werden dadurch neu
    // ausgewertet (Funktionsaufrufe allein erzeugen keine Abhängigkeit).
    property int readRevision: 0

    readonly property int unreadCount: {
        var _rev = readRevision
        var n = 0
        for (var i = 0; i < posts.length; ++i)
            if (isUnread(posts[i]))
                ++n
        return n
    }

    property Settings _store: Settings {
        category: "ForumNews"
        property real readBase: 0
        property string readIds: ""
    }

    // Gelesene Beitrags-IDs als Liste (Reihenfolge = Alter, für die Begrenzung)
    // und als Map (schnelles Nachschlagen).
    property var _readList: []
    property var _readMap: ({})

    property Timer _refreshTimer: Timer {
        interval: forumNews.refreshIntervalMs
        repeat: true
        triggeredOnStart: true
        running: forumNews.enabled
        // Qt.callLater: der erste Auslöser fällt mit der Erzeugung des
        // Singletons zusammen – zu diesem Zeitpunkt kann die Bindung von
        // `enabled` an die Einstellung (pokerth.qml) noch nicht stehen, sonst
        // liefe bei abgeschalteter Funktion einmalig doch ein Abruf.
        onTriggered: Qt.callLater(function() { forumNews.refresh(true) })
    }

    Component.onCompleted: {
        var list = []
        try {
            var parsed = JSON.parse(_store.readIds || "[]")
            if (Array.isArray(parsed))
                list = parsed
        } catch (e) {
            list = []
        }
        _readList = list
        var map = ({})
        for (var i = 0; i < list.length; ++i)
            map[list[i]] = true
        _readMap = map
    }

    // ── Gelesen-Status ───────────────────────────────────────────────────────
    function isUnread(post) {
        if (!post)
            return false
        if (post.ts <= _store.readBase)
            return false
        return !_readMap[post.id]
    }

    function markRead(post) {
        if (!post || _readMap[post.id])
            return
        _readMap[post.id] = true
        var list = _readList.slice()
        list.push(post.id)
        if (list.length > maxReadIds) {
            var dropped = list.splice(0, list.length - maxReadIds)
            for (var i = 0; i < dropped.length; ++i)
                delete _readMap[dropped[i]]
        }
        _readList = list
        _store.readIds = JSON.stringify(list)
        ++readRevision
    }

    // „Alles als gelesen markieren": Wasserzeichen auf den jüngsten Beitrag
    // (mindestens jetzt) setzen, Einzel-IDs darunter werden überflüssig.
    function markAllRead() {
        var mx = Date.now()
        for (var i = 0; i < posts.length; ++i)
            if (posts[i].ts > mx)
                mx = posts[i].ts
        _store.readBase = mx
        _store.readIds = ""
        _readList = []
        _readMap = ({})
        ++readRevision
    }

    // ── Abruf ────────────────────────────────────────────────────────────────
    // force = TTL übergehen (Timer, manuelles Neuladen). Bei Fehlern bleiben
    // die zuletzt geholten Beiträge stehen.
    function refresh(force) {
        if (!enabled || loading)
            return
        if (!force && posts.length > 0 && (Date.now() - lastFetchMs) < cacheTtlMs)
            return

        loading = true
        var xhr = new XMLHttpRequest()
        xhr.open("GET", feedUrl)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            forumNews.loading = false
            if (xhr.status !== 200 || xhr.responseText.length === 0) {
                console.warn("ForumNews: Abruf fehlgeschlagen, Status", xhr.status)
                if (forumNews.posts.length === 0)
                    forumNews.errorText = qsTr("The forum feed could not be loaded.")
                return
            }
            var parsed = []
            try {
                parsed = forumNews._dedup(forumNews._parseFeed(xhr.responseText))
            } catch (e) {
                console.warn("ForumNews: Feed nicht lesbar:", e)
            }
            if (parsed.length === 0) {
                if (forumNews.posts.length === 0)
                    forumNews.errorText = qsTr("The forum feed could not be loaded.")
                return
            }
            forumNews.posts = parsed
            forumNews.lastFetchMs = Date.now()
            forumNews.errorText = ""
        }
        xhr.send()
    }

    // ── Atom-Feed lesen ──────────────────────────────────────────────────────
    // Bewusst per regulärem Ausdruck wie im Web-Relay (proxy.js
    // forumParseAtom): der Feed ist maschinell erzeugt und immer gleich
    // aufgebaut, ein XML-DOM wäre dafür deutlich teurer.
    function _parseFeed(xml) {
        var out = []
        var chunks = String(xml).split("<entry>")
        for (var i = 1; i < chunks.length && out.length < maxPosts; ++i) {
            var c = chunks[i]
            var rawTitle = _decode(_group(c, /<title[^>]*><!\[CDATA\[([\s\S]*?)\]\]><\/title>/))
            var link = _group(c, /<link href="([^"]+)"\s*\/?>/)
            if (rawTitle === "" || link === "")
                continue
            // phpBB-Titel lauten "Forum • Thema"; das Forum steht zusätzlich im
            // <category term> – das wird bevorzugt, sonst der Titel-Präfix.
            var forum = _decode(_group(c, /<category term="([^"]*)"/))
            var title = rawTitle
            var bi = rawTitle.indexOf(" • ")
            if (bi > 0) {
                if (forum === "")
                    forum = rawTitle.slice(0, bi)
                title = rawTitle.slice(bi + 3)
            }
            var date = _group(c, /<published>([^<]+)<\/published>/)
            if (date === "")
                date = _group(c, /<updated>([^<]+)<\/updated>/)
            out.push({
                id: _decode(link),
                link: _decode(link),
                forum: forum,
                title: title,
                author: _decode(_group(c, /<author><name><!\[CDATA\[([\s\S]*?)\]\]>/)),
                date: date,
                ts: Date.parse(date) || 0,
                html: _group(c, /<content[^>]*><!\[CDATA\[([\s\S]*?)\]\]><\/content>/)
            })
        }
        return out
    }

    // Nur der jüngste Beitrag je Thema. Der Feed besteht zum größten Teil aus
    // den automatischen BBC-/WEC-Ergebnismeldungen, ohne das hier wäre die
    // Liste eine einzige Wiederholung (gleiche Regel wie im Web-Client).
    function _dedup(list) {
        var seen = ({})
        var out = []
        for (var i = 0; i < list.length; ++i) {
            var p = list[i]
            var key = p.forum + "|" + p.title.replace(/^Re:\s*/i, "").trim().toLowerCase()
            if (seen[key])
                continue
            seen[key] = true
            out.push(p)
        }
        return out
    }

    function _group(text, re) {
        var m = re.exec(text)
        return m ? m[1] : ""
    }

    function _decode(s) {
        return String(s || "")
            .replace(/&lt;/g, "<").replace(/&gt;/g, ">")
            .replace(/&quot;/g, "\"").replace(/&apos;/g, "'")
            .replace(/&nbsp;/g, " ")
            .replace(/&#x([0-9a-fA-F]+);/g, function(all, h) {
                return String.fromCharCode(parseInt(h, 16))
            })
            .replace(/&#(\d+);/g, function(all, d) {
                return String.fromCharCode(parseInt(d, 10))
            })
            .replace(/&amp;/g, "&")   // zuletzt, sonst würden &amp;lt; & Co. doppelt aufgelöst
    }

    // ── Farbcode der Forums-Plakette ─────────────────────────────────────────
    // Die großen Foren haben einen festen Farbton (BBC bernstein, WEC petrol,
    // Bugs rot, General blau …), alles andere bekommt über einen Hash einen der
    // acht Töne – so bleibt die Farbe eines Forums stabil, ohne gepflegte
    // Liste. Werte 1:1 aus pokerth.css (.fn-c0 … .fn-c7).
    readonly property var forumPalette: [
        "#c98f1f", "#2a9d8f", "#d05050", "#4d8fd0",
        "#9d6fd0", "#5da45d", "#c86a9a", "#8a97a8"
    ]
    readonly property var _forumIndex: ({
        "bbc": 0, "wec": 1, "bugs": 2, "general": 3, "feature requests": 4,
        "monthly cup": 5, "newbie": 6, "rules": 7
    })

    function forumColor(name) {
        var k = String(name || "").trim().toLowerCase()
        if (k === "")
            return forumPalette[7]
        if (_forumIndex[k] !== undefined)
            return forumPalette[_forumIndex[k]]
        var h = 0
        for (var i = 0; i < k.length; ++i)
            h = (h * 31 + k.charCodeAt(i)) >>> 0
        return forumPalette[h % 8]
    }

    // Datum/Uhrzeit eines Beitrags in der Landessprache (Kurzformat).
    function formatDate(ts) {
        if (!ts)
            return ""
        return Qt.formatDateTime(new Date(ts), Locale.ShortFormat)
    }

    // ── Beitrag für die Anzeige aufbereiten ──────────────────────────────────
    // Ergebnis ist eine Liste von Blöcken, die ForumPostPage untereinander
    // zeichnet:
    //   { type: "html",  value }  → RichText-Abschnitt
    //   { type: "image", value }  → eigenständiges Bild
    // Bilder werden herausgezogen, weil Qt-RichText kein max-width kennt: ein
    // Handy-Screenshot als Anhang (z. B. 1080×2400) würde sonst über den Rand
    // hinauslaufen. Als Image-Element lässt es sich sauber auf die Spaltenbreite
    // begrenzen. Smileys bleiben im Fließtext (klein und mitten im Satz).
    // opts: { dark: bool, basePx: real }
    function postBlocks(post, opts) {
        var dark = !opts || opts.dark === undefined ? true : opts.dark
        var basePx = (opts && opts.basePx) || 14
        var s = _stripFooter(post && post.html ? post.html : "")
        var blocks = []
        var re = /<img\b[^>]*>/gi
        var last = 0
        var m
        while ((m = re.exec(s)) !== null) {
            var src = _attr(m[0], "src")
            if (_isInlineImage(src))
                continue
            _pushHtml(blocks, s.slice(last, m.index), dark, basePx)
            blocks.push({ type: "image", value: _absUrl(_decode(src)) })
            last = m.index + m[0].length
        }
        _pushHtml(blocks, s.slice(last), dark, basePx)
        return blocks
    }

    // Reiner Text eines Beitrags (Vorschauzeile, Suchen).
    function plainText(html, limit) {
        var s = _stripFooter(html || "")
            .replace(/<(?:br|\/p|\/div|\/li|hr)[^>]*>/gi, " ")
            .replace(/<[^>]+>/g, "")
        s = _decode(s).replace(/\s+/g, " ").trim()
        if (limit && s.length > limit) {
            s = s.slice(0, limit)
            var sp = s.lastIndexOf(" ")
            if (sp > limit * 0.6)
                s = s.slice(0, sp)
            s += "…"
        }
        return s
    }

    // phpBB hängt an jeden Feed-Beitrag "Statistics: Posted by … — <Datum>" an;
    // Autor und Datum zeigt die Seite selbst, der Absatz fliegt raus.
    function _stripFooter(html) {
        var s = String(html || "")
        var m = /<p[^>]*>\s*Statistics: Posted by/i.exec(s)
        if (m)
            s = s.slice(0, m.index)
        else {
            var i = s.indexOf("Statistics: Posted by")
            if (i >= 0)
                s = s.slice(0, i)
        }
        return s.replace(/(?:\s|<hr\s*\/?>|<br\s*\/?>)+$/i, "")
    }

    function _pushHtml(blocks, html, dark, basePx) {
        if (html === "")
            return
        // Reine Tag-Reste ohne Inhalt (z. B. das <div>, in dem nur ein
        // herausgelöstes Bild stand) nicht als leeren Block zeichnen.
        if (plainText(html) === "" && !/<hr[\s/>]/i.test(html))
            return
        blocks.push({ type: "html", value: _fixHtml(html, dark, basePx) })
    }

    // Smileys und andere Miniaturbilder bleiben inline im RichText.
    function _isInlineImage(src) {
        return /\/images\/smilies\//i.test(src || "")
    }

    function _attr(tag, name) {
        var m = new RegExp(name + "=\"([^\"]*)\"", "i").exec(tag)
        return m ? m[1] : ""
    }

    function _absUrl(u) {
        var s = String(u || "").trim()
        if (s === "")
            return ""
        if (/^https?:\/\//i.test(s))
            return s
        if (s.indexOf("//") === 0)
            return "https:" + s
        if (s.indexOf("./") === 0)
            return siteBase + "/" + s.slice(2)
        if (s.charAt(0) === "/")
            return siteBase + s
        return siteBase + "/" + s
    }

    // phpBB-HTML → Qt-RichText (unterstützt nur eine HTML-4-Teilmenge):
    //   • relative URLs absolut machen (Feed nutzt "/images/…" und "./…")
    //   • font-family raus (App-Schrift beibehalten), line-height kennt Qt nicht
    //   • font-size in Prozent → px (Qt versteht nur pt/px)
    //   • Farben so anpassen, dass sie auf dem Hintergrund lesbar bleiben
    function _fixHtml(html, dark, basePx) {
        var s = String(html)
        s = s.replace(/(href|src)="([^"]*)"/gi, function(all, attr, val) {
            return attr + "=\"" + _absUrl(val) + "\""
        })
        s = s.replace(/font-family\s*:[^;"'>]*;?/gi, "")
        s = s.replace(/line-height\s*:[^;"'>]*;?/gi, "")
        s = s.replace(/font-size\s*:\s*(\d+(?:\.\d+)?)%/gi, function(all, pct) {
            var px = Math.round(basePx * parseFloat(pct) / 100)
            return "font-size:" + Math.max(11, Math.min(30, px)) + "px"
        })
        // Nur "color:", nicht "background-color:" – daher das Trennzeichen davor.
        s = s.replace(/(^|[;"'\s])color\s*:\s*([^;"'>]+)/gi, function(all, pre, val) {
            return pre + "color:" + _readableColor(val, dark)
        })
        return s
    }

    // Im Forum wird bunt geschrieben – "color:black" auf dunklem Grund (oder
    // hellgelb auf hellem) wäre unlesbar. Zu dunkle bzw. zu helle Farben werden
    // deshalb Richtung Hintergrund-Gegenpol gemischt; der Farbton bleibt.
    function _readableColor(value, dark) {
        var rgb = _toRgb(value)
        if (!rgb)
            return value
        var lum = 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2]
        var t = 0
        var target = 1
        if (dark && lum < 0.55) {
            t = (0.55 - lum) / (1 - lum)          // Richtung Weiß aufhellen
            target = 1
        } else if (!dark && lum > 0.62) {
            t = 1 - 0.45 / Math.max(lum, 0.0001)  // Richtung Schwarz abdunkeln
            target = 0
        } else {
            return value
        }
        t = Math.max(0, Math.min(1, t))
        var out = "#"
        for (var i = 0; i < 3; ++i) {
            var c = Math.round(255 * (rgb[i] * (1 - t) + target * t))
            out += (c < 16 ? "0" : "") + c.toString(16)
        }
        return out
    }

    // Farbnamen, die im Forum tatsächlich vorkommen (phpBB-Farbwähler und die
    // alten BBCode-Namen wie "brightred", das kein CSS-Name ist).
    readonly property var _namedColors: ({
        "black": "#000000", "white": "#ffffff", "red": "#ff0000",
        "brightred": "#ff0000", "darkred": "#8b0000", "maroon": "#800000",
        "green": "#008000", "darkgreen": "#006400", "limegreen": "#32cd32",
        "lime": "#00ff00", "olive": "#808000", "blue": "#0000ff",
        "darkblue": "#00008b", "navy": "#000080", "royalblue": "#4169e1",
        "skyblue": "#87ceeb", "cyan": "#00ffff", "aqua": "#00ffff",
        "teal": "#008080", "magenta": "#ff00ff", "fuchsia": "#ff00ff",
        "purple": "#800080", "violet": "#ee82ee", "indigo": "#4b0082",
        "orange": "#ffa500", "darkorange": "#ff8c00", "yellow": "#ffff00",
        "gold": "#ffd700", "goldenrod": "#daa520", "brown": "#a52a2a",
        "sienna": "#a0522d", "silver": "#c0c0c0", "gray": "#808080",
        "grey": "#808080", "darkgray": "#a9a9a9", "darkgrey": "#a9a9a9",
        "pink": "#ffc0cb", "beige": "#f5f5dc", "tan": "#d2b48c"
    })

    // "#abc" | "#aabbcc" | "rgb(…)" | Farbname → [r, g, b] in 0…1, sonst null.
    function _toRgb(value) {
        var s = String(value || "").trim().toLowerCase()
        if (_namedColors[s] !== undefined)
            s = _namedColors[s]
        var m = /^#([0-9a-f])([0-9a-f])([0-9a-f])$/.exec(s)
        if (m)
            return [parseInt(m[1] + m[1], 16) / 255,
                    parseInt(m[2] + m[2], 16) / 255,
                    parseInt(m[3] + m[3], 16) / 255]
        m = /^#([0-9a-f]{2})([0-9a-f]{2})([0-9a-f]{2})$/.exec(s)
        if (m)
            return [parseInt(m[1], 16) / 255, parseInt(m[2], 16) / 255,
                    parseInt(m[3], 16) / 255]
        m = /^rgba?\(\s*(\d+)[\s,]+(\d+)[\s,]+(\d+)/.exec(s)
        if (m)
            return [parseInt(m[1], 10) / 255, parseInt(m[2], 10) / 255,
                    parseInt(m[3], 10) / 255]
        return null
    }
}
