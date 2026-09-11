pragma Singleton
import QtQuick
import QtCore

// Forum news – a port of the web client feature (there
// public/modules/ui/forumnews.mjs + proxy.js /api/forumfeed) to the
// QML client: the latest forum posts from www.pokerth.net with a counter of
// unread posts in the top bar.
//
// The data source is the phpBB Atom feed. The web client needs a
// relay for it (the feed sends no CORS header) – the QML client fetches it directly
// via XHR. The user agent "PokerTH/2.0 (Qt Network)" expected by Cloudflare is
// set globally by the WebNetworkAccessManagerFactory (see pokerth.cpp), XHR from
// QML must not set the header itself.
//
// The difference to the web client: the post is NOT opened in the external
// browser. The feed delivers the complete post HTML right away; it is
// prepared here for Qt rich text (colours matched to the theme, percentage font sizes,
// absolute URLs) and shown in the app (ForumPostPage).
//
// The read status stays local (a settings category of its own, no server):
//   readBase  the watermark of "mark everything as read" (ms)
//   readIds   individually read topics, limited to maxReadIds
//
// NO reference to other config singletons: inside the module Config,
// `import Config` is a circular dependency in Qt 6 (see Theme.qml). What comes from
// outside is passed in: `enabled` via a binding from pokerth.qml, theme
// dependent values as an opts object to postBlocks().
QtObject {
    id: forumNews

    readonly property string feedUrl:  "https://www.pokerth.net/app.php/feed"
    readonly property string forumUrl: "https://www.pokerth.net/"
    readonly property string siteBase: "https://www.pokerth.net"

    // Feed cache: opening the page again does not reload every time.
    readonly property int cacheTtlMs: 5 * 60 * 1000
    // Background refresh of the counter (the feed is ~40 kB).
    readonly property int refreshIntervalMs: 15 * 60 * 1000
    readonly property int maxPosts: 40
    readonly property int maxReadIds: 120

    // Bound by pokerth.qml to Parameters.showForumNews – off = no fetch.
    property bool enabled: true

    // Posts, deduplicated by topic (see _dedup).
    property var posts: []
    property bool loading: false
    property string errorText: ""
    property real lastFetchMs: 0

    // Incremented on every change of the read status: bindings that
    // call isUnread() read this property and are thereby re-evaluated
    // (function calls alone create no dependency).
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

    // Read topics as a list of topic keys (the order = the age, for the
    // limiting) and as a map (fast lookup).
    property var _readList: []
    property var _readMap: ({})

    property Timer _refreshTimer: Timer {
        interval: forumNews.refreshIntervalMs
        repeat: true
        triggeredOnStart: true
        running: forumNews.enabled
        // Qt.callLater: the first trigger coincides with the creation of the
        // singleton – at that point the binding of
        // `enabled` to the setting (pokerth.qml) cannot stand yet, otherwise
        // a single fetch would run after all with the function switched off.
        onTriggered: Qt.callLater(function() { forumNews.refresh(true) })
    }

    Component.onCompleted: {
        var list = []
        try {
            var parsed = JSON.parse(_store.readIds || "[]")
            if (Array.isArray(parsed)) {
                // Up to 2.1.8 the post link was stored instead of the topic key;
                // such entries can never match again - drop them.
                for (var k = 0; k < parsed.length; ++k)
                    if (String(parsed[k]).indexOf("://") < 0)
                        list.push(parsed[k])
            }
        } catch (e) {
            list = []
        }
        _readList = list
        var map = ({})
        for (var i = 0; i < list.length; ++i)
            map[list[i]] = true
        _readMap = map
    }

    // ── Read status ──────────────────────────────────────────────────────────
    // The read status is kept per topic, not per post: the list holds one
    // entry per topic (see _dedup) and which post represents it changes with
    // every reply. Keyed by post ID an already read topic would jump back to
    // "unread" as soon as the feed delivers a newer post of it.
    function isUnread(post) {
        if (!post)
            return false
        if (post.ts <= _store.readBase)
            return false
        return !_readMap[_topicKey(post)]
    }

    function markRead(post) {
        if (!post)
            return
        var key = _topicKey(post)
        if (_readMap[key])
            return
        _readMap[key] = true
        var list = _readList.slice()
        list.push(key)
        if (list.length > maxReadIds) {
            var dropped = list.splice(0, list.length - maxReadIds)
            for (var i = 0; i < dropped.length; ++i)
                delete _readMap[dropped[i]]
        }
        _readList = list
        _store.readIds = JSON.stringify(list)
        ++readRevision
    }

    // "Mark everything as read": set the watermark to the most recent post
    // (at least now), individual topic keys below it become superfluous.
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

    // ── Fetch ────────────────────────────────────────────────────────────────
    // force = bypass the TTL (timer, manual reload). On errors the posts
    // fetched last stay.
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
                console.warn("ForumNews: fetch failed, status", xhr.status)
                if (forumNews.posts.length === 0)
                    forumNews.errorText = qsTr("The forum feed could not be loaded.")
                return
            }
            var parsed = []
            try {
                parsed = forumNews._dedup(forumNews._parseFeed(xhr.responseText))
            } catch (e) {
                console.warn("ForumNews: feed not readable:", e)
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

    // ── Read the Atom feed ───────────────────────────────────────────────────
    // Deliberately by regular expression as in the web relay (proxy.js
    // forumParseAtom): the feed is generated automatically and always has the same
    // structure, an XML DOM would be considerably more expensive for that.
    function _parseFeed(xml) {
        var out = []
        var chunks = String(xml).split("<entry>")
        for (var i = 1; i < chunks.length && out.length < maxPosts; ++i) {
            var c = chunks[i]
            var rawTitle = _decode(_group(c, /<title[^>]*><!\[CDATA\[([\s\S]*?)\]\]><\/title>/))
            var link = _group(c, /<link href="([^"]+)"\s*\/?>/)
            if (rawTitle === "" || link === "")
                continue
            // phpBB titles read "forum • topic"; the forum is additionally in the
            // <category term> – that is preferred, otherwise the title prefix.
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

    // Only the most recent post per topic. The feed consists for the most part of
    // the automatic BBC/WEC result announcements; without this the
    // list would be one single repetition (the same rule as in the web client).
    function _dedup(list) {
        var seen = ({})
        var out = []
        for (var i = 0; i < list.length; ++i) {
            var p = list[i]
            var key = _topicKey(p)
            if (seen[key])
                continue
            seen[key] = true
            out.push(p)
        }
        return out
    }

    // The identity of a topic: forum + title without the reply prefix. The
    // same key for the deduplication and for the read status.
    function _topicKey(post) {
        if (!post)
            return ""
        return String(post.forum || "") + "|"
             + String(post.title || "").replace(/^Re:\s*/i, "").trim().toLowerCase()
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
            .replace(/&amp;/g, "&")   // last, otherwise &amp;lt; & co. would be resolved twice
    }

    // ── Colour code of the forum badge ───────────────────────────────────────
    // The large forums have a fixed hue (BBC amber, WEC petrol,
    // bugs red, general blue …), everything else gets one of the eight hues via a
    // hash – that way the colour of a forum stays stable without a maintained
    // list. The values 1:1 from pokerth.css (.fn-c0 … .fn-c7).
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

    // Date/time of a post in the local language (short format).
    function formatDate(ts) {
        if (!ts)
            return ""
        return Qt.formatDateTime(new Date(ts), Locale.ShortFormat)
    }

    // ── Prepare a post for display ───────────────────────────────────────────
    // The result is a list of blocks that ForumPostPage draws below each
    // other:
    //   { type: "html",  value }  → a rich text section
    //   { type: "image", value }  → a standalone image
    // Images are pulled out because Qt rich text knows no max-width: a
    // phone screenshot as an attachment (e.g. 1080×2400) would otherwise run beyond the
    // edge. As an image element it can be limited cleanly to the column width.
    // Smileys stay in the body text (small and in the middle of a sentence).
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

    // The plain text of a post (preview line, searching).
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

    // phpBB appends "Statistics: Posted by … — <date>" to every feed post;
    // the author and the date are shown by the page itself, so the paragraph flies out.
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
        // Do not draw pure tag remnants without content (e.g. the <div> that only
        // contained an extracted image) as an empty block.
        if (plainText(html) === "" && !/<hr[\s/>]/i.test(html))
            return
        blocks.push({ type: "html", value: _fixHtml(html, dark, basePx) })
    }

    // Smileys and other thumbnails stay inline in the rich text.
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

    // phpBB HTML → Qt rich text (it supports only a subset of HTML 4):
    //   • make relative URLs absolute (the feed uses "/images/…" and "./…")
    //   • remove font-family (keep the app font), Qt does not know line-height
    //   • font-size in percent → px (Qt only understands pt/px)
    //   • adjust the colours so that they stay readable on the background
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
        // Only "color:", not "background-color:" – hence the separator in front of it.
        s = s.replace(/(^|[;"'\s])color\s*:\s*([^;"'>]+)/gi, function(all, pre, val) {
            return pre + "color:" + _readableColor(val, dark)
        })
        return s
    }

    // People write in colour in the forum – "color:black" on a dark ground (or
    // light yellow on a light one) would be unreadable. Colours that are too dark or too light are
    // therefore mixed towards the opposite pole of the background; the hue stays.
    function _readableColor(value, dark) {
        var rgb = _toRgb(value)
        if (!rgb)
            return value
        var lum = 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2]
        var t = 0
        var target = 1
        if (dark && lum < 0.55) {
            t = (0.55 - lum) / (1 - lum)          // Brighten towards white
            target = 1
        } else if (!dark && lum > 0.62) {
            t = 1 - 0.45 / Math.max(lum, 0.0001)  // Darken towards black
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

    // Colour names that actually occur in the forum (the phpBB colour picker and the
    // old BBCode names such as "brightred", which is no CSS name).
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

    // "#abc" | "#aabbcc" | "rgb(…)" | a colour name → [r, g, b] in 0…1, otherwise null.
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
