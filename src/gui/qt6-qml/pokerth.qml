pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.VectorImage
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts
import QtQuick.Effects

import "config" as Config
import "pages"
import "components"

ApplicationWindow {
    id: mainWindow

    Universal.theme: Config.StaticData.isDark ? Universal.Dark : Universal.Light

    // portraitMode is now provided by Config.Responsive.portrait
    property StartPage startPage: StartPage {}
    property SideMenu sideMenu: SideMenu {}
    // Start-Auflösung = Default-Größe des Qt-Widgets-Clients am Gametable
    // (gametable.ui: 1024×621). Beim Komponenten-Aufbau wird die Größe
    // zusätzlich auf den verfügbaren Bildschirm geclampt.
    width: 1024
    height: 621
    // Initiale Portrait-Breite als untere Schranke – das Fenster darf nicht
    // schmaler werden als der Standard-Portrait-Modus, damit das Layout
    // (Slot-Spalten, Self-Box, Action-Buttons) immer komplett ins Bild passt.
    minimumWidth: 390
    minimumHeight: 600
    // TRY to center the window, doesn't work on my Ubuntu but should work on other platforms.
    visible: true
    title: qsTr("PokerTH - v2.1.0preview")

    // Android hardware back button: intercept close and navigate back instead
    // of destroying the QML scene while background threads are still running.
    onClosing: (close) => {
        if (mainStackView.depth > 1) {
            close.accepted = false
            navigateBackFromTopBar()
        }
        // depth === 1: allow close → app.exec() returns → proper C++ cleanup
    }

    // Keep Responsive singleton in sync with the actual window dimensions
    onWidthChanged: {
        Config.Responsive.windowWidth = width
        Config.Theme.windowWidth      = width
    }
    onHeightChanged: {
        Config.Responsive.windowHeight = height
        Config.Theme.windowHeight      = height
    }

    Component.onCompleted: {
        // Aspect-erhaltender Clamp auf den verfügbaren Bildschirm. 2316×1080
        // ist die Phone-Landscape-Testgröße (Aspect 2.144); auf Notebooks mit
        // 1920×1080 oder 2560×1440 würde das Fenster sonst entweder rausragen
        // oder sein Seitenverhältnis verlieren — beides hebelt den
        // landscapeCompact-Modus aus (Aspect-Schwelle 1.85).
        if (screen) {
            var maxW = screen.width  - 20
            var maxH = screen.height - 60   // Taskleiste/Titelbar
            var scale = Math.min(maxW / width, maxH / height, 1.0)
            if (scale < 1.0) {
                width  = Math.max(minimumWidth,  Math.floor(width  * scale))
                height = Math.max(minimumHeight, Math.floor(height * scale))
            }
        }
        Config.Responsive.windowWidth  = width
        Config.Responsive.windowHeight = height
        Config.Theme.windowWidth       = width
        Config.Theme.windowHeight      = height
        x = screen.width / 2 - width / 2
        y = screen.height / 2 - height / 2
        LanguageManager.switchLanguage(Config.Parameters.language)
        // Initialise dark/light mode from stored preference
        var dm = SettingsManager ? SettingsManager.readConfigInt("DarkMode") : 1
        Config.StaticData.darkMode = dm
        Config.Theme.darkMode = dm
    }

    function navigateBackFromTopBar() {
        if (mainStackView.depth <= 1)
            return false

        var current = mainStackView.currentItem

        // Warteraum: das Spiel sauber über den Server verlassen (wie der
        // "Leave Game"-Button). Der StackView wird durch onRemovedFromGame
        // gepoppt – hier NICHT direkt poppen.
        if (current && current.objectName === "gameWaitPage") {
            if (typeof Lobby !== "undefined" && Lobby)
                Lobby.leaveGame()
            return true
        }

        // Laufendes Spiel: vor dem Verlassen IMMER nachfragen (egal ob per
        // Esc, Android-Back oder Tür-Icon), damit ein versehentlicher
        // Tastendruck das Spiel nicht ungewollt beendet. Das eigentliche
        // Verlassen erledigt performLeaveGame() nach Bestätigung.
        if (current && current.objectName === "gamePage") {
            leaveGameConfirmPopup.open()
            return true
        }

        mainStackView.pop()
        return true
    }

    function performLeaveGame() {
        var current = mainStackView.currentItem
        console.log("[NAV] performLeaveGame | currentItem:", current ? (current.objectName || current.toString()) : "null", "| depth:", mainStackView.depth)
        var isGamePage = current && current.objectName === "gamePage"
        var localGame = isGamePage
                        && (typeof GameTable !== "undefined")
                        && GameTable
                        && GameTable.isLocalGameRunning()

        // Laufendes Netzwerkspiel: serverseitig verlassen und zurück in die
        // LOBBY (nicht in den darunterliegenden Warteraum). Der StackView wird
        // durch onRemovedFromGame bis zur Lobby gepoppt – hier NICHT poppen.
        if (isGamePage && !localGame) {
            if (typeof Lobby !== "undefined" && Lobby)
                Lobby.leaveGame()
            return
        }

        if (localGame)
            GameTable.endLocalGame()

        mainStackView.pop()
        if (localGame && mainStackView.depth > 1)
            mainStackView.pop()
    }
    
    Rectangle {
        anchors.fill: parent
        color: Config.StaticData.palette.secondary.col700
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        Layout.alignment: Qt.AlignTop
        spacing: 0

        Rectangle {
            id: topBar
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 38
            Layout.alignment: Qt.AlignTop
            color: Config.StaticData.palette.secondary.col700

            RowLayout {
                id: topBarColumns
                anchors.fill: parent
                spacing: 8

                VectorImage {
                    id: topBarMenuIcon
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 26
                    Layout.margins: 6
                    source: "resources/threeLines.svg"
                    visible: true
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        colorization: 1.0
                        colorizationColor: menuArea.containsMouse
                            ? Config.StaticData.palette.secondary.col100
                            : Config.StaticData.palette.secondary.col200
                    }

                    MouseArea {
                        id: menuArea
                        anchors.fill: topBarMenuIcon
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: {
                            if (!navigateBackFromTopBar()) {
                                topBarMenuIcon.source = !sideMenu.visible ? "resources/caretLeft.svg" : "resources/threeLines.svg";
                                sideMenu.visible = !sideMenu.visible;
                            }
                        }
                    }
                }

                Item {
                    id: topBarMenuSpace
                    Layout.fillWidth: true
                    Layout.horizontalStretchFactor: 2
                }

                VectorImage {
                    id: topBarSettingsIcon
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    Layout.margins: 6
                    source: "resources/settings.svg"
                    visible: true
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        colorization: 1.0
                        colorizationColor: settingsArea.containsMouse
                            ? Config.StaticData.palette.secondary.col100
                            : Config.StaticData.palette.secondary.col200
                    }

                    MouseArea {
                        id: settingsArea
                        anchors.fill: topBarSettingsIcon
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: {
                            mainStackView.push("pages/SettingsPage.qml");
                            sideMenu.visible = false;
                        }

                    }
                }
            }
        }

        StackView {
            id: mainStackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            initialItem: PreLoader {}

            replaceEnter: Transition {
                YAnimator {
                    from: (mainStackView.mirrored ? -1 : 1) * -mainStackView.height
                    to: 0
                    duration: 400
                    easing.type: Easing.OutCubic
                }
            }

            replaceExit: Transition {
                YAnimator {
                    from: 0
                    to: (mainStackView.mirrored ? -1 : 1) * mainStackView.height
                    duration: 400
                    easing.type: Easing.OutCubic
                }
            }

            onCurrentItemChanged: {
                // console.log("[NAV] Stack depth:", depth, "| currentItem:", currentItem ? (currentItem.objectName || currentItem.toString()) : "null")
                var isLobby = (currentItem && currentItem.objectName === "lobbyPage");
                var isGame  = (currentItem && currentItem.objectName === "gamePage");
                var isGameWait = (currentItem && currentItem.objectName === "gameWaitPage");
                if (depth <= 1) {
                    topBarSettingsIcon.visible = true;
                    topBarMenuIcon.source = sideMenu.visible ? "resources/caretLeft.svg" : "resources/threeLines.svg";
                } else if (isLobby || isGame || isGameWait) {
                    // Lobby, Spiel UND Warteraum: Tür-Icon zum Verlassen.
                    topBarSettingsIcon.visible = true;
                    topBarMenuIcon.source = "resources/doorExit.svg";
                } else {
                    topBarSettingsIcon.visible = true;
                    topBarMenuIcon.source = "resources/caretLeft.svg";
                }
                // Bildschirm während Spiel und Warteraum wach halten (Android:
                // FLAG_KEEP_SCREEN_ON via JNI). Beim Verlassen freigeben.
                ScreenHelper.setKeepScreenOn(isGame || isGameWait);
            }
        }
    }

    // ── Tastenkürzel ──────────────────────────────────────────────────────────
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (!navigateBackFromTopBar() && sideMenu.visible) {
                sideMenu.visible = false
                topBarMenuIcon.source = "resources/threeLines.svg"
            }
        }
    }

    Shortcut {
        sequence: StandardKey.Back
        onActivated: {
            navigateBackFromTopBar()
        }
    }

    Shortcut {
        sequence: "Alt+S"
        onActivated: {
            if (mainStackView.depth === 1) {
                mainStackView.push("pages/SettingsPage.qml")
                sideMenu.visible = false
            }
        }
    }

    SideMenu {}

    Connections {
        target: mainStackView
        Component.onDestruction: topBarMenuIcon.source = mainStackView.depth === 1 ? "resources/threeLines.svg" : "resources/caretLeft.svg"
    }

    // Re-apply FLAG_KEEP_SCREEN_ON when the app returns to the foreground.
    // Android may clear window flags during lifecycle transitions (pause/resume),
    // so we can't rely solely on the one-time call from onCurrentItemChanged.
    // ── AFK-Timeout-Warnung (Port von timeoutMsgBoxImpl, Lobby wie ingame) ──
    // Erscheint global über allen Seiten; OK stoppt den Server-Countdown
    // (resetNetworkTimeout). Der Beep kommt aus LobbyHandler::onTimeoutWarning.
    Popup {
        id: timeoutWarningPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 20
        closePolicy: Popup.CloseOnEscape

        property int reason: 0          // NetTimeoutReason
        property int remainingSec: 0
        property bool expired: false

        function show(theReason, sec) {
            reason = theReason
            remainingSec = sec
            expired = false
            open()
        }

        Timer {
            interval: 1000
            running: timeoutWarningPopup.opened && !timeoutWarningPopup.expired
            repeat: true
            onTriggered: {
                if (timeoutWarningPopup.remainingSec > 0)
                    timeoutWarningPopup.remainingSec--
                if (timeoutWarningPopup.remainingSec <= 0)
                    timeoutWarningPopup.expired = true
            }
        }

        background: Rectangle {
            color: Config.StaticData.palette.secondary.col700
            border.color: Config.StaticData.palette.secondary.col400
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            spacing: 12
            width: Math.min(mainWindow.width * 0.85, 380)

            Label {
                Layout.fillWidth: true
                text: qsTr("Timeout Warning")
                color: Config.StaticData.palette.secondary.col100
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 15
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                // Texte 1:1 wie timeoutMsgBoxImpl::timerRefresh.
                text: {
                    if (timeoutWarningPopup.expired)
                        return timeoutWarningPopup.reason === 2
                               ? qsTr("Timeout expired. You are being removed from the game.")
                               : qsTr("Timeout expired. You will be disconnected.")
                    if (timeoutWarningPopup.reason === 1)
                        return qsTr("You are game-admin of an open game which will time out in %1 seconds.")
                               .arg(timeoutWarningPopup.remainingSec)
                    if (timeoutWarningPopup.reason === 2)
                        return qsTr("You did not act in the game recently. You will be removed from the game in %1 seconds.")
                               .arg(timeoutWarningPopup.remainingSec)
                    return qsTr("Your connection is about to time out due to inactivity in %1 seconds.")
                           .arg(timeoutWarningPopup.remainingSec)
                }
                color: Config.StaticData.palette.secondary.col200
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            Label {
                Layout.fillWidth: true
                visible: !timeoutWarningPopup.expired
                text: qsTr("Please click \"OK\" to stop the countdown!")
                color: Config.StaticData.palette.secondary.col300
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
            CustomButton {
                Layout.fillWidth: true
                text: qsTr("OK")
                enabled: !timeoutWarningPopup.expired
                onClicked: {
                    Lobby.resetNetworkTimeout()
                    timeoutWarningPopup.close()
                }
            }
        }
    }

    // ── Server-Meldung (Port von startWindowImpl::networkMessage) ──────────
    Popup {
        id: networkMessagePopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 20
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property string message: ""

        background: Rectangle {
            color: Config.StaticData.palette.secondary.col700
            border.color: Config.StaticData.palette.secondary.col400
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            spacing: 12
            width: Math.min(mainWindow.width * 0.85, 380)

            Label {
                Layout.fillWidth: true
                text: qsTr("Server Message")
                color: Config.StaticData.palette.secondary.col100
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 15
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                text: networkMessagePopup.message
                textFormat: Text.RichText
                color: Config.StaticData.palette.secondary.col200
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            CustomButton {
                Layout.fillWidth: true
                text: qsTr("Close")
                onClicked: networkMessagePopup.close()
            }
        }
    }

    // ── Bestätigung beim Verlassen eines laufenden Spiels ─────────────────
    // Erscheint bei Esc / Android-Back / Tür-Icon, solange man sich auf der
    // GamePage befindet, damit ein versehentlicher Tastendruck nicht
    // ungewollt das laufende Spiel beendet.
    Popup {
        id: leaveGameConfirmPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 20
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: Config.StaticData.palette.secondary.col700
            border.color: Config.StaticData.palette.secondary.col400
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            spacing: 12
            width: Math.min(mainWindow.width * 0.85, 380)

            Label {
                Layout.fillWidth: true
                text: qsTr("Leave Game")
                color: Config.StaticData.palette.secondary.col100
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 15
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Attention! Do you really want to leave the current game\nand go back to the lobby?")
                color: Config.StaticData.palette.secondary.col200
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                CustomButton {
                    Layout.fillWidth: true
                    text: qsTr("Cancel")
                    onClicked: leaveGameConfirmPopup.close()
                }
                CustomButton {
                    Layout.fillWidth: true
                    text: qsTr("Leave Game")
                    onClicked: {
                        leaveGameConfirmPopup.close()
                        mainWindow.performLeaveGame()
                    }
                }
            }
        }
    }

    Connections {
        target: Lobby
        function onTimeoutWarningReceived(reason, remainingSec) {
            timeoutWarningPopup.show(reason, remainingSec)
        }
        function onNetworkMessageReceived(message) {
            networkMessagePopup.message = message
            networkMessagePopup.open()
        }
    }

    Connections {
        target: Qt.application
        function onStateChanged() {
            if (Qt.application.state === Qt.ApplicationActive) {
                var item = mainStackView.currentItem
                ScreenHelper.setKeepScreenOn(
                    item !== null &&
                    (item.objectName === "gamePage" || item.objectName === "gameWaitPage")
                )
            }
        }
    }
}
