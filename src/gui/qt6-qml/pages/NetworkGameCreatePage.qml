import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// "Netzwerkspiel erstellen" – portiert aus dem Qt-Widgets createNetworkGameDialog.
// Konfiguriert die Spielregeln und startet einen eingebetteten lokalen Server
// (NetworkGame.createGame), der das Spiel hostet und den Host über den
// bestehenden Lobby-/Warteraum-Fluss (ServerConnection.showLobby → LobbyPage →
// GameWaitPage) führt.
Rectangle {
    id: networkGameCreatePage
    objectName: "networkGameCreatePage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    property bool connecting: false
    property string statusMessage: ""

    function cfgInt(key, dflt) {
        if (typeof SettingsManager !== "undefined" && SettingsManager) {
            var v = SettingsManager.readConfigInt(key)
            if (v !== undefined && v !== null)
                return v
        }
        return dflt
    }

    Component.onCompleted: {
        maxPlayersSpinBox.value      = cfgInt("NetNumberOfPlayers", 10)
        startCashSpinBox.value       = cfgInt("NetStartCash", 3000)
        firstBlindSpinBox.value      = cfgInt("NetFirstSmallBlind", 10)
        raiseEveryHandsSpinBox.value = cfgInt("NetRaiseSmallBlindEveryHands", 8)
        raiseEveryMinutesSpinBox.value = cfgInt("NetRaiseSmallBlindEveryMinutes", 5)
        var byHands = cfgInt("NetRaiseBlindsAtHands", 1) === 1
        raiseByHandsRadio.checked = byHands
        raiseByMinutesRadio.checked = !byHands
        doubleBlindsSwitch.checked = cfgInt("NetAlwaysDoubleBlinds", 1) === 1
        timeoutSpinBox.value = cfgInt("NetTimeOutPlayerAction", 20)
        delaySpinBox.value   = cfgInt("NetDelayBetweenHands", 7)
    }

    // ── Backend-Signale: Navigation in den Lobby-/Warteraum-Fluss ─────────────
    Connections {
        target: (typeof ServerConnection !== "undefined") ? ServerConnection : null
        function onShowLobby() {
            mainStackView.replace(mainStackView.currentItem, "LobbyPage.qml")
        }
        function onConnectionFailed(errorMessage) {
            networkGameCreatePage.connecting = false
            networkGameCreatePage.statusMessage = errorMessage
        }
    }
    Connections {
        target: (typeof NetworkGame !== "undefined") ? NetworkGame : null
        function onHostingFailed(message) {
            networkGameCreatePage.connecting = false
            networkGameCreatePage.statusMessage = message
        }
    }

    function startHosting() {
        if (typeof NetworkGame === "undefined" || !NetworkGame) return
        networkGameCreatePage.statusMessage = ""
        networkGameCreatePage.connecting = true
        NetworkGame.createGame(
            maxPlayersSpinBox.value,
            startCashSpinBox.value,
            firstBlindSpinBox.value,
            raiseByHandsRadio.checked,
            raiseEveryHandsSpinBox.value,
            raiseEveryMinutesSpinBox.value,
            doubleBlindsSwitch.checked,
            timeoutSpinBox.value,
            delaySpinBox.value)
    }

    // ── Formular ──────────────────────────────────────────────────────────────
    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        visible: !networkGameCreatePage.connecting
        // AlwaysOff statt transientem Default: sonst blitzt die Scrollbar beim
        // Seitenaufbau sekundenlang auf, obwohl nichts zu scrollen ist.
        ScrollBar.vertical.policy: scrollView.contentHeight > scrollView.height + 1
                                   ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 0

            // Header
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
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Netzwerkspiel erstellen")
                        color: Config.StaticData.palette.secondary.col100
                        font.family: Config.StaticData.loadedFont.font.family
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

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12
                Layout.bottomMargin: 8
                spacing: 12

                // ── Spiel-Einstellungen ──────────────────────────────────────
                Label {
                    text: qsTr("Spiel-Einstellungen")
                    color: Config.StaticData.palette.secondary.col300
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: 13
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Max. Spieler")
                        color: Config.StaticData.palette.secondary.col200
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                    }
                    CustomSpinBox {
                        id: maxPlayersSpinBox
                        from: 2
                        to: 10
                        value: 10
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Startgeld")
                        color: Config.StaticData.palette.secondary.col200
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                    }
                    CustomSpinBox {
                        id: startCashSpinBox
                        from: 1000
                        to: 1000000
                        stepSize: 50
                        value: 3000
                        textFromValue: function(val) { return "$ " + val }
                        valueFromText: function(text) { return parseInt(text.replace(/[^0-9]/g, "")) || 0 }
                    }
                }

                // ── Blind-Einstellungen ──────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Config.StaticData.palette.secondary.col500
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                }
                Label {
                    text: qsTr("Blind-Einstellungen")
                    color: Config.StaticData.palette.secondary.col300
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: 13
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Erster Small Blind")
                        color: Config.StaticData.palette.secondary.col200
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                    }
                    CustomSpinBox {
                        id: firstBlindSpinBox
                        from: 5
                        to: 20000
                        value: 10
                        textFromValue: function(val) { return "$ " + val }
                        valueFromText: function(text) { return parseInt(text.replace(/[^0-9]/g, "")) || 0 }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Label {
                        text: qsTr("Blind-Erhöhungsintervall")
                        color: Config.StaticData.palette.secondary.col200
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 12
                    }
                    RowLayout {
                        spacing: 8
                        Layout.preferredHeight: 36
                        RadioButton {
                            id: raiseByHandsRadio
                            checked: true
                            text: qsTr("Alle")
                        }
                        CustomSpinBox {
                            id: raiseEveryHandsSpinBox
                            from: 1
                            to: 999
                            value: 8
                            enabled: raiseByHandsRadio.checked
                            implicitWidth: 110
                        }
                        Label {
                            text: qsTr("Hände")
                            color: raiseByHandsRadio.checked
                                ? Config.StaticData.palette.secondary.col200
                                : Config.StaticData.palette.secondary.col400
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    RowLayout {
                        spacing: 8
                        Layout.preferredHeight: 36
                        RadioButton {
                            id: raiseByMinutesRadio
                            checked: false
                            text: qsTr("Alle")
                        }
                        CustomSpinBox {
                            id: raiseEveryMinutesSpinBox
                            from: 1
                            to: 60
                            value: 5
                            enabled: raiseByMinutesRadio.checked
                            implicitWidth: 110
                        }
                        Label {
                            text: qsTr("Minuten")
                            color: raiseByMinutesRadio.checked
                                ? Config.StaticData.palette.secondary.col200
                                : Config.StaticData.palette.secondary.col400
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Blinds immer verdoppeln")
                        color: Config.StaticData.palette.secondary.col200
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                    }
                    Switch {
                        id: doubleBlindsSwitch
                        checked: true
                    }
                }

                // ── Zeitlimits ───────────────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Config.StaticData.palette.secondary.col500
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                }
                Label {
                    text: qsTr("Zeitlimits")
                    color: Config.StaticData.palette.secondary.col300
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: 13
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Zeitlimit Spieleraktion")
                        color: Config.StaticData.palette.secondary.col200
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                    }
                    CustomSpinBox {
                        id: timeoutSpinBox
                        from: 5
                        to: 60
                        value: 20
                        textFromValue: function(val) { return val + " s" }
                        valueFromText: function(text) { return parseInt(text) || 0 }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Pause zwischen Händen")
                        color: Config.StaticData.palette.secondary.col200
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                    }
                    CustomSpinBox {
                        id: delaySpinBox
                        from: 5
                        to: 20
                        value: 7
                        textFromValue: function(val) { return val + " s" }
                        valueFromText: function(text) { return parseInt(text) || 0 }
                    }
                }

                Label {
                    visible: networkGameCreatePage.statusMessage !== ""
                    text: networkGameCreatePage.statusMessage
                    color: "#ef4444"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: 12
                }

                // ── Aktionen ─────────────────────────────────────────────────
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
                        text: qsTr("Spiel erstellen")
                        Layout.fillWidth: true
                        onClicked: networkGameCreatePage.startHosting()
                    }
                }
            }
        }
    }

    // ── Verbindungsaufbau-Ansicht ─────────────────────────────────────────────
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.8, 360)
        spacing: 20
        visible: networkGameCreatePage.connecting

        BusyIndicator {
            running: networkGameCreatePage.connecting
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 48
            implicitHeight: 48
        }
        Label {
            text: qsTr("Server wird gestartet …")
            Layout.alignment: Qt.AlignHCenter
            color: Config.StaticData.palette.secondary.col200
            font.family: Config.StaticData.loadedFont.font.family
            font.pixelSize: Config.Theme.fontSizeTitle
            font.bold: true
        }
        CustomButton {
            text: qsTr("Abbrechen")
            Layout.fillWidth: true
            onClicked: {
                if (typeof ServerConnection !== "undefined" && ServerConnection)
                    ServerConnection.cancelConnection()
                networkGameCreatePage.connecting = false
            }
        }
    }
}
