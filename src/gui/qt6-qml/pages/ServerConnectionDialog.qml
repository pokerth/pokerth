import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config
import "../components"


Rectangle {
    id: serverConnectionPage
    // Bind to the visible area (StackView below the top bar), not to the whole
    // window – otherwise the box is not centred vertically.
    width: mainStackView.width
    height: mainStackView.height
    color: "transparent"

    // Inner margin of the card (border → StackLayout). It is also needed for the
    // minimum height of the card, hence a property instead of a literal.
    readonly property real cardPadding: 28

    Image {
        id: serverConnectionBackground
        anchors.fill: parent
        source: "../resources/startWindowBackground.png"
        fillMode: Image.PreserveAspectCrop
    }

    // Initial focus when opening the page. It MUST hang off the page root: the
    // onVisibleChanged of the first StackLayout view already fires while the
    // page is being built, when the page has no focus in the StackView yet – in the
    // real client that was a race (sometimes the focus took, sometimes not, and
    // Enter then did nothing). StackView.onActivated only fires once the page
    // really is in front; Qt.callLater additionally skips the animation.
    StackView.onActivated: Qt.callLater(serverConnectionPage.applyInitialFocus)

    // Focus on the element that makes sense in the current view.
    function applyInitialFocus() {
        if (mainStack.currentIndex === 0) {
            loginAsUserButton.forceActiveFocus()
        } else if (mainStack.currentIndex === 1) {
            // Not on mobile devices: that would pull up the on-screen keyboard.
            if (!Config.Responsive.isMobile)
                (usernameInput.text.length > 0 ? passwordInput : usernameInput).forceActiveFocus()
        } else {
            cancelButton.forceActiveFocus()
        }
    }

    // Back step inside the page, asked by navigateBackFromTopBar()
    // in pokerth.qml (Escape, Android back, the arrow in the header). true = handled
    // here, the page stays. A Keys.onEscapePressed of its own would be
    // without effect: the Escape shortcut on the window fires first.
    function handleBack() {
        if (mainStack.currentIndex === 1) {       // Formular → Auswahl
            mainStack.currentIndex = 0
            return true
        }
        if (mainStack.currentIndex === 2) {       // Verbindungsaufbau → abbrechen
            cancelButton.clicked()
            return true
        }
        return false
    }

    // Submit the login – shared by the login button and the Enter key
    // in the input fields.
    function submitLogin() {
        if (usernameInput.text.length === 0) {
            usernameInput.forceActiveFocus()
            return
        }
        usernameLabel.text = usernameInput.text
        connectionProgress.value = 0
        mainStack.currentIndex = 2
        ServerConnection.connectToServer(usernameInput.text, passwordInput.text, false, rememberMeCheckbox.checked)
    }

    Component.onCompleted: {
        // Load saved credentials from config
        usernameInput.text = ServerConnection.savedUsername
        passwordInput.text = ServerConnection.savedPassword
        rememberMeCheckbox.checked = ServerConnection.rememberPassword
    }

    // Connections to backend signals
    Connections {
        target: ServerConnection

        function onConnectionProgressChanged(progress) {
            connectionProgress.value = progress
        }

        function onStatusMessageChanged(message) {
            statusText.text = message
        }

        function onConnectionSucceeded() {
            // console.log("Connection succeeded!")
        }

        function onConnectionFailed(errorMessage) {
            // console.log("Connection failed:", errorMessage)
            statusText.text = errorMessage
            statusText.color = Config.Theme.colorError
            // Couple the reading time to the message length: the detailed texts
            // (e.g. connection attempt failed) could not be read within the fixed
            // 3.5 s before the page jumped back.
            errorResetTimer.interval = Math.min(12000, 3500 + errorMessage.length * 50)
            errorResetTimer.restart()
        }

        function onShowLobby() {
            // console.log("[NAV] onShowLobby → replace currentItem with LobbyPage.qml | depth before:", mainStackView.depth)
            mainStackView.replace(mainStackView.currentItem, "LobbyPage.qml")
        }
    }

    Timer {
        id: errorResetTimer
        interval: 3500
        repeat: false
        onTriggered: {
            mainStack.currentIndex = 0
            statusText.color = Config.StaticData.palette.secondary.col300
        }
    }

    Flickable {
        id: serverConnScroller
        anchors.fill: parent
        contentWidth: serverConnScroller.width
        contentHeight: serverConnScrollContent.height
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ScrollBar.vertical: ScrollBar {
            // AlwaysOff instead of AsNeeded: AsNeeded shows the bar transiently on
            // every contentHeight change (a fade of several seconds) –
            // visible as a scrollbar flashing up briefly while the page builds,
            // although there is nothing to scroll.
            policy: serverConnScroller.contentHeight > serverConnScroller.height + 1
                    ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        }

        Item {
            id: serverConnScrollContent
            width: serverConnScroller.width
            // Minimum height = viewport → the box stays centred vertically to the
            // window edge; with a window that is too low it can be scrolled.
            height: Math.max(serverConnScroller.height,
                             loginCard.height + Config.Theme.margin * 2)

        // Card – as on the StartPage (centred vertically)
        Rectangle {
            id: loginCard
            anchors.centerIn: parent
            width: Math.min(parent.width - Config.Theme.margin * 2, Config.Theme.brandBoxWidth)
            // The target height as the box of the StartPage – but it grows along when the
            // content needs more. Without this maximum the fixed height squeezed
            // the StackLayout below its minimum height on low windows (brandBoxHeight runs
            // into its floor of 380): the buttons ran out of the card at the
            // bottom instead of the card growing along.
            height: Math.max(Config.Theme.brandBoxHeight,
                             mainStack.anchors.topMargin + mainStack.implicitHeight
                             + serverConnectionPage.cardPadding)
            color: "transparent"

            Rectangle {
                anchors.fill: parent
                color: Config.Theme.colorBox
                opacity: 0.92
                radius: 5
            }

            // PokerTH logo + card symbols at the top of the card, positioned fixed –
            // exactly as on the StartPage (Config.Theme.margin from the upper edge).
            BrandHeader {
                id: loginLogo
                anchors.top: parent.top
                anchors.topMargin: Config.Theme.margin
                anchors.horizontalCenter: parent.horizontalCenter
                logoSize: Config.Theme.brandLogoSize
            }

            StackLayout {
                id: mainStack
                anchors.fill: parent
                anchors.margins: serverConnectionPage.cardPadding
                anchors.topMargin: loginLogo.y + loginLogo.height + 12   // Logo + spacing
                currentIndex: 0

                // View 0: Auswahl
                ColumnLayout {
                    id: initialChoicesView
                    spacing: 18

                    // Switching back from the form: the focus lands on the
                    // first button. The FIRST build, by contrast, runs via
                    // StackView.onActivated on the page root (see above).
                    onVisibleChanged: {
                        if (visible)
                            Qt.callLater(loginAsUserButton.forceActiveFocus)
                    }

                    Item { Layout.fillHeight: true }

                    CustomButton {
                        id: loginAsUserButton
                        text: qsTr("Login as User")
                        Layout.fillWidth: true
                        onClicked: mainStack.currentIndex = 1
                    }

                    CustomButton {
                        text: qsTr("Register")
                        Layout.fillWidth: true
                        onClicked: ServerConnection.openExternalUrl(ServerConnection.registerUrl)
                    }

                    CustomButton {
                        text: qsTr("Continue as Guest")
                        Layout.fillWidth: true
                        onClicked: {
                            var guestName = "Guest" + Math.floor(Math.random() * 10000)
                            usernameLabel.text = guestName
                            connectionProgress.value = 0
                            mainStack.currentIndex = 2
                            ServerConnection.connectToServer(guestName, "", true, false)
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                // View 1: Login-Formular
                ColumnLayout {
                    id: loginFormView
                    spacing: 12

                    // When fading in, focus right into the first still empty field
                    // – otherwise the Enter key only works after a
                    // mouse click into the field.
                    // When fading in, focus into the first still empty field.
                    // NOT on mobile devices – that would pull up the
                    // on-screen keyboard unasked.
                    onVisibleChanged: {
                        if (visible && !Config.Responsive.isMobile)
                            Qt.callLater((usernameInput.text.length > 0
                                          ? passwordInput : usernameInput).forceActiveFocus)
                    }

                    // Default button of the form: Enter submits the login,
                    // no matter which field has the focus (password, checkbox …).
                    // A focused button consumes Return itself and thereby keeps
                    // precedence – Enter on "Back" therefore goes back.
                    // Escape does not run here but via handleBack().
                    Keys.onReturnPressed: serverConnectionPage.submitLogin()
                    Keys.onEnterPressed: serverConnectionPage.submitLogin()

                    Item { Layout.fillHeight: true }

                    AppLabel {
                        text: qsTr("User Login")
                        Layout.alignment: Qt.AlignHCenter
                        font.bold: true
                        font.pixelSize: Config.Theme.fontSizeTitle
                        color: Config.StaticData.palette.secondary.col200
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Config.StaticData.palette.secondary.col500
                    }

                    TextField {
                        id: usernameInput
                        placeholderText: qsTr("Username")
                        // Enter jumps into the password field while it is empty;
                        // with a stored password it logs in directly. The event
                        // is consumed, otherwise the default button of the form
                        // would run as well and would submit the login with an
                        // empty password.
                        Keys.onReturnPressed: (event) => {
                            if (passwordInput.text.length === 0)
                                passwordInput.forceActiveFocus()
                            else
                                serverConnectionPage.submitLogin()
                            event.accepted = true
                        }
                        Keys.onEnterPressed: (event) => {
                            if (passwordInput.text.length === 0)
                                passwordInput.forceActiveFocus()
                            else
                                serverConnectionPage.submitLogin()
                            event.accepted = true
                        }
                        Layout.fillWidth: true
                        Layout.preferredHeight: Config.Theme.touchTarget
                        font.family: Config.StaticData.loadedFont.font.family
                        color: Config.StaticData.palette.secondary.col200
                        placeholderTextColor: Config.StaticData.palette.secondary.col400
                        background: Rectangle {
                            color: Config.StaticData.palette.secondary.col600
                            border.color: Config.StaticData.palette.secondary.col500
                            border.width: 1
                            radius: 3
                        }
                    }

                    TextField {
                        id: passwordInput
                        placeholderText: qsTr("Password")
                        echoMode: TextInput.Password
                        Layout.fillWidth: true
                        Layout.preferredHeight: Config.Theme.touchTarget
                        font.family: Config.StaticData.loadedFont.font.family
                        color: Config.StaticData.palette.secondary.col200
                        placeholderTextColor: Config.StaticData.palette.secondary.col400
                        background: Rectangle {
                            color: Config.StaticData.palette.secondary.col600
                            border.color: Config.StaticData.palette.secondary.col500
                            border.width: 1
                            radius: 3
                        }
                    }

                    CheckBox {
                        id: rememberMeCheckbox
                        text: qsTr("Remember me")
                        checked: false
                        // Space toggles (AbstractButton), Enter submits –
                        // that is done by the default button of the form.
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Config.Theme.spacing

                        CustomButton {
                            text: qsTr("Back")
                            Layout.fillWidth: true
                            onClicked: mainStack.currentIndex = 0
                        }

                        CustomButton {
                            text: qsTr("Login")
                            Layout.fillWidth: true
                            onClicked: serverConnectionPage.submitLogin()
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                // View 2: Verbindungsaufbau
                ColumnLayout {
                    id: guestLoginView
                    spacing: 20

                    // While the connection is being established, cancelling is the only
                    // action: the initial focus is on it, Escape likewise.
                    onVisibleChanged: {
                        if (visible)
                            Qt.callLater(cancelButton.forceActiveFocus)
                    }

                    Item { Layout.fillHeight: true }

                    AppText {
                        text: qsTr("Connecting as...")
                        font.pixelSize: Config.Theme.fontSizeBody
                        color: Config.StaticData.palette.secondary.col300
                        Layout.alignment: Qt.AlignHCenter
                    }

                    AppText {
                        id: usernameLabel
                        text: qsTr("Username/Guest")
                        font.pixelSize: Config.Theme.fontSizeTitle
                        font.bold: true
                        color: Config.StaticData.palette.secondary.col200
                        Layout.alignment: Qt.AlignHCenter
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ProgressBar {
                            id: connectionProgress
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            from: 0
                            to: 100
                            value: 0

                            background: Rectangle {
                                implicitWidth: 200
                                implicitHeight: 8
                                color: Config.StaticData.palette.secondary.col600
                                border.color: Config.StaticData.palette.secondary.col500
                                border.width: 1
                                radius: 4
                            }

                            contentItem: Item {
                                implicitWidth: 200
                                implicitHeight: 6

                                Rectangle {
                                    width: connectionProgress.visualPosition * parent.width
                                    height: parent.height
                                    radius: 4
                                    color: Config.StaticData.palette.secondary.col300
                                }
                            }
                        }

                        AppText {
                            id: statusText
                            text: qsTr("Initializing connection...")
                            font.pixelSize: Config.Theme.fontSizeBody
                            color: Config.StaticData.palette.secondary.col300
                            // Error messages are multi-line (see ServerConnectionHandler::
                            // networkErrorMessage) – without wrapping the text ran out of the box.
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    Item { Layout.fillHeight: true }

                    CustomButton {
                        id: cancelButton
                        text: qsTr("Cancel")
                        Layout.fillWidth: true
                        onClicked: {
                            ServerConnection.cancelConnection()
                            connectionProgress.value = 0
                            mainStack.currentIndex = 0
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }
        }
    }
}