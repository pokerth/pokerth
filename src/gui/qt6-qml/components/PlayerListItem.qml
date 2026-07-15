import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

ItemDelegate {
    id: playerItem

    required property int index
    required property int collapseResetCounter
    required property var listView

    readonly property int playerListRevision: Lobby ? Lobby.playerListRevision : 0
    readonly property var playerEntry: {
        var _revision = playerListRevision
        return Lobby ? Lobby.playerListEntry(index) : ({})
    }
    readonly property int targetPlayerId: playerEntry.playerId || 0
    readonly property string displayName: playerEntry.playerName || ""
    readonly property bool adminPlayer: !!playerEntry.isAdmin
    readonly property bool guestPlayer: !!playerEntry.isGuest
    readonly property string playerCountryCode: playerEntry.countryCode || ""

    // Wide-Screen → Aktionen als Icons inline rechts vom Namen, kein Collapse.
    // Portrait → bestehendes Expand/Collapse mit gestapelten Buttons.
    readonly property bool wideLayout: Config.Responsive.landscape

    width: listView.width
    height: visible
            ? ((!wideLayout && expanded && hasActions) ? expandedHeight : rowHeight)
            : 0

    // Default-Padding des ItemDelegate (Basic-Style: 12px) schiebt den Inhalt
    // bei fester Zeilenhöhe 30 nach unten → Name vertikal zentrieren.
    topPadding: 0
    bottomPadding: 0

    property bool expanded: false
    readonly property bool playerIgnored: {
        var _rev = Lobby ? Lobby.playerIgnoreListRevision : 0
        return Lobby ? Lobby.isPlayerIgnored(targetPlayerId) : false
    }
    readonly property int rowHeight: 30
    readonly property int actionButtonHeight: 24
    readonly property int actionSpacing: 3
    readonly property int actionCount: (canInvite ? 1 : 0)
                                     + (canIgnore ? 1 : 0)
                                     + (canUnignore ? 1 : 0)
                                     + (canShowPlayerStats ? 1 : 0)
                                     + (canAdminModerate ? 1 : 0)
    readonly property int expandedHeight: rowHeight + 5
                                        + (actionCount * actionButtonHeight)
                                        + (Math.max(0, actionCount - 1) * actionSpacing)

    readonly property bool isSelf: Lobby && targetPlayerId === Lobby.myPlayerId
    // gameListRevision als reaktive Abhängigkeit: erzwingt Neuauswertung
    // wenn Spieler einem Spiel beitreten oder es verlassen.
    readonly property bool canInvite: Lobby && Lobby.canInviteFromCurrentGame && !isSelf && !guestPlayer
        && (Lobby.gameListRevision >= 0 && !Lobby.isPlayerInAnyGame(targetPlayerId))
    readonly property bool canAdminModerate: Lobby && Lobby.isCurrentPlayerAdmin && !isSelf
    readonly property bool canShowPlayerStats: !guestPlayer
    readonly property bool canIgnore: !isSelf && !guestPlayer && !playerIgnored
    readonly property bool canUnignore: !isSelf && !guestPlayer && playerIgnored
    readonly property bool hasActions: canInvite || canAdminModerate || canIgnore || canUnignore || canShowPlayerStats

    readonly property color inviteColor: Config.StaticData.chartColor(0, true)
    readonly property color ignoreColor: Config.StaticData.chartColor(8, true)
    readonly property color statsColor: Config.StaticData.chartColor(9, true)
    readonly property color banColor: Config.StaticData.chartColor(5, true)

    // Rückfrage vor dem Einladen eines Spielers ins eigene Spiel.
    function confirmInvite() {
        invitePopup.openWith(
            qsTr("Invite to Game"),
            qsTr("Are you sure you want to invite \"%1\" to your game?").arg(displayName),
            qsTr("Invite"))
    }

    onCollapseResetCounterChanged: {
        expanded = false
        listView.expandedPlayerIndex = -1
    }

    Connections {
        target: listView
        function onExpandedPlayerIndexChanged() {
            if (listView.expandedPlayerIndex !== playerItem.index)
                playerItem.expanded = false
        }
    }

    // Wenn das Layout in Wide-Screen wechselt, eventuell offenen Expander
    // schließen – sonst bliebe der Item-Container unnötig hoch beim Resize.
    onWideLayoutChanged: {
        if (wideLayout) {
            expanded = false
            if (listView.expandedPlayerIndex === playerItem.index)
                listView.expandedPlayerIndex = -1
        }
    }

    Behavior on height {
        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
    }
    
    contentItem: ColumnLayout {
        spacing: 0
        
        // Header row: flag + name + right-aligned expander
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            Layout.topMargin: 0
            Layout.bottomMargin: 0
            spacing: 5

            // Flag
            Image {
                visible: playerCountryCode !== ""
                source: playerCountryCode !== ""
                    ? "qrc:/resources/cflags/" + playerCountryCode.toLowerCase() + ".svg"
                        : ""
                Layout.preferredWidth: 18
                Layout.preferredHeight: 14
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
            
            // Player name
            AppText {
                text: displayName
                font.pixelSize: listView.height > 100 ? 12 : 11
                color: Config.StaticData.palette.secondary.col200
                font.bold: false
                verticalAlignment: Text.AlignVCenter
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                elide: Text.ElideRight

                // Desktop-Hover-Tooltip: "spielt in ..." / "spielt derzeit nicht"
                // (Widget-Client zeigt dieselbe Info im Nickliste-Kontextmenü).
                // Nur bei Hover abfragen; gameListRevision als reaktive
                // Abhängigkeit, falls der Spieler währenddessen (bei)tritt.
                readonly property string inGameName: {
                    var _rev = Lobby ? Lobby.gameListRevision : 0
                    return (Lobby && nameHover.hovered)
                        ? Lobby.playerInGameName(playerItem.targetPlayerId) : ""
                }

                HoverHandler { id: nameHover }

                ToolTip.text: inGameName !== ""
                              ? qsTr("%1 is playing in \"%2\".").arg(displayName).arg(inGameName)
                              : qsTr("%1 is not playing at the moment.").arg(displayName)
                ToolTip.visible: nameHover.hovered && !Config.Responsive.isMobile
                                 && Config.Parameters.showTooltips
                ToolTip.delay: 400
            }

            // Wide-Screen: Action-Icons inline, rechtsbündig.
            Row {
                id: wideActionsRow
                visible: playerItem.wideLayout && playerItem.hasActions
                spacing: 2
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                PlayerActionIcon {
                    visible: playerItem.canInvite
                    source: "qrc:/resources/personAdd.svg"
                    baseColor: playerItem.inviteColor
                    tooltipText: qsTr("Invite to Game")
                    onTriggered: playerItem.confirmInvite()
                }
                PlayerActionIcon {
                    visible: playerItem.canIgnore
                    source: "qrc:/resources/block.svg"
                    baseColor: playerItem.ignoreColor
                    tooltipText: qsTr("Ignore player")
                    onTriggered: { if (Lobby) Lobby.ignorePlayer(playerItem.targetPlayerId) }
                }
                PlayerActionIcon {
                    visible: playerItem.canUnignore
                    source: "qrc:/resources/checkCircle.svg"
                    baseColor: playerItem.ignoreColor
                    tooltipText: qsTr("Unignore player")
                    onTriggered: { if (Lobby) Lobby.unignorePlayer(playerItem.targetPlayerId) }
                }
                PlayerActionIcon {
                    visible: playerItem.canShowPlayerStats
                    source: "qrc:/resources/barChart.svg"
                    baseColor: playerItem.statsColor
                    tooltipText: qsTr("Show player stats")
                    onTriggered: { if (Lobby) Lobby.showPlayerStats(playerItem.targetPlayerId) }
                }
                PlayerActionIcon {
                    visible: playerItem.canAdminModerate
                    source: "qrc:/resources/gavel.svg"
                    baseColor: playerItem.banColor
                    tooltipText: qsTr("Total kickban")
                    onTriggered: { if (Lobby) Lobby.adminBanPlayer(playerItem.targetPlayerId) }
                }
            }

            // Portrait: Expander-Caret (Wide-Screen blendet ihn aus).
            SvgIcon {
                id: expanderCaret
                source: "qrc:/resources/caretLeft.svg"
                rotation: expanded ? -180 : -90
                Behavior on rotation { NumberAnimation { duration: 150 } }
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                visible: !playerItem.wideLayout && hasActions

                MultiEffect {
                    source: expanderCaret
                    anchors.fill: expanderCaret
                    colorization: 1.0
                    colorizationColor: Config.Theme.colorTextMuted
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (playerItem.hasActions) {
                            const opening = !playerItem.expanded
                            playerItem.listView.expandedPlayerIndex = opening ? playerItem.index : -1
                            playerItem.expanded = opening
                        }
                    }
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
        
        // Portrait: Action-Buttons (aufgeklappt)
        ColumnLayout {
            visible: !playerItem.wideLayout && expanded && hasActions
            Layout.fillWidth: true
            Layout.topMargin: 5
            spacing: 3
            
            // Invite to game
            Button {
                text: qsTr("Invite to Game")
                visible: canInvite
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 10
                
                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }
                
                background: Rectangle {
                    color: parent.pressed ? Qt.darker(playerItem.inviteColor, 1.35)
                           : parent.hovered ? playerItem.inviteColor
                           : Qt.darker(playerItem.inviteColor, 1.18)
                    radius: 3
                    border.width: 1
                    border.color: Qt.darker(playerItem.inviteColor, 1.55)
                }
                
                contentItem: AppText {
                    text: parent.text
                    color: "white"
                    font.pixelSize: parent.font.pixelSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    playerItem.confirmInvite()
                    playerItem.expanded = false
                }
            }

            // Ignore player
            Button {
                text: qsTr("Ignore player")
                visible: canIgnore
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 10

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }

                background: Rectangle {
                      color: parent.pressed ? Qt.darker(playerItem.ignoreColor, 1.35)
                          : parent.hovered ? playerItem.ignoreColor
                          : Qt.darker(playerItem.ignoreColor, 1.18)
                    radius: 3
                    border.width: 1
                      border.color: Qt.darker(playerItem.ignoreColor, 1.55)
                }

                contentItem: AppText {
                    text: parent.text
                    color: "white"
                    font.pixelSize: parent.font.pixelSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (Lobby) Lobby.ignorePlayer(targetPlayerId)
                    playerItem.expanded = false
                }
            }

            // Unignore player
            Button {
                text: qsTr("Unignore player")
                visible: canUnignore
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 10

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }

                background: Rectangle {
                      color: parent.pressed ? Qt.darker(playerItem.ignoreColor, 1.35)
                          : parent.hovered ? playerItem.ignoreColor
                          : Qt.darker(playerItem.ignoreColor, 1.18)
                    radius: 3
                    border.width: 1
                      border.color: Qt.darker(playerItem.ignoreColor, 1.55)
                }

                contentItem: AppText {
                    text: parent.text
                    color: "white"
                    font.pixelSize: parent.font.pixelSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (Lobby) Lobby.unignorePlayer(targetPlayerId)
                    playerItem.expanded = false
                }
            }

            // Show player stats (widget parity)
            Button {
                text: qsTr("Show player stats")
                visible: canShowPlayerStats
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 10

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }

                background: Rectangle {
                      color: parent.pressed ? Qt.darker(playerItem.statsColor, 1.35)
                          : parent.hovered ? playerItem.statsColor
                          : Qt.darker(playerItem.statsColor, 1.18)
                    radius: 3
                    border.width: 1
                      border.color: Qt.darker(playerItem.statsColor, 1.55)
                }

                contentItem: AppText {
                    text: parent.text
                    color: "white"
                    font.pixelSize: parent.font.pixelSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (Lobby) Lobby.showPlayerStats(targetPlayerId)
                    playerItem.expanded = false
                }
            }
            
            // Admin action (widget parity)
            Button {
                text: qsTr("Total kickban")
                visible: canAdminModerate
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 10
                
                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }
                
                background: Rectangle {
                    color: parent.pressed ? Qt.darker(playerItem.banColor, 1.35)
                           : parent.hovered ? playerItem.banColor
                           : Qt.darker(playerItem.banColor, 1.18)
                    radius: 3
                    border.width: 1
                    border.color: Qt.darker(playerItem.banColor, 1.55)
                }
                
                contentItem: AppText {
                    text: parent.text
                    color: "white"
                    font.pixelSize: parent.font.pixelSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    if (Lobby) Lobby.adminBanPlayer(targetPlayerId)
                    playerItem.expanded = false
                }
            }
        }
    }
    
    background: Rectangle {
        color: playerItem.hovered
               ? Qt.lighter(Config.StaticData.palette.secondary.col700, 1.2)
               : "transparent"
        radius: 3
        Behavior on color { ColorAnimation { duration: 130 } }
    }

    ConfirmPopup {
        id: invitePopup
        onConfirmed: { if (Lobby) Lobby.invitePlayer(playerItem.targetPlayerId) }
    }
}
