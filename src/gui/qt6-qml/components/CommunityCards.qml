import QtQuick
import QtQuick.Effects

import "../config" as Config

// Gemeinschaftskarten (Flop/Turn/River) + Pott-Badge in der Tischmitte.
// Größe = Kartenreihe; Position/Skalierung/z setzt der Aufrufer (anchors +
// scale), damit die Karten je nach Tisch-Layout zentriert bleiben.
Item {
    id: root

    // Querformat? Steuert den Abstand des Pott-Badges zur Kartenreihe.
    property bool wide: false

    // Effektive Tisch-Skalierung dieser Kartenreihe (communityScale × Zoom), an
    // die Karten weitergereicht, damit ihr SVG-Raster die echte Bildschirmgröße
    // trifft (s. CardImage.renderScale).
    property real cardRenderScale: 1.0

    // True, solange irgendein Board-Slot gerade neue Karten aufdeckt
    // (Stagger/Rückseite/Flip). Die Action-Bar sperrt darauf die Buttons –
    // während Aufdeck-Animationen ist keine Aktion möglich.
    readonly property bool dealing: slot0.revealing || slot1.revealing
                                    || slot2.revealing || slot3.revealing
                                    || slot4.revealing

    width: cardRow.width
    height: cardRow.height
    transformOrigin: Item.Center

    // Inline-Komponente für einen einzelnen Board-Card-Slot.
    // Karten-Seitenverhältnis 120:168 (≈0,714).
    //
    // Deal-Sequenz (wie Widget-Client):
    //   1. Platzhalter-Rahmen sichtbar (Ausgangszustand)
    //   2. isDealt → nach Stagger-Pause: _displayIndex = -1  (Rückseite erscheint)
    //   3. Kurze Pause (wie dealCardsSpeed im Widget-Client)
    //   4. _displayIndex = actualCardIndex  → CardImage.onIsBackChanged feuert den Flip
    component CommunitySlot: Item {
        id: slot
        property int boardIndex: 0
        width: 46; height: 64

        // Showdown-Spotlight: gehört diese Board-Karte nicht zum Siegerblatt,
        // wird sie auf 25 % Deckkraft abgeblendet (Einstellung „Ausblend-
        // Animation für Verliererkarten", Config-Key ShowFadeOutCardsAnimation).
        // Daten: GameHandler.boardCardFade (5 Bools je Board-Karte).
        readonly property bool fadeLosingCards:
            (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
                ? SettingsManager.readConfigInt("ShowFadeOutCardsAnimation") !== 0 : true
        readonly property bool faded: {
            if (!fadeLosingCards)
                return false
            var f = (typeof GameTable !== "undefined" && GameTable) ? GameTable.boardCardFade : null
            return (f && boardIndex < f.length) ? f[boardIndex] === true : false
        }
        opacity: faded ? 0.25 : 1.0
        Behavior on opacity { NumberAnimation { duration: 400; easing.type: Easing.InOutQuad } }

        // Deckt dieser Slot gerade auf? (Stagger + Rückseite + Flip). Solange
        // true, sperrt die Action-Bar die Buttons. dealAnim deckt mit der
        // abschließenden Pause auch die ~470 ms CardImage-Flip-Animation ab.
        readonly property bool revealing: dealAnim.running

        readonly property bool isDealt: {
            var cnt = (typeof GameTable !== "undefined" && GameTable)
                      ? GameTable.boardCardCount : 0
            return boardIndex < cnt
        }

        readonly property int actualCardIndex: {
            var cards = (typeof GameTable !== "undefined" && GameTable)
                        ? GameTable.boardCards : null
            return (cards && boardIndex < cards.length) ? cards[boardIndex] : -1
        }

        // -2 = Platzhalter (noch nichts), -1 = Rückseite, 0-51 = Vorderseite
        property int _displayIndex: -2

        // Platzhalter-Rahmen: nur sichtbar solange noch keine Karte gezeigt wird
        Rectangle {
            anchors.fill: parent
            visible: slot._displayIndex === -2
            radius: 4
            color: Qt.rgba(0, 0, 0, 0.30)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.38)
        }

        // Karte: Rückseite (cardIndex -1) oder Vorderseite (cardIndex 0-51)
        CardImage {
            anchors.fill: parent
            visible: slot._displayIndex !== -2
            cardIndex: slot._displayIndex >= 0 ? slot._displayIndex : -1
            renderScale: root.cardRenderScale
        }

        onIsDealtChanged: {
            if (isDealt) {
                dealAnim.restart()
            } else {
                dealAnim.stop()
                slot._displayIndex = -2
            }
        }

        // Stagger → Rückseite zeigen → Flip auf Vorderseite (ScriptAction setzt
        // _displayIndex, was CardImage.onIsBackChanged und damit den Flip auslöst).
        SequentialAnimation {
            id: dealAnim
            // Flop gestaffelt (0 / 140 / 280 ms); Turn/River ohne Delay
            PauseAnimation { duration: slot.boardIndex < 3 ? slot.boardIndex * 220 : 0 }
            ScriptAction   { script: { slot._displayIndex = -1 } }
            // Pause: Rückseite ist sichtbar (analog dealCardsSpeed im Widget-Client)
            PauseAnimation { duration: 160 }
            // Jetzt auf Vorderseite drehen → CardImage-Flip-Animation feuert
            ScriptAction   { script: { slot._displayIndex = slot.actualCardIndex } }
            // Flip-Dauer abwarten (CardImage: 170 + 300 ms), damit `revealing`
            // bis zum Ende der Aufdeck-Animation true bleibt.
            PauseAnimation { duration: 480 }
        }
    }

    // Weicher Lichtschein hinter den Gemeinschaftskarten → Fokus auf die
    // Tischmitte (dezent, warm).
    Rectangle {
        anchors.centerIn: cardRow
        width: cardRow.width + 80
        height: cardRow.height + 54
        radius: height / 2
        color: Qt.rgba(1.0, 0.93, 0.72, 0.12)
        z: -1
        layer.enabled: Config.Theme.effectsEnabled
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 1.0
            blurMax: 48
            autoPaddingEnabled: true
        }
    }

    // 5 Slots: Flop (0-2) | Turn (3) | River (4)
    Row {
        id: cardRow
        anchors.centerIn: parent
        spacing: 3

        CommunitySlot { id: slot0; boardIndex: 0 }
        CommunitySlot { id: slot1; boardIndex: 1 }
        CommunitySlot { id: slot2; boardIndex: 2 }

        Item { width: 8; height: 1 }

        CommunitySlot { id: slot3; boardIndex: 3 }

        Item { width: 8; height: 1 }

        CommunitySlot { id: slot4; boardIndex: 4 }
    }

    // Pot prominent in der Tischmitte (über den Karten): Chip-Icon +
    // Betrag mit goldenem Glow. Poppt bei Pot-Erhöhung (Mikroanimation).
    Item {
        id: potBadge
        anchors.horizontalCenter: cardRow.horizontalCenter
        anchors.bottom: cardRow.top
        // Gleicher Abstand zur Kartenreihe wie das Winning-Hand-Badge
        // darunter; Portrait kompakter (6) als Querformat (8). Skaliert
        // mit oppScale, da innerhalb communityArea.
        anchors.bottomMargin: root.wide ? 8 : 6
        visible: (typeof GameTable !== "undefined" && GameTable) ? GameTable.totalPot > 0 : false
        width: potRow.width + 16
        height: 24
        transformOrigin: Item.Center

        // Hintergrund mit goldenem Glow. Eigener Layer, damit der MultiEffect
        // nur die Pille (nicht Chip/Text) rendert – sonst werden Schrift und
        // Puck vom Glow überlagert und beim Hochskalieren (oppScale) unscharf.
        Rectangle {
            anchors.fill: parent
            radius: 12
            color: Qt.rgba(0, 0, 0, 0.62)
            border.color: Config.Theme.colorAccent
            border.width: 1

            layer.enabled: Config.Theme.effectsEnabled
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: Config.Theme.colorAccent
                shadowOpacity: 0.45
                shadowBlur: 0.9
                shadowVerticalOffset: 0
            }
        }

        // Chip + Betrag liegen über dem Glow und werden direkt (vektoriell,
        // ohne Layer-Textur) gerendert → bleiben gestochen scharf.
        Row {
            id: potRow
            anchors.centerIn: parent
            z: 1
            spacing: 4
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 16; height: 16
                source: "../resources/chipStack.svg"
                fillMode: Image.PreserveAspectFit
            }
            AppText {
                anchors.verticalCenter: parent.verticalCenter
                text: "$" + (GameTable ? GameTable.totalPot : 0)
                color: Config.Theme.colorAccent
                font.pixelSize: 13
                font.bold: true
                font.letterSpacing: 0.3
            }
        }

        SequentialAnimation {
            id: potPop
            NumberAnimation { target: potBadge; property: "scale"; from: 1.0; to: 1.18; duration: 110; easing.type: Easing.OutQuad }
            NumberAnimation { target: potBadge; property: "scale"; to: 1.0; duration: 170; easing.type: Easing.OutBack }
        }
        Connections {
            target: (typeof GameTable !== "undefined") ? GameTable : null
            function onTotalPotChanged() {
                if (GameTable && GameTable.totalPot > 0) potPop.restart()
            }
        }
    }
}
