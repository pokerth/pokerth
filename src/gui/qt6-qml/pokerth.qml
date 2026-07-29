pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Window

import "config" as Config
import "pages"
import "components"

ApplicationWindow {
    id: mainWindow

    Universal.theme: Config.StaticData.isDark ? Universal.Dark : Universal.Light

    // portraitMode is now provided by Config.Responsive.portrait
    // Topbar-Icons im Splash/PreLoader ausblenden – sonst kann ein zu früher
    // Klick eine Seite über den PreLoader pushen, die dann von dessen
    // replaceCurrentItem(startPage) den Stack durcheinanderbringt.
    readonly property bool topBarIconsVisible:
        mainStackView.currentItem
        && mainStackView.currentItem.objectName !== "preLoaderPage"

    // True, sobald die Lobby betreten wurde (Lobby-Seite liegt im Stack) – steuert
    // die globale Statusleiste. Re-Eval bei jeder Navigation (depth/currentItem).
    readonly property bool inLobbySession: {
        var _d = mainStackView.depth
        var _c = mainStackView.currentItem
        return mainStackView.find(function(it) {
            return it && it.objectName === "lobbyPage"
        }) !== null
    }

    // Overlay-Seiten der Topbar-Icons (Settings + Community/Ranking inkl. der
    // Unterseiten). Alles, was NICHT hier steht, gilt als Basisseite
    // (Gametable, Lobby, Startseite) – dorthin wird beim Schließen zurückgesetzt.
    readonly property var settingsSectionPages: ["settingsPage"]
    readonly property var rankingSectionPages:
        ["communityRankingPage", "rankingPage", "bbcRankingPage", "wecRankingPage",
         "pokerthPlayerPage", "communityPlayerPage"]

    // Gemerkter Ranking-Unterstapel beim Schließen über den Globus, damit ein
    // erneutes Toggle wieder auf der letzten Ranking-Seite landet (statt auf der
    // Auswahlseite). Liste von { url, props } in Stack-Reihenfolge.
    property var savedRankingStack: []

    // Aktiv = oberste Seite gehört zur jeweiligen Sektion → Icon hervorheben.
    readonly property bool settingsSectionActive:
        topBarSectionOpen(settingsSectionPages)
    readonly property bool rankingSectionActive:
        topBarSectionOpen(rankingSectionPages)

    function topBarSectionOpen(sectionPages) {
        var c = mainStackView.currentItem
        return c && sectionPages.indexOf(c.objectName) !== -1
    }

    // Alle Settings-/Ranking-Overlay-Seiten vom Stack poppen, sodass die
    // darunterliegende Basisseite (Gametable, Lobby oder Startseite) wieder
    // erscheint. Hält NIE auf der Zwischen-Auswahlseite (CommunityRankingPage).
    function closeTopBarOverlay() {
        var overlay = settingsSectionPages.concat(rankingSectionPages)
        for (var i = mainStackView.depth - 1; i >= 0; --i) {
            var item = mainStackView.get(i)
            if (!item || overlay.indexOf(item.objectName) === -1) {
                mainStackView.pop(item)
                return
            }
        }
    }

    // objectName → Quell-URL (relativ zu pokerth.qml) für das Wiederherstellen
    // eines gemerkten Overlay-Stacks.
    function overlayUrlFor(objectName) {
        switch (objectName) {
        case "communityRankingPage": return "pages/CommunityRankingPage.qml"
        case "rankingPage":          return "pages/RankingPage.qml"
        case "bbcRankingPage":       return "pages/BbcRankingPage.qml"
        case "wecRankingPage":       return "pages/WecRankingPage.qml"
        case "pokerthPlayerPage":    return "pages/PokerthPlayerPage.qml"
        case "communityPlayerPage":  return "components/CommunityPlayerView.qml"
        case "settingsPage":         return "pages/SettingsPage.qml"
        }
        return ""
    }

    // Konstruktions-Properties, die eine Seite zum Wiederaufbau braucht.
    function overlayPropsFor(item) {
        if (item.objectName === "pokerthPlayerPage")
            return { playerId: item.playerId, username: item.username }
        if (item.objectName === "communityPlayerPage")
            return { baseUrl: item.baseUrl, nickname: item.nickname, blocks: item.blocks }
        // Ranking-Listen-Seiten merken ihren Filter-Zustand über captureState().
        if (typeof item.captureState === "function")
            return { restoreState: item.captureState() }
        return {}
    }

    // Aktuellen Ranking-Overlay-Unterstapel (über der Basisseite) als Liste von
    // { url, props } sichern, um ihn später 1:1 wiederherzustellen.
    function saveOverlayStack() {
        var overlay = settingsSectionPages.concat(rankingSectionPages)
        var saved = []
        for (var i = mainStackView.depth - 1; i >= 0; --i) {
            var item = mainStackView.get(i)
            if (!item || overlay.indexOf(item.objectName) === -1) {
                for (var j = i + 1; j < mainStackView.depth; ++j) {
                    var it = mainStackView.get(j)
                    saved.push({ url: overlayUrlFor(it.objectName), props: overlayPropsFor(it) })
                }
                break
            }
        }
        savedRankingStack = saved
    }

    function restoreOverlayStack(saved) {
        for (var i = 0; i < saved.length; ++i) {
            if (saved[i].url !== "")
                mainStackView.push(saved[i].url, saved[i].props)
        }
    }

    // Topbar-Icon als Toggle: ist die Sektion bereits offen, wird sie (und jede
    // andere offene Overlay-Sektion) bis zur Basisseite geschlossen; sonst wird
    // ihre Einstiegsseite geöffnet – ggf. nach Kollaps einer anderen Sektion.
    // restore=true (Ranking) merkt sich beim Schließen den Unterstapel und stellt
    // ihn beim erneuten Öffnen wieder her (statt nur die Einstiegsseite).
    function toggleTopBarSection(entryUrl, sectionPages, restore) {
        var open = topBarSectionOpen(sectionPages)
        if (open && restore)
            saveOverlayStack()
        closeTopBarOverlay()
        if (!open) {
            if (restore && savedRankingStack.length > 0)
                restoreOverlayStack(savedRankingStack)
            else
                mainStackView.push(entryUrl)
        }
        sideMenu.visible = false
    }

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
    title: qsTr("PokerTH - v2.1.5")

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
        // Sprache kommt aus dem ConfigFile (Key "Language") – derselbe Wert, den
        // auch der Widgets-Client nutzt. Parameters.language ist nur noch der
        // Laufzeitwert für die Oberfläche.
        Config.Parameters.language = Config.StaticData.configLanguageToLocale(
                    SettingsManager ? SettingsManager.language : "")
        LanguageManager.switchLanguage(Config.Parameters.language)
        // Initialise dark/light mode from stored preference
        var dm = SettingsManager ? SettingsManager.readConfigInt("DarkMode") : 1
        Config.StaticData.darkMode = dm
        Config.Theme.darkMode = dm
        // Dekorative Effekte (Schatten/Glow/Blur) aus persistenter Einstellung.
        Config.Theme.effectsEnabled = SettingsManager
            ? SettingsManager.readConfigInt("QmlReduceEffects") === 0 : true
    }

    function navigateBackFromTopBar() {
        if (mainStackView.depth <= 1)
            return false

        var current = mainStackView.currentItem

        // Warteraum und laufendes Spiel: vor dem Verlassen IMMER nachfragen
        // (egal ob per Esc, Android-Back oder Tür-Icon), damit ein
        // versehentlicher Tastendruck das Spiel nicht ungewollt verlässt. Das
        // eigentliche Verlassen erledigt performLeaveGame() nach Bestätigung.
        if (current && (current.objectName === "gameWaitPage"
                        || current.objectName === "gamePage")) {
            leaveGameConfirmPopup.open()
            return true
        }

        // Lobby: vor dem Zurückkehren zur Startseite IMMER nachfragen und die
        // Server-Verbindung trennen (sonst bleibt man im Hintergrund verbunden
        // und erhält weiter Lobby-Chat/Mentions). Das eigentliche Verlassen
        // erledigt performLeaveLobby() nach Bestätigung.
        if (current && current.objectName === "lobbyPage") {
            leaveLobbyConfirmPopup.open()
            return true
        }

        mainStackView.pop()
        return true
    }

    // Sicherheitsnetz beim Verlassen eines NETZWERK-Spiels.
    //
    // Regulär vollzieht nicht performLeaveGame() den Wechsel zurück zur Lobby,
    // sondern erst die Server-Bestätigung: Lobby.leaveGame() schickt das Paket,
    // der Server antwortet mit removedFromGame, und GameWaitPage (liegt unter der
    // GamePage im Stack) poppt bis zur Lobby. Ist die Verbindung tot – auf iOS
    // reisst das System beim Suspendieren TCP-Sockets ab, ohne dass der Client es
    // merkt –, kommt diese Antwort NIE. Der Nutzer sitzt dann dauerhaft im
    // Spielbildschirm fest: die Abfrage erscheint, „Ja" bewirkt aber nichts, und
    // nur ein Neustart der App hilft (genau so im Testbericht + Debug-Log:
    // zwei Leave-Versuche, beide ohne Wirkung).
    //
    // Der Timer poppt deshalb nach Ablauf selbst zur Lobby. Er wird von
    // onRemovedFromGame gestoppt, sodass er im Normalfall (Antwort in
    // Millisekunden) nie feuert und das Verhalten unverändert bleibt.
    Timer {
        id: leaveGameFallbackTimer
        interval: 5000
        repeat: false
        onTriggered: {
            // Nur eingreifen, wenn Spiel/Warteraum ueberhaupt noch im Stack liegen.
            // Bewusst der ganze Stack statt nur currentItem: der Nutzer kann
            // waehrend des Wartens ein Overlay (z.B. die Einstellungen) geoeffnet
            // haben – im Testbericht war genau das moeglich, waehrend das Spiel
            // stand. Das Overlay wird vom pop() zur Lobby mit entfernt.
            var stuck = mainStackView.find(function(item) {
                return item && (item.objectName === "gamePage"
                                || item.objectName === "gameWaitPage")
            })
            if (!stuck)
                return
            console.warn("[NAV] leaveGame: keine Server-Bestätigung nach "
                         + (leaveGameFallbackTimer.interval / 1000)
                         + "s – verlasse das Spiel clientseitig (Verbindung tot?)")
            var lobby = mainStackView.find(function(item) {
                return item && item.objectName === "lobbyPage"
            })
            if (lobby)
                mainStackView.pop(lobby)
            else
                mainStackView.pop(null)   // keine Lobby im Stack → zur Startseite
        }
    }

    function performLeaveLobby() {
        // Bewusster Disconnect meldet keinen connectionFailed – eine offene
        // Timeout-Warnung der beendeten Session hier direkt schließen.
        timeoutWarningPopup.close()
        if (typeof Lobby !== "undefined" && Lobby)
            Lobby.leaveServer()
        mainStackView.pop()
    }

    function performLeaveGame() {
        var current = mainStackView.currentItem
        // console.log("[NAV] performLeaveGame | currentItem:", current ? (current.objectName || current.toString()) : "null", "| depth:", mainStackView.depth)
        var isGamePage = current && current.objectName === "gamePage"
        var isWaitPage = current && current.objectName === "gameWaitPage"
        var localGame = isGamePage
                        && (typeof GameTable !== "undefined")
                        && GameTable
                        && GameTable.isLocalGameRunning()

        // Warteraum bzw. laufendes Netzwerkspiel: serverseitig verlassen und
        // zurück in die LOBBY (nicht in den darunterliegenden Warteraum). Der
        // StackView wird durch onRemovedFromGame bis zur Lobby gepoppt – hier
        // NICHT poppen.
        if (isWaitPage || (isGamePage && !localGame)) {
            if (typeof Lobby !== "undefined" && Lobby)
                Lobby.leaveGame()
            // Sicherheitsnetz starten: kommt die Server-Bestätigung nicht, holt
            // uns leaveGameFallbackTimer trotzdem aus dem Spiel (s. dort).
            leaveGameFallbackTimer.restart()
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
            // Kompakter App-Header auf kurzen Landscape-Phones (spart vertikalen
            // Platz für den Tisch -> weniger Gegnerbox-Überlappung).
            Layout.preferredHeight: Config.Responsive.landscapeCompact ? 30 : 38
            Layout.alignment: Qt.AlignTop
            color: Config.StaticData.palette.secondary.col700

            RowLayout {
                id: topBarColumns
                anchors.fill: parent
                spacing: 8

                SvgIcon {
                    id: topBarMenuIcon
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 26
                    Layout.margins: Config.Responsive.landscapeCompact ? 2 : 6
                    source: "resources/threeLines.svg"
                    visible: mainWindow.topBarIconsVisible
                    // Tooltip folgt der Funktion des Buttons: Tür-Icon = Lobby/Spiel
                    // verlassen (je nach Seite), Caret = Zurück, sonst Menü.
                    ToolTip.visible: menuArea.containsMouse
                                     && !Config.Responsive.isMobile && Config.Parameters.showTooltips
                    ToolTip.delay: 600
                    ToolTip.text: {
                        var src = String(source)
                        if (src.indexOf("doorExit") !== -1) {
                            return (mainStackView.currentItem
                                    && mainStackView.currentItem.objectName === "lobbyPage")
                                ? qsTr("Leave Lobby") : qsTr("Leave Game")
                        }
                        if (src.indexOf("caretLeft") !== -1)
                            return qsTr("Back")
                        return qsTr("Menu")
                    }
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

                // Community / Ranking – überall erreichbar (auch in Lobby & Spiel).
                SvgIcon {
                    id: topBarRankingIcon
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    Layout.margins: Config.Responsive.landscapeCompact ? 2 : 6
                    source: "resources/trophy.svg"
                    visible: mainWindow.topBarIconsVisible && Config.Parameters.showCommunityContent
                    ToolTip.visible: rankingArea.containsMouse
                                     && !Config.Responsive.isMobile && Config.Parameters.showTooltips
                    ToolTip.delay: 600
                    ToolTip.text: qsTr("Community / Ranking")
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        colorization: 1.0
                        colorizationColor: mainWindow.rankingSectionActive
                            ? Config.Theme.colorAccent
                            : rankingArea.containsMouse
                                ? Config.StaticData.palette.secondary.col100
                                : Config.StaticData.palette.secondary.col200
                    }

                    MouseArea {
                        id: rankingArea
                        anchors.fill: topBarRankingIcon
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: mainWindow.toggleTopBarSection(
                            "pages/CommunityRankingPage.qml",
                            mainWindow.rankingSectionPages, true)
                    }
                }

                SvgIcon {
                    id: topBarSettingsIcon
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    Layout.margins: Config.Responsive.landscapeCompact ? 2 : 6
                    source: "resources/settings.svg"
                    visible: mainWindow.topBarIconsVisible
                    ToolTip.visible: settingsArea.containsMouse
                                     && !Config.Responsive.isMobile && Config.Parameters.showTooltips
                    ToolTip.delay: 600
                    ToolTip.text: qsTr("Settings")
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        colorization: 1.0
                        colorizationColor: mainWindow.settingsSectionActive
                            ? Config.Theme.colorAccent
                            : settingsArea.containsMouse
                                ? Config.StaticData.palette.secondary.col100
                                : Config.StaticData.palette.secondary.col200
                    }

                    MouseArea {
                        id: settingsArea
                        anchors.fill: topBarSettingsIcon
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: mainWindow.toggleTopBarSection(
                            "pages/SettingsPage.qml",
                            mainWindow.settingsSectionPages)
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
                // Sichtbarkeit der Topbar-Icons folgt dem Binding
                // topBarIconsVisible (im Splash aus) – hier nur das Quell-Icon
                // des Menü-/Zurück-Buttons je nach Seite wählen.
                if (depth <= 1) {
                    topBarMenuIcon.source = sideMenu.visible ? "resources/caretLeft.svg" : "resources/threeLines.svg";
                } else if (isLobby || isGame || isGameWait) {
                    // Lobby, Spiel UND Warteraum: Tür-Icon zum Verlassen.
                    topBarMenuIcon.source = "resources/doorExit.svg";
                } else {
                    topBarMenuIcon.source = "resources/caretLeft.svg";
                }
                // Bildschirm während Spiel und Warteraum wach halten (Android:
                // FLAG_KEEP_SCREEN_ON via JNI). Beim Verlassen freigeben.
                ScreenHelper.setKeepScreenOn(isGame || isGameWait);
            }
        }

        // Globale Statusleiste (verbundene Spieler / laufende & offene Spiele):
        // erscheint unten auf allen Seiten, sobald die Lobby betreten wurde –
        // ausgenommen der Spieltisch (GamePage hat eine eigene Statusleiste und
        // braucht den vertikalen Platz).
        LobbyStatsBar {
            Layout.fillWidth: true
            Layout.leftMargin: Config.Theme.margin
            Layout.rightMargin: Config.Theme.margin
            Layout.bottomMargin: Config.Responsive.compact ? 6 : 8
            Layout.topMargin: 4
            visible: mainWindow.inLobbySession
                     && !(mainStackView.currentItem
                          && mainStackView.currentItem.objectName === "gamePage")
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

    // Vollbild: gilt auf JEDER Seite (Startseite, Lobby, Warteraum, Tisch) –
    // deshalb am Fenster und nicht auf der GamePage. Zuschauer eingeschlossen.
    Shortcut {
        sequence: "F11"
        context: Qt.ApplicationShortcut
        onActivated: mainWindow.visibility = (mainWindow.visibility === Window.FullScreen)
                                             ? Window.Windowed : Window.FullScreen
    }

    Shortcut {
        sequence: "Alt+S"
        onActivated: {
            // Nicht im Splash/PreLoader öffnen (Stack-Reset, s. topBarIconsVisible).
            if (mainWindow.topBarIconsVisible)
                mainWindow.toggleTopBarSection(
                    "pages/SettingsPage.qml", mainWindow.settingsSectionPages)
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
        width: Math.min(mainWindow.width * 0.85, 380)
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
            width: timeoutWarningPopup.availableWidth

            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Timeout Warning")
                color: Config.StaticData.palette.secondary.col100
                font.pixelSize: 15
                font.bold: true
            }
            AppLabel {
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
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            AppLabel {
                Layout.fillWidth: true
                visible: !timeoutWarningPopup.expired
                text: qsTr("Please click \"OK\" to stop the countdown!")
                color: Config.StaticData.palette.secondary.col300
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
        width: Math.min(mainWindow.width * 0.85, 380)
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
            width: networkMessagePopup.availableWidth

            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Server Message")
                color: Config.StaticData.palette.secondary.col100
                font.pixelSize: 15
                font.bold: true
            }
            AppLabel {
                Layout.fillWidth: true
                text: networkMessagePopup.message
                textFormat: Text.RichText
                color: Config.StaticData.palette.secondary.col200
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
        width: Math.min(mainWindow.width * 0.85, 380)
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: Config.StaticData.palette.secondary.col700
            border.color: Config.StaticData.palette.secondary.col400
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            spacing: 12
            width: leaveGameConfirmPopup.availableWidth

            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Leave Game")
                color: Config.StaticData.palette.secondary.col100
                font.pixelSize: 15
                font.bold: true
            }
            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Attention! Do you really want to leave the current game\nand go back to the lobby?")
                color: Config.StaticData.palette.secondary.col200
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

    // ── Bestätigung beim Verlassen der Lobby (zurück zur Startseite) ───────
    // Erscheint bei Esc / Android-Back / Tür-Icon, solange man sich in der
    // Lobby befindet. Bei Bestätigung wird die Server-Verbindung getrennt.
    Popup {
        id: leaveLobbyConfirmPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 20
        width: Math.min(mainWindow.width * 0.85, 380)
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: Config.StaticData.palette.secondary.col700
            border.color: Config.StaticData.palette.secondary.col400
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            spacing: 12
            width: leaveLobbyConfirmPopup.availableWidth

            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Leave Lobby")
                color: Config.StaticData.palette.secondary.col100
                font.pixelSize: 15
                font.bold: true
            }
            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Attention! Do you really want to leave the lobby\nand disconnect from the server?")
                color: Config.StaticData.palette.secondary.col200
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                CustomButton {
                    Layout.fillWidth: true
                    text: qsTr("Cancel")
                    onClicked: leaveLobbyConfirmPopup.close()
                }
                CustomButton {
                    Layout.fillWidth: true
                    text: qsTr("Leave Lobby")
                    onClicked: {
                        leaveLobbyConfirmPopup.close()
                        mainWindow.performLeaveLobby()
                    }
                }
            }
        }
    }

    // ── Verbindungsverlust nach dem Login (Lobby/Warteraum/Spiel) ──────────
    // Die Verbindungs-/Beitrittsseiten behandeln connectionFailed selbst
    // (Statuszeile während des Verbindens); nach dem Login gab es aber keinen
    // Konsumenten: Ein Verbindungsabbruch im laufenden Spiel blieb unsichtbar.
    // Global melden und zur Login-Seite zurückkehren – nach dem erneuten Login
    // bietet der Server ggf. das Rejoin ins laufende Spiel an (LobbyPage-Popup).
    Popup {
        id: connectionLostPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 20
        width: Math.min(mainWindow.width * 0.85, 380)
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
            width: connectionLostPopup.availableWidth

            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Connection lost")
                color: Config.StaticData.palette.secondary.col100
                font.pixelSize: 15
                font.bold: true
            }
            AppLabel {
                Layout.fillWidth: true
                text: connectionLostPopup.message
                color: Config.StaticData.palette.secondary.col200
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            CustomButton {
                Layout.fillWidth: true
                text: qsTr("OK")
                onClicked: connectionLostPopup.close()
            }
        }
    }

    Connections {
        target: ServerConnection
        function onConnectionFailed(errorMessage) {
            // Timeout-Warnung ist mit der Verbindung obsolet – IMMER schließen,
            // auch wenn die Lobby bereits verlassen wurde (sonst bleibt das
            // Popup nach Ablauf mangels aktivem OK-Button für immer offen).
            timeoutWarningPopup.close()
            // Nur nach abgeschlossenem Login (Lobby im Stack) – während des
            // Verbindens zeigen die Verbindungsseiten den Fehler selbst an.
            if (!mainWindow.inLobbySession)
                return
            // Offene Modals (Verlassen-Bestätigungen) schließen.
            leaveGameConfirmPopup.close()
            leaveLobbyConfirmPopup.close()
            connectionLostPopup.message = errorMessage
            // Zurück zur StartPage (baut Lobby-/Spiel-Seiten ab) und direkt die
            // Login-Seite öffnen, damit ein erneuter Login nur einen Tap kostet.
            mainStackView.pop(null)
            mainStackView.push("pages/ServerConnectionDialog.qml")
            connectionLostPopup.open()
        }
    }

    Connections {
        target: Lobby
        function onTimeoutWarningReceived(reason, remainingSec) {
            timeoutWarningPopup.show(reason, remainingSec)
        }
        // Nach Ablauf des Countdowns trennt der Server NICHT immer die
        // Verbindung: beim AFK-Kick im Spiel und beim Admin-Timeout eines
        // offenen Spiels wird man nur aus dem Spiel entfernt (Session lebt
        // weiter). Der Widget-Client versteckt den Dialog dafür in
        // networkNotification() – Pendant hier: die Entfernung aus dem Spiel
        // macht die Warnung gegenstandslos, Popup schließen.
        function onRemovedFromGame(reason) {
            // Server hat das Verlassen bestätigt → das Sicherheitsnetz
            // (leaveGameFallbackTimer) wird nicht mehr gebraucht. Ohne dieses
            // Stoppen würde es nach einem regulären Verlassen nachfeuern und
            // könnte eine inzwischen geöffnete Seite wegpoppen.
            leaveGameFallbackTimer.stop()
            timeoutWarningPopup.close()
        }
        function onNetworkMessageReceived(message) {
            networkMessagePopup.message = message
            networkMessagePopup.open()
        }
        // "Show player stats" (Lobby-Icon / Tisch-Kontextmenü): native
        // Player-Page statt Browser-Link. Quelle = im Backend vorausgewählte
        // Default-Community (bei aktiven Community-Inhalten), sonst PokerTH.
        function onPlayerStatsRequested(playerName) {
            var comm = (Config.Parameters.showCommunityContent
                        && Config.Community.has(Config.Parameters.defaultCommunity))
                       ? Config.Parameters.defaultCommunity : "pokerth"
            var c = mainStackView.currentItem
            // Doppelklick-Schutz: Page desselben Spielers liegt bereits oben
            // (PokerTH per username, BBC/WEC per nickname).
            if (c && ((comm === "pokerth" && c.objectName === "pokerthPlayerPage"
                       && c.username === playerName)
                      || (comm !== "pokerth" && c.objectName === "communityPlayerPage"
                          && c.community === comm && c.nickname === playerName)))
                return
            mainStackView.push(Config.Community.playerPageUrl(comm),
                               Config.Community.playerPageProps(comm, playerName))
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
                // Resume-Probe: Nach einer Hintergrund-Phase aktiv ein Paket
                // schicken (AFK-Reset – Rückkehr IST Nutzeraktivität). Ist die
                // Verbindung im Hintergrund gestorben, schlägt der Send fehl
                // und der Verbindungsverlust wird sofort gemeldet (Popup +
                // Login-Seite) statt erst beim ersten Tap oder per Keepalive.
                if (mainWindow.inLobbySession)
                    Lobby.resetNetworkTimeout()
            }
        }
    }
}
