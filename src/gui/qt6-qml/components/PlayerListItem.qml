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
    required property string searchFilter

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

    // ── Filter (Spielersuche) ────────────────────────────────────────
    readonly property bool matchesFilter: {
        var f = searchFilter.toLowerCase()
        return f.length === 0 || displayName.toLowerCase().includes(f)
    }

    width: listView.width
    visible: matchesFilter
    height: matchesFilter
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
    readonly property int actionCount: (canSendPm ? 1 : 0)
                                     + (canInvite ? 1 : 0)
                                     + (canIgnore ? 1 : 0)
                                     + (canUnignore ? 1 : 0)
                                     + (canShowPlayerStats ? 1 : 0)
                                     + (canAdminModerate ? 1 : 0)
    // Zwei Zeilen à 13 px für die "spielt gerade in ..."-Info über den Buttons.
    readonly property int inGameLineHeight: 26
    readonly property int expandedHeight: rowHeight + 5
                                        + inGameLineHeight + actionSpacing
                                        + (actionCount * actionButtonHeight)
                                        + (Math.max(0, actionCount - 1) * actionSpacing)

    readonly property bool isSelf: Lobby && targetPlayerId === Lobby.myPlayerId
    // gameListRevision als reaktive Abhängigkeit: erzwingt Neuauswertung
    // wenn Spieler einem Spiel beitreten oder es verlassen.
    readonly property bool canInvite: Lobby && Lobby.canInviteFromCurrentGame && !isSelf && !guestPlayer
        && (Lobby.gameListRevision >= 0 && !Lobby.isPlayerInAnyGame(targetPlayerId))
    readonly property bool canAdminModerate: Lobby && Lobby.isCurrentPlayerAdmin && !isSelf
    // Private Nachricht: Gäste dürfen serverseitig gar nicht chatten – weder
    // als Absender noch als EMPFÄNGER –, und der Server verwirft PMs an Spieler,
    // die an einem LAUFENDEN Tisch sitzen. Alles drei hier ausblenden, sonst
    // käme statt der Nachricht nur ein "Chat rejected" zurück.
    // gameListRevision hält die Prüfung reaktiv.
    readonly property bool canSendPm: Lobby && !isSelf && !Lobby.isMyPlayerGuest && !guestPlayer
        && (Lobby.gameListRevision >= 0 && !Lobby.isPlayerInRunningGame(targetPlayerId))
    readonly property bool canShowPlayerStats: !guestPlayer
    // "Spielt gerade in ..."-Info. Nur abfragen, wenn sie auch jemand sieht:
    // Desktop beim Hovern über dem Namen, Touch im aufgeklappten Bereich
    // (dort gibt es keinen Hover). gameListRevision als reaktive Abhängigkeit,
    // falls der Spieler währenddessen einem Spiel bei- oder es verlässt.
    readonly property string inGameName: {
        var _rev = Lobby ? Lobby.gameListRevision : 0
        return (Lobby && (nameHover.hovered || (expanded && !wideLayout)))
            ? Lobby.playerInGameName(targetPlayerId) : ""
    }
    readonly property bool canIgnore: !isSelf && !guestPlayer && !playerIgnored
    readonly property bool canUnignore: !isSelf && !guestPlayer && playerIgnored
    readonly property bool hasActions: canSendPm || canInvite || canAdminModerate || canIgnore || canUnignore || canShowPlayerStats

    // ── Feste Icon-Spalten (Wide-Layout) ─────────────────────────────────────
    // Damit jedes Icon in ALLEN Zeilen an derselben x-Position sitzt, behält ein
    // für diese Zeile nicht verfügbares Icon seinen Platz (PlayerActionIcon.active)
    // statt aus der Row zu fallen. Eine Spalte wird aber nur dann überhaupt
    // reserviert, wenn die Aktion für die LISTE in Frage kommt – die Kickban-
    // Spalte etwa nur für Server-Admins, sonst bliebe sie bei allen dauerhaft leer.
    // Ignorieren/Entignorieren schließen sich aus und teilen sich eine Spalte.
    readonly property bool slotPm: Lobby && !Lobby.isMyPlayerGuest
    readonly property bool slotInvite: Lobby && Lobby.canInviteFromCurrentGame
    readonly property bool slotIgnore: true
    readonly property bool slotStats: true
    readonly property bool slotAdmin: Lobby && Lobby.isCurrentPlayerAdmin

    readonly property color pmColor: Config.StaticData.chartColor(3, true)
    readonly property color inviteColor: Config.StaticData.chartColor(0, true)
    readonly property color ignoreColor: Config.StaticData.chartColor(8, true)
    readonly property color statsColor: Config.StaticData.chartColor(9, true)
    readonly property color banColor: Config.StaticData.chartColor(5, true)

    // Das Eingabe-Popup liegt bewusst in der Seite und nicht im Delegate:
    // ein Listen-Update während des Tippens würde den Delegate (und damit den
    // Text) verwerfen.
    signal privateMessageRequested(int playerId, string playerName)

    // Rückfrage vor dem Einladen eines Spielers ins eigene Spiel.
    function confirmInvite() {
        invitePopup.openWith(
            qsTr("Invite to Game"),
            qsTr("Are you sure you want to invite \"%1\" to your game?").arg(displayName),
            qsTr("Invite"))
    }

    // Rückfrage vor dem Ignorieren eines Spielers (versehentlicher Klick).
    function confirmIgnore() {
        ignorePopup.openWith(
            qsTr("Ignore player"),
            qsTr("Are you sure you want to ignore \"%1\"?").arg(displayName),
            qsTr("Ignore player"))
    }

    // Rückfrage vor dem Aufheben der Ignorierung eines Spielers.
    function confirmUnignore() {
        unignorePopup.openWith(
            qsTr("Unignore player"),
            qsTr("Are you sure you want to unignore \"%1\"?").arg(displayName),
            qsTr("Unignore player"))
    }

    // Rückfrage vor dem endgültigen Kickban (Admin) eines Spielers.
    function confirmBan() {
        banPopup.openWith(
            qsTr("Total kickban"),
            qsTr("Are you sure you want to totally kickban \"%1\"?").arg(displayName),
            qsTr("Total kickban"))
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
                // (Widget-Client zeigt dieselbe Info im Nickliste-Kontextmenü,
                // Touch-Geräte im aufgeklappten Bereich – siehe unten).
                HoverHandler { id: nameHover }

                ToolTip.text: playerItem.inGameName !== ""
                              ? qsTr("%1 is playing in \"%2\".").arg(displayName).arg(playerItem.inGameName)
                              : qsTr("%1 is not playing at the moment.").arg(displayName)
                ToolTip.visible: nameHover.hovered && !Config.Responsive.isMobile
                                 && Config.Parameters.showTooltips
                ToolTip.delay: 400
            }

            // Wide-Screen: Action-Icons inline, rechtsbündig – in festen Spalten
            // (siehe slot*-Properties oben). Die Row bleibt auch dann stehen,
            // wenn für diese Zeile keine Aktion verfügbar ist, damit die Spalten
            // über alle Zeilen dieselbe Breite behalten.
            Row {
                id: wideActionsRow
                visible: playerItem.wideLayout
                spacing: 2
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                PlayerActionIcon {
                    visible: playerItem.slotPm
                    active: playerItem.canSendPm
                    source: "qrc:/resources/mail.svg"
                    baseColor: playerItem.pmColor
                    tooltipText: qsTr("Send private message")
                    onTriggered: playerItem.privateMessageRequested(playerItem.targetPlayerId,
                                                                   playerItem.displayName)
                }
                PlayerActionIcon {
                    visible: playerItem.slotInvite
                    active: playerItem.canInvite
                    source: "qrc:/resources/personAdd.svg"
                    baseColor: playerItem.inviteColor
                    tooltipText: qsTr("Invite to Game")
                    onTriggered: playerItem.confirmInvite()
                }
                // Eine Spalte für beide Zustände: ignoriert ⇄ nicht ignoriert.
                PlayerActionIcon {
                    visible: playerItem.slotIgnore
                    active: playerItem.canIgnore || playerItem.canUnignore
                    source: playerItem.playerIgnored ? "qrc:/resources/checkCircle.svg"
                                                     : "qrc:/resources/block.svg"
                    baseColor: playerItem.ignoreColor
                    tooltipText: playerItem.playerIgnored ? qsTr("Unignore player")
                                                          : qsTr("Ignore player")
                    onTriggered: {
                        if (playerItem.playerIgnored)
                            playerItem.confirmUnignore()
                        else
                            playerItem.confirmIgnore()
                    }
                }
                PlayerActionIcon {
                    visible: playerItem.slotStats
                    active: playerItem.canShowPlayerStats
                    source: "qrc:/resources/barChart.svg"
                    baseColor: playerItem.statsColor
                    tooltipText: qsTr("Show player stats")
                    onTriggered: { if (Lobby) Lobby.showPlayerStats(playerItem.targetPlayerId) }
                }
                PlayerActionIcon {
                    visible: playerItem.slotAdmin
                    active: playerItem.canAdminModerate
                    source: "qrc:/resources/gavel.svg"
                    baseColor: playerItem.banColor
                    tooltipText: qsTr("Total kickban")
                    onTriggered: playerItem.confirmBan()
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
                // Einfärbung per layer.effect statt MultiEffect-Kind: VectorImage
                // (Qt >= 6.8) ist kein Texture-Provider und darf nicht per source
                // referenziert werden (sonst schwarz/zerrissen gerendert).
                layer.enabled: true
                layer.effect: MultiEffect {
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

            // Touch-Pendant zum Desktop-Hover-Tooltip über dem Namen: am
            // Telefon gibt es keinen Hover, also steht dieselbe Info hier –
            // gleicher Wortlaut, gleiche Quelle (playerItem.inGameName).
            AppText {
                Layout.fillWidth: true
                Layout.preferredHeight: playerItem.inGameLineHeight
                text: playerItem.inGameName !== ""
                      ? qsTr("%1 is playing in \"%2\".").arg(playerItem.displayName)
                                                         .arg(playerItem.inGameName)
                      : qsTr("%1 is not playing at the moment.").arg(playerItem.displayName)
                font.pixelSize: 10
                color: playerItem.inGameName !== "" ? playerItem.inviteColor
                                                    : Config.Theme.colorTextMuted
                // Feste Höhe für zwei Zeilen: die Zeilenhöhe geht in
                // expandedHeight ein, ein dynamischer Umbruch würde den
                // aufgeklappten Bereich unten abschneiden.
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            // Send private message
            Button {
                text: qsTr("Send private message")
                visible: canSendPm
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 10

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }

                background: Rectangle {
                    color: parent.pressed ? Qt.darker(playerItem.pmColor, 1.35)
                           : parent.hovered ? playerItem.pmColor
                           : Qt.darker(playerItem.pmColor, 1.18)
                    radius: 3
                    border.width: 1
                    border.color: Qt.darker(playerItem.pmColor, 1.55)
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
                    playerItem.privateMessageRequested(playerItem.targetPlayerId,
                                                       playerItem.displayName)
                    playerItem.expanded = false
                }
            }

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
                    playerItem.confirmIgnore()
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
                    playerItem.confirmUnignore()
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
                    playerItem.confirmBan()
                    playerItem.expanded = false
                }
            }
        }
    }
    
    background: Rectangle {
        color: playerItem.hovered
               ? Config.Theme.colorHover
               : "transparent"
        radius: 3
        Behavior on color { ColorAnimation { duration: 130 } }
    }

    ConfirmPopup {
        id: invitePopup
        onConfirmed: { if (Lobby) Lobby.invitePlayer(playerItem.targetPlayerId) }
    }

    ConfirmPopup {
        id: ignorePopup
        onConfirmed: { if (Lobby) Lobby.ignorePlayer(playerItem.targetPlayerId) }
    }

    ConfirmPopup {
        id: unignorePopup
        onConfirmed: { if (Lobby) Lobby.unignorePlayer(playerItem.targetPlayerId) }
    }

    ConfirmPopup {
        id: banPopup
        onConfirmed: { if (Lobby) Lobby.adminBanPlayer(playerItem.targetPlayerId) }
    }
}
