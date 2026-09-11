import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

// Reusable table block for the community cup rankings (BBC/WEC).
// Both run on Laravel (CSRF protected) and deliver the ranking via
//   POST <baseUrl>/results/ranking   with a page specific body.
// The initial data is already rendered into the page HTML as a Vue prop – it is
// parsed on load (without CSRF), so that something is always shown;
// filter changes then go through the POST endpoint.
//
// The concrete page draws the title + filter bar itself and embeds this
// view as the table area. It sets baseUrl, extraColumns and makeBody,
// reacts to initialData(html) (seasons/year/month + embedded rows)
// and calls load() / applyFilter().
Rectangle {
    id: view

    property string baseUrl: ""
    // Additional, page specific columns: [{ label, field, width }]
    property var extraColumns: []
    // Raw data rows (array of objects with nickname/score/games/…).
    property var rows: []
    property string searchText: ""
    // Client side filtered subset – the basis for the ListView.
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
    // Body for the POST – set by the page (function → object).
    property var makeBody: function() { return {} }

    // ── Sorting & pagination (client side) ────────────────────────────────────
    // All rows come from the server at once; sorting/paging happens here.
    property string sortKey: "score"        // Default: by score (= the ranking)
    property string sortOrder: "desc"       // "asc" | "desc"
    property int currentPage: 1
    property int pageSize: 25
    readonly property bool ascending: sortOrder.indexOf("asc") === 0

    // Filtered → sorted. Numeric fields (score/games/…) are compared
    // numerically, everything else (nickname) alphabetically.
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
    // Visible excerpt of the current page.
    readonly property var pageRows: {
        var start = (currentPage - 1) * pageSize
        return sortedRows.slice(start, start + pageSize)
    }

    // Click on a column header: the same field → reverse the direction, otherwise a
    // new field (numbers descending, names ascending as a sensible default).
    function requestSort(key) {
        if (sortKey === key)
            sortOrder = ascending ? "desc" : "asc"
        else {
            sortKey = key
            sortOrder = (key === "nickname") ? "asc" : "desc"
        }
        currentPage = 1
    }

    // New data set or a changed search → back to page 1.
    onRowsChanged: currentPage = 1
    onSearchTextChanged: currentPage = 1

    // Mobile/narrow: hide the secondary columns, show only #/nickname/score, so that
    // the table fits without horizontal scrolling.
    readonly property bool compact: Config.Responsive.compact

    signal initialData(string html)
    // Click on a player name – the page opens the player page.
    signal playerActivated(string nickname)

    color: Config.StaticData.palette.secondary.col600
    border.color: Config.StaticData.palette.secondary.col500
    border.width: 1
    radius: 4

    // ── HTML attribute helpers ─────────────────────────────────────────────
    // Reads an (HTML entity encoded) Vue prop such as :results="[…]" from the
    // page HTML and returns the decoded string ("" if it is not there).
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

    // ── Network ─────────────────────────────────────────────────────────────
    // Fetches the CSRF token from the page HTML (GET). The session cookie is
    // cached by the network manager of the QML engine and sent along
    // automatically with the POST. cb(ok, html) is called at the end.
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

    // Initial call: fetch the token + hand the embedded initial data to the page
    // (works without CSRF, so it always shows the current ranking).
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

    // Applies the current filters (POST with makeBody()). On 419 (token/
    // session expired) the token is renewed exactly once and the POST is
    // repeated – no further attempt after that (prevents endless loops
    // in case the session cookie is not carried along).
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

        // Header row of the table
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
                    onSortRequested: function(key) { view.requestSort(key) }
                }
                RankingHeaderCell {
                    label: qsTr("Games")
                    visible: !view.compact
                    Layout.preferredWidth: 70
                    horizontalAlignment: Text.AlignRight
                    sortKey: "games"
                    activeKey: view.sortKey
                    sortOrder: view.sortOrder
                    onSortRequested: function(key) { view.requestSort(key) }
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
                        onSortRequested: function(key) { view.requestSort(key) }
                    }
                }
                RankingHeaderCell {
                    label: qsTr("Score")
                    Layout.preferredWidth: view.compact ? 56 : 80
                    horizontalAlignment: Text.AlignRight
                    sortKey: "score"
                    activeKey: view.sortKey
                    sortOrder: view.sortOrder
                    onSortRequested: function(key) { view.requestSort(key) }
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
                // Continuous position across all pages (1 based).
                readonly property int rankNo: (view.currentPage - 1) * view.pageSize + index + 1
                width: ListView.view.width
                height: 34

                Rectangle {
                    anchors.fill: parent
                    color: rankDelegate.index % 2 === 0
                           ? Config.Theme.colorBox
                           : Config.StaticData.palette.secondary.col600
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    // Room for the scrollbar when it is visible.
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
                        // Clickable → slightly highlighted/underlined on hover.
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
                            // Get the value from the row by field name.
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

        // Page navigation – client side, since all rows are already loaded.
        // Only visible if there is more than one page.
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
