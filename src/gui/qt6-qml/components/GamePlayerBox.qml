import QtQuick
import QtQuick.Controls

import "../config" as Config

SeatBox {
    id: root

    // Winner-Badge unterhalb statt oberhalb der Box anzeigen – nur für die oberste
    // Box (Player 5) im Hochformat sinnvoll, sonst würde es oben anstoßen.
    property bool winnerBelow: false
    // Seite, auf der Einsatz-Chip + Dealer/Blind-Button angezeigt werden:
    // "top" | "bottom" | "left" | "right". Default leitet sich aus 'up' ab.
    property string betSide: up ? "bottom" : "top"
    // Geteilte Anzeige: Dealer/Blind-Button LINKS neben der Box, Einsatz
    // RECHTS neben der Box – beides vertikal mittig. Für die oberste Box im
    // landscapeCompact, deren Badge sonst unterhalb mit dem Pot-Badge
    // kollidiert. Übersteuert betSide.
    property bool betSplit: false

    // Effektive Tisch-Skalierung, Sitzdaten und Sockel-Maße: siehe SeatBox.
    // Dynamische Breite: 2×hMargin(4) + AvatarCardRow.implicitWidth(avatarH+4+2·cardW+4)
    readonly property int _topRowH: bodyH - (wideLayout ? 44 : 28)
    readonly property int _cardW:   Math.round(_topRowH * 120 / 168)
    implicitWidth: 2 * 4 + _topRowH + 4 + 2 * _cardW + 4
    implicitHeight: 84 + betStripH

    // Ausgeschieden (kein Geld mehr) - eigener Default gegenüber der Self-Box.
    readonly property bool isActive: seatData ? seatData.active : false
    // Avatare ignorierter Spieler ausblenden (Basis-Property gegenbinden),
    // sofern DontHideAvatarsOfIgnored das nicht abschaltet.
    hideIgnoredAvatar:
        playerIgnored
        && ((typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
                ? SettingsManager.readConfigInt("DontHideAvatarsOfIgnored") === 0 : true)

    // ── Kontextaktionen ──────────────────────────────────────────────────────
    // Rechtsklick (Desktop) bzw. langer Druck (Touch) auf eine Gegnerbox öffnet
    // ein Kontextmenü mit „Ignore Player", „Unignore Player", „Show player
    // stats" und der Spieler-Notiz – wie der Qt-Widgets-Client (MyAvatarLabel)
    // bzw. die Lobby-Spielerliste (PlayerListItem). Die Aktionen greifen nur im
    // Netzwerkspiel: nur dort trägt seatData eine playerId (für lokale
    // Spiele/CPU-Gegner 0 → kein Menü).
    readonly property bool targetIsComputer:
        seatData && seatData.isComputer !== undefined ? seatData.isComputer : false
    readonly property int targetPlayerId:
        seatData && seatData.playerId !== undefined ? seatData.playerId : 0
    readonly property bool targetIsGuest:
        seatData && seatData.isGuest !== undefined ? seatData.isGuest : false
    readonly property bool targetIsSelf:
        targetPlayerId !== 0 && typeof Lobby !== "undefined" && Lobby && targetPlayerId === Lobby.myPlayerId
    readonly property bool playerIgnored: {
        var _rev = (typeof Lobby !== "undefined" && Lobby) ? Lobby.playerIgnoreListRevision : 0
        return (typeof Lobby !== "undefined" && Lobby && targetPlayerId !== 0)
            ? Lobby.isPlayerIgnored(targetPlayerId) : false
    }
    readonly property bool canIgnore: !targetIsGuest && !targetIsSelf && !playerIgnored
    readonly property bool canUnignore: !targetIsGuest && !targetIsSelf && playerIgnored
    readonly property bool canShowStats: !targetIsGuest
    // Avatar melden: nur im Internet-Spiel und nur wenn der Spieler einen
    // (existierenden) Avatar gesetzt hat – 1:1 wie der Qt-Widgets-Client
    // (MyAvatarLabel). seatData.avatar ist nur bei vorhandener Datei gesetzt.
    readonly property bool canReportAvatar:
        !targetIsSelf
        && (typeof GameTable !== "undefined" && GameTable && GameTable.isInternetGameRunning())
        && !!(seatData && seatData.avatar && seatData.avatar !== "")
    readonly property bool hasContextActions:
        targetPlayerId !== 0 && !targetIsComputer
        && (canIgnore || canUnignore || canShowStats || canReportAvatar || canEditNote)

    // ── Spieler-Notiz und -Bewertung ─────────────────────────────────────────
    // Eigene, rein lokale Notiz (Sterne + Text) zu einem Mitspieler, gespeichert
    // im selben Config-Eintrag wie im Qt-Widgets-Client (siehe
    // SettingsManager::setPlayerNote). Wie dort nur im Internet-Spiel: nur da
    // steht hinter dem Namen ein dauerhaft registriertes Konto, an dem eine
    // namensbasierte Notiz überhaupt hängen bleiben kann.
    readonly property bool canEditNote:
        !targetIsGuest && !targetIsSelf && !targetIsComputer
        && (typeof GameTable !== "undefined" && GameTable && GameTable.isInternetGameRunning())
    readonly property int playerRating: {
        var _rev = (typeof SettingsManager !== "undefined" && SettingsManager)
                   ? SettingsManager.playerNotesRevision : 0
        return (canEditNote && typeof SettingsManager !== "undefined" && SettingsManager)
            ? SettingsManager.playerRating(root.targetPlayerName) : 0
    }
    readonly property string playerNote: {
        var _rev = (typeof SettingsManager !== "undefined" && SettingsManager)
                   ? SettingsManager.playerNotesRevision : 0
        return (canEditNote && typeof SettingsManager !== "undefined" && SettingsManager)
            ? SettingsManager.playerNote(root.targetPlayerName) : ""
    }

    // Widescreen-Layout: Box ist groß genug für 2-zeilige Info (Name + Flagge/Cash).
    // Nutzt height >= 76 als Proxy für tableZone.wide (oppBaseHeight = wide ? 84 : 71).
    // Bewusst NICHT Config.Responsive.landscape – die Tablezone kann breiter als
    // hoch sein, auch wenn das Gesamtfenster (inkl. Toolbar) hochformat-mäßig ist.
    // Sockelhöhe herausrechnen: sonst würde eine Portrait-Box (71 + Sockel)
    // fälschlich über die 76er-Schwelle rutschen und den 2-zeiligen
    // Landscape-Footer bekommen.
    readonly property bool wideLayout: bodyH >= 76

    // Nur anzeigen wenn der Sitz besetzt ist
    visible: root.seatData !== null && root.seatData.name !== ""

    // Informationsdichte: wer raus ist (kein Geld mehr → !isActive) wird deutlich
    // abgedunkelt, wer nur gefoldet hat dezent zurückgenommen. So heben sich der
    // aktive Spieler und die noch laufende Hand klarer hervor.
    opacity: !root.isActive ? Config.Theme.dimmedOpacity
           : (root.folded ? 0.72 : 1.0)
    Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.OutQuad } }

    // ── Hauptbox ────────────────────────────────────────────────────────────────
    Rectangle {
        id: playerBox
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        // Körper + (aufgeklappter) Sockel. Der Rest der reservierten Höhe
        // bleibt leer, solange der Spieler nichts gesetzt hat.
        height: root.bodyH + betStrip.height
        color: "transparent"
        property int hMargin: 4

        // Aktiver Spieler leicht „angehoben" → mehr Tiefe/Fokus (sanfter Übergang).
        scale: root.isAtTurn ? 1.04 : 1.0
        transformOrigin: Item.Center
        Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }

        // Karten-Hintergrund mit dezentem Verlauf + weichem Schlagschatten → die
        // Box wirkt als angehobene Karte statt als flache Fläche.
        PlayerBoxBackground {}

        // Highlight: aktiver Spieler bekommt einen gold Rahmen + weichen Glow.
        PlayerTurnGlow { active: root.isAtTurn }

        // Avatar + Karten: AvatarCardRow garantiert cardH == topRowH (keine
        // Rundungsdifferenz). Abstände: 4 px links, 4 px Avatar↔Karten,
        // 4 px zwischen den Karten, 4 px rechts (= implicitWidth-Formel oben).
        AvatarCardRow {
            id: cardRow
            x: playerBox.hMargin
            y: 4
            height: root.wideLayout ? (root.bodyH - 44) : (root.bodyH - 28)

            cardRenderScale: root.cardRenderScale
            card0: root.card0
            card1: root.card1
            fade0: root.fade0
            fade1: root.fade1
            avatarSource: root.avatarSource
            folded: root.folded
            playerActive: root.isActive
        }

        // Portrait: Name + (Notiz-Badge) + Stack einzeilig. Anker statt fester
        // Hälften, damit das Badge nur den Platz nimmt, den es wirklich braucht,
        // und der Name um genau diesen Betrag früher elidiert.
        Item {
            visible: !root.wideLayout
            width: parent.width - 2 * playerBox.hMargin
            height: 15
            x: playerBox.hMargin
            y: root.bodyH - height - 4

            AppText {
                anchors.left: parent.left
                anchors.right: compactBadge.visible ? compactBadge.left : compactStack.left
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignLeft
                color: "#eff1f5"
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 0.3
                elide: Text.ElideRight
                text: root.seatData && root.seatData.name !== "" ? root.seatData.name : "---"
            }

            PlayerNoteBadge {
                id: compactBadge
                anchors.right: compactStack.left
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                rating: root.playerRating
                note: root.playerNote
                glyphSize: 10
            }

            AppText {
                id: compactStack
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignRight
                color: Config.Theme.colorAccent
                font.pixelSize: 12
                font.bold: true
                text: root.seatData && root.seatData.name !== "" ? "$" + root.seatData.stack : ""
            }
        }

        // Widescreen: Name + Flagge + Stack 2-zeilig
        Item {
            id: infoBar
            visible: root.wideLayout
            width: parent.width - 2 * playerBox.hMargin
            height: 36
            x: playerBox.hMargin
            y: root.bodyH - height - 4

            AppText {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.right: wideBadge.visible ? wideBadge.left : parent.right
                anchors.rightMargin: wideBadge.visible ? 4 : 2
                horizontalAlignment: Text.AlignLeft
                color: "#eff1f5"
                font.pixelSize: 15
                font.weight: Font.DemiBold
                font.letterSpacing: 0.3
                elide: Text.ElideRight
                text: root.seatData && root.seatData.name !== "" ? root.seatData.name : "---"
            }

            // Notiz/Bewertung in die NAMENSZEILE, nicht in die untere Zeile: dort
            // stehen bereits Flagge (22+6) und Stack (bis ~55 px bei sechs
            // Stellen) – zusammen mit dem Badge wäre das mehr als die 106 px
            // Innenbreite der Box (oppBaseWidth 114 − 2×hMargin). Hier oben
            // konkurriert es nur mit dem Namen, der ohnehin elidiert.
            PlayerNoteBadge {
                id: wideBadge
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 2
                rating: root.playerRating
                note: root.playerNote
                glyphSize: 13
            }

            Image {
                visible: root.countryCode !== ""
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                width: 22
                height: 15
                source: root.countryCode !== ""
                    ? "qrc:/resources/cflags/" + root.countryCode + ".svg" : ""
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            AppText {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                horizontalAlignment: Text.AlignRight
                color: Config.Theme.colorAccent
                font.pixelSize: 15
                font.bold: true
                text: root.seatData && root.seatData.name !== "" ? "$" + root.seatData.stack : ""
            }
        }

        // Einsatz-Sockel am unteren Boxrand (Sitz-Stil "inset"). 1 px innerhalb
        // des Rahmens von PlayerBoxBackground, damit dessen Rand sichtbar bleibt.
        PlayerBetStrip {
            id: betStrip
            open: root.stripOpen
            amount: root.bet
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }

        // Winner-Hervorhebung: goldener Rahmen (verdeckt die Karten NICHT) +
        // „WINNER"-Badge. Badge standardmäßig über der Box; nur die oberste Box
        // (winnerBelow) zeigt es unterhalb, sonst stieße es am Bildschirmrand an.
        // Kind von playerBox (nicht von root): der Rahmen soll den KÖRPER
        // umschließen, nicht die darunter reservierte, leere Sockelhöhe.
        PlayerWinnerOverlay {
            active: root.isWinner
            below: root.winnerBelow
        }
    }

    // Aktions-Anzeige (Fold/Check/Call/Bet/Raise/All-In) – zentriert über den
    // Hole-Cards in den normalen Player-Boxen.
    PlayerActionBadge {
        id: actionBadge
        visible: root.actionText !== "" && !root.isWinner
        action: root.action
        label: root.actionText
        z: 18

        readonly property real cardsCenterX: playerBox.hMargin + cardRow.cardsCenterX
        readonly property real cardsCenterY: cardRow.y + cardRow.height / 2
        x: cardsCenterX - width / 2
        y: cardsCenterY - height / 2
    }

    // Action-Timeout: schlanker Fortschrittsbalken an der Stelle des Action-
    // Badges, solange dieser Sitz am Zug ist (zählt über die Timeout-Dauer runter).
    PlayerTimeoutBar {
        id: timeoutBar
        readonly property bool atTurn: (typeof GameTable !== "undefined" && GameTable)
                                       && GameTable.timeoutSeatId === root.seatIndex
        active: atTurn
        visible: atTurn && !root.isWinner && root.actionText === ""
        width: 44
        height: 9
        z: 18
        x: actionBadge.cardsCenterX - width / 2
        y: actionBadge.cardsCenterY - height / 2
    }

    // Einsatz (Chip + Betrag) + Dealer/Small-/Big-Blind-Button – gruppiert.
    // Oben/unten-Mitte (betSide top/bottom): volle Boxbreite; Einsatz zentriert,
    // Button rechtsbündig mit 6px Außenabstand – identisch zur Self-Box.
    // Seiten (betSide left/right): Button unter dem Einsatz, beides vertikal zentriert.
    Item {
        id: betGroup
        visible: (root.bet > 0 && !root.betInset) || root.buttonVisible
        z: 25

        readonly property bool split: root.betSplit
        readonly property bool horizontal: !split && (root.betSide === "bottom" || root.betSide === "top")
        // Im Stil "inset" steckt der Einsatz im Box-Sockel – die Gruppe trägt
        // dann nur noch den Dealer-/Blind-Puck.
        readonly property real betW: (root.bet > 0 && !root.betInset) ? betRow.width : 0
        readonly property real betH: (root.bet > 0 && !root.betInset) ? betRow.height : 0
        readonly property real btnW: root.buttonVisible ? buttonImg.width : 0
        readonly property real btnH: root.buttonVisible ? buttonImg.height : 0

        width: (horizontal || split) ? playerBox.width : Math.max(betW, btnW)
        height: horizontal ? Math.max(betH, btnH) : playerBox.height

        x: split ? 0
         : root.betSide === "right" ? playerBox.width + 8
         : root.betSide === "left"  ? -width - 8
         : 0
        y: split ? 0
         : root.betSide === "bottom" ? playerBox.height + 7
         : root.betSide === "top"    ? -height - 7
         : (playerBox.height - height) / 2

        BetChip {
            id: betRow
            visible: root.bet > 0 && !root.betInset
            amount: root.bet
            textColor: "#f0f0f0"
            // split: Einsatz rechts NEBEN der Box; sonst innerhalb zentriert.
            x: betGroup.split ? betGroup.width + 8 : (betGroup.width - width) / 2
            y: (betGroup.height - height) / 2
        }

        // Dealer/Blind-Button – split: links NEBEN der Box; horizontal:
        // rechtsbündig 6px vom Boxrand; Seiten: unterer Slot.
        BlindButtonImage {
            id: buttonImg
            visible: root.buttonVisible
            button: root.button
            x: betGroup.split
               ? -width - 8
               : betGroup.horizontal
               ? (betGroup.width - width - 6)
               : (root.betSide === "right" ? 0 : (betGroup.width - width))
            // Seitlich (betSide left/right) saß der Puck im UNTEREN Slot, weil
            // darüber der Einsatz stand. Steckt der Einsatz im Sockel, ist der
            // Slot frei → Puck vertikal mittig neben die Box.
            y: (betGroup.horizontal || betGroup.split || root.betInset)
               ? (betGroup.height - height) / 2
               : (betGroup.height * 5 / 6 - height / 2)
        }
    }

    // ── Rechtsklick-Kontextmenü (nur Desktop) ────────────────────────────────
    // Fängt nur die rechte Maustaste ab; linke Klicks/Hover fallen an die
    // darunterliegenden Elemente durch. Erscheint nur, wenn der Sitz einen
    // echten Online-Mitspieler trägt (hasContextActions).
    MouseArea {
        // Nur über dem Boxkörper – die darunter reservierte Sockelhöhe ist
        // leerer Tisch, dort darf kein Kontextmenü aufgehen.
        id: contextArea
        anchors.fill: playerBox
        z: 30
        enabled: root.hasContextActions
        acceptedButtons: Qt.RightButton
        onClicked: (mouse) => contextMenu.popup(mouse.x, mouse.y)

        // Touch hat keine rechte Maustaste: dort öffnet ein langer Druck
        // dasselbe Menü. Bewusst auf Touch-Geräte beschränkt, damit ein
        // gehaltener Linksklick mit der Maus weiterhin nichts auslöst.
        TapHandler {
            acceptedDevices: PointerDevice.TouchScreen
            enabled: root.hasContextActions
            onLongPressed: contextMenu.popup(point.position.x, point.position.y)
        }
    }

    // Einheitlich gestylter Menüeintrag (dunkles Theme, kollabiert wenn unsichtbar).
    component CtxItem: MenuItem {
        height: visible ? implicitHeight : 0
        contentItem: AppText {
            text: parent.text
            color: parent.enabled
                   ? (parent.highlighted ? Config.Theme.colorAccent : Config.Theme.colorTextPrimary)
                   : Config.Theme.colorTextMuted
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8
        }
        background: Rectangle {
            color: parent.highlighted ? Config.StaticData.palette.secondary.col600 : "transparent"
        }
    }

    Menu {
        id: contextMenu

        // Breite an den breitesten sichtbaren Eintrag anpassen (min. 180).
        // Nötig, weil das ListView-contentItem des Menüs keine implicitWidth
        // meldet – ohne das würde die Breite allein vom Background bestimmt und
        // längere (auch übersetzte) Labels wie „Report inappropriate avatar"
        // abgeschnitten.
        implicitWidth: {
            var w = 180
            for (var i = 0; i < count; ++i) {
                var it = itemAt(i)
                if (it && it.visible)
                    w = Math.max(w, it.implicitWidth)
            }
            return w
        }

        // Dunkles Theme passend zur Tischoberfläche.
        background: Rectangle {
            implicitWidth: 180
            color: Config.Theme.colorBox
            border.width: 1
            border.color: Config.StaticData.palette.secondary.col500
            radius: Config.Theme.radiusSmall
        }

        CtxItem {
            text: qsTr("Ignore player")
            visible: root.canIgnore
            onTriggered: root.confirmIgnore()
        }
        CtxItem {
            text: qsTr("Unignore player")
            visible: root.canUnignore
            onTriggered: root.confirmUnignore()
        }
        CtxItem {
            text: qsTr("Show player stats")
            visible: root.canShowStats
            onTriggered: { if (typeof Lobby !== "undefined" && Lobby) Lobby.showPlayerStats(root.targetPlayerId) }
        }
        CtxItem {
            text: qsTr("Report inappropriate avatar")
            visible: root.canReportAvatar
            onTriggered: root.confirmReportAvatar()
        }
        CtxItem {
            text: qsTr("Note about player ...")
            visible: root.canEditNote
            onTriggered: notePopup.openFor(root.targetPlayerName)
        }
    }

    PlayerNoteDialog { id: notePopup }

    readonly property string targetPlayerName: root.seatData ? (root.seatData.name || "") : ""

    // Rückfrage vor dem Ignorieren eines Spielers (versehentlicher Klick).
    function confirmIgnore() {
        ignorePopup.openWith(
            qsTr("Ignore player"),
            qsTr("Are you sure you want to ignore \"%1\"?").arg(root.targetPlayerName),
            qsTr("Ignore player"))
    }

    // Rückfrage vor dem Aufheben der Ignorierung eines Spielers.
    function confirmUnignore() {
        unignorePopup.openWith(
            qsTr("Unignore player"),
            qsTr("Are you sure you want to unignore \"%1\"?").arg(root.targetPlayerName),
            qsTr("Unignore player"))
    }

    // Rückfrage vor dem Melden eines unangemessenen Avatars (Port der
    // Bestätigung aus MyAvatarLabel::reportBadAvatar).
    function confirmReportAvatar() {
        reportAvatarPopup.openWith(
            qsTr("Report inappropriate avatar"),
            qsTr("Are you sure you want to report the avatar of \"%1\" as inappropriate?").arg(root.targetPlayerName),
            qsTr("Report"))
    }

    ConfirmPopup {
        id: ignorePopup
        onConfirmed: { if (typeof Lobby !== "undefined" && Lobby) Lobby.ignorePlayer(root.targetPlayerId) }
    }

    ConfirmPopup {
        id: unignorePopup
        onConfirmed: { if (typeof Lobby !== "undefined" && Lobby) Lobby.unignorePlayer(root.targetPlayerId) }
    }

    ConfirmPopup {
        id: reportAvatarPopup
        onConfirmed: { if (typeof GameTable !== "undefined" && GameTable) GameTable.reportAvatar(root.seatIndex) }
    }
}
