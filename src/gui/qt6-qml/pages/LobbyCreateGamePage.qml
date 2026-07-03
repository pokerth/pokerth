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
    property string nameError: ""

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

        ColumnLayout {
            width: scrollView.availableWidth
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
                        text: qsTr("My Online Game")
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
                    }
                }

                // Passwort-Zeile
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    AppLabel {
                        text: qsTr("Passwort")
                        color: lobbyCreateGamePage.isRanking
                            ? Config.StaticData.palette.secondary.col400
                            : Config.StaticData.palette.secondary.col200
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Switch {
                        id: passwordToggle
                        checked: false
                        enabled: !lobbyCreateGamePage.isRanking
                    }
                }
                StyledField {
                    id: passwordField
                    Layout.fillWidth: true
                    visible: passwordToggle.checked && !lobbyCreateGamePage.isRanking
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
                        color: lobbyCreateGamePage.isRanking
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
                        value: lobbyCreateGamePage.isRanking ? 10 : 10
                        enabled: !lobbyCreateGamePage.isRanking
                    }
                }

                // Startgeld
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    AppLabel {
                        text: qsTr("Startgeld")
                        color: lobbyCreateGamePage.isRanking
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
                        value: lobbyCreateGamePage.isRanking ? 10000 : 3000
                        enabled: !lobbyCreateGamePage.isRanking
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
                        color: lobbyCreateGamePage.isRanking
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
                        value: lobbyCreateGamePage.isRanking ? 50 : 10
                        enabled: !lobbyCreateGamePage.isRanking
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
                        color: lobbyCreateGamePage.isRanking
                            ? Config.StaticData.palette.secondary.col400
                            : Config.StaticData.palette.secondary.col200
                        font.pixelSize: 12
                    }

                    RowLayout {
                        spacing: 8
                        Layout.preferredHeight: 36
                        RadioButton {
                            id: raiseByHandsRadio
                            checked: true
                            enabled: !lobbyCreateGamePage.isRanking
                            text: qsTr("Alle")
                        }
                        CustomSpinBox {
                            id: raiseEveryHandsSpinBox
                            from: 1
                            to: 999
                            value: lobbyCreateGamePage.isRanking ? 11 : 8
                            enabled: !lobbyCreateGamePage.isRanking && raiseByHandsRadio.checked
                            implicitWidth: 110
                        }
                        AppLabel {
                            text: qsTr("Hände")
                            color: (raiseByHandsRadio.checked && !lobbyCreateGamePage.isRanking)
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
                            enabled: !lobbyCreateGamePage.isRanking
                            text: qsTr("Alle")
                        }
                        CustomSpinBox {
                            id: raiseEveryMinutesSpinBox
                            from: 1
                            to: 60
                            value: 5
                            enabled: !lobbyCreateGamePage.isRanking && raiseByMinutesRadio.checked
                            implicitWidth: 110
                        }
                        AppLabel {
                            text: qsTr("Minuten")
                            color: (raiseByMinutesRadio.checked && !lobbyCreateGamePage.isRanking)
                                ? Config.StaticData.palette.secondary.col200
                                : Config.StaticData.palette.secondary.col400
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            Layout.alignment: Qt.AlignVCenter
                        }
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
                        color: Config.StaticData.palette.secondary.col200
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
                        color: Config.StaticData.palette.secondary.col200
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
                            var rMode   = 1  // immer verdoppeln (DOUBLE_BLINDS)
                            var specs   = isRanking ? true : spectatorsToggle.checked
                            var pw      = (passwordToggle.checked && !isRanking) ? passwordField.text : ""

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
                                delayBetweenHandsSpinBox.value
                            )
                            // Navigation erfolgt über onSelfJoinedGame in LobbyPage
                        }
                    }
                }

            }
        }
    }
}
