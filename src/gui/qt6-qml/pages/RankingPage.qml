import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// Ranking view – shows the leaderboard of pokerth.net natively.
// Instead of embedding the VueJS website in a WebView, the JSON endpoint used
// by the website is queried directly and the result is displayed as a native
// QML table (no QtWebEngine/QtWebView needed, suitable for mobile).
//   POST https://www.pokerth.net/pthranking/ranking/leaderboard/<season>
//   Body: { page, pageSize, sort:{prop,order}, filters:{value,props}|null }
Rectangle {
    id: rankingPage
    objectName: "rankingPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    readonly property bool compact: Config.Responsive.compact
    readonly property string baseUrl: "https://www.pokerth.net"

    // Initial focus into the search field – NOT on mobile devices, that would pull
    // up the on-screen keyboard unasked.
    StackView.onActivated: {
        if (!Config.Responsive.isMobile)
            Qt.callLater(searchField.forceActiveFocus)
    }

    // ── Zustand ───────────────────────────────────────────────────────────────
    property var rows: []
    property var seasons: ["current"]
    property string season: "current"
    property string searchQuery: ""
    property int currentPage: 1
    property int pageSize: 50
    property int total: 0
    property bool loading: false
    property string errorText: ""

    // Sorting – server side (body sort:{prop,order}). Default as before:
    // by rank position. A click on a column header switches the field/direction.
    property string sortProp: "rank_pos"
    property string sortOrder: "descending"     // "ascending" | "descending"

    // Set when restoring via the globe toggle → restore the filter state
    // (season, search, page) instead of loading the defaults.
    property var restoreState: null
    property bool restoring: false

    readonly property int pageCount: Math.max(1, Math.ceil(total / pageSize))

    // Save the current filter state for restoring it later.
    function captureState() {
        return { season: season, searchQuery: searchQuery, currentPage: currentPage,
                 sortProp: sortProp, sortOrder: sortOrder }
    }

    // Click on a column header: the same field → reverse the direction, otherwise a
    // new field (numbers descending, name ascending as a sensible default).
    // The sorting is server side → reload and go back to page 1.
    function requestSort(prop) {
        if (sortProp === prop)
            sortOrder = (sortOrder === "ascending" ? "descending" : "ascending")
        else {
            sortProp = prop
            sortOrder = (prop === "username") ? "ascending" : "descending"
        }
        currentPage = 1
        loadData()
    }

    function loadData() {
        loading = true
        errorText = ""

        var payload = {
            page: currentPage,
            pageSize: pageSize,
            sort: { prop: sortProp, order: sortOrder },
            filters: searchQuery !== ""
                     ? { value: searchQuery, props: "username" } : null
        }

        var xhr = new XMLHttpRequest()
        xhr.open("POST", baseUrl + "/pthranking/ranking/leaderboard/" + season)
        xhr.setRequestHeader("Content-Type", "application/json")
        xhr.setRequestHeader("X-Requested-With", "XMLHttpRequest")
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            rankingPage.loading = false
            if (xhr.status !== 200) {
                rankingPage.errorText =
                    qsTr("Could not load ranking (HTTP %1).").arg(xhr.status || 0)
                rankingPage.rows = []
                return
            }
            try {
                var res = JSON.parse(xhr.responseText)
                rankingPage.rows = res.data || []
                rankingPage.total = res.total || 0
                var s = res.seasons || []
                if (s.indexOf("current") < 0)
                    s.unshift("current")
                rankingPage.seasons = s
            } catch (e) {
                rankingPage.errorText = qsTr("Could not parse server response.")
                rankingPage.rows = []
            }
        }
        xhr.send(JSON.stringify(payload))
    }

    Component.onCompleted: {
        if (restoreState) {
            restoring = true
            season = restoreState.season || "current"
            searchQuery = restoreState.searchQuery || ""
            currentPage = restoreState.currentPage || 1
            sortProp = restoreState.sortProp || "rank_pos"
            sortOrder = restoreState.sortOrder || "descending"
            searchField.text = searchQuery     // triggers onTextChanged (the timer suppresses it)
            restoring = false
        }
        loadData()
    }

    // Debounce the input in the search field so that not every keystroke sends
    // a request.
    Timer {
        id: searchTimer
        interval: 400
        onTriggered: {
            rankingPage.currentPage = 1
            rankingPage.loadData()
        }
    }

    // ── Aufbau ────────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        AppLabel {
            text: qsTr("Ranking")
            color: Config.StaticData.palette.secondary.col200
            font.pointSize: 14
            font.bold: true
        }

        // Season selection + player filter – arranged as at the top of the website:
        // season on the left, the player search fills the rest of the row (or stacked
        // on narrow/mobile screens).
        GridLayout {
            Layout.fillWidth: true
            columns: rankingPage.compact ? 1 : 2
            columnSpacing: 12
            rowSpacing: 8

            RowLayout {
                spacing: 8
                AppLabel {
                    text: qsTr("Season:")
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeBody
                }
                ComboBox {
                    id: seasonCombo
                    Layout.preferredWidth: 160
                    model: rankingPage.seasons
                    // Anzeigename: "current" → "Current season"
                    displayText: currentText === "current"
                                 ? qsTr("Current season") : currentText
                    onActivated: {
                        if (rankingPage.season === currentText)
                            return
                        rankingPage.season = currentText
                        rankingPage.currentPage = 1
                        rankingPage.loadData()
                    }
                    // Keep the selection in sync when the model is reloaded.
                    Component.onCompleted: currentIndex = Math.max(0, rankingPage.seasons.indexOf(rankingPage.season))
                    Connections {
                        target: rankingPage
                        function onSeasonsChanged() {
                            seasonCombo.currentIndex = Math.max(0, rankingPage.seasons.indexOf(rankingPage.season))
                        }
                    }
                }
                Item { Layout.fillWidth: true }
            }

            // Player filter – server side (filters.props="username"), debounced;
            // 1:1 the behaviour of the filter on pokerth.net. A plain control →
            // the colours come from the global universal theme (dark/light).
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Username")
                // Enter searches immediately instead of waiting for the 400 ms
                // of the debounce – as in every search field.
                onAccepted: {
                    searchTimer.stop()
                    rankingPage.currentPage = 1
                    rankingPage.loadData()
                }
                onTextChanged: {
                    rankingPage.searchQuery = text.trim()
                    // Do not trigger the timer/page reset when restoring.
                    if (!rankingPage.restoring)
                        searchTimer.restart()
                }
            }
        }

        // Header row of the table
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: Config.StaticData.palette.secondary.col600
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                RankingHeaderCell {
                    label: qsTr("#")
                    Layout.preferredWidth: rankingPage.compact ? 32 : 40
                    sortKey: "rank_pos"
                    activeKey: rankingPage.sortProp
                    sortOrder: rankingPage.sortOrder
                    onSortRequested: function(key) { rankingPage.requestSort(key) }
                }
                RankingHeaderCell {
                    label: qsTr("Player")
                    Layout.fillWidth: true
                    sortKey: "username"
                    activeKey: rankingPage.sortProp
                    sortOrder: rankingPage.sortOrder
                    onSortRequested: function(key) { rankingPage.requestSort(key) }
                }
                RankingHeaderCell {
                    label: qsTr("Games")
                    visible: !rankingPage.compact
                    Layout.preferredWidth: 70
                    horizontalAlignment: Text.AlignRight
                    sortKey: "season_games"
                    activeKey: rankingPage.sortProp
                    sortOrder: rankingPage.sortOrder
                    onSortRequested: function(key) { rankingPage.requestSort(key) }
                }
                RankingHeaderCell {
                    label: qsTr("Avg")
                    visible: !rankingPage.compact
                    Layout.preferredWidth: 60
                    horizontalAlignment: Text.AlignRight
                    sortKey: "average_score"
                    activeKey: rankingPage.sortProp
                    sortOrder: rankingPage.sortOrder
                    onSortRequested: function(key) { rankingPage.requestSort(key) }
                }
                RankingHeaderCell {
                    label: qsTr("Score")
                    Layout.preferredWidth: rankingPage.compact ? 56 : 80
                    horizontalAlignment: Text.AlignRight
                    sortKey: "final_score"
                    activeKey: rankingPage.sortProp
                    sortOrder: rankingPage.sortOrder
                    onSortRequested: function(key) { rankingPage.requestSort(key) }
                }
            }
        }

        // Tabelle
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Config.StaticData.palette.secondary.col600
            border.color: Config.StaticData.palette.secondary.col500
            border.width: 1
            radius: 4

            ListView {
                id: rankList
                anchors.fill: parent
                anchors.margins: 1
                clip: true
                model: rankingPage.rows
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: rankList.contentHeight > rankList.height + 4
                            ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                }

                delegate: Item {
                    id: rankDelegate
                    required property int index
                    required property var modelData
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
                            text: rankDelegate.modelData.rank_pos
                            Layout.preferredWidth: rankingPage.compact ? 32 : 40
                            // Top-3 hervorheben.
                            color: rankDelegate.modelData.rank_pos <= 3
                                   ? Config.Theme.colorAccent
                                   : Config.StaticData.palette.secondary.col100
                            font.pixelSize: Config.Theme.fontSizeBody
                            font.bold: rankDelegate.modelData.rank_pos <= 3
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            // Country flag (if present) – the code comes from
                            // gender_country.country or country_iso. Uses the
                            // flags bundled in the client (resources/cflags),
                            // same scheme as PlayerListItem/LobbyPage.
                            Image {
                                readonly property string code: String(
                                    (rankDelegate.modelData.gender_country
                                     && rankDelegate.modelData.gender_country.country)
                                    || rankDelegate.modelData.country_iso || "").toLowerCase()
                                visible: code !== ""
                                source: code !== ""
                                        ? "qrc:/resources/cflags/" + code + ".svg" : ""
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 14
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                            }

                            AppLabel {
                                id: nickLabel
                                text: rankDelegate.modelData.username
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                // Clickable → player page (by player_id, otherwise username).
                                color: nickHover.hovered ? Config.Theme.colorAccent
                                                         : Config.StaticData.palette.secondary.col100
                                font.pixelSize: Config.Theme.fontSizeBody
                                font.underline: nickHover.hovered

                                HoverHandler { id: nickHover; cursorShape: Qt.PointingHandCursor }
                                TapHandler {
                                    onTapped: rankingPage.StackView.view.push("PokerthPlayerPage.qml", {
                                        playerId: rankDelegate.modelData.player_id || 0,
                                        username: rankDelegate.modelData.username || ""
                                    })
                                }
                            }
                        }
                        AppLabel {
                            text: rankDelegate.modelData.season_games
                            visible: !rankingPage.compact
                            Layout.preferredWidth: 70
                            horizontalAlignment: Text.AlignRight
                            color: Config.StaticData.palette.secondary.col200
                            font.pixelSize: Config.Theme.fontSizeBody
                        }
                        AppLabel {
                            text: rankDelegate.modelData.average_score
                            visible: !rankingPage.compact
                            Layout.preferredWidth: 60
                            horizontalAlignment: Text.AlignRight
                            color: Config.StaticData.palette.secondary.col200
                            font.pixelSize: Config.Theme.fontSizeBody
                        }
                        AppLabel {
                            text: rankDelegate.modelData.final_score
                            Layout.preferredWidth: rankingPage.compact ? 56 : 80
                            horizontalAlignment: Text.AlignRight
                            color: Config.StaticData.palette.secondary.col100
                            font.pixelSize: Config.Theme.fontSizeBody
                            font.bold: true
                        }
                    }
                }
            }

            // Lade-Anzeige
            BusyIndicator {
                anchors.centerIn: parent
                running: rankingPage.loading
                visible: running
                implicitWidth: 48
                implicitHeight: 48
            }

            // Leer- / Fehlerhinweis
            AppLabel {
                anchors.centerIn: parent
                width: parent.width - 32
                visible: !rankingPage.loading
                         && (rankingPage.errorText !== "" || rankingPage.rows.length === 0)
                text: rankingPage.errorText !== ""
                      ? rankingPage.errorText
                      : qsTr("No entries.")
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                color: rankingPage.errorText !== ""
                       ? "#d05050" : Config.StaticData.palette.secondary.col300
                font.pixelSize: Config.Theme.fontSizeBody
            }
        }

        // Seiten-Navigation
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            CustomButton {
                text: qsTr("◀ Prev")
                Layout.preferredWidth: rankingPage.compact ? 84 : 110
                enabled: !rankingPage.loading && rankingPage.currentPage > 1
                onClicked: {
                    rankingPage.currentPage--
                    rankingPage.loadData()
                }
            }

            Item { Layout.fillWidth: true }

            AppLabel {
                // Terse on mobile ("1 / 13"), with the player count on the desktop.
                text: rankingPage.compact
                      ? qsTr("%1 / %2").arg(rankingPage.currentPage).arg(rankingPage.pageCount)
                      : qsTr("Page %1 / %2  ·  %3 players")
                          .arg(rankingPage.currentPage)
                          .arg(rankingPage.pageCount)
                          .arg(rankingPage.total)
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                color: Config.StaticData.palette.secondary.col200
                font.pixelSize: Config.Theme.fontSizeCaption
            }

            Item { Layout.fillWidth: true }

            CustomButton {
                text: qsTr("Next ▶")
                Layout.preferredWidth: rankingPage.compact ? 84 : 110
                enabled: !rankingPage.loading
                         && rankingPage.currentPage < rankingPage.pageCount
                onClicked: {
                    rankingPage.currentPage++
                    rankingPage.loadData()
                }
            }
        }

        // Rating explanation – the same "ranking calculation" that stands below the
        // ranking on pokerth.net/app.php/leaderboard. The values are deliberately
        // taken over verbatim (not translatable), only the captions are
        // translated. It applies to the PokerTH ranking alone.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: calcColumn.implicitHeight + 16
            color: Config.StaticData.palette.secondary.col600
            border.color: Config.StaticData.palette.secondary.col500
            border.width: 1
            radius: 4

            ColumnLayout {
                id: calcColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 3

                AppLabel {
                    text: qsTr("Ranking calculation:")
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                    font.bold: true
                }
                AppLabel {
                    Layout.fillWidth: true
                    text: qsTr("Placement points:")
                          + "  1. = 15 | 2. = 9 | 3. = 6 | 4. = 4 | 5. = 3 | 6. = 2 | 7. = 1"
                    wrapMode: Text.WordWrap
                    color: Config.StaticData.palette.secondary.col300
                    font.pixelSize: Config.Theme.fontSizeCaption
                }
                AppLabel {
                    Layout.fillWidth: true
                    text: qsTr("Formula:")
                          + "  25 * average * (1 - 10000 / (10000 + games^3))"
                    wrapMode: Text.WordWrap
                    color: Config.StaticData.palette.secondary.col300
                    font.pixelSize: Config.Theme.fontSizeCaption
                }
            }
        }
    }
}
