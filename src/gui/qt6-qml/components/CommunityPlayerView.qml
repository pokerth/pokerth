import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

// Spielerprofil für die Community-Cups (BBC/WEC). Die Player-Seite
//   GET <baseUrl>/player/<nickname>
// bettet Spieler- und Statistik-Blöcke als Vue-Props ins HTML ein – die werden
// hier geparst (kein CSRF nötig). `blocks` legt fest, welche Stat-Blöcke (z.B.
// season/alltime bei BBC, month/year/alltime bei WEC) in welcher Reihenfolge
// und mit welcher Überschrift angezeigt werden.
Rectangle {
    id: playerView
    objectName: "communityPlayerPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    property string baseUrl: ""
    property string nickname: ""
    // [{ label, key }] – key referenziert einen Block in stats.
    property var blocks: []

    readonly property bool compact: Config.Responsive.compact
    // Awards: responsive – auf Desktop groß genug zum Lesen, auf Mobilgeräten kompakter.
    readonly property int awardSize: compact ? 80 : 120

    property var player: null
    property var stats: null
    property var awards: []
    property bool loading: false
    property string errorText: ""
    // Avatar-URL aus PokerTH – wird nach dem Laden des Players nachgeladen.
    property string avatarUrl: ""

    function datePart(s) { return s ? String(s).substring(0, 10) : "" }

    function attr(html, name) {
        var m = html.match(new RegExp(":" + name + "=\"([^\"]*)\""))
        if (!m)
            return ""
        return m[1].replace(/&quot;/g, "\"").replace(/&#39;/g, "'")
                   .replace(/&lt;/g, "<").replace(/&gt;/g, ">").replace(/&amp;/g, "&")
    }
    function jsonAttr(html, name) {
        var s = attr(html, name)
        if (s === "")
            return null
        try { return JSON.parse(s) } catch (e) { return null }
    }

    // Avatar von PokerTH nachladen (BBC/WEC haben kein eigenes Avatar-System).
    function loadAvatar(nick) {
        var xhr = new XMLHttpRequest()
        xhr.open("GET", "https://www.pokerth.net/pthranking/player/show?username="
                        + encodeURIComponent(nick))
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            if (xhr.status !== 200)
                return
            try {
                var res = JSON.parse(xhr.responseText)
                var p = res && res.player
                if (p && p.avatar_hash)
                    playerView.avatarUrl = "https://www.pokerth.net/images/avatars/game/"
                                          + p.avatar_hash + "." + p.avatar_mime
            } catch (e) {}
        }
        xhr.send()
    }

    function load() {
        loading = true
        errorText = ""
        avatarUrl = ""
        var xhr = new XMLHttpRequest()
        xhr.open("GET", baseUrl + "/player/" + encodeURIComponent(nickname))
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            playerView.loading = false
            if (xhr.status !== 200) {
                playerView.errorText = xhr.status === 404
                    ? qsTr("Player not found.")
                    : qsTr("Could not load player (HTTP %1).").arg(xhr.status || 0)
                return
            }
            playerView.player = playerView.jsonAttr(xhr.responseText, "player")
            playerView.stats = playerView.jsonAttr(xhr.responseText, "stats")
            playerView.awards = playerView.jsonAttr(xhr.responseText, "awards") || []
            if (!playerView.player) {
                playerView.errorText = qsTr("Could not parse server response.")
                return
            }
            // Avatar asynchron von PokerTH nachladen.
            playerView.loadAvatar(playerView.player.nickname || playerView.nickname)
        }
        xhr.send()
    }

    Component.onCompleted: load()

    Flickable {
        id: contentFlick
        anchors.fill: parent
        anchors.topMargin: 16
        anchors.bottomMargin: 16
        anchors.leftMargin: 16
        // Scrollbar näher an den Fensterrand rücken, statt rechts Platz zu
        // verschwenden – der gewonnene Raum dient als Abstand zum Inhalt.
        anchors.rightMargin: 6
        contentWidth: width
        contentHeight: content.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        // Inhalt schmaler halten, solange die Scrollbar sichtbar ist, damit sie
        // den Inhalt rechts nicht überlappt.
        readonly property bool scrolling: contentHeight > height
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: content
            width: contentFlick.width - (contentFlick.scrolling ? 16 : 0)
            spacing: 14

            // ── Kopf: Avatar + Name + Eckdaten ──────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                Rectangle {
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 72
                    Layout.alignment: Qt.AlignTop
                    radius: 6
                    color: Config.StaticData.palette.secondary.col600
                    clip: true
                    visible: playerView.avatarUrl !== ""

                    Image {
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        source: playerView.avatarUrl
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    AppLabel {
                        text: playerView.player ? playerView.player.nickname : playerView.nickname
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        color: Config.StaticData.palette.secondary.col100
                        font.pointSize: 16
                        font.bold: true
                    }
                    AppLabel {
                        visible: playerView.player && playerView.player.created_at
                        text: qsTr("Member since %1").arg(playerView.datePart(playerView.player ? playerView.player.created_at : ""))
                        color: Config.StaticData.palette.secondary.col300
                        font.pixelSize: Config.Theme.fontSizeCaption
                    }
                }
            }

            // ── Awards (BBC) – eigene Zeile über die volle Breite, damit der
            // Kopf im Portrait nicht überläuft. Der dünne Scroll-Indikator sitzt
            // UNTER den Awards (überlappt sie nicht).
            ColumnLayout {
                Layout.fillWidth: true
                visible: playerView.awards.length > 0
                spacing: 4

                AppLabel {
                    text: qsTr("Awards")
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeBody
                    font.bold: true
                }

                Flickable {
                    id: awardsFlick
                    Layout.fillWidth: true
                    // +10 px reservierte Höhe für den dünnen Scroll-Indikator darunter.
                    Layout.preferredHeight: playerView.awardSize + 10
                    contentWidth: awardsRow.implicitWidth
                    contentHeight: playerView.awardSize
                    flickableDirection: Flickable.HorizontalFlick
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true

                    Row {
                        id: awardsRow
                        spacing: 8
                        height: playerView.awardSize

                        Repeater {
                            model: playerView.awards
                            Image {
                                required property var modelData
                                width: playerView.awardSize
                                height: playerView.awardSize
                                source: modelData.filename
                                        ? playerView.baseUrl + modelData.filename : ""
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                                smooth: true

                                ToolTip.visible: modelData.title && awardHover.hovered
                                ToolTip.text: modelData.title || ""
                                ToolTip.delay: 600

                                HoverHandler { id: awardHover; cursorShape: Qt.PointingHandCursor }
                                TapHandler {
                                    onTapped: {
                                        awardPopup.imageUrl = modelData.filename
                                            ? playerView.baseUrl + modelData.filename : ""
                                        awardPopup.open()
                                    }
                                }
                            }
                        }
                    }

                    // Schlanker Indikator (5 px) am unteren Rand – nur wenn scrollbar.
                    ScrollBar.horizontal: ScrollBar {
                        id: awardsScroll
                        height: 5
                        policy: awardsRow.implicitWidth > awardsFlick.width
                                ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                        contentItem: Rectangle {
                            implicitHeight: 5
                            radius: 2.5
                            color: Config.StaticData.palette.secondary.col400
                            opacity: awardsScroll.pressed ? 0.9 : 0.55
                        }
                    }
                }
            }

            // ── Tickets (BBC) ────────────────────────────────────────────────
            // s2/s3/s4_tickets sind nur im BBC-Player-Objekt vorhanden.
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                visible: playerView.player !== null &&
                         playerView.player.s2_tickets !== undefined

                AppLabel {
                    text: qsTr("Tickets")
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeBody
                    font.bold: true
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: 8
                    rowSpacing: 8

                    Repeater {
                        model: [
                            { label: qsTr("Step 2"), value: playerView.player ? ("" + (playerView.player.s2_tickets || 0)) : "–" },
                            { label: qsTr("Step 3"), value: playerView.player ? ("" + (playerView.player.s3_tickets || 0)) : "–" },
                            { label: qsTr("Step 4"), value: playerView.player ? ("" + (playerView.player.s4_tickets || 0)) : "–" }
                        ]
                        Rectangle {
                            id: ticketCell
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 56
                            radius: 6
                            color: Config.StaticData.palette.secondary.col600
                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2
                                AppLabel {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: ticketCell.modelData.value
                                    color: Config.StaticData.palette.secondary.col100
                                    font.pixelSize: Config.Theme.fontSizeTitle
                                    font.bold: true
                                }
                                AppLabel {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: ticketCell.modelData.label
                                    color: Config.StaticData.palette.secondary.col300
                                    font.pixelSize: Config.Theme.fontSizeCaption
                                }
                            }
                        }
                    }
                }
            }

            // ── Stat-Blöcke ─────────────────────────────────────────────────
            Repeater {
                model: playerView.blocks
                ColumnLayout {
                    id: blockItem
                    required property var modelData
                    readonly property var statData:
                        (playerView.stats && playerView.stats[modelData.key])
                        ? playerView.stats[modelData.key] : null
                    Layout.fillWidth: true
                    spacing: 6

                    AppLabel {
                        text: blockItem.modelData.label
                        color: Config.StaticData.palette.secondary.col200
                        font.pixelSize: Config.Theme.fontSizeBody
                        font.bold: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: playerView.compact ? 2 : 4
                        columnSpacing: 8
                        rowSpacing: 8

                        Repeater {
                            model: [
                                { label: qsTr("Rank"),   value: (blockItem.statData && blockItem.statData.pos !== "" && blockItem.statData.pos != null) ? ("#" + blockItem.statData.pos) : "–" },
                                { label: qsTr("Score"),  value: blockItem.statData ? blockItem.statData.score : "–" },
                                { label: qsTr("Games"),  value: blockItem.statData ? ("" + blockItem.statData.games) : "–" },
                                { label: qsTr("Points"), value: blockItem.statData ? ("" + blockItem.statData.points) : "–" }
                            ]
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
                                    AppLabel {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: statCell.modelData.value
                                        color: Config.StaticData.palette.secondary.col100
                                        font.pixelSize: Config.Theme.fontSizeTitle
                                        font.bold: true
                                    }
                                    AppLabel {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: statCell.modelData.label
                                        color: Config.StaticData.palette.secondary.col300
                                        font.pixelSize: Config.Theme.fontSizeCaption
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ── Award-Vollbild-Popup ──────────────────────────────────────────────
    Popup {
        id: awardPopup
        parent: Overlay.overlay
        // Explizites x/y – zuverlässige Zentrierung auf jeder Auflösung.
        readonly property int sz: parent
            ? Math.min(parent.width - 48, parent.height - 96, 480) : 360
        x: parent ? Math.round((parent.width  - width)  / 2) : 0
        y: parent ? Math.round((parent.height - height) / 2) : 0
        width:  sz
        height: sz
        modal: true
        dim: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 0
        background: null

        property string imageUrl: ""

        contentItem: Rectangle {
            radius: 10
            color: Config.StaticData.palette.secondary.col700
            clip: true

            Image {
                anchors.fill: parent
                anchors.margins: 16
                source: awardPopup.imageUrl
                fillMode: Image.PreserveAspectFit
                smooth: true
                asynchronous: true
            }

            TapHandler { onTapped: awardPopup.close() }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: playerView.loading
        visible: running
        implicitWidth: 48
        implicitHeight: 48
    }

    AppLabel {
        anchors.centerIn: parent
        width: parent.width - 32
        visible: !playerView.loading && playerView.errorText !== ""
        text: playerView.errorText
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        color: "#d05050"
        font.pixelSize: Config.Theme.fontSizeBody
    }
}
