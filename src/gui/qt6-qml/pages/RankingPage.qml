import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// Ranglisten-Ansicht – zeigt das Leaderboard von pokerth.net nativ an.
// Statt die VueJS-Webseite in einer WebView einzubetten, wird direkt der von
// der Webseite genutzte JSON-Endpunkt abgefragt und das Ergebnis als native
// QML-Tabelle dargestellt (kein QtWebEngine/QtWebView nötig, mobil-tauglich).
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

    // Beim Wiederherstellen über den Globus-Toggle gesetzt → Filter-Zustand
    // (Saison, Suche, Seite) wiederherstellen statt Defaults laden.
    property var restoreState: null
    property bool restoring: false

    readonly property int pageCount: Math.max(1, Math.ceil(total / pageSize))

    // Aktuellen Filter-Zustand für das spätere Wiederherstellen sichern.
    function captureState() {
        return { season: season, searchQuery: searchQuery, currentPage: currentPage }
    }

    function loadData() {
        loading = true
        errorText = ""

        var payload = {
            page: currentPage,
            pageSize: pageSize,
            sort: { prop: "rank_pos", order: "descending" },
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
            searchField.text = searchQuery     // löst onTextChanged aus (Timer unterdrückt)
            restoring = false
        }
        loadData()
    }

    // Suchfeld-Eingabe entprellen, damit nicht bei jedem Tastendruck eine
    // Anfrage rausgeht.
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

        // Saison-Auswahl + Player-Filter – wie auf der Webseite oben angeordnet:
        // Saison links, Spielersuche füllt den Rest der Zeile (bzw. gestapelt
        // auf schmalen/mobilen Bildschirmen).
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
                    // Auswahl synchron halten, wenn das Modell neu geladen wird.
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

            // Player-Filter – serverseitig (filters.props="username"), entprellt;
            // 1:1 das Verhalten des Filters auf pokerth.net. Schlichtes Control →
            // Farben kommen vom globalen Universal-Theme (dark/light).
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: qsTr("Username")
                onTextChanged: {
                    rankingPage.searchQuery = text.trim()
                    // Beim Wiederherstellen kein Timer/Seiten-Reset auslösen.
                    if (!rankingPage.restoring)
                        searchTimer.restart()
                }
            }
        }

        // Kopfzeile der Tabelle
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

                AppLabel {
                    text: qsTr("#")
                    Layout.preferredWidth: rankingPage.compact ? 32 : 40
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                    font.bold: true
                }
                AppLabel {
                    text: qsTr("Player")
                    Layout.fillWidth: true
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                    font.bold: true
                }
                AppLabel {
                    text: qsTr("Games")
                    visible: !rankingPage.compact
                    Layout.preferredWidth: 70
                    horizontalAlignment: Text.AlignRight
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                    font.bold: true
                }
                AppLabel {
                    text: qsTr("Avg")
                    visible: !rankingPage.compact
                    Layout.preferredWidth: 60
                    horizontalAlignment: Text.AlignRight
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                    font.bold: true
                }
                AppLabel {
                    text: qsTr("Score")
                    Layout.preferredWidth: rankingPage.compact ? 56 : 80
                    horizontalAlignment: Text.AlignRight
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                    font.bold: true
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

                            // Länderflagge (falls vorhanden) – Code stammt aus
                            // gender_country.country bzw. country_iso. Nutzt die
                            // im Client gebündelten Flaggen (resources/cflags),
                            // gleiches Schema wie PlayerListItem/LobbyPage.
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
                                // Klickbar → Player-Page (per player_id, sonst username).
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
                // Auf Mobil knapp ("1 / 13"), auf Desktop mit Spielerzahl.
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
    }
}
