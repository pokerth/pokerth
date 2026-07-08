import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

// Wiederverwendbarer Tabellen-Block für die Community-Cup-Ranglisten (BBC/WEC).
// Beide laufen auf Laravel (CSRF-geschützt) und liefern die Rangliste über
//   POST <baseUrl>/results/ranking   mit einem seiten-spezifischen Body.
// Die Initialdaten sind bereits als Vue-Prop ins Seiten-HTML gerendert – die
// werden beim Laden geparst (ohne CSRF), sodass immer etwas angezeigt wird;
// Filter-Änderungen gehen danach über den POST-Endpunkt.
//
// Die konkrete Seite zeichnet Titel + Filterleiste selbst und bettet diese
// View als Tabellenbereich ein. Sie setzt baseUrl, extraColumns und makeBody,
// reagiert auf initialData(html) (Seasons/Jahr/Monat + eingebettete Zeilen)
// und ruft load() / applyFilter().
Rectangle {
    id: view

    property string baseUrl: ""
    // Zusätzliche, seiten-spezifische Spalten: [{ label, field, width }]
    property var extraColumns: []
    // Rohdaten-Zeilen (Array von Objekten mit nickname/score/games/…).
    property var rows: []
    property string searchText: ""
    // Clientseitig gefiltertes Subset – Basis für die ListView.
    readonly property var filteredRows: {
        if (searchText === "")
            return rows
        var q = searchText.toLowerCase()
        var result = []
        for (var i = 0; i < rows.length; i++) {
            if ((rows[i].nickname || "").toLowerCase().indexOf(q) !== -1)
                result.push(rows[i])
        }
        return result
    }
    property bool loading: false
    property string errorText: ""
    property string csrfToken: ""
    // Body für den POST – wird von der Seite gesetzt (Funktion → Objekt).
    property var makeBody: function() { return {} }

    // ── Sortierung & Pagination (clientseitig) ────────────────────────────────
    // Alle Zeilen kommen auf einmal vom Server; Sortieren/Blättern passiert hier.
    property string sortKey: "score"        // Default: nach Score (= Rangliste)
    property string sortOrder: "desc"       // "asc" | "desc"
    property int currentPage: 1
    property int pageSize: 25
    readonly property bool ascending: sortOrder.indexOf("asc") === 0

    // Gefiltert → sortiert. Numerische Felder (score/games/…) werden numerisch
    // verglichen, alles andere (nickname) alphabetisch.
    readonly property var sortedRows: {
        var arr = filteredRows.slice()
        var key = sortKey
        var dir = ascending ? 1 : -1
        arr.sort(function(a, b) {
            var av = a[key], bv = b[key]
            var an = parseFloat(av), bn = parseFloat(bv)
            var numeric = !isNaN(an) && !isNaN(bn)
                          && String(av).trim() !== "" && String(bv).trim() !== ""
            if (numeric)
                return an === bn ? 0 : (an < bn ? -dir : dir)
            var as = String(av === undefined || av === null ? "" : av).toLowerCase()
            var bs = String(bv === undefined || bv === null ? "" : bv).toLowerCase()
            return as === bs ? 0 : (as < bs ? -dir : dir)
        })
        return arr
    }
    readonly property int total: filteredRows.length
    readonly property int pageCount: Math.max(1, Math.ceil(total / pageSize))
    // Sichtbarer Ausschnitt der aktuellen Seite.
    readonly property var pageRows: {
        var start = (currentPage - 1) * pageSize
        return sortedRows.slice(start, start + pageSize)
    }

    // Klick auf einen Spaltenkopf: gleiches Feld → Richtung umkehren, sonst neues
    // Feld (Zahlen absteigend, Namen aufsteigend als sinnvolle Voreinstellung).
    function requestSort(key) {
        if (sortKey === key)
            sortOrder = ascending ? "desc" : "asc"
        else {
            sortKey = key
            sortOrder = (key === "nickname") ? "asc" : "desc"
        }
        currentPage = 1
    }

    // Neuer Datensatz oder geänderte Suche → zurück auf Seite 1.
    onRowsChanged: currentPage = 1
    onSearchTextChanged: currentPage = 1

    // Mobil/schmal: Nebenspalten ausblenden, nur #/Nickname/Score zeigen, damit
    // die Tabelle ohne horizontales Scrollen passt.
    readonly property bool compact: Config.Responsive.compact

    signal initialData(string html)
    // Klick auf einen Spielernamen – die Seite öffnet die Player-Page.
    signal playerActivated(string nickname)

    color: Config.StaticData.palette.secondary.col600
    border.color: Config.StaticData.palette.secondary.col500
    border.width: 1
    radius: 4

    // ── HTML-Attribut-Helfer ───────────────────────────────────────────────
    // Liest ein (HTML-entity-kodiertes) Vue-Prop wie :results="[…]" aus dem
    // Seiten-HTML und gibt den dekodierten String zurück ("" wenn nicht da).
    function attr(html, name) {
        var m = html.match(new RegExp(":" + name + "=\"([^\"]*)\""))
        if (!m)
            return ""
        return m[1].replace(/&quot;/g, "\"").replace(/&#39;/g, "'")
                   .replace(/&lt;/g, "<").replace(/&gt;/g, ">")
                   .replace(/&amp;/g, "&")
    }
    function jsonAttr(html, name) {
        var s = attr(html, name)
        if (s === "")
            return null
        try { return JSON.parse(s) } catch (e) { return null }
    }

    // ── Netzwerk ────────────────────────────────────────────────────────────
    // Holt das CSRF-Token aus dem Seiten-HTML (GET). Das Session-Cookie wird
    // vom Netzwerk-Manager der QML-Engine zwischengespeichert und beim POST
    // automatisch mitgeschickt. cb(ok, html) wird am Ende aufgerufen.
    function fetchToken(cb) {
        var xhr = new XMLHttpRequest()
        xhr.open("GET", baseUrl + "/results/ranking")
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            if (xhr.status !== 200) {
                cb(false, "")
                return
            }
            var m = xhr.responseText.match(/<meta name="csrf-token" content="([^"]+)"/)
            view.csrfToken = m ? m[1] : ""
            cb(true, xhr.responseText)
        }
        xhr.send()
    }

    // Initialer Aufruf: Token holen + eingebettete Initialdaten an die Seite
    // geben (funktioniert ohne CSRF, zeigt also immer die aktuelle Rangliste).
    function load() {
        loading = true
        errorText = ""
        fetchToken(function(ok, html) {
            view.loading = false
            if (!ok) {
                view.errorText = qsTr("Could not load ranking.")
                return
            }
            view.initialData(html)
        })
    }

    // Wendet die aktuellen Filter an (POST mit makeBody()). Bei 419 (Token/
    // Session abgelaufen) wird das Token genau einmal erneuert und der POST
    // wiederholt – kein erneuter Versuch danach (verhindert Endlosschleifen,
    // falls das Session-Cookie nicht mitgeführt wird).
    function applyFilter(isRetry) {
        loading = true
        errorText = ""
        if (csrfToken === "") {
            fetchToken(function(ok) {
                if (ok) view.applyFilter(true)
                else { view.loading = false; view.errorText = qsTr("Could not load ranking.") }
            })
            return
        }
        var xhr = new XMLHttpRequest()
        xhr.open("POST", baseUrl + "/results/ranking")
        xhr.setRequestHeader("Content-Type", "application/json")
        xhr.setRequestHeader("X-Requested-With", "XMLHttpRequest")
        xhr.setRequestHeader("X-CSRF-TOKEN", csrfToken)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            if (xhr.status === 419 && !isRetry) {
                view.csrfToken = ""
                view.fetchToken(function(ok) {
                    if (ok) view.applyFilter(true)
                    else { view.loading = false; view.errorText = qsTr("Could not load ranking.") }
                })
                return
            }
            view.loading = false
            if (xhr.status !== 200) {
                view.errorText = qsTr("Could not load ranking (HTTP %1).").arg(xhr.status || 0)
                return
            }
            try {
                var res = JSON.parse(xhr.responseText)
                view.rows = (res.success && res.stats) ? res.stats : []
            } catch (e) {
                view.errorText = qsTr("Could not parse server response.")
                view.rows = []
            }
        }
        xhr.send(JSON.stringify(makeBody()))
    }

    // ── Aufbau: Kopfzeile + Tabelle ─────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        // Kopfzeile der Tabelle
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: Config.StaticData.palette.secondary.col600

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                AppLabel {
                    text: qsTr("#")
                    Layout.preferredWidth: view.compact ? 32 : 40
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                    font.bold: true
                }
                RankingHeaderCell {
                    label: qsTr("Nickname")
                    Layout.fillWidth: true
                    sortKey: "nickname"
                    activeKey: view.sortKey
                    sortOrder: view.sortOrder
                    onSortRequested: view.requestSort(key)
                }
                RankingHeaderCell {
                    label: qsTr("Games")
                    visible: !view.compact
                    Layout.preferredWidth: 70
                    horizontalAlignment: Text.AlignRight
                    sortKey: "games"
                    activeKey: view.sortKey
                    sortOrder: view.sortOrder
                    onSortRequested: view.requestSort(key)
                }
                Repeater {
                    model: view.extraColumns
                    RankingHeaderCell {
                        required property var modelData
                        visible: !view.compact
                        label: modelData.label
                        Layout.preferredWidth: modelData.width || 70
                        horizontalAlignment: Text.AlignRight
                        sortKey: modelData.field
                        activeKey: view.sortKey
                        sortOrder: view.sortOrder
                        onSortRequested: view.requestSort(key)
                    }
                }
                RankingHeaderCell {
                    label: qsTr("Score")
                    Layout.preferredWidth: view.compact ? 56 : 80
                    horizontalAlignment: Text.AlignRight
                    sortKey: "score"
                    activeKey: view.sortKey
                    sortOrder: view.sortOrder
                    onSortRequested: view.requestSort(key)
                }
            }
        }

        // Datenzeilen
        ListView {
            id: rankList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: view.pageRows
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {
                policy: rankList.contentHeight > rankList.height + 4
                        ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }

            delegate: Item {
                id: rankDelegate
                required property int index
                required property var modelData
                // Fortlaufende Position über alle Seiten hinweg (1-basiert).
                readonly property int rankNo: (view.currentPage - 1) * view.pageSize + index + 1
                width: ListView.view.width
                height: 34

                Rectangle {
                    anchors.fill: parent
                    color: rankDelegate.index % 2 === 0
                           ? Config.StaticData.palette.secondary.col700
                           : Config.StaticData.palette.secondary.col600
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    // Platz für die Scrollbar, wenn sie sichtbar ist.
                    anchors.rightMargin: rankList.contentHeight > rankList.height + 4 ? 16 : 10
                    spacing: 8

                    AppLabel {
                        text: rankDelegate.rankNo
                        Layout.preferredWidth: view.compact ? 32 : 40
                        color: rankDelegate.rankNo <= 3
                               ? Config.Theme.colorAccent
                               : Config.StaticData.palette.secondary.col100
                        font.pixelSize: Config.Theme.fontSizeBody
                        font.bold: rankDelegate.rankNo <= 3
                    }
                    AppLabel {
                        id: nickLabel
                        text: rankDelegate.modelData.nickname || ""
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        // Klickbar → leicht hervorgehoben/unterstrichen beim Hover.
                        color: nickHover.hovered ? Config.Theme.colorAccent
                                                 : Config.StaticData.palette.secondary.col100
                        font.pixelSize: Config.Theme.fontSizeBody
                        font.underline: nickHover.hovered

                        HoverHandler { id: nickHover; cursorShape: Qt.PointingHandCursor }
                        TapHandler {
                            onTapped: view.playerActivated(rankDelegate.modelData.nickname || "")
                        }
                    }
                    AppLabel {
                        text: rankDelegate.modelData.games
                        visible: !view.compact
                        Layout.preferredWidth: 70
                        horizontalAlignment: Text.AlignRight
                        color: Config.StaticData.palette.secondary.col200
                        font.pixelSize: Config.Theme.fontSizeBody
                    }
                    Repeater {
                        model: view.extraColumns
                        AppLabel {
                            required property var modelData
                            visible: !view.compact
                            // Wert per Feldname aus der Zeile holen.
                            text: rankDelegate.modelData[modelData.field] !== undefined
                                  ? rankDelegate.modelData[modelData.field] : ""
                            Layout.preferredWidth: modelData.width || 70
                            horizontalAlignment: Text.AlignRight
                            color: Config.StaticData.palette.secondary.col200
                            font.pixelSize: Config.Theme.fontSizeBody
                        }
                    }
                    AppLabel {
                        text: rankDelegate.modelData.score
                        Layout.preferredWidth: view.compact ? 56 : 80
                        horizontalAlignment: Text.AlignRight
                        color: Config.StaticData.palette.secondary.col100
                        font.pixelSize: Config.Theme.fontSizeBody
                        font.bold: true
                    }
                }
            }
        }

        // Seiten-Navigation – clientseitig, da alle Zeilen bereits geladen sind.
        // Nur sichtbar, wenn es mehr als eine Seite gibt.
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.preferredHeight: visible ? 40 : 0
            visible: view.total > view.pageSize
            spacing: 8

            CustomButton {
                text: qsTr("◀ Prev")
                Layout.preferredWidth: view.compact ? 84 : 110
                enabled: view.currentPage > 1
                onClicked: view.currentPage--
            }

            Item { Layout.fillWidth: true }

            AppLabel {
                text: view.compact
                      ? qsTr("%1 / %2").arg(view.currentPage).arg(view.pageCount)
                      : qsTr("Page %1 / %2  ·  %3 players")
                          .arg(view.currentPage).arg(view.pageCount).arg(view.total)
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                color: Config.StaticData.palette.secondary.col200
                font.pixelSize: Config.Theme.fontSizeCaption
            }

            Item { Layout.fillWidth: true }

            CustomButton {
                text: qsTr("Next ▶")
                Layout.preferredWidth: view.compact ? 84 : 110
                enabled: view.currentPage < view.pageCount
                onClicked: view.currentPage++
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: view.loading
        visible: running
        implicitWidth: 48
        implicitHeight: 48
    }

    AppLabel {
        anchors.centerIn: parent
        width: parent.width - 32
        visible: !view.loading && (view.errorText !== "" || view.filteredRows.length === 0)
        text: view.errorText !== "" ? view.errorText
                                    : (view.rows.length === 0 ? qsTr("No entries.")
                                                              : qsTr("No matches."))
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        color: view.errorText !== "" ? "#d05050"
                                     : Config.StaticData.palette.secondary.col300
        font.pixelSize: Config.Theme.fontSizeBody
    }
}
