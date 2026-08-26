import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config
import "../components"

Rectangle {
    id: lobbyCreateGamePage
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    // Ranking-Konstanten (vom Server vorgegeben)
    readonly property bool isRanking: gameTypeCombo.currentIndex === 3
    readonly property bool isInviteOnly: gameTypeCombo.currentIndex === 2
    // Gäste dürfen serverseitig nur Standardspiele mit ihrem eigenen Spielnamen
    // eröffnen (PLAYER_RIGHTS_GUEST, NTF_NET_JOIN_GUEST_FORBIDDEN).
    readonly property bool isGuest: Lobby ? Lobby.isMyPlayerGuest : false
    // Rangliste: der Server verbietet ein Passwort; Einladungsspiele regeln den
    // Zugang über die Einladung.
    readonly property bool passwordAllowed: !isRanking && !isInviteOnly
    property string nameError: ""
    // Erst nach dem Befüllen des Formulars reagieren die Eingabefelder
    // aufeinander (ComboBox-Signale feuern schon beim Seitenaufbau).
    property bool formReady: false

    // ── Community-Vorlagen (BBC / Monthly Cup / WEC) ─────────────────────────
    // Offizielle Turnier-Settings der PokerTH-Community. Nur für Custom-Spiele
    // vom Typ "Nur eingeladene Spieler" und nur bei aktiviertem Community-
    // Inhalt wählbar. BBC Steps nutzen eine feste Blindliste (Erhöhung alle
    // 5 Minuten), Monthly Cup und WEC verdoppeln die Blinds nach Handzahl.
    // Community-Vorlagen liegen im Singleton Config.BotSuggest – dieselbe
    // Tabelle dient dort der Typ-Erkennung fremder Tische (Settings-Fingerprint),
    // sie darf deshalb nicht doppelt gepflegt werden.
    readonly property var communityPresets: Config.BotSuggest.presets
    readonly property var activePreset: (Config.Parameters.showCommunityContent
                                         && isInviteOnly
                                         && presetCombo.currentIndex > 0)
        ? communityPresets[presetCombo.currentIndex - 1] : null
    readonly property bool presetActive: activePreset !== null
    // Sperrt die vom Server (Ranking) bzw. von der Vorlage vorgegebenen Felder.
    readonly property bool fieldsLocked: isRanking || presetActive

    // In den Netzwerkspiel-Optionen hinterlegte manuelle Blindreihenfolge.
    property bool savedManualBlindsOrder: false
    property var savedManualBlinds: []

    // Blindliste des Spiels: eine Community-Vorlage sticht die gespeicherte
    // Reihenfolge. Eine leere Liste bedeutet "Blinds verdoppeln".
    readonly property var effectiveBlinds: presetActive
        ? activePreset.blinds
        : (savedManualBlindsOrder ? savedManualBlinds : [])

    // Überträgt die gewählte Vorlage in die Formularfelder bzw. stellt bei
    // "Eigene Einstellungen" die Werte aus den Optionen wieder her.
    function applyPreset() {
        var p = activePreset
        if (!p) {
            applyGameType()
            loadTimingSettings()
            return
        }
        gameNameField.text = p.name
        // Vorlagen mit monatlich wechselndem Tischnamen (Monthly Cup): aktuellen
        // Titel-Prefix aus den botfiles ziehen und – sofern der Nutzer den
        // (Fallback-)Namen nicht selbst geändert hat und dieselbe Vorlage noch
        // gewählt ist – übernehmen. Asynchron; bei Fehlschlag bleibt p.name.
        if (p.titleCommand) {
            var fallbackName = p.name
            Config.BotSuggest.gameTitlePrefix(p.titleCommand, function(title) {
                if (title.length > 0
                    && lobbyCreateGamePage.activePreset === p
                    && gameNameField.text === fallbackName)
                    gameNameField.text = title
            })
        }
        maxPlayersSpinBox.value = 10
        startCashSpinBox.value = p.startCash
        firstBlindSpinBox.value = p.firstSmallBlind
        raiseByHandsRadio.checked = p.raiseOnHands
        raiseByMinutesRadio.checked = !p.raiseOnHands
        raiseEveryHandsSpinBox.value = p.raiseEveryHands
        raiseEveryMinutesSpinBox.value = p.raiseEveryMinutes
        playerActionTimeoutSpinBox.value = p.playerActionTimeout
        delayBetweenHandsSpinBox.value = 7   // alle Vorlagen: DelayBetweenHands=7
    }

    // Zeitlimits aus den Netzwerkspiel-Optionen. Sie hängen – wie im Widget-
    // Client – nicht vom Spieltyp ab und werden nur einmal übernommen.
    function loadTimingSettings() {
        if (!SettingsManager)
            return
        playerActionTimeoutSpinBox.value = SettingsManager.readConfigInt("NetTimeOutPlayerAction")
        delayBetweenHandsSpinBox.value   = SettingsManager.readConfigInt("NetDelayBetweenHands")
    }

    // Tisch- und Blind-Einstellungen aus den Netzwerk-/Internetspiel-Optionen.
    function loadGameSettings() {
        if (!SettingsManager)
            return
        maxPlayersSpinBox.value        = SettingsManager.readConfigInt("NetNumberOfPlayers")
        startCashSpinBox.value         = SettingsManager.readConfigInt("NetStartCash")
        spectatorsToggle.checked       = SettingsManager.readConfigInt("InternetGameAllowSpectators") !== 0
        firstBlindSpinBox.value        = SettingsManager.readConfigInt("NetFirstSmallBlind")
        var raiseAtHands = SettingsManager.readConfigInt("NetRaiseBlindsAtHands") !== 0
        raiseByHandsRadio.checked      = raiseAtHands
        raiseByMinutesRadio.checked    = !raiseAtHands
        raiseEveryHandsSpinBox.value   = SettingsManager.readConfigInt("NetRaiseSmallBlindEveryHands")
        raiseEveryMinutesSpinBox.value = SettingsManager.readConfigInt("NetRaiseSmallBlindEveryMinutes")
        savedManualBlindsOrder         = SettingsManager.readConfigInt("NetManualBlindsOrder") !== 0
        // readConfigIntList() liefert eine QList<int>-Sequenz; als echtes
        // JS-Array ist sie stabil und lässt sich per join()/length auswerten.
        var saved = SettingsManager.readConfigIntList("NetManualBlindsList")
        var blinds = []
        for (var i = 0; i < saved.length; ++i)
            blinds.push(saved[i])
        savedManualBlinds = blinds
    }

    // Pendant zu createInternetGameDialogImpl::gameTypeChanged(): Ranglisten-
    // spiele sind vom Server vorgegeben, alle anderen Spieltypen starten mit
    // den gespeicherten Einstellungen.
    function applyGameType() {
        if (!passwordAllowed)
            passwordToggle.checked = false
        if (isRanking) {
            maxPlayersSpinBox.value      = 10     // RANKING_GAME_NUMBER_OF_PLAYERS
            startCashSpinBox.value       = 10000  // RANKING_GAME_START_CASH
            firstBlindSpinBox.value      = 50     // RANKING_GAME_START_SBLIND
            raiseByHandsRadio.checked    = true
            raiseByMinutesRadio.checked  = false
            raiseEveryHandsSpinBox.value = 11     // RANKING_GAME_RAISE_EVERY_HAND
            spectatorsToggle.checked     = true
            return
        }
        loadGameSettings()
    }

    // Pendant zu createInternetGameDialogImpl::fillFormular().
    Component.onCompleted: {
        if (isGuest) {
            // Gäste eröffnen immer ein Standardspiel unter ihrem Spielernamen.
            gameTypeCombo.currentIndex = 0
            gameNameField.text = qsTr("%1's game").arg(Lobby ? Lobby.myPlayerName : "")
        } else if (SettingsManager) {
            var type = SettingsManager.readConfigInt("InternetGameType")
            gameTypeCombo.currentIndex = (type >= 0 && type <= 3) ? type : 0
            var name = SettingsManager.readConfigString("InternetGameName")
            if (name.trim().length > 0)
                gameNameField.text = name
        }
        // Auch nötig, wenn der gespeicherte Spieltyp dem Vorgabeindex 0
        // entspricht und onCurrentIndexChanged deshalb nicht auslöst.
        applyGameType()
        loadTimingSettings()
        if (SettingsManager && passwordAllowed
                && SettingsManager.readConfigInt("UseInternetGamePassword")) {
            passwordToggle.checked = true
            passwordField.text = SettingsManager.readConfigString("InternetGamePassword")
        }
        formReady = true
    }

    // ── Hilfsfunktion: gestylter ComboBox-Popup ──────────────────────────────
    component StyledCombo: ComboBox {
        id: combo
        property var iconSources: []
        font.family: Config.StaticData.loadedFont.font.family
        font.pixelSize: 12
        implicitHeight: 36
        leftPadding: 8
        rightPadding: indicator.width + spacing + 4

        contentItem: RowLayout {
            spacing: 6

            SvgIcon {
                visible: combo.iconSources.length > combo.currentIndex
                source: combo.iconSources.length > combo.currentIndex ? combo.iconSources[combo.currentIndex] : ""
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                Layout.alignment: Qt.AlignVCenter
                layer.enabled: visible
                layer.effect: MultiEffect {
                    colorization: 1.0
                    colorizationColor: combo.enabled
                        ? Config.StaticData.palette.secondary.col200
                        : Config.StaticData.palette.secondary.col400
                }
            }
            Text {
                Layout.fillWidth: true
                text: combo.displayText
                font: combo.font
                color: combo.enabled
                    ? Config.StaticData.palette.secondary.col100
                    : Config.StaticData.palette.secondary.col400
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
        background: Rectangle {
            radius: 6
            color: Config.StaticData.palette.secondary.col600
            border.color: combo.hovered || combo.pressed
                ? Config.StaticData.palette.secondary.col100
                : Config.StaticData.palette.secondary.col300
            border.width: 1
        }
        delegate: ItemDelegate {
            width: combo.width
            implicitHeight: 36
            leftPadding: 8
            contentItem: RowLayout {
                spacing: 6
                SvgIcon {
                    visible: combo.iconSources.length > index
                    source: combo.iconSources.length > index ? combo.iconSources[index] : ""
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    Layout.alignment: Qt.AlignVCenter
                    layer.enabled: visible
                    layer.effect: MultiEffect {
                        colorization: 1.0
                        colorizationColor: Config.StaticData.palette.secondary.col100
                    }
                }
                AppText {
                    Layout.fillWidth: true
                    text: modelData
                    color: Config.StaticData.palette.secondary.col100
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                }
            }
            background: Rectangle {
                color: highlighted
                    ? Config.StaticData.palette.secondary.col500
                    : Config.StaticData.palette.secondary.col600
            }
            highlighted: combo.highlightedIndex === index
        }
        popup: Popup {
            y: combo.height
            width: combo.width
            padding: 0
            background: Rectangle {
                color: Config.StaticData.palette.secondary.col600
                border.color: Config.StaticData.palette.secondary.col300
                border.width: 1
                radius: 6
            }
            contentItem: ListView {
                implicitHeight: contentHeight
                model: combo.delegateModel
                clip: true
            }
        }
    }

    // ── Hilfsfunktion: gestylter TextField ───────────────────────────────────
    component StyledField: TextField {
        id: field
        font.family: Config.StaticData.loadedFont.font.family
        font.pixelSize: 12
        color: Config.StaticData.palette.secondary.col100
        implicitHeight: 36
        leftPadding: 8
        background: Rectangle {
            radius: 6
            color: Config.StaticData.palette.secondary.col600
            border.color: field.activeFocus
                ? Config.StaticData.palette.secondary.col200
                : Config.StaticData.palette.secondary.col400
            border.width: 1
        }
        placeholderTextColor: Config.StaticData.palette.secondary.col400
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        // Die vertikale Scrollleiste liegt als Overlay über dem Inhalt und ist
        // in availableWidth nicht enthalten - ohne Abzug schneidet sie bei
        // schmalem Fenster in den Text.
        readonly property real scrollBarSpace: ScrollBar.vertical.visible ? 12 : 0

        ColumnLayout {
            width: scrollView.availableWidth - scrollView.scrollBarSpace
            spacing: 0

            // ── Header ───────────────────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 56
                color: Config.StaticData.palette.secondary.col600

                RowLayout {
                    anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                    spacing: 10

                    CustomButton {
                        text: qsTr("← Zurück")
                        implicitWidth: 90
                        implicitHeight: 36
                        onClicked: mainStackView.pop()
                    }

                    AppLabel {
                        Layout.fillWidth: true
                        text: qsTr("Spiel erstellen")
                        color: Config.StaticData.palette.secondary.col100
                        font.pixelSize: 18
                        font.bold: true
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Config.StaticData.palette.secondary.col500
            }

            // ── Formular ─────────────────────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12
                Layout.bottomMargin: 8
                spacing: 12

                // Spielname
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    AppLabel {
                        text: qsTr("Spielname")
                        color: Config.StaticData.palette.secondary.col200
                        font.pixelSize: 12
                    }
                    StyledField {
                        id: gameNameField
                        Layout.fillWidth: true
                        enabled: !lobbyCreateGamePage.isGuest
                        maximumLength: 48
                        placeholderText: qsTr("Spielname eingeben …")
                        background: Rectangle {
                            radius: 6
                            color: Config.StaticData.palette.secondary.col600
                            border.color: gameNameField.activeFocus
                                ? Config.StaticData.palette.secondary.col200
                                : (lobbyCreateGamePage.nameError !== ""
                                    ? "#ef4444"
                                    : Config.StaticData.palette.secondary.col400)
                            border.width: 1
                        }
                        onTextChanged: {
                            if (text.trim().length > 0)
                                lobbyCreateGamePage.nameError = ""
                        }
                    }
                    AppLabel {
                        visible: lobbyCreateGamePage.nameError !== ""
                        text: lobbyCreateGamePage.nameError
                        color: "#ef4444"
                        font.pixelSize: 11
                    }
                }

                // Spieltyp
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    AppLabel {
                        text: qsTr("Spieltyp")
                        color: Config.StaticData.palette.secondary.col200
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                    }
                    StyledCombo {
                        id: gameTypeCombo
                        Layout.fillWidth: true
                        enabled: !lobbyCreateGamePage.isGuest
                        iconSources: [
                            "../resources/user.svg",
                            "../resources/userSquare.svg",
                            "../resources/users.svg",
                            "../resources/chipStack.svg"
                        ]
                        model: [
                            qsTr("Normal"),
                            qsTr("Nur registrierte Spieler"),
                            qsTr("Nur eingeladene Spieler"),
                            qsTr("Ranglistenspiel")
                        ]
                        onCurrentIndexChanged: {
                            // Beim Aufbau der Seite füllt Component.onCompleted
                            // das Formular; erst danach ist ein Wechsel echt.
                            if (!lobbyCreateGamePage.formReady)
                                return
                            // Vorlagen gelten nur für "Nur eingeladene Spieler":
                            // beim Wechsel des Spieltyps Vorlage zurücksetzen.
                            var hadPreset = presetCombo.currentIndex > 0
                            if (currentIndex !== 2)
                                presetCombo.currentIndex = 0
                            lobbyCreateGamePage.applyGameType()
                            // Die Vorlage hatte auch die Zeitlimits gesetzt.
                            if (hadPreset)
                                lobbyCreateGamePage.loadTimingSettings()
                        }
                    }
                }

                // Community-Vorlage (nur für Einladungsspiele bei aktiviertem
                // Community-Inhalt)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: Config.Parameters.showCommunityContent
                             && lobbyCreateGamePage.isInviteOnly
                    AppLabel {
                        text: qsTr("Community-Vorlage")
                        color: Config.StaticData.palette.secondary.col200
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                    }
                    StyledCombo {
                        id: presetCombo
                        Layout.fillWidth: true
                        model: [
                            qsTr("Eigene Einstellungen"),
                            "BBC Step 1", "BBC Step 2", "BBC Step 3", "BBC Step 4",
                            "Monthly Cup", "Monthly Cup Final",
                            "WEC", "WEC Monthly Final", "WEC Grand Final"
                        ]
                        onActivated: lobbyCreateGamePage.applyPreset()
                    }
                }

                // Passwort-Zeile
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    AppLabel {
                        text: qsTr("Passwort")
                        color: lobbyCreateGamePage.passwordAllowed
                            ? Config.StaticData.palette.secondary.col200
                            : Config.StaticData.palette.secondary.col400
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Switch {
                        id: passwordToggle
                        checked: false
                        enabled: lobbyCreateGamePage.passwordAllowed
                    }
                }
                StyledField {
                    id: passwordField
                    Layout.fillWidth: true
                    visible: passwordToggle.checked && lobbyCreateGamePage.passwordAllowed
                    echoMode: TextInput.Password
                    placeholderText: qsTr("Passwort eingeben …")
                    maximumLength: 48
                }

                // Zuschauer erlaubt
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    AppLabel {
                        text: qsTr("Zuschauer erlaubt")
                        color: lobbyCreateGamePage.isRanking
                            ? Config.StaticData.palette.secondary.col400
                            : Config.StaticData.palette.secondary.col200
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Switch {
                        id: spectatorsToggle
                        checked: true
                        enabled: !lobbyCreateGamePage.isRanking
                    }
                }

                // Max. Spieler
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    AppLabel {
                        text: qsTr("Max. Spieler")
                        color: lobbyCreateGamePage.fieldsLocked
                            ? Config.StaticData.palette.secondary.col400
                            : Config.StaticData.palette.secondary.col200
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                    }
                    CustomSpinBox {
                        id: maxPlayersSpinBox
                        from: 2
                        to: 10
                        value: 10
                        enabled: !lobbyCreateGamePage.fieldsLocked
                    }
                }

                // Startgeld
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    AppLabel {
                        text: qsTr("Startgeld")
                        color: lobbyCreateGamePage.fieldsLocked
                            ? Config.StaticData.palette.secondary.col400
                            : Config.StaticData.palette.secondary.col200
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                    }
                    CustomSpinBox {
                        id: startCashSpinBox
                        from: 1000
                        to: 1000000
                        stepSize: 50
                        value: 3000
                        enabled: !lobbyCreateGamePage.fieldsLocked
                        textFromValue: function(val) { return "$\u2009" + val }
                        valueFromText: function(text) { return parseInt(text.replace(/[^0-9]/g, "")) || 0 }
                    }
                }

                // ══ SECTION: Blind-Einstellungen ═════════════════════════════
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Config.StaticData.palette.secondary.col500
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                }

                AppLabel {
                    text: qsTr("Blind-Einstellungen")
                    color: Config.StaticData.palette.secondary.col300
                    font.pixelSize: 13
                    font.bold: true
                }

                // Erster Small Blind
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    AppLabel {
                        text: qsTr("Erster Small Blind")
                        color: lobbyCreateGamePage.fieldsLocked
                            ? Config.StaticData.palette.secondary.col400
                            : Config.StaticData.palette.secondary.col200
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                    }
                    CustomSpinBox {
                        id: firstBlindSpinBox
                        from: 5
                        to: 20000
                        value: 10
                        enabled: !lobbyCreateGamePage.fieldsLocked
                        textFromValue: function(val) { return "$\u2009" + val }
                        valueFromText: function(text) { return parseInt(text.replace(/[^0-9]/g, "")) || 0 }
                    }
                }

                // Erhöhungsintervall
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    AppLabel {
                        text: qsTr("Blind-Erhöhungsintervall")
                        color: lobbyCreateGamePage.fieldsLocked
                            ? Config.StaticData.palette.secondary.col400
                            : Config.StaticData.palette.secondary.col200
                        font.pixelSize: 12
                    }

                    // Die beiden Radios liegen in unterschiedlichen RowLayouts
                    // und wären ohne explizite Gruppe nicht wechselseitig
                    // exklusiv (autoExclusive wirkt nur unter Geschwistern).
                    ButtonGroup {
                        id: raiseIntervalGroup
                        buttons: [raiseByHandsRadio, raiseByMinutesRadio]
                    }

                    RowLayout {
                        spacing: 8
                        Layout.preferredHeight: 36
                        RadioButton {
                            id: raiseByHandsRadio
                            checked: true
                            enabled: !lobbyCreateGamePage.fieldsLocked
                            text: qsTr("Alle")
                        }
                        CustomSpinBox {
                            id: raiseEveryHandsSpinBox
                            from: 1
                            to: 999
                            value: 8
                            enabled: !lobbyCreateGamePage.fieldsLocked && raiseByHandsRadio.checked
                            implicitWidth: 110
                        }
                        AppLabel {
                            text: qsTr("Hände")
                            color: (raiseByHandsRadio.checked && !lobbyCreateGamePage.fieldsLocked)
                                ? Config.StaticData.palette.secondary.col200
                                : Config.StaticData.palette.secondary.col400
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }

                    RowLayout {
                        spacing: 8
                        Layout.preferredHeight: 36
                        RadioButton {
                            id: raiseByMinutesRadio
                            checked: false
                            enabled: !lobbyCreateGamePage.fieldsLocked
                            text: qsTr("Alle")
                        }
                        CustomSpinBox {
                            id: raiseEveryMinutesSpinBox
                            from: 1
                            to: 60
                            value: 5
                            enabled: !lobbyCreateGamePage.fieldsLocked && raiseByMinutesRadio.checked
                            implicitWidth: 110
                        }
                        AppLabel {
                            text: qsTr("Minuten")
                            color: (raiseByMinutesRadio.checked && !lobbyCreateGamePage.fieldsLocked)
                                ? Config.StaticData.palette.secondary.col200
                                : Config.StaticData.palette.secondary.col400
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }

                    // Feste Blindliste: aus der Community-Vorlage (BBC) oder aus
                    // der in den Optionen gespeicherten Blindreihenfolge.
                    AppLabel {
                        visible: !lobbyCreateGamePage.isRanking
                                 && lobbyCreateGamePage.effectiveBlinds.length > 0
                        Layout.fillWidth: true
                        text: visible
                            ? qsTr("Blindliste: %1").arg(lobbyCreateGamePage.effectiveBlinds.join(" · "))
                            : ""
                        color: Config.StaticData.palette.secondary.col300
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                    }
                }

                // ══ SECTION: Zeitlimits ═══════════════════════════════════════
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Config.StaticData.palette.secondary.col500
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                }

                AppLabel {
                    text: qsTr("Zeitlimits")
                    color: Config.StaticData.palette.secondary.col300
                    font.pixelSize: 13
                    font.bold: true
                }

                // Zeitlimit Spieleraktion
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    AppLabel {
                        text: qsTr("Zeitlimit Spieleraktion")
                        color: lobbyCreateGamePage.presetActive
                            ? Config.StaticData.palette.secondary.col400
                            : Config.StaticData.palette.secondary.col200
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                    }
                    CustomSpinBox {
                        id: playerActionTimeoutSpinBox
                        from: 5
                        to: 60
                        value: 20
                        enabled: !lobbyCreateGamePage.presetActive
                        textFromValue: function(val) { return val + "\u2009s" }
                        valueFromText: function(text) { return parseInt(text) || 0 }
                    }
                }

                // Pause zwischen Händen
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    AppLabel {
                        text: qsTr("Pause zwischen Händen")
                        color: lobbyCreateGamePage.presetActive
                            ? Config.StaticData.palette.secondary.col400
                            : Config.StaticData.palette.secondary.col200
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                    }
                    CustomSpinBox {
                        id: delayBetweenHandsSpinBox
                        from: 5
                        to: 20
                        value: 7
                        enabled: !lobbyCreateGamePage.presetActive
                        textFromValue: function(val) { return val + "\u2009s" }
                        valueFromText: function(text) { return parseInt(text) || 0 }
                    }
                }

                // ══ AKTIONEN ══════════════════════════════════════════════════
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Config.StaticData.palette.secondary.col500
                    Layout.topMargin: 8
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    spacing: 10

                    CustomButton {
                        text: qsTr("Abbrechen")
                        Layout.fillWidth: true
                        onClicked: mainStackView.pop()
                    }

                    CustomButton {
                        id: createBtn
                        text: qsTr("Spiel erstellen")
                        Layout.fillWidth: true
                        onClicked: {
                            // Validierung
                            if (gameNameField.text.trim().length === 0) {
                                lobbyCreateGamePage.nameError = qsTr("Bitte einen Spielnamen eingeben.")
                                return
                            }

                            // Ranking: Werte vom Server vorgegeben
                            var gType   = gameTypeCombo.currentIndex + 1  // 1-basiert
                            var maxP    = isRanking ? 10    : maxPlayersSpinBox.value
                            var sCash   = isRanking ? 10000 : startCashSpinBox.value
                            var fBlind  = isRanking ? 50    : firstBlindSpinBox.value
                            var riMode  = isRanking ? 1 : (raiseByHandsRadio.checked ? 1 : 2)
                            var rHands  = isRanking ? 11   : raiseEveryHandsSpinBox.value
                            var rMins   = raiseEveryMinutesSpinBox.value
                            // Feste Blindliste (Community-Vorlage oder gespeicherte
                            // Reihenfolge) → manuelle Blindreihenfolge, sonst
                            // immer verdoppeln. Ranglistenspiele verdoppeln stets.
                            var blinds  = isRanking ? [] : lobbyCreateGamePage.effectiveBlinds
                            var rMode   = blinds.length > 0 ? 2 : 1  // MANUAL_BLINDS_ORDER : DOUBLE_BLINDS
                            var specs   = isRanking ? true : spectatorsToggle.checked
                            var pw      = (passwordToggle.checked && passwordAllowed) ? passwordField.text : ""

                            // Suggest-Typ des erstellten Spiels merken (explizit
                            // aus dem Preset, NICHT aus dem – frei editierbaren –
                            // Namen). Ohne Community-Preset: kein Suggest.
                            Config.BotSuggest.createdSuggestType =
                                (activePreset && activePreset.suggestType)
                                    ? activePreset.suggestType : ""

                            Lobby.createGame(
                                gameNameField.text.trim(),
                                pw,
                                gType,
                                specs,
                                maxP,
                                sCash,
                                fBlind,
                                riMode,
                                rHands,
                                rMins,
                                rMode,
                                playerActionTimeoutSpinBox.value,
                                delayBetweenHandsSpinBox.value,
                                blinds
                            )
                            // Navigation erfolgt über onSelfJoinedGame in LobbyPage
                        }
                    }
                }

            }
        }
    }
}
