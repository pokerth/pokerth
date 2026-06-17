import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// PokerTH-Spielerprofil – https://www.pokerth.net/player?p=<id> bzw. ?u=<name>
// Daten nativ über  GET /pthranking/player/show?player_id=<id>|username=<name>
// (kein CSRF). Aufrufer setzt playerId (aus der Ranking-Zeile) oder username.
Rectangle {
    id: playerPage
    objectName: "pokerthPlayerPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    readonly property bool compact: Config.Responsive.compact
    readonly property string baseUrl: "https://www.pokerth.net"

    property int playerId: 0
    property string username: ""

    property var player: null
    property int pos: 0
    property var last5: []
    property var games: []
    property bool loading: false
    property string errorText: ""

    function score2(v) { return (Number(v) / 100).toFixed(2) }
    function datePart(s) { return s ? String(s).substring(0, 10) : "" }

    function load() {
        loading = true
        errorText = ""
        var q = playerId > 0 ? ("player_id=" + playerId)
                             : ("username=" + encodeURIComponent(username))
        var xhr = new XMLHttpRequest()
        xhr.open("GET", baseUrl + "/pthranking/player/show?" + q)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            playerPage.loading = false
            if (xhr.status !== 200) {
                playerPage.errorText = qsTr("Could not load player (HTTP %1).").arg(xhr.status || 0)
                return
            }
            try {
                var res = JSON.parse(xhr.responseText)
                if (!res.status) {
                    playerPage.errorText = qsTr("Player not found.")
                    return
                }
                playerPage.player = res.player
                playerPage.pos = res.pos || 0
                playerPage.last5 = res.last5 || []
                playerPage.games = res.games || []
            } catch (e) {
                playerPage.errorText = qsTr("Could not parse server response.")
            }
        }
        xhr.send()
    }

    Component.onCompleted: load()

    // ── Inhalt ──────────────────────────────────────────────────────────────
    Flickable {
        anchors.fill: parent
        anchors.margins: 16
        contentWidth: width
        contentHeight: content.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: content
            width: parent.width
            spacing: 14

            // ── Kopf: Avatar + Name + Land + Eckdaten ───────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                Rectangle {
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 72
                    radius: 6
                    color: Config.StaticData.palette.secondary.col600
                    clip: true

                    Image {
                        anchors.fill: parent
                        visible: source != ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        source: (playerPage.player && playerPage.player.avatar_hash)
                                ? playerPage.baseUrl + "/images/avatars/game/"
                                  + playerPage.player.avatar_hash + "." + playerPage.player.avatar_mime
                                : ""
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Image {
                            readonly property string code:
                                (playerPage.player && playerPage.player.country_iso
                                 ? String(playerPage.player.country_iso) : "").toLowerCase()
                            visible: code !== ""
                            source: code !== "" ? "qrc:/resources/cflags/" + code + ".svg" : ""
                            Layout.preferredWidth: 22
                            Layout.preferredHeight: 15
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                        Label {
                            text: playerPage.player ? playerPage.player.username : ""
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            color: Config.StaticData.palette.secondary.col100
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pointSize: 16
                            font.bold: true
                        }
                    }
                    Label {
                        visible: playerPage.player && playerPage.player.created
                        text: qsTr("Member since %1").arg(playerPage.datePart(playerPage.player ? playerPage.player.created : ""))
                        color: Config.StaticData.palette.secondary.col300
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: Config.Theme.fontSizeCaption
                    }
                    Label {
                        visible: playerPage.player && playerPage.player.last_login
                        text: qsTr("Last login %1").arg(playerPage.datePart(playerPage.player ? playerPage.player.last_login : ""))
                        color: Config.StaticData.palette.secondary.col300
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: Config.Theme.fontSizeCaption
                    }
                }
            }

            // ── Aktuelle-Saison-Kennzahlen ──────────────────────────────────
            Label {
                text: qsTr("Current season")
                color: Config.StaticData.palette.secondary.col200
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: Config.Theme.fontSizeBody
                font.bold: true
            }

            GridLayout {
                Layout.fillWidth: true
                columns: playerPage.compact ? 2 : 5
                columnSpacing: 8
                rowSpacing: 8

                Repeater {
                    model: {
                        var r = (playerPage.player && playerPage.player.ranking) ? playerPage.player.ranking : null
                        return [
                            { label: qsTr("Rank"),   value: playerPage.pos > 0 ? ("#" + playerPage.pos) : "–" },
                            { label: qsTr("Score"),  value: r ? playerPage.score2(r.final_score) : "–" },
                            { label: qsTr("Avg"),    value: r ? playerPage.score2(r.average_score) : "–" },
                            { label: qsTr("Games"),  value: r ? ("" + r.season_games) : "–" },
                            { label: qsTr("Points"), value: r ? ("" + r.points_sum) : "–" }
                        ]
                    }
                    Rectangle {
                        id: statCell
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        radius: 6
                        color: Config.StaticData.palette.secondary.col600
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 2
                            Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: statCell.modelData.value
                                color: Config.StaticData.palette.secondary.col100
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: Config.Theme.fontSizeTitle
                                font.bold: true
                            }
                            Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: statCell.modelData.label
                                color: Config.StaticData.palette.secondary.col300
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: Config.Theme.fontSizeCaption
                            }
                        }
                    }
                }
            }

            // ── Letzte 5 Platzierungen ──────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                visible: playerPage.last5.length > 0
                spacing: 8
                Label {
                    text: qsTr("Last 5:")
                    color: Config.StaticData.palette.secondary.col200
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: Config.Theme.fontSizeBody
                }
                Repeater {
                    model: playerPage.last5
                    Rectangle {
                        required property var modelData
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        radius: 14
                        color: modelData === 1 ? Config.Theme.colorAccent
                                               : Config.StaticData.palette.secondary.col600
                        Label {
                            anchors.centerIn: parent
                            text: modelData
                            color: modelData === 1 ? "#101010" : Config.StaticData.palette.secondary.col100
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: Config.Theme.fontSizeBody
                            font.bold: true
                        }
                    }
                }
                Item { Layout.fillWidth: true }
            }

            // ── Letzte Spiele ───────────────────────────────────────────────
            Label {
                text: qsTr("Recent games")
                visible: playerPage.games.length > 0
                color: Config.StaticData.palette.secondary.col200
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: Config.Theme.fontSizeBody
                font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                visible: playerPage.games.length > 0
                Layout.preferredHeight: gamesCol.implicitHeight + 2
                color: Config.StaticData.palette.secondary.col600
                border.color: Config.StaticData.palette.secondary.col500
                border.width: 1
                radius: 4

                Column {
                    id: gamesCol
                    anchors.fill: parent
                    anchors.margins: 1

                    Repeater {
                        model: playerPage.games
                        Item {
                            id: gameRow
                            required property int index
                            required property var modelData
                            width: gamesCol.width
                            height: 30

                            Rectangle {
                                anchors.fill: parent
                                color: gameRow.index % 2 === 0
                                       ? Config.StaticData.palette.secondary.col700
                                       : Config.StaticData.palette.secondary.col600
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8
                                Label {
                                    text: qsTr("#%1").arg(gameRow.modelData.place)
                                    Layout.preferredWidth: 40
                                    color: gameRow.modelData.place === 1
                                           ? Config.Theme.colorAccent
                                           : Config.StaticData.palette.secondary.col100
                                    font.family: Config.StaticData.loadedFont.font.family
                                    font.pixelSize: Config.Theme.fontSizeBody
                                    font.bold: gameRow.modelData.place === 1
                                }
                                Label {
                                    text: (gameRow.modelData.game && gameRow.modelData.game.name) ? gameRow.modelData.game.name : ""
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    color: Config.StaticData.palette.secondary.col100
                                    font.family: Config.StaticData.loadedFont.font.family
                                    font.pixelSize: Config.Theme.fontSizeBody
                                }
                                Label {
                                    text: playerPage.datePart(gameRow.modelData.start_time)
                                    visible: !playerPage.compact
                                    color: Config.StaticData.palette.secondary.col300
                                    font.family: Config.StaticData.loadedFont.font.family
                                    font.pixelSize: Config.Theme.fontSizeCaption
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: playerPage.loading
        visible: running
        implicitWidth: 48
        implicitHeight: 48
    }

    Label {
        anchors.centerIn: parent
        width: parent.width - 32
        visible: !playerPage.loading && playerPage.errorText !== ""
        text: playerPage.errorText
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        color: "#d05050"
        font.family: Config.StaticData.loadedFont.font.family
        font.pixelSize: Config.Theme.fontSizeBody
    }
}
