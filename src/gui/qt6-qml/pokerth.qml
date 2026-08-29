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

    // ── Private Nachrichten (Posteingang) ──────────────────────────────────
    // Zähler und Anzahl der Unterhaltungen für das Brief-Symbol der Kopfzeile.
    // privateMessagesRevision ist die reaktive Abhängigkeit des Verlaufs.
    readonly property int unreadPrivateMessages:
        (typeof Lobby !== "undefined" && Lobby) ? Lobby.unreadPrivateMessages : 0
    readonly property int privateConversationCount: {
        var _rev = (typeof Lobby !== "undefined" && Lobby) ? Lobby.privateMessagesRevision : 0
        return (typeof Lobby !== "undefined" && Lobby)
                ? Lobby.privateConversationPartners().length : 0
    }

    // Öffnet den Posteingang; playerName leer => zuletzt aktives Gespräch.
    // Die Spielerlisten der Seiten rufen das mit dem angeklickten Spieler auf.
    function openPrivateMessages(playerName) {
        privateMessageDialog.openWith(playerName)
    }

    // True zwischen Beginn einer automatischen Wiederverbindung und ihrem
    // Ausgang. Trägt den Fall, dass die Wiederverbindung aufgegeben wird:
    // die Lobby ist dann bereits abgebaut, inLobbySession also false, die
    // Fehlermeldung muss den Spieler aber trotzdem erreichen.
    property bool reconnectPending: false

    // True, sobald die Lobby betreten wurde (Lobby-Seite liegt im Stack) – steuert
    // die globale Statusleiste. Re-Eval bei jeder Navigation (depth/currentItem).
    readonly property bool inLobbySession: {
        var _d = mainStackView.depth
        var _c = mainStackView.currentItem
        return mainStackView.find(function(it) {
            return it && it.objectName === "lobbyPage"
        }) !== null
    }

    // Overlay-Seiten der Topbar-Icons (Settings + Community/Ranking + Forum-
    // Neuigkeiten inkl. der Unterseiten). Alles, was NICHT hier steht, gilt als
    // Basisseite (Gametable, Lobby, Startseite) – dorthin wird beim Schließen
    // zurückgesetzt.
    readonly property var settingsSectionPages: ["settingsPage"]
    readonly property var rankingSectionPages:
        ["communityRankingPage", "rankingPage", "bbcRankingPage", "wecRankingPage",
         "pokerthPlayerPage", "communityPlayerPage"]
    readonly property var forumSectionPages: ["forumNewsPage", "forumPostPage"]

    // Alle Overlay-Seiten zusammen – Grundlage von closeTopBarOverlay() und
    // saveOverlayStack().
    readonly property var overlaySectionPages:
        settingsSectionPages.concat(rankingSectionPages, forumSectionPages)

    // Gemerkter Ranking-Unterstapel beim Schließen über den Globus, damit ein
    // erneutes Toggle wieder auf der letzten Ranking-Seite landet (statt auf der
    // Auswahlseite). Liste von { url, props } in Stack-Reihenfolge.
    property var savedRankingStack: []

    // Aktiv = oberste Seite gehört zur jeweiligen Sektion → Icon hervorheben.
    readonly property bool settingsSectionActive:
        topBarSectionOpen(settingsSectionPages)
    readonly property bool rankingSectionActive:
        topBarSectionOpen(rankingSectionPages)
    readonly property bool forumSectionActive:
        topBarSectionOpen(forumSectionPages)

    function topBarSectionOpen(sectionPages) {
        var c = mainStackView.currentItem
        return c && sectionPages.indexOf(c.objectName) !== -1
    }

    // Alle Overlay-Seiten (Settings/Ranking/Forum) vom Stack poppen, sodass die
    // darunterliegende Basisseite (Gametable, Lobby oder Startseite) wieder
    // erscheint. Hält NIE auf der Zwischen-Auswahlseite (CommunityRankingPage).
    function closeTopBarOverlay() {
        var overlay = overlaySectionPages
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
        case "forumNewsPage":        return "pages/ForumNewsPage.qml"
        case "forumPostPage":        return "pages/ForumPostPage.qml"
        }
        return ""
    }

    // Konstruktions-Properties, die eine Seite zum Wiederaufbau braucht.
    function overlayPropsFor(item) {
        if (item.objectName === "forumPostPage")
            return { post: item.post }
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
        var overlay = overlaySectionPages
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
    //
    // NUR DESKTOP. Auf Android/iOS gibt es kein frei skalierbares Fenster: die
    // Fläche IST der Bildschirm. Eine Mindestbreite, die über der logischen
    // Display-Breite liegt, lässt Qt die Szene breiter aufziehen als die
    // Anzeige – der Rand wird abgeschnitten, der Spieler sieht "nicht die
    // volle Breite". Genau das passiert auf verbreiteten 1080p-Phones: ein
    // Samsung S20 FE (1080×2400) meldet je nach gerundetem devicePixelRatio
    // 360×800 dp – 30 dp schmaler als die hier geforderten 390.
    minimumWidth: Config.Responsive.isMobile ? 0 : 390
    minimumHeight: Config.Responsive.isMobile ? 0 : 600
    // TRY to center the window, doesn't work on my Ubuntu but should work on other platforms.
    visible: true
    title: qsTr("PokerTH - v2.1.8")

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
        //
        // Reine DESKTOP-Logik: Größe, Mindestgröße und Zentrierung ergeben nur
        // dort einen Sinn, wo es ein Fenster IM Bildschirm gibt. Auf Android/
        // iOS legt das System die Fläche fest (Vollbild); jede eigene Geometrie
        // arbeitet dagegen. Konkret rechnete der Clamp auf einem 360×800-dp-
        // Phone: scale = (360−20)/1024 = 0.33 → width = max(390, 340) = 390 und
        // x = 180 − 195 = −15, also ein Fenster BREITER als das Display und
        // dazu nach links versetzt → beide Ränder der Szene liegen außerhalb
        // der Anzeige. Auf Mobil daher gar nichts anfassen.
        if (!Config.Responsive.isMobile && screen) {
            var maxW = screen.width  - 20
            var maxH = screen.height - 60   // Taskleiste/Titelbar
            var scale = Math.min(maxW / width, maxH / height, 1.0)
            if (scale < 1.0) {
                width  = Math.max(minimumWidth,  Math.floor(width  * scale))
                height = Math.max(minimumHeight, Math.floor(height * scale))
            }
            x = screen.width / 2 - width / 2
            y = screen.height / 2 - height / 2
        }
        Config.Responsive.windowWidth  = width
        Config.Responsive.windowHeight = height
        Config.Theme.windowWidth       = width
        Config.Theme.windowHeight      = height
        // Sprache kommt aus dem ConfigFile (Key "Language") – derselbe Wert, den
        // auch der Widgets-Client nutzt. Parameters.language ist nur noch der
        // Laufzeitwert für die Oberfläche.
        Config.Parameters.language = Config.StaticData.configLanguageToLocale(
                    SettingsManager ? SettingsManager.language : "")
        LanguageManager.switchLanguage(Config.Parameters.language)
        // Initialise dark/light mode from stored preference. "Automatisch"
        // (2) folgt dem System – der Wert kommt aus C++ (darkmode.h) und wird
        // von systemDarkSync nachgeführt, wenn das System-Theme wechselt.
        var dm = SettingsManager ? SettingsManager.readConfigInt("DarkMode") : 1
        applySystemDark()
        Config.StaticData.darkMode = dm
        Config.Theme.darkMode = dm
        // Dekorative Effekte (Schatten/Glow/Blur) aus persistenter Einstellung.
        Config.Theme.effectsEnabled = SettingsManager
            ? SettingsManager.readConfigInt("QmlReduceEffects") === 0 : true
        // Sitz-Stil der Spielerboxen (Einsatz im Sockel oder daneben). Ein
        // leerer Wert bedeutet "Vorgabe der Plattform" – dann bleibt der
        // Default des Singletons stehen.
        var seatStyle = SettingsManager
            ? SettingsManager.readConfigString("QmlSeatStyle") : ""
        if (seatStyle === "inset" || seatStyle === "classic")
            Config.SeatStyle.variant = seatStyle
    }

    // Hell/Dunkel des Betriebssystems in die Singletons spiegeln (die eine
    // Context-Property nicht selbst lesen können). Wirkt nur bei DarkMode =
    // "Automatisch"; bei fest eingestelltem Hell/Dunkel bleibt der Wert
    // ungenutzt.
    function applySystemDark() {
        var sd = SettingsManager ? SettingsManager.systemDark : true
        Config.StaticData.systemDark = sd
        Config.Theme.systemDark      = sd
    }

    // System-Theme-Wechsel im laufenden Betrieb (Windows/macOS Hell↔Dunkel).
    Connections {
        id: systemDarkSync
        target: SettingsManager
        function onSystemDarkChanged() { mainWindow.applySystemDark() }
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
        // Wer bewusst geht, will nicht automatisch zurückgeholt werden.
        if (typeof ServerConnection !== "undefined" && ServerConnection)
            ServerConnection.abortAutoReconnect()
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
            color: Config.Theme.colorBox

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

                // Posteingang für private Nachrichten. Steht links neben den
                // Neuigkeiten und trägt – wie diese – seinen Zähler als
                // Plakette am Icon (kein Kind des Icons: der MultiEffect-Layer
                // würde sie sonst mit einfärben).
                Item {
                    id: topBarInboxButton
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    Layout.margins: Config.Responsive.landscapeCompact ? 2 : 6
                    // Nur online: der Verlauf überdauert Sitzungen, ein Posteingang
                    // auf der Startseite (ohne Verbindung) könnte aber nichts als
                    // alte Nachrichten zeigen. Am laufenden Tisch ebenfalls weg –
                    // dort sind PMs gesperrt.
                    visible: mainWindow.topBarIconsVisible
                             && mainWindow.inLobbySession
                             && !(typeof Lobby !== "undefined" && Lobby && Lobby.atRunningTable)
                             && (mainWindow.privateConversationCount > 0)

                    ToolTip.visible: inboxArea.containsMouse
                                     && !Config.Responsive.isMobile && Config.Parameters.showTooltips
                    ToolTip.delay: 600
                    ToolTip.text: qsTr("Private messages")

                    SvgIcon {
                        id: topBarInboxIcon
                        anchors.fill: parent
                        source: "resources/mail.svg"
                        layer.enabled: true
                        layer.effect: MultiEffect {
                            colorization: 1.0
                            colorizationColor: inboxArea.containsMouse
                                ? Config.StaticData.palette.secondary.col100
                                : Config.StaticData.palette.secondary.col200
                        }
                    }

                    Rectangle {
                        id: inboxUnreadBadge
                        visible: mainWindow.unreadPrivateMessages > 0
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.topMargin: -5
                        anchors.rightMargin: -5
                        width: Math.max(15, inboxUnreadLabel.implicitWidth + 7)
                        height: 15
                        radius: 7.5
                        color: Config.Theme.colorDanger
                        border.color: Config.Theme.colorBox
                        border.width: 1.5

                        AppText {
                            id: inboxUnreadLabel
                            anchors.centerIn: parent
                            text: mainWindow.unreadPrivateMessages > 9
                                  ? "9+" : mainWindow.unreadPrivateMessages
                            color: "#FFFFFF"
                            font.pixelSize: 9
                            font.bold: true
                        }

                        // Kurz aufpoppen, sobald eine neue PM eintrifft – zusammen
                        // mit dem Ton der Hinweis, dass etwas eingegangen ist.
                        onVisibleChanged: if (visible) inboxPop.restart()
                        Connections {
                            target: (typeof Lobby !== "undefined") ? Lobby : null
                            function onUnreadPrivateMessagesChanged() {
                                if (mainWindow.unreadPrivateMessages > 0)
                                    inboxPop.restart()
                            }
                        }
                        SequentialAnimation {
                            id: inboxPop
                            NumberAnimation { target: inboxUnreadBadge; property: "scale"
                                              from: 0.6; to: 1.2; duration: 110; easing.type: Easing.OutQuad }
                            NumberAnimation { target: inboxUnreadBadge; property: "scale"
                                              to: 1.0; duration: 130; easing.type: Easing.OutBack }
                        }
                    }

                    MouseArea {
                        id: inboxArea
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onClicked: mainWindow.openPrivateMessages("")
                    }
                }

                // Forum-Neuigkeiten – wie das Ranking überall erreichbar. Der
                // Zähler ungelesener Beiträge sitzt als Plakette am Icon; er
                // darf KEIN Kind des Icons sein, sonst färbt dessen
                // MultiEffect-Layer ihn mit ein.
                Item {
                    id: topBarForumButton
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    Layout.margins: Config.Responsive.landscapeCompact ? 2 : 6
                    visible: mainWindow.topBarIconsVisible && Config.Parameters.showForumNews

                    ToolTip.visible: forumArea.containsMouse
                                     && !Config.Responsive.isMobile && Config.Parameters.showTooltips
                    ToolTip.delay: 600
                    ToolTip.text: qsTr("Forum news")

                    SvgIcon {
                        id: topBarForumIcon
                        anchors.fill: parent
                        source: "resources/newspaper.svg"
                        layer.enabled: true
                        layer.effect: MultiEffect {
                            colorization: 1.0
                            colorizationColor: mainWindow.forumSectionActive
                                ? Config.Theme.colorAccent
                                : forumArea.containsMouse
                                    ? Config.StaticData.palette.secondary.col100
                                    : Config.StaticData.palette.secondary.col200
                        }
                    }

                    Rectangle {
                        id: forumUnreadBadge
                        visible: Config.ForumNews.unreadCount > 0
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.topMargin: -5
                        anchors.rightMargin: -5
                        width: Math.max(15, forumUnreadLabel.implicitWidth + 7)
                        height: 15
                        radius: 7.5
                        color: Config.Theme.colorDanger
                        border.color: Config.Theme.colorBox
                        border.width: 1.5

                        AppText {
                            id: forumUnreadLabel
                            anchors.centerIn: parent
                            text: Config.ForumNews.unreadCount > 9
                                  ? "9+" : Config.ForumNews.unreadCount
                            color: "#FFFFFF"
                            font.pixelSize: 9
                            font.bold: true
                        }
                    }

                    MouseArea {
                        id: forumArea
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true

                        onClicked: mainWindow.toggleTopBarSection(
                            "pages/ForumNewsPage.qml",
                            mainWindow.forumSectionPages)
                    }
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

    // Der Forum-Abruf folgt der Einstellung: ausgeschaltet = kein Netzverkehr
    // und kein Zähler. (Config.ForumNews darf die Parameters nicht selbst
    // lesen – innerhalb des Moduls Config wäre das eine Zirkelabhängigkeit.)
    Binding {
        target: Config.ForumNews
        property: "enabled"
        value: Config.Parameters.showForumNews
    }

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
            color: Config.Theme.colorBox
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
            color: Config.Theme.colorBox
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
            color: Config.Theme.colorBox
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
            color: Config.Theme.colorBox
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

    // ── Automatische Wiederverbindung läuft ───────────────────────────────
    // Ein Verbindungsverlust im laufenden Betrieb (Android: App war im
    // Hintergrund; Desktop: WLAN-Schlaf) wirft den Spieler nicht mehr sofort
    // auf die Login-Seite: Der ServerConnectionHandler meldet sich still neu
    // an, der LobbyHandler nimmt das Rejoin-Angebot des Servers automatisch
    // an, und der vorhandene showLobby-Weg baut Lobby/Warteraum/Tisch wieder
    // auf. Sichtbar ist davon nur dieser Hinweis – mit Abbruch-Möglichkeit,
    // denn niemand soll gegen seinen Willen festgehalten werden.
    Popup {
        id: reconnectPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 20
        width: Math.min(mainWindow.width * 0.85, 380)
        // Nicht wegtippbar: Der Zustand endet von selbst (Erfolg, Aufgabe)
        // oder über den Abbrechen-Knopf.
        closePolicy: Popup.NoAutoClose

        property int attempt: 0
        property int maxAttempts: 0

        background: Rectangle {
            color: Config.Theme.colorBox
            border.color: Config.StaticData.palette.secondary.col400
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            spacing: 12
            width: reconnectPopup.availableWidth

            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Connection interrupted")
                color: Config.StaticData.palette.secondary.col100
                font.pixelSize: 15
                font.bold: true
                wrapMode: Text.WordWrap
            }
            AppLabel {
                Layout.fillWidth: true
                text: reconnectPopup.maxAttempts > 0
                      ? qsTr("Reconnecting to the server… (attempt %1 of %2)")
                        .arg(reconnectPopup.attempt).arg(reconnectPopup.maxAttempts)
                      : qsTr("Reconnecting to the server…")
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Your seat at the table stays reserved for a few minutes.")
                font.pixelSize: 12
                opacity: 0.8
                wrapMode: Text.WordWrap
            }
            CustomButton {
                Layout.fillWidth: true
                text: qsTr("Cancel")
                onClicked: {
                    ServerConnection.abortAutoReconnect()
                    reconnectPopup.close()
                    mainWindow.reconnectPending = false
                    mainStackView.pop(null)
                    mainStackView.push("pages/ServerConnectionDialog.qml")
                }
            }
        }
    }

    // ── Verbindungsverlust nach dem Login (Lobby/Warteraum/Spiel) ──────────
    // Die Verbindungs-/Beitrittsseiten behandeln connectionFailed selbst
    // (Statuszeile während des Verbindens); nach dem Login gab es aber keinen
    // Konsumenten: Ein Verbindungsabbruch im laufenden Spiel blieb unsichtbar.
    // Greift jetzt erst, wenn die automatische Wiederverbindung aufgegeben hat.
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
            color: Config.Theme.colorBox
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

    // Posteingang für private Nachrichten. Bewusst hier und nicht in den Seiten:
    // Kopfzeilen-Symbol und Brief-Symbol der Spielerliste sollen denselben
    // Dialog öffnen, und ein Seitenwechsel darf ein offenes Gespräch nicht
    // mitsamt der Seite verwerfen.
    PrivateMessageDialog {
        id: privateMessageDialog
    }

    // Verlässt man die Lobby (oder bricht die Verbindung ab), gehört auch der
    // offene Posteingang weg – ohne Verbindung ließe sich darin nur noch lesen.
    onInLobbySessionChanged: if (!inLobbySession) privateMessageDialog.close()

    Connections {
        target: ServerConnection

        // Beginn/Ende der automatischen Wiederverbindung (nur Android/iOS –
        // am Desktop wird das Signal nie ausgelöst).
        function onReconnectingChanged() {
            if (ServerConnection.reconnecting) {
                mainWindow.reconnectPending = true
                timeoutWarningPopup.close()
                leaveGameConfirmPopup.close()
                leaveLobbyConfirmPopup.close()
                // Wie beim harten Verbindungsverlust zur StartPage abbauen und
                // die Verbindungsseite auflegen – deren onShowLobby bringt uns
                // nach erfolgreichem Re-Login ohne Zutun zurück in die Lobby,
                // von dort übernimmt der automatische Rejoin.
                mainStackView.pop(null)
                mainStackView.push("pages/ServerConnectionDialog.qml")
                reconnectPopup.open()
            } else {
                reconnectPopup.close()
            }
        }

        function onReconnectAttempt(attempt, maxAttempts) {
            reconnectPopup.attempt = attempt
            reconnectPopup.maxAttempts = maxAttempts
        }

        function onConnectionFailed(errorMessage) {
            // Timeout-Warnung ist mit der Verbindung obsolet – IMMER schließen,
            // auch wenn die Lobby bereits verlassen wurde (sonst bleibt das
            // Popup nach Ablauf mangels aktivem OK-Button für immer offen).
            timeoutWarningPopup.close()
            // Nach aufgegebener Wiederverbindung ist die Lobby längst abgebaut,
            // inLobbySession also false – der Spieler braucht die Meldung aber
            // gerade dann. reconnectPending trägt diesen Fall.
            const afterReconnect = mainWindow.reconnectPending
            // Nur nach abgeschlossenem Login (Lobby im Stack) – während des
            // Verbindens zeigen die Verbindungsseiten den Fehler selbst an.
            if (!mainWindow.inLobbySession && !afterReconnect)
                return
            mainWindow.reconnectPending = false
            reconnectPopup.close()
            // Offene Modals (Verlassen-Bestätigungen) schließen.
            leaveGameConfirmPopup.close()
            leaveLobbyConfirmPopup.close()
            connectionLostPopup.message = errorMessage
            // Zurück zur StartPage (baut Lobby-/Spiel-Seiten ab) und direkt die
            // Login-Seite öffnen, damit ein erneuter Login nur einen Tap kostet.
            // Nach einer gescheiterten Wiederverbindung liegt die Login-Seite
            // bereits oben – dann nicht noch einmal umbauen.
            if (!afterReconnect) {
                mainStackView.pop(null)
                mainStackView.push("pages/ServerConnectionDialog.qml")
            }
            connectionLostPopup.open()
        }

        // Erfolgreiche Wiederverbindung: Die Lobby ist wieder da, der Rejoin
        // läuft automatisch weiter. Nur noch den Merker zurücksetzen.
        function onShowLobby() {
            mainWindow.reconnectPending = false
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
