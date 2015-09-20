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

    property int radioButtonLabelFontSize: fontSizeH5
    property int textFieldFontSize: fontSizeH3

    property int rowLayoutSpacing: Math.round(appHeight*0.06)
    property int columnLayoutSpacing: Math.round(appHeight*0.02)

    //Selector Box (white on darken background)
    property int selectorBoxHeight: Math.round(appHeight*0.8)
    property int selectorBoxWidth: Math.round(appWidth*0.6)
    property int selectorBoxRadius: Math.round(appWidth*0.01)
    property int selectorBoxTitleFontSize: fontSizeH4
    property bool selectorBoxTitleFontBold: false
    property int selectorBoxTitleTopMargin: -Math.round(appHeight*0.02)
    property int selectorBoxValueFontSize: fontSizeH5
    property int selectorBoxButtonFontSize: fontSizeH5
    property int selectorBoxButtonMouseAreaMargin: -Math.round(appHeight*0.02)
    property int selectorBoxButtonRowLayoutSpacing: Math.round(appHeight*0.10)
    property bool selectorBoxButtonFontBold: false
    property int selectorBoxContentMargins: Math.round(appHeight*0.05)
    property int selectorBoxRealContentLeftMargin: Math.round(appHeight*0.05)
    property int selectorBoxRealContentRightMargin: Math.round(appHeight*0.05)

    //SpinBoxSelector
    property int spinBoxSelectorTextFieldPrefixExtraLeftMargin: Math.round(appHeight*0.03)

    //SliderStyle
    property int sliderHandleWidth: Math.round(appHeight*0.10)
    property int sliderGrooveHeight: Math.round(appHeight*0.033)
    property int sliderGrooveRectHeight: Math.round(appHeight*0.025)

    //ListViewDelegate
    property real listViewDelegateHeightBasedOnContentFactor: 1.5
    property int listViewDelegateTitleAndValueMargins: 30
    property int listViewDelegateTitleAndValueSpacing: -3
    property int listViewDelegateTitleFontSize: fontSizeH5
    property int listViewDelegateValueFontSize: fontSizeH6
    property int listViewDelegateSeperatorLineIndent: 30
    property int listViewDelegateSeperatorLineHeight: 1

    Component.onCompleted: {
        console.log("pixelDensity: "+Screen.pixelDensity);
        console.log("logicalPixelDensity: " +Screen.logicalPixelDensity);
        console.log("devicePixelRatio: " +Screen.devicePixelRatio);
    }
}
