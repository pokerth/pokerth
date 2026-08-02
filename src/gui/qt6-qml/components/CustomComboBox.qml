import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

ComboBox {
    id: comboBox
    model: Config.StaticData.languages
    textRole: "langName"

    Layout.leftMargin: 6
    Layout.preferredHeight: 24
    Layout.preferredWidth: 256

    Component.onCompleted: {
        var currentLangCode = Config.Parameters.language;
        if (model) {
            for (var i = 0; i < model.length; ++i) {
                if (model[i].code === currentLangCode) {
                    comboBox.currentIndex = i;
                    return;
                }
            }
            if (comboBox.currentIndex === -1 && model.length > 0) {
                comboBox.currentIndex = 0;
                console.warn("Language index is out of range, set to 0");
            }
        } else {
            console.warn("Language model is not valid.");
        }
    }

    onActivated: function (index) {
        const code = model[index].code;
        Config.Parameters.language = code;
        // console.log("Language setting updated to code:", code);

        // trigger dynamic translation
        LanguageManager.switchLanguage(code);
    }

    background: Rectangle {
        border.width: 1
        border.color: hoverArea.hovered ? Config.StaticData.palette.secondary.col100 : Config.StaticData.palette.secondary.col200
        color: hoverArea.hovered ? Config.StaticData.palette.secondary.col500 : Config.Theme.colorBox
    }

    HoverHandler {
        id: hoverArea
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        cursorShape: Qt.PointingHandCursor
    }

    delegate: ItemDelegate {
        id: comboBoxItem
        width: comboBox.width
        height: 24
        background: Rectangle {
            anchors.fill: parent
            color: boxItemMouse.hovered ? Config.StaticData.palette.secondary.col400 : Config.StaticData.palette.secondary.col600
        }
        contentItem: Text {
            anchors.fill: parent
            text: modelData.langName
            leftPadding: 12
            rightPadding: 12
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            color: boxItemMouse.hovered ? Config.StaticData.palette.secondary.col100 : Config.StaticData.palette.secondary.col200
        }

        HoverHandler {
            id: boxItemMouse
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            cursorShape: Qt.PointingHandCursor
        }
    }
}
