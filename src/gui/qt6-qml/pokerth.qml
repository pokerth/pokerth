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
    // Hide the top bar icons in the splash/PreLoader – otherwise a click that is
    // too early can push a page above the PreLoader, which then messes up the
    // stack with its replaceCurrentItem(startPage).
    readonly property bool topBarIconsVisible:
        mainStackView.currentItem
        && mainStackView.currentItem.objectName !== "preLoaderPage"

    // ── Private messages (the inbox) ───────────────────────────────────────
    // The counter and the number of conversations for the letter symbol of the header.
    // privateMessagesRevision is the reactive dependency of the history.
    readonly property int unreadPrivateMessages:
        (typeof Lobby !== "undefined" && Lobby) ? Lobby.unreadPrivateMessages : 0
    readonly property int privateConversationCount: {
        var _rev = (typeof Lobby !== "undefined" && Lobby) ? Lobby.privateMessagesRevision : 0
        return (typeof Lobby !== "undefined" && Lobby)
                ? Lobby.privateConversationPartners().length : 0
    }

    // Opens the inbox; an empty playerName => the conversation that was active last.
    // The player lists of the pages call it with the player that was clicked.
    function openPrivateMessages(playerName) {
        privateMessageDialog.openWith(playerName)
    }

    // True between the beginning of an automatic reconnect and its
    // outcome. It covers the case that the reconnect is given up:
    // the lobby is already torn down then, so inLobbySession is false, but the
    // error message still has to reach the player.
    property bool reconnectPending: false

    // True as soon as the lobby has been entered (the lobby page lies in the stack) – it controls
    // the global status bar. It is re-evaluated on every navigation (depth/currentItem).
    readonly property bool inLobbySession: {
        var _d = mainStackView.depth
        var _c = mainStackView.currentItem
        return mainStackView.find(function(it) {
            return it && it.objectName === "lobbyPage"
        }) !== null
    }

    // The overlay pages of the top bar icons (settings + community/ranking + forum
    // news incl. their sub-pages). Everything that is NOT listed here counts as a
    // base page (the game table, the lobby, the start page) – that is where it is reset
    // to when closing.
    readonly property var settingsSectionPages: ["settingsPage"]
    readonly property var rankingSectionPages:
        ["communityRankingPage", "rankingPage", "bbcRankingPage", "wecRankingPage",
         "pokerthPlayerPage", "communityPlayerPage"]
    readonly property var forumSectionPages: ["forumNewsPage", "forumPostPage"]

    // All overlay pages together – the basis of closeTopBarOverlay() and
    // saveOverlayStack().
    readonly property var overlaySectionPages:
        settingsSectionPages.concat(rankingSectionPages, forumSectionPages)

    // The ranking substack remembered when closing via the globe, so that
    // toggling again lands on the last ranking page (instead of on the
    // selection page). A list of { url, props } in stack order.
    property var savedRankingStack: []

    // Active = the topmost page belongs to the respective section → highlight the icon.
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

    // Pop all overlay pages (settings/ranking/forum) off the stack, so that the
    // base page below (the game table, the lobby or the start page)
    // appears again. It NEVER stops at the intermediate selection page (CommunityRankingPage).
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

    // objectName → the source URL (relative to pokerth.qml) for restoring
    // a remembered overlay stack.
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

    // The construction properties a page needs for the rebuild.
    function overlayPropsFor(item) {
        if (item.objectName === "forumPostPage")
            return { post: item.post }
        if (item.objectName === "pokerthPlayerPage")
            return { playerId: item.playerId, username: item.username }
        if (item.objectName === "communityPlayerPage")
            return { baseUrl: item.baseUrl, nickname: item.nickname, blocks: item.blocks }
        // The ranking list pages remember their filter state via captureState().
        if (typeof item.captureState === "function")
            return { restoreState: item.captureState() }
        return {}
    }

    // Save the current ranking overlay substack (above the base page) as a list of
    // { url, props }, in order to restore it 1:1 later.
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

    // The top bar icon as a toggle: if the section is already open, it (and every
    // other open overlay section) is closed down to the base page; otherwise
    // its entry page is opened – possibly after collapsing another section.
    // restore=true (ranking) remembers the substack when closing and restores
    // it when opening again (instead of only the entry page).
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
    // The start resolution = the default size of the Qt widgets client at the game table
    // (gametable.ui: 1024×621). When the component is built, the size is
    // additionally clamped to the available screen.
    width: 1024
    height: 621
    // The initial portrait width as a lower bound – the window must not become
    // narrower than the standard portrait mode, so that the layout
    // (slot columns, the self box, the action buttons) always fits into the picture completely.
    //
    // DESKTOP ONLY. On Android/iOS there is no freely scalable window: the
    // area IS the screen. A minimum width that lies above the logical
    // display width makes Qt draw the scene wider than the
    // display – the edge is cut off, the player sees "not the
    // full width". Exactly that happens on widespread 1080p phones: a
    // Samsung S20 FE (1080×2400) reports 360×800 dp depending on the rounded
    // devicePixelRatio – 30 dp narrower than the 390 demanded here.
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
        // An aspect preserving clamp to the available screen. 2316×1080
        // is the phone landscape test size (aspect 2.144); on notebooks with
        // 1920×1080 or 2560×1440 the window would otherwise either stick out
        // or lose its aspect ratio — both disable the
        // landscapeCompact mode (the aspect threshold 1.85).
        //
        // Purely DESKTOP logic: the size, the minimum size and the centring only make
        // sense where there is a window IN a screen. On Android/
        // iOS the system determines the area (fullscreen); any geometry of our own
        // works against it. Concretely the clamp computed on a 360×800 dp
        // phone: scale = (360−20)/1024 = 0.33 → width = max(390, 340) = 390 and
        // x = 180 − 195 = −15, i.e. a window WIDER than the display and
        // offset to the left as well → both edges of the scene lie outside
        // the display. So do not touch anything at all on mobile.
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
        // The language comes from the ConfigFile (the key "Language") – the same value that
        // the widgets client uses as well. Parameters.language is only the
        // runtime value for the user interface.
        Config.Parameters.language = Config.StaticData.configLanguageToLocale(
                    SettingsManager ? SettingsManager.language : "")
        LanguageManager.switchLanguage(Config.Parameters.language)
        // Initialise dark/light mode from the stored preference. "Automatic"
        // (2) follows the system – the value comes from C++ (darkmode.h) and is
        // updated by systemDarkSync when the system theme changes.
        var dm = SettingsManager ? SettingsManager.readConfigInt("DarkMode") : 1
        applySystemDark()
        Config.StaticData.darkMode = dm
        Config.Theme.darkMode = dm
        // Decorative effects (shadow/glow/blur) from the persistent setting.
        Config.Theme.effectsEnabled = SettingsManager
            ? SettingsManager.readConfigInt("QmlReduceEffects") === 0 : true
        // The seat style of the player boxes (the bet in the base or next to it). An
        // empty value means "default" – then the default of the
        // singleton stays.
        var seatStyle = SettingsManager
            ? SettingsManager.readConfigString("QmlSeatStyle") : ""
        if (seatStyle === "inset" || seatStyle === "classic")
            Config.SeatStyle.variant = seatStyle
    }

    // Mirror the light/dark of the operating system into the singletons (which
    // cannot read a context property themselves). It only applies with DarkMode =
    // "automatic"; with a fixed light/dark setting the value stays
    // unused.
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

        // The waiting room and a running game: ALWAYS ask before leaving
        // (no matter whether via Esc, Android back or the door icon), so that an
        // accidental keystroke does not leave the game unintentionally. The
        // actual leaving is done by performLeaveGame() after the confirmation.
        if (current && (current.objectName === "gameWaitPage"
                        || current.objectName === "gamePage")) {
            leaveGameConfirmPopup.open()
            return true
        }

        // The lobby: ALWAYS ask before returning to the start page and disconnect
        // from the server (otherwise you stay connected in the background
        // and keep receiving lobby chat/mentions). The actual leaving is done
        // by performLeaveLobby() after the confirmation.
        if (current && current.objectName === "lobbyPage") {
            leaveLobbyConfirmPopup.open()
            return true
        }

        // Pages with a back step of their own (e.g. the login form →
        // the selection) take precedence over leaving the page. Escape
        // does not reach the pages itself: the shortcut above applies BEFORE the
        // Keys handlers of the items, a Keys.onEscapePressed on a page
        // would never fire. That is why the navigation asks here.
        // qmllint disable missing-property
        if (current && typeof current.handleBack === "function" && current.handleBack())
            return true
        // qmllint enable missing-property

        mainStackView.pop()
        return true
    }

    // A safety net when leaving a NETWORK game.
    //
    // Regularly it is not performLeaveGame() that performs the change back to the lobby
    // but the server confirmation: Lobby.leaveGame() sends the packet,
    // the server answers with removedFromGame, and GameWaitPage (which lies below the
    // GamePage in the stack) pops up to the lobby. If the connection is dead – on iOS
    // the system tears TCP sockets down when suspending, without the client
    // noticing –, this answer NEVER arrives. The user is then stuck in the
    // game screen permanently: the prompt appears, but "yes" does nothing, and
    // only a restart of the app helps (exactly so in the test report + the debug log:
    // two leave attempts, both without effect).
    //
    // The timer therefore pops to the lobby itself when it expires. It is stopped by
    // onRemovedFromGame, so that in the normal case (an answer within
    // milliseconds) it never fires and the behaviour stays unchanged.
    Timer {
        id: leaveGameFallbackTimer
        interval: 5000
        repeat: false
        onTriggered: {
            // Only intervene if the game/waiting room is still in the stack at all.
            // Deliberately the whole stack instead of only currentItem: the user may have
            // opened an overlay (e.g. the settings) while waiting – in the
            // test report exactly that was possible while the game
            // stood still. The overlay is removed by the pop() to the lobby as well.
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
                mainStackView.pop(null)   // no lobby in the stack → to the start page
        }
    }

    function performLeaveLobby() {
        // A deliberate disconnect reports no connectionFailed – close an open
        // timeout warning of the session that has ended directly here.
        timeoutWarningPopup.close()
        // Whoever leaves deliberately does not want to be fetched back automatically.
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

        // The waiting room or a running network game: leave it on the server side and
        // go back into the LOBBY (not into the waiting room below it). The
        // StackView is popped to the lobby by onRemovedFromGame – do NOT
        // pop here.
        if (isWaitPage || (isGamePage && !localGame)) {
            if (typeof Lobby !== "undefined" && Lobby)
                Lobby.leaveGame()
            // Start the safety net: if the server confirmation does not arrive,
            // leaveGameFallbackTimer gets us out of the game anyway (see there).
            leaveGameFallbackTimer.restart()
            return
        }

        if (localGame)
            GameTable.endLocalGame()

        mainStackView.pop()
        if (localGame && mainStackView.depth > 1)
            mainStackView.pop()
    }
    
    // ── Edge-to-edge display (Android 15+) ──────────────────────────────────
    // From targetSdk 35 on, Android draws edge to edge MANDATORILY: the "fullscreen" flag
    // of the app theme is ignored, the status and navigation bar lie above
    // the window content. The insets that have to stay free for them are reported by
    // SafeArea (QtQuick 6.9) – encapsulated in a file of its own, because the type
    // does not exist yet on Qt 6.7 (the Android 8 APK variant) and a direct
    // access there would make the whole window fail to load. If the
    // loader fails, the insets stay 0 and everything is as before; likewise when the
    // platform already keeps the bars free itself.
    Loader {
        id: safeAreaLoader
        anchors.fill: parent
        z: -1
        active: Config.Responsive.isMobile
        source: "components/SafeAreaInsets.qml"
    }
    readonly property real safeAreaTop:
        safeAreaLoader.item ? safeAreaLoader.item.insetTop : 0
    readonly property real safeAreaBottom:
        safeAreaLoader.item ? safeAreaLoader.item.insetBottom : 0
    readonly property real safeAreaLeft:
        safeAreaLoader.item ? safeAreaLoader.item.insetLeft : 0
    readonly property real safeAreaRight:
        safeAreaLoader.item ? safeAreaLoader.item.insetRight : 0

    // The background deliberately WITHOUT safe area insets: the system bars should lie on
    // the app background colour, not on a black strip.
    Rectangle {
        anchors.fill: parent
        color: Config.StaticData.palette.secondary.col700
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        // Keep the content out of the status/navigation bar and the notch.
        anchors.topMargin:    mainWindow.safeAreaTop
        anchors.bottomMargin: mainWindow.safeAreaBottom
        anchors.leftMargin:   mainWindow.safeAreaLeft
        anchors.rightMargin:  mainWindow.safeAreaRight
        Layout.alignment: Qt.AlignTop
        spacing: 0

        Rectangle {
            id: topBar
            Layout.preferredWidth: parent.width
            // A compact app header on short landscape phones (it saves vertical
            // room for the table -> less opponent box overlap).
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
                    // The tooltip follows the function of the button: the door icon = leave the lobby/game
                    // (depending on the page), the caret = back, otherwise the menu.
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

                // The inbox for private messages. It stands to the left of the
                // news and carries – like it – its counter as a
                // badge on the icon (not a child of the icon: the MultiEffect layer
                // would otherwise colourise it as well).
                Item {
                    id: topBarInboxButton
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    Layout.margins: Config.Responsive.landscapeCompact ? 2 : 6
                    // Online only: the history outlives sessions, but an inbox
                    // on the start page (without a connection) could show nothing but
                    // old messages. At a running table it is gone as well –
                    // PMs are blocked there.
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

                        // Pop briefly as soon as a new PM arrives – together
                        // with the sound, the hint that something has come in.
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

                // The forum news – reachable everywhere like the ranking. The
                // counter of unread posts sits as a badge on the icon; it
                // must NOT be a child of the icon, otherwise its
                // MultiEffect layer colourises it as well.
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

                // Community / ranking – reachable everywhere (in the lobby & the game as well).
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
                // The visibility of the top bar icons follows the binding
                // topBarIconsVisible (off in the splash) – here only the source icon
                // of the menu/back button is chosen depending on the page.
                if (depth <= 1) {
                    topBarMenuIcon.source = sideMenu.visible ? "resources/caretLeft.svg" : "resources/threeLines.svg";
                } else if (isLobby || isGame || isGameWait) {
                    // The lobby, the game AND the waiting room: a door icon for leaving.
                    topBarMenuIcon.source = "resources/doorExit.svg";
                } else {
                    topBarMenuIcon.source = "resources/caretLeft.svg";
                }
                // Keep the screen awake during the game and the waiting room (Android:
                // FLAG_KEEP_SCREEN_ON via JNI). Release it when leaving.
                ScreenHelper.setKeepScreenOn(isGame || isGameWait);
            }
        }

        // The global status bar (connected players / running & open games):
        // it appears at the bottom on all pages as soon as the lobby has been entered –
        // except at the game table (the GamePage has a status bar of its own and
        // needs the vertical room).
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

    // ── Keyboard shortcuts ────────────────────────────────────────────────────
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

    // Fullscreen: it applies on EVERY page (the start page, the lobby, the waiting room, the table) –
    // which is why it is on the window and not on the GamePage. Spectators included.
    Shortcut {
        sequence: "F11"
        context: Qt.ApplicationShortcut
        onActivated: mainWindow.visibility = (mainWindow.visibility === Window.FullScreen)
                                             ? Window.Windowed : Window.FullScreen
    }

    Shortcut {
        sequence: "Alt+S"
        onActivated: {
            // Do not open it in the splash/PreLoader (a stack reset, see topBarIconsVisible).
            if (mainWindow.topBarIconsVisible)
                mainWindow.toggleTopBarSection(
                    "pages/SettingsPage.qml", mainWindow.settingsSectionPages)
        }
    }

    SideMenu {}

    // The forum fetch follows the setting: switched off = no network traffic
    // and no counter. (Config.ForumNews must not read the Parameters itself –
    // inside the module Config that would be a circular dependency.)
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
    // ── The AFK timeout warning (a port of timeoutMsgBoxImpl, lobby as well as in-game) ──
    // It appears globally above all pages; OK stops the server countdown
    // (resetNetworkTimeout). The beep comes from LobbyHandler::onTimeoutWarning.
    Popup {
        id: timeoutWarningPopup
        // Without focus:true the popup would get no keyboard input: Escape would be
        // swallowed (the popup would stay open, and the Escape shortcut of the window
        // does not apply with an open popup either) and Enter would go nowhere.
        focus: true
        // The initial focus on OK – Enter stops the countdown.
        onOpened: timeoutOkButton.forceActiveFocus()
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
                // The texts 1:1 as in timeoutMsgBoxImpl::timerRefresh.
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
                id: timeoutOkButton
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

    // ── A server message (a port of startWindowImpl::networkMessage) ───────
    Popup {
        id: networkMessagePopup
        // Without focus:true the popup would get no keyboard input: Escape would be
        // swallowed (the popup would stay open, and the Escape shortcut of the window
        // does not apply with an open popup either) and Enter would go nowhere.
        focus: true
        // The initial focus on close – Enter acknowledges the message.
        onOpened: networkMessageCloseButton.forceActiveFocus()
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
                id: networkMessageCloseButton
                Layout.fillWidth: true
                text: qsTr("Close")
                onClicked: networkMessagePopup.close()
            }
        }
    }

    // ── The confirmation when leaving a running game ─────────────────────
    // It appears on Esc / Android back / the door icon while you are on the
    // GamePage, so that an accidental keystroke does not
    // end the running game unintentionally.
    Popup {
        id: leaveGameConfirmPopup
        // Without focus:true the popup would get no keyboard input: Escape would be
        // swallowed (the popup would stay open, and the Escape shortcut of the window
        // does not apply with an open popup either) and Enter would go nowhere.
        focus: true
        // The initial focus deliberately on CANCEL: according to
        // navigateBackFromTopBar() this prompt exists exactly so that an accidental
        // keystroke does not leave the game. Enter must not undermine that;
        // to leave, press Tab once.
        onOpened: leaveGameCancelButton.forceActiveFocus()
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
                    id: leaveGameCancelButton
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

    // ── The confirmation when leaving the lobby (back to the start page) ───────
    // It appears on Esc / Android back / the door icon while you are in the
    // lobby. On confirmation the server connection is cut.
    Popup {
        id: leaveLobbyConfirmPopup
        // Without focus:true the popup would get no keyboard input: Escape would be
        // swallowed (the popup would stay open, and the Escape shortcut of the window
        // does not apply with an open popup either) and Enter would go nowhere.
        focus: true
        // The initial focus deliberately on CANCEL – as when leaving the game:
        // Enter should not cut the connection accidentally.
        onOpened: leaveLobbyCancelButton.forceActiveFocus()
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
                    id: leaveLobbyCancelButton
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

    // ── An automatic reconnect is running ─────────────────────────────────
    // A connection loss during operation (Android: the app was in the
    // background; desktop: WLAN sleep) no longer throws the player onto the
    // login page immediately: the ServerConnectionHandler logs in again
    // silently, the LobbyHandler accepts the rejoin offer of the server automatically,
    // and the existing showLobby path rebuilds the lobby/waiting room/table.
    // The only visible part of that is this notice – with the possibility to cancel,
    // because nobody should be held against their will.
    Popup {
        id: reconnectPopup
        // Without focus:true the popup would get no keyboard input: Escape would be
        // swallowed (the popup would stay open, and the Escape shortcut of the window
        // does not apply with an open popup either) and Enter would go nowhere.
        focus: true
        // The only action during the reconnect: the initial focus on cancel.
        // (closePolicy NoAutoClose – Escape deliberately does not close here.)
        onOpened: reconnectCancelButton.forceActiveFocus()
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 20
        width: Math.min(mainWindow.width * 0.85, 380)
        // It cannot be tapped away: the state ends by itself (success, giving up)
        // or via the cancel button.
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
                id: reconnectCancelButton
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

    // ── A connection loss after the login (lobby/waiting room/game) ──────────
    // The connection/join pages handle connectionFailed themselves
    // (the status line while connecting); after the login there was no
    // consumer though: a connection loss during a running game stayed invisible.
    // It now only applies once the automatic reconnect has given up.
    Popup {
        id: connectionLostPopup
        // Without focus:true the popup would get no keyboard input: Escape would be
        // swallowed (the popup would stay open, and the Escape shortcut of the window
        // does not apply with an open popup either) and Enter would go nowhere.
        focus: true
        // The initial focus on OK – Enter acknowledges.
        onOpened: connectionLostOkButton.forceActiveFocus()
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
                id: connectionLostOkButton
                Layout.fillWidth: true
                text: qsTr("OK")
                onClicked: connectionLostPopup.close()
            }
        }
    }

    // The inbox for private messages. Deliberately here and not in the pages:
    // the header symbol and the letter symbol of the player list should open the same
    // dialog, and a page change must not discard an open conversation
    // together with the page.
    PrivateMessageDialog {
        id: privateMessageDialog
    }

    // If you leave the lobby (or the connection breaks), the open inbox
    // has to go as well – without a connection you could only read in it.
    onInLobbySessionChanged: if (!inLobbySession) privateMessageDialog.close()

    Connections {
        target: ServerConnection

        // The beginning/end of the automatic reconnect (Android/iOS only –
        // on the desktop the signal is never triggered).
        function onReconnectingChanged() {
            if (ServerConnection.reconnecting) {
                mainWindow.reconnectPending = true
                timeoutWarningPopup.close()
                leaveGameConfirmPopup.close()
                leaveLobbyConfirmPopup.close()
                // Tear down to the StartPage as with a hard connection loss and
                // put the connection page on top – its onShowLobby brings us
                // back into the lobby without any action after a successful re-login,
                // and from there the automatic rejoin takes over.
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
            // The timeout warning is obsolete with the connection – ALWAYS close it,
            // even when the lobby has already been left (otherwise the popup stays
            // open forever after it expires, for lack of an active OK button).
            timeoutWarningPopup.close()
            // After a reconnect that was given up, the lobby has long been torn down,
            // so inLobbySession is false – but the player needs the message
            // exactly then. reconnectPending covers this case.
            const afterReconnect = mainWindow.reconnectPending
            // Only after a completed login (the lobby in the stack) – while
            // connecting, the connection pages show the error themselves.
            if (!mainWindow.inLobbySession && !afterReconnect)
                return
            mainWindow.reconnectPending = false
            reconnectPopup.close()
            // Close open modals (the leave confirmations).
            leaveGameConfirmPopup.close()
            leaveLobbyConfirmPopup.close()
            connectionLostPopup.message = errorMessage
            // Back to the StartPage (it tears the lobby/game pages down) and open the
            // login page directly, so that another login costs only one tap.
            // After a failed reconnect the login page already lies
            // on top – then do not rebuild it again.
            if (!afterReconnect) {
                mainStackView.pop(null)
                mainStackView.push("pages/ServerConnectionDialog.qml")
            }
            connectionLostPopup.open()
        }

        // A successful reconnect: the lobby is back, the rejoin
        // continues automatically. Only reset the flag.
        function onShowLobby() {
            mainWindow.reconnectPending = false
        }
    }

    Connections {
        target: Lobby
        function onTimeoutWarningReceived(reason, remainingSec) {
            timeoutWarningPopup.show(reason, remainingSec)
        }
        // After the countdown expires the server does NOT always cut the
        // connection: on an AFK kick during a game and on an admin timeout of an
        // open game you are only removed from the game (the session lives
        // on). For that the widget client hides the dialog in
        // networkNotification() – the counterpart here: the removal from the game
        // makes the warning void, so close the popup.
        function onRemovedFromGame(reason) {
            // The server has confirmed the leaving → the safety net
            // (leaveGameFallbackTimer) is not needed any more. Without this
            // stopping it would fire after a regular leaving and
            // could pop away a page that has been opened meanwhile.
            leaveGameFallbackTimer.stop()
            timeoutWarningPopup.close()
        }
        function onNetworkMessageReceived(message) {
            networkMessagePopup.message = message
            networkMessagePopup.open()
        }
        // "Show player stats" (the lobby icon / the table context menu): the native
        // player page instead of a browser link. The source = the default community
        // preselected in the backend (with community content active), otherwise PokerTH.
        function onPlayerStatsRequested(playerName) {
            var comm = (Config.Parameters.showCommunityContent
                        && Config.Community.has(Config.Parameters.defaultCommunity))
                       ? Config.Parameters.defaultCommunity : "pokerth"
            var c = mainStackView.currentItem
            // Double click protection: the page of the same player already lies on top
            // (PokerTH by username, BBC/WEC by nickname).
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
                // The resume probe: after a background phase actively send a
                // packet (an AFK reset – returning IS user activity). If the
                // connection has died in the background, the send fails
                // and the connection loss is reported immediately (a popup +
                // the login page) instead of only at the first tap or via the keepalive.
                if (mainWindow.inLobbySession)
                    Lobby.resetNetworkTimeout()
            }
        }
    }
}
