import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

// Player profile for the community cups (BBC/WEC). The player page
//   GET <baseUrl>/player/<nickname>
// embeds player and statistics blocks as Vue props into the HTML – those are
// parsed here (no CSRF needed). `blocks` defines which stat blocks (e.g.
// season/alltime for BBC, month/year/alltime for WEC) are shown in which order
// and with which heading.
Rectangle {
    id: playerView
    objectName: "communityPlayerPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    // Source ("bbc" | "wec") – defines the base URL and the stat blocks.
    property string community: ""
    property string nickname: ""

    readonly property string baseUrl: community !== "" ? Config.Community.baseUrlFor(community) : ""
    // [{ label, key }] – key references a block in stats. Order +
    // heading per source (BBC: season/all-time, WEC: month/year/all-time).
    readonly property var blocks: {
        if (community === "bbc")
            return [ { label: qsTr("This season"), key: "season" },
                     { label: qsTr("All-time"),    key: "alltime" } ]
        if (community === "wec")
            return [ { label: qsTr("This month"), key: "month" },
                     { label: qsTr("This year"),  key: "year" },
                     { label: qsTr("All-time"),   key: "alltime" } ]
        return []
    }

    readonly property bool compact: Config.Responsive.compact
    // Awards: responsive – large enough to read on the desktop, more compact on mobile devices.
    readonly property int awardSize: compact ? 80 : 120

    property var player: null
    property var stats: null
    property var awards: []
    property bool loading: false
    property string errorText: ""
    // Avatar URL from PokerTH – loaded after the player has been loaded.
    property string avatarUrl: ""

    function datePart(s) { return s ? String(s).substring(0, 10) : "" }

    // Source switch: replaces this page with the player page of the selected
    // source (same nickname). As a function, because depending on the layout the
    // switch sits in two places (inline / its own row) and both need the same logic.
    function switchCommunity(community) {
        var nick = (player && player.nickname) ? player.nickname : nickname
        StackView.view.replace(Config.Community.playerPageUrl(community),
                               Config.Community.playerPageProps(community, nick))
    }

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

    // Load the avatar from PokerTH (BBC/WEC have no avatar system of their own).
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
            // Load the avatar from PokerTH asynchronously.
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
        // Move the scrollbar closer to the window edge instead of wasting room on
        // the right – the space gained serves as the distance to the content.
        anchors.rightMargin: 6
        contentWidth: width
        contentHeight: content.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        // Keep the content narrower while the scrollbar is visible, so that it
        // does not overlap the content on the right.
        readonly property bool scrolling: contentHeight > height
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: content
            width: contentFlick.width - (contentFlick.scrolling ? 16 : 0)
            spacing: 14

            // ── Header: avatar + name + key data ────────────────────────────
            // In compact/portrait mode the source switch moves into a row of its
            // own below it (right-aligned), so that the header does not get wider
            // than the display. On desktop/tablet it stays inline at the top right.
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

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
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            visible: playerView.player && playerView.player.created_at
                            text: qsTr("Member since %1").arg(playerView.datePart(playerView.player ? playerView.player.created_at : ""))
                            color: Config.StaticData.palette.secondary.col300
                            font.pixelSize: Config.Theme.fontSizeCaption
                        }
                    }

                    // Desktop/tablet: switch inline at the top right.
                    CommunitySwitch {
                        visible: !playerView.compact
                        Layout.alignment: Qt.AlignTop
                        current: playerView.community
                        onSelected: function(community) { playerView.switchCommunity(community) }
                    }
                }

                // Compact/portrait: switch in its own row, right-aligned.
                RowLayout {
                    Layout.fillWidth: true
                    visible: playerView.compact
                    Item { Layout.fillWidth: true }
                    CommunitySwitch {
                        current: playerView.community
                        onSelected: function(community) { playerView.switchCommunity(community) }
                    }
                }
            }

            // ── Awards (BBC) – a row of its own over the full width, so that the
            // header does not overflow in portrait. The thin scroll indicator sits
            // BELOW the awards (it does not overlap them).
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
                    // +10 px of reserved height for the thin scroll indicator below it.
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
                                                 && Config.Parameters.showTooltips
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

                    // Slim indicator (5 px) at the lower edge – only if it is scrollable.
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
            // s2/s3/s4_tickets only exist in the BBC player object.
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

            // ── Stat blocks ─────────────────────────────────────────────────
            Repeater {
                model: playerView.blocks
                ColumnLayout {
                    id: blockItem
                    required property var modelData
                    readonly property var statData:
                        (playerView.stats && playerView.stats[modelData.key])
                        ? playerView.stats[modelData.key] : null
                    // BBC vs. WEC: for BBC places is an array of steps (each a
                    // 10 element array place 1–10 or null), for WEC a single flat
                    // 10 element distribution. `stepped` = the BBC form (step tabs), otherwise exactly
                    // one result without tabs.
                    readonly property bool stepped: {
                        var pl = (statData && statData.places) ? statData.places : []
                        return pl.length > 0 && Array.isArray(pl[0])
                    }
                    readonly property var steps: {
                        var out = []
                        var pl = (statData && statData.places) ? statData.places : []
                        if (pl.length === 0)
                            return out
                        if (Array.isArray(pl[0])) {
                            for (var i = 0; i < pl.length; ++i)
                                if (pl[i] && pl[i].length)
                                    out.push({ label: qsTr("Step %1").arg(i + 1), data: pl[i] })
                        } else {
                            out.push({ label: "", data: pl })
                        }
                        return out
                    }
                    readonly property var stepLabels: steps.map(function(s) { return s.label })
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

                    // ── Results: Platzierungs-Verteilung je Step ────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 14
                        spacing: 8
                        visible: blockItem.steps.length > 0

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            AppLabel {
                                text: qsTr("Results:")
                                color: Config.StaticData.palette.secondary.col200
                                font.pixelSize: Config.Theme.fontSizeBody
                                font.bold: true
                            }
                            CustomTabBar {
                                id: stepBar
                                visible: blockItem.stepped
                                Layout.fillWidth: false
                                Layout.preferredWidth: Math.max(1, blockItem.steps.length) * 74
                                tabHeight: 26
                                model: blockItem.stepLabels
                            }
                            Item { Layout.fillWidth: true }
                        }

                        PlacementResult {
                            Layout.fillWidth: true
                            readonly property int step:
                                Math.min(stepBar.currentIndex, blockItem.steps.length - 1)
                            values: (blockItem.steps.length > 0 && step >= 0)
                                    ? blockItem.steps[step].data : []
                            barColors: Config.StaticData.heatColors
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
        // Explicit x/y – reliable centring at every resolution.
        readonly property int sz: parent
            ? Math.min(parent.width - 48, parent.height - 96, 480) : 360
        x: parent ? Math.round((parent.width  - width)  / 2) : 0
        y: parent ? Math.round((parent.height - height) / 2) : 0
        width:  sz
        height: sz
        modal: true
        dim: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        // Pure image display – without focus:true the popup swallows Escape without
        // closing.
        focus: true
        padding: 0
        background: null

        property string imageUrl: ""

        contentItem: Rectangle {
            radius: 10
            color: Config.Theme.colorBox
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
