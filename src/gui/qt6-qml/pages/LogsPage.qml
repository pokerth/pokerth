import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// Log viewer – ported from the Qt widgets LogFileDialog.
// Lists the SQLite log files (.pdb) in the LogDir, allows selecting a game, shows
// a formatted preview and supports export (HTML/TXT), save as,
// delete as well as the analysis (upload to pokerth.net).
Rectangle {
    id: logsPage
    objectName: "logsPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    // ── Zustand ───────────────────────────────────────────────────────────────
    property var files: (typeof LogStore !== "undefined" && LogStore) ? LogStore.logFiles : []
    property int selectedIndex: -1
    readonly property string selectedPath:
        (selectedIndex >= 0 && selectedIndex < files.length) ? files[selectedIndex].path : ""
    property var gameModel: []

    // Portrait / a narrow window → stack vertically instead of side by side.
    readonly property bool compact: Config.Responsive.compact

    readonly property int selectedGameId:
        (gameCombo.currentIndex >= 0 && gameCombo.currentIndex < gameModel.length)
            ? gameModel[gameCombo.currentIndex] : 0

    function reloadGames() {
        if (typeof LogStore === "undefined" || !LogStore || selectedPath === "") {
            gameModel = []
            previewText.text = ""
            return
        }
        gameModel = LogStore.gameList(selectedPath)
        gameCombo.currentIndex = gameModel.length > 0 ? 0 : -1
        reloadPreview()
    }

    function reloadPreview() {
        if (typeof LogStore === "undefined" || !LogStore || selectedPath === "") {
            previewText.text = ""
            return
        }
        previewText.text = LogStore.previewHtml(selectedPath, selectedGameId)
    }

    onSelectedPathChanged: reloadGames()
    onFilesChanged: {
        if (files.length === 0)
            selectedIndex = -1
        else if (selectedIndex < 0)
            selectedIndex = 0
        else if (selectedIndex >= files.length)
            selectedIndex = files.length - 1
    }

    Component.onCompleted: {
        if (typeof LogStore !== "undefined" && LogStore)
            LogStore.refresh()
        selectedIndex = files.length > 0 ? 0 : -1
        reloadGames()
    }

    Connections {
        target: (typeof LogStore !== "undefined") ? LogStore : null
        // On success the browser is opened directly in C++ (cleaned environment).
        function onAnalyseFailed(message) {
            messageLabel.text = message
            messageDialog.open()
        }
    }

    // ── Aufbau ────────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        AppLabel {
            text: qsTr("Logs")
            color: Config.StaticData.palette.secondary.col200
            font.pointSize: 14
            font.bold: true
        }

        // Game-Auswahl
        RowLayout {
            spacing: 8
            AppLabel {
                text: qsTr("Game:")
                color: Config.StaticData.palette.secondary.col200
                font.pixelSize: Config.Theme.fontSizeBody
            }
            ComboBox {
                id: gameCombo
                Layout.preferredWidth: 140
                model: logsPage.gameModel
                enabled: logsPage.gameModel.length > 0
                onActivated: logsPage.reloadPreview()
            }
            Item { Layout.fillWidth: true }
        }

        // The file list + the preview – side by side (wide) or stacked (narrow)
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: logsPage.compact ? 1 : 2
            columnSpacing: 12
            rowSpacing: 10

            // List of the log files
            Rectangle {
                Layout.fillWidth: logsPage.compact
                Layout.preferredWidth: logsPage.compact ? 0 : 220
                Layout.fillHeight: !logsPage.compact
                // narrow: limited, scrollable height; wide: fills the column
                Layout.preferredHeight: logsPage.compact
                    ? Math.min(fileList.contentHeight + 2, logsPage.height * 0.30) : 0
                color: Config.StaticData.palette.secondary.col600
                border.color: Config.StaticData.palette.secondary.col500
                border.width: 1
                radius: 4

                ListView {
                    id: fileList
                    // Keyboard operation: Tab leads into the list, the arrows change
                    // the file. keyNavigationEnabled stays OFF because it would
                    // assign currentIndex directly – that would destroy the
                    // binding to selectedIndex below. Instead the arrows change
                    // the selection of the page, and currentIndex follows.
                    activeFocusOnTab: true
                    keyNavigationEnabled: false
                    Keys.onUpPressed: {
                        if (logsPage.selectedIndex > 0)
                            logsPage.selectedIndex = logsPage.selectedIndex - 1
                    }
                    Keys.onDownPressed: {
                        if (logsPage.selectedIndex < logsPage.files.length - 1)
                            logsPage.selectedIndex = logsPage.selectedIndex + 1
                    }
                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true
                    model: logsPage.files
                    currentIndex: logsPage.selectedIndex
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar {
                        policy: fileList.contentHeight > fileList.height + 4
                                ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                    }

                    delegate: ItemDelegate {
                        id: fileDelegate
                        required property int index
                        required property var modelData
                        width: ListView.view.width
                        height: 30

                        background: Rectangle {
                            color: fileDelegate.modelData.current
                                   ? "#a01818"
                                   : (logsPage.selectedIndex === fileDelegate.index
                                      ? Config.Theme.colorAccent
                                      : (fileDelegate.index % 2 === 0
                                         ? Config.Theme.colorBox
                                         : Config.StaticData.palette.secondary.col600))
                        }
                        contentItem: AppText {
                            text: fileDelegate.modelData.name
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                            color: fileDelegate.modelData.current ? "#FFFFFF"
                                 : (logsPage.selectedIndex === fileDelegate.index ? "#101010"
                                    : Config.StaticData.palette.secondary.col100)
                            font.pixelSize: 12
                        }
                        onClicked: logsPage.selectedIndex = index
                    }
                }
            }

            // Vorschau
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                AppLabel {
                    text: qsTr("Preview:")
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeBody
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    // Dark/light: panel slightly set apart from the page background (col700)
                    color: Config.StaticData.palette.secondary.col600
                    border.color: Config.StaticData.palette.secondary.col500
                    border.width: 1
                    radius: 4

                    ScrollView {
                        id: previewScroll
                        anchors.fill: parent
                        anchors.margins: 6
                        clip: true

                        TextEdit {
                            id: previewText
                            readOnly: true
                            selectByMouse: true
                            textFormat: TextEdit.RichText
                            wrapMode: TextEdit.WordWrap
                            // The text colour follows the theme (the HTML contains no colours)
                            color: Config.StaticData.palette.secondary.col100
                            font.pixelSize: 13
                        }
                    }
                }
            }
        }

        // Action buttons – narrow: 2 columns (filled); wide: 4 next to each other
        GridLayout {
            Layout.fillWidth: true
            columns: logsPage.compact ? 2 : 4
            columnSpacing: 8
            rowSpacing: 8

            CustomButton {
                Layout.fillWidth: logsPage.compact
                text: qsTr("Export as HTML")
                enabled: logsPage.selectedPath !== ""
                onClicked: LogStore.exportHtmlDialog(logsPage.selectedPath)
            }
            CustomButton {
                Layout.fillWidth: logsPage.compact
                text: qsTr("Export as txt")
                enabled: logsPage.selectedPath !== ""
                onClicked: LogStore.exportTxtDialog(logsPage.selectedPath)
            }
            CustomButton {
                Layout.fillWidth: logsPage.compact
                text: qsTr("Save as ...")
                enabled: logsPage.selectedPath !== ""
                onClicked: LogStore.saveAsDialog(logsPage.selectedPath)
            }
            CustomButton {
                Layout.fillWidth: logsPage.compact
                text: qsTr("Delete")
                enabled: logsPage.selectedPath !== ""
                         && !(logsPage.files[logsPage.selectedIndex] && logsPage.files[logsPage.selectedIndex].current)
                onClicked: deleteDialog.open()
            }
        }

        // Analyse
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            BusyIndicator {
                running: (typeof LogStore !== "undefined" && LogStore) ? LogStore.uploadInProgress : false
                visible: running
                implicitWidth: 28
                implicitHeight: 28
            }
            Item { Layout.fillWidth: !logsPage.compact }
            CustomButton {
                Layout.fillWidth: logsPage.compact
                // Opens the client debug log (pokerth-debug.log) directly in the
                // app. On mobile devices the file lies in private storage that
                // the user cannot reach otherwise – that way a player can read
                // and pass on the log, e.g. for a rejoin problem.
                text: qsTr("Show debug log")
                onClicked: {
                    debugLogView.text = (typeof LogStore !== "undefined" && LogStore)
                                        ? LogStore.debugLogTail(200000) : ""
                    debugLogDialog.open()
                }
            }
            CustomButton {
                Layout.fillWidth: logsPage.compact
                text: qsTr("Analyse Logfile ...")
                enabled: logsPage.selectedPath !== ""
                         && !((typeof LogStore !== "undefined" && LogStore) ? LogStore.uploadInProgress : false)
                onClicked: LogStore.analyse(logsPage.selectedPath)
            }
        }
    }

    // ── Dialoge ───────────────────────────────────────────────────────────────
    Dialog {
        id: deleteDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("PokerTH - Delete log files")
        standardButtons: Dialog.Yes | Dialog.No
        // Without focus:true the dialog stays unreachable for the keyboard.
        // Qt Quick has no default button: a focused standard button
        // only reacts to space, which is why the content accepts Enter.
        focus: true
        onOpened: deleteDialogLabel.forceActiveFocus()
        AppLabel {
            id: deleteDialogLabel
            text: qsTr("Do you really want to delete the selected log files?")
            color: Config.StaticData.palette.secondary.col100
            Keys.onReturnPressed: deleteDialog.accept()
            Keys.onEnterPressed: deleteDialog.accept()
        }
        onAccepted: {
            if (typeof LogStore !== "undefined" && LogStore && logsPage.selectedPath !== "")
                LogStore.deleteFiles([logsPage.selectedPath])
        }
    }

    Dialog {
        id: messageDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("Uploading log file")
        standardButtons: Dialog.Close
        // Without focus:true the dialog stays unreachable for the keyboard.
        // Qt Quick has no default button: a focused standard button
        // only reacts to space, which is why the content accepts Enter.
        focus: true
        onOpened: messageLabel.forceActiveFocus()
        AppLabel {
            id: messageLabel
            wrapMode: Text.WordWrap
            width: 360
            color: Config.StaticData.palette.secondary.col100
            Keys.onReturnPressed: messageDialog.close()
            Keys.onEnterPressed: messageDialog.close()
        }
    }

    // Debug log viewer: shows the (tailed) pokerth-debug.log readably. The text
    // is selectable → copy/share; "Save as ..." exports the file.
    Dialog {
        id: debugLogDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("Debug log")
        standardButtons: Dialog.Close
        // Escape closes (popup), Enter as well – the content is only reading matter.
        // The Keys handlers have to hang off the content: Keys on a dialog/popup
        // is silently without effect (that is not an Item).
        focus: true
        onOpened: debugLogDialogContent.forceActiveFocus()
        width: Math.min(logsPage.width * 0.92, 680)
        height: Math.min(logsPage.height * 0.88, 680)

        property alias text: debugLogView.text

        ColumnLayout {
            id: debugLogDialogContent
            anchors.fill: parent
            spacing: 8
            Keys.onReturnPressed: debugLogDialog.close()
            Keys.onEnterPressed: debugLogDialog.close()

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Config.Theme.colorBox
                border.color: Config.StaticData.palette.secondary.col500
                border.width: 1
                radius: 4

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 6
                    clip: true

                    TextArea {
                        id: debugLogView
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextArea.NoWrap
                        color: Config.StaticData.palette.secondary.col100
                        font.family: "monospace"
                        font.pixelSize: 12
                        text: ""
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                CustomButton {
                    text: qsTr("Refresh")
                    onClicked: debugLogView.text =
                        (typeof LogStore !== "undefined" && LogStore)
                            ? LogStore.debugLogTail(200000) : ""
                }
                CustomButton {
                    text: qsTr("Save as ...")
                    onClicked: {
                        if (typeof LogStore !== "undefined" && LogStore)
                            LogStore.saveDebugLogDialog()
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }
    }
}
