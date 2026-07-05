import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// Tisch-Ranking – native Version der pokerth.net-Tischansicht, die der
// Tischnamen-Link bisher im Browser öffnete. Die Web-Kette dahinter:
//   redirect_user_profile.php?tableview=1&nickN=…&table=…
//     → 301 /gametable?u1=…&u10=…  (VueJS, <gametable-component>)
//     → POST /pthranking/gametable/show  Body: { u1:…, …, u10:… }
// Antwort: { status, msg: [{ player_id, username, rank_pos, final_score,
//            average_score, season_games, points_sum }] } – Scores ×100.
// Statt der WebView wird der JSON-Endpunkt direkt abgefragt (Muster wie
// RankingPage/PokerthPlayerPage). Unbekannte Nicks (Gäste/ohne Saisonwertung)
// lässt der Server einfach weg. Aufrufer setzt nicks (Seat-Reihenfolge,
// GameTable.tableStatsNicks()) und tableName.
Rectangle {
    id: tableStatsPage
    objectName: "gameTableStatsPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    readonly property bool compact: Config.Responsive.compact
    readonly property string baseUrl: "https://www.pokerth.net"

    property var nicks: []
    property string tableName: ""

    property var rows: []
    property bool loading: false
    property string errorText: ""

    function score2(v) { return (Number(v) / 100).toFixed(2) }

    function loadData() {
        loading = true
        errorText = ""

        // Payload exakt wie die Webseite: immer u1…u10, fehlende Plätze leer.
        var payload = {}
        for (var i = 1; i <= 10; ++i)
            payload["u" + i] = (i <= nicks.length && nicks[i - 1]) ? String(nicks[i - 1]) : ""

        var xhr = new XMLHttpRequest()
        xhr.open("POST", baseUrl + "/pthranking/gametable/show")
        xhr.setRequestHeader("Content-Type", "application/json")
        xhr.setRequestHeader("X-Requested-With", "XMLHttpRequest")
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            tableStatsPage.loading = false
            if (xhr.status !== 200) {
                tableStatsPage.errorText =
                    qsTr("Could not load table ranking (HTTP %1).").arg(xhr.status || 0)
                tableStatsPage.rows = []
                return
            }
            try {
                var res = JSON.parse(xhr.responseText)
                var list = (res.status && res.msg) ? res.msg : []
                // Der Server liefert in Anfrage-(Seat-)Reihenfolge – für die
                // Ranking-Tabelle nach Platzierung sortieren (beste zuerst).
                list.sort(function(a, b) { return a.rank_pos - b.rank_pos })
                tableStatsPage.rows = list
            } catch (e) {
                tableStatsPage.errorText = qsTr("Could not parse server response.")
                tableStatsPage.rows = []
            }
        }
        xhr.send(JSON.stringify(payload))
    }

    Component.onCompleted: loadData()

    // ── Aufbau ────────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        AppLabel {
            text: tableStatsPage.tableName !== ""
                  ? qsTr("Table ranking – %1").arg(tableStatsPage.tableName)
                  : qsTr("Table ranking")
            Layout.fillWidth: true
            elide: Text.ElideRight
            color: Config.StaticData.palette.secondary.col200
            font.pointSize: 14
            font.bold: true
        }

        AppLabel {
            text: qsTr("Current season standings of the players at this table.")
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Config.StaticData.palette.secondary.col300
            font.pixelSize: Config.Theme.fontSizeCaption
        }

        // Kopfzeile der Tabelle – Spalten wie RankingPage (#, Player, Games,
        // Avg, Score), damit beide Ranglisten-Ansichten gleich lesen.
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
                    Layout.preferredWidth: tableStatsPage.compact ? 48 : 60
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
                    visible: !tableStatsPage.compact
                    Layout.preferredWidth: 70
                    horizontalAlignment: Text.AlignRight
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                    font.bold: true
                }
                AppLabel {
                    text: qsTr("Avg")
                    visible: !tableStatsPage.compact
                    Layout.preferredWidth: 60
                    horizontalAlignment: Text.AlignRight
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                    font.bold: true
                }
                AppLabel {
                    text: qsTr("Score")
                    Layout.preferredWidth: tableStatsPage.compact ? 56 : 80
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
                id: statsList
                anchors.fill: parent
                anchors.margins: 1
                clip: true
                model: tableStatsPage.rows
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: statsList.contentHeight > statsList.height + 4
                            ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                }

                delegate: Item {
                    id: statsDelegate
                    required property int index
                    required property var modelData
                    width: ListView.view.width
                    height: 34

                    Rectangle {
                        anchors.fill: parent
                        color: statsDelegate.index % 2 === 0
                               ? Config.StaticData.palette.secondary.col700
                               : Config.StaticData.palette.secondary.col600
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        // Platz für die Scrollbar, wenn sie sichtbar ist.
                        anchors.rightMargin: statsList.contentHeight > statsList.height + 4 ? 16 : 10
                        spacing: 8

                        AppLabel {
                            text: statsDelegate.modelData.rank_pos
                            Layout.preferredWidth: tableStatsPage.compact ? 48 : 60
                            // Top-3 der Gesamtrangliste hervorheben.
                            color: statsDelegate.modelData.rank_pos <= 3
                                   ? Config.Theme.colorAccent
                                   : Config.StaticData.palette.secondary.col100
                            font.pixelSize: Config.Theme.fontSizeBody
                            font.bold: statsDelegate.modelData.rank_pos <= 3
                        }
                        AppLabel {
                            id: nickLabel
                            text: statsDelegate.modelData.username
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            // Klickbar → Player-Page (per player_id, sonst username).
                            color: nickHover.hovered ? Config.Theme.colorAccent
                                                     : Config.StaticData.palette.secondary.col100
                            font.pixelSize: Config.Theme.fontSizeBody
                            font.underline: nickHover.hovered

                            HoverHandler { id: nickHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler {
                                onTapped: tableStatsPage.StackView.view.push("PokerthPlayerPage.qml", {
                                    playerId: statsDelegate.modelData.player_id || 0,
                                    username: statsDelegate.modelData.username || ""
                                })
                            }
                        }
                        AppLabel {
                            text: statsDelegate.modelData.season_games
                            visible: !tableStatsPage.compact
                            Layout.preferredWidth: 70
                            horizontalAlignment: Text.AlignRight
                            color: Config.StaticData.palette.secondary.col200
                            font.pixelSize: Config.Theme.fontSizeBody
                        }
                        AppLabel {
                            text: tableStatsPage.score2(statsDelegate.modelData.average_score)
                            visible: !tableStatsPage.compact
                            Layout.preferredWidth: 60
                            horizontalAlignment: Text.AlignRight
                            color: Config.StaticData.palette.secondary.col200
                            font.pixelSize: Config.Theme.fontSizeBody
                        }
                        AppLabel {
                            text: tableStatsPage.score2(statsDelegate.modelData.final_score)
                            Layout.preferredWidth: tableStatsPage.compact ? 56 : 80
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
                running: tableStatsPage.loading
                visible: running
                implicitWidth: 48
                implicitHeight: 48
            }

            // Leer- / Fehlerhinweis (Gäste und Spieler ohne Saisonwertung lässt
            // der Server weg – bei reinen Gast-Tischen bleibt die Liste leer).
            AppLabel {
                anchors.centerIn: parent
                width: parent.width - 32
                visible: !tableStatsPage.loading
                         && (tableStatsPage.errorText !== "" || tableStatsPage.rows.length === 0)
                text: tableStatsPage.errorText !== ""
                      ? tableStatsPage.errorText
                      : qsTr("No ranking data for the players at this table yet.")
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                color: tableStatsPage.errorText !== ""
                       ? "#d05050" : Config.StaticData.palette.secondary.col300
                font.pixelSize: Config.Theme.fontSizeBody
            }
        }
    }
}
