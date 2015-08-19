pragma Singleton
import QtQuick 2.4
import QtQuick.Window 2.0

QtObject {
    id: myAppStyle

    property int appHeight: 0
    property int appWidth: 0

    property int fontSizeH1: Math.round(appHeight*0.10)
    property int fontSizeH2: Math.round(appHeight*0.09)
    property int fontSizeH3: Math.round(appHeight*0.08)
    property int fontSizeH4: Math.round(appHeight*0.07)
    property int fontSizeH5: Math.round(appHeight*0.06)
    property int fontSizeH6: Math.round(appHeight*0.05)

    property int listViewTitleFontSize: fontSizeH5
    property int listViewValueFontSize: fontSizeH6
    property int selectorTitleFontSize: fontSizeH2
    property int selectorValueFontSize: fontSizeH5
    property int selectorButtonFontSize: fontSizeH6
    property int radioButtonLabelFontSize: fontSizeH5
    property int textFieldFontSize: fontSizeH2

    property int rowLayoutSpacing: Math.round(appHeight*0.06)
    property int columnLayoutSpacing: Math.round(appHeight*0.06)

    Component.onCompleted: {
        console.log("pixelDensity: "+Screen.pixelDensity);
        console.log("logicalPixelDensity: " +Screen.logicalPixelDensity);
        console.log("devicePixelRatio: " +Screen.devicePixelRatio);
    }
}
