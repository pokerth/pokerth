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

    // Ranking constants (given by the server)
    readonly property bool isRanking: gameTypeCombo.currentIndex === 3
    readonly property bool isInviteOnly: gameTypeCombo.currentIndex === 2
    // On the server side, guests may only open standard games with their own
    // player name (PLAYER_RIGHTS_GUEST, NTF_NET_JOIN_GUEST_FORBIDDEN).
    readonly property bool isGuest: Lobby ? Lobby.isMyPlayerGuest : false
    // Ranking: the server forbids a password; invitation games control
    // access via the invitation.
    readonly property bool passwordAllowed: !isRanking && !isInviteOnly
    property string nameError: ""

    // Default button of the page: Enter creates the game, no matter where the focus is.
    // A focused button consumes Return itself and keeps precedence.
    Keys.onReturnPressed: createBtn.clicked()
    Keys.onEnterPressed: createBtn.clicked()

    // Initial focus into the game name field – NOT on mobile devices, that would
    // pull up the on-screen keyboard unasked. Guests may not change the name.
    StackView.onActivated: {
        if (!Config.Responsive.isMobile && !lobbyCreateGamePage.isGuest)
            Qt.callLater(gameNameField.forceActiveFocus)
    }
    // The input fields only react to each other after the form has been filled
    // (ComboBox signals already fire while the page is being built).
    property bool formReady: false

    // ── Community templates (BBC / monthly cup / WEC) ────────────────────────
    // Official tournament settings of the PokerTH community. Selectable only for custom
    // games of the type "invited players only" and only with community
    // content enabled. BBC steps use a fixed blind list (raised every
    // 5 minutes), the monthly cup and WEC double the blinds after a number of hands.
    // The community templates live in the singleton Config.BotSuggest – the same
    // table serves there to detect the type of foreign tables (a settings fingerprint),
    // so it must not be maintained twice.
    readonly property var communityPresets: Config.BotSuggest.presets
    readonly property var activePreset: (Config.Parameters.showCommunityContent
                                         && isInviteOnly
                                         && presetCombo.currentIndex > 0)
        ? communityPresets[presetCombo.currentIndex - 1] : null
    readonly property bool presetActive: activePreset !== null
    // Locks the fields given by the server (ranking) or by the template.
    readonly property bool fieldsLocked: isRanking || presetActive

    // The manual blind order stored in the network game options.
    property bool savedManualBlindsOrder: false
    property var savedManualBlinds: []

    // Blind list of the game: a community template trumps the stored
    // order. An empty list means "double the blinds".
    readonly property var effectiveBlinds: presetActive
        ? activePreset.blinds
        : (savedManualBlindsOrder ? savedManualBlinds : [])

    // Transfers the selected template into the form fields or, for
    // "custom settings", restores the values from the options.
    function applyPreset() {
        var p = activePreset
        if (!p) {
            applyGameType()
            loadTimingSettings()
            return
        }
        gameNameField.text = p.name
        // Templates with a monthly changing table name (monthly cup): pull the current
        // title prefix from the botfiles and – provided the user has not changed the
        // (fallback) name themselves and the same template is still
        // selected – adopt it. Asynchronous; on failure p.name stays.
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
        delayBetweenHandsSpinBox.value = 7   // all templates: DelayBetweenHands=7
    }

    // Time limits from the network game options. As in the widget
    // client they do not depend on the game type and are adopted only once.
    function loadTimingSettings() {
        if (!SettingsManager)
            return
        playerActionTimeoutSpinBox.value = SettingsManager.readConfigInt("NetTimeOutPlayerAction")
        delayBetweenHandsSpinBox.value   = SettingsManager.readConfigInt("NetDelayBetweenHands")
    }

    // Table and blind settings from the network/internet game options.
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
        // readConfigIntList() delivers a QList<int> sequence; as a real
        // JS array it is stable and can be evaluated via join()/length.
        var saved = SettingsManager.readConfigIntList("NetManualBlindsList")
        var blinds = []
        for (var i = 0; i < saved.length; ++i)
            blinds.push(saved[i])
        savedManualBlinds = blinds
    }

    // Counterpart to createInternetGameDialogImpl::gameTypeChanged(): ranking
    // games are given by the server, all other game types start with
    // the stored settings.
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

    // Counterpart to createInternetGameDialogImpl::fillFormular().
    Component.onCompleted: {
        if (isGuest) {
            // Guests always open a standard game under their player name.
            gameTypeCombo.currentIndex = 0
            gameNameField.text = qsTr("%1's game").arg(Lobby ? Lobby.myPlayerName : "")
        } else if (SettingsManager) {
            var type = SettingsManager.readConfigInt("InternetGameType")
            gameTypeCombo.currentIndex = (type >= 0 && type <= 3) ? type : 0
            var name = SettingsManager.readConfigString("InternetGameName")
            if (name.trim().length > 0)
                gameNameField.text = name
        }
        // Also needed when the stored game type corresponds to the default index 0
        // and onCurrentIndexChanged therefore does not fire.
        applyGameType()
        loadTimingSettings()
        if (SettingsManager && passwordAllowed
                && SettingsManager.readConfigInt("UseInternetGamePassword")) {
            passwordToggle.checked = true
            passwordField.text = SettingsManager.readConfigString("InternetGamePassword")
        }
        // Fetch the titles of the community games already now: when selecting
        // a monthly cup template the current name (e.g. "August Cup
        // Final") then stands in the field immediately instead of being delivered asynchronously –
        // otherwise a quick click on "create game" sends the
        // template fallback name.
        if (Config.Parameters.showCommunityContent)
            Config.BotSuggest.prefetchGameTitles()
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
        // The vertical scrollbar lies as an overlay above the content and is
        // not included in availableWidth - without subtracting it, it cuts into
        // the text in a narrow window.
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
                            // While the page is being built, Component.onCompleted fills
                            // the form; only after that is a change a real one.
                            if (!lobbyCreateGamePage.formReady)
                                return
                            // Templates only apply to "invited players only":
                            // reset the template when the game type changes.
                            var hadPreset = presetCombo.currentIndex > 0
                            if (currentIndex !== 2)
                                presetCombo.currentIndex = 0
                            lobbyCreateGamePage.applyGameType()
                            // The template had set the time limits as well.
                            if (hadPreset)
                                lobbyCreateGamePage.loadTimingSettings()
                        }
                    }
                }

                // Community template (only for invitation games with community
                // content enabled)
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

                // Password row
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

                // Max. players
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

                // Raise interval
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

                    // The two radios lie in different RowLayouts
                    // and without an explicit group would not be mutually
                    // exclusive (autoExclusive only works among siblings).
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

                    // Fixed blind list: from the community template (BBC) or from
                    // the blind order stored in the options.
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

                // Pause between hands
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

                            // Ranking: the values are given by the server
                            var gType   = gameTypeCombo.currentIndex + 1  // 1 based
                            var maxP    = isRanking ? 10    : maxPlayersSpinBox.value
                            var sCash   = isRanking ? 10000 : startCashSpinBox.value
                            var fBlind  = isRanking ? 50    : firstBlindSpinBox.value
                            var riMode  = isRanking ? 1 : (raiseByHandsRadio.checked ? 1 : 2)
                            var rHands  = isRanking ? 11   : raiseEveryHandsSpinBox.value
                            var rMins   = raiseEveryMinutesSpinBox.value
                            // A fixed blind list (community template or the stored
                            // order) → a manual blind order, otherwise
                            // always double. Ranking games always double.
                            var blinds  = isRanking ? [] : lobbyCreateGamePage.effectiveBlinds
                            var rMode   = blinds.length > 0 ? 2 : 1  // MANUAL_BLINDS_ORDER : DOUBLE_BLINDS
                            var specs   = isRanking ? true : spectatorsToggle.checked
                            var pw      = (passwordToggle.checked && passwordAllowed) ? passwordField.text : ""

                            // Remember the suggest type of the created game (explicitly
                            // from the preset, NOT from the – freely editable –
                            // name). Without a community preset: no suggest.
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
                            // The navigation happens via onSelfJoinedGame in LobbyPage
                        }
                    }
                }

            }
        }
    }
}
