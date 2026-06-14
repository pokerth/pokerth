pragma Singleton
import QtQuick 6.5
import QtCore

Settings {
    property string language: StaticData.findSupportedLocale(Qt.locale().name)

    property bool displayRightToolboxCheckbox: true
    property bool displayLeftToolboxCheckbox: true
    property bool fadeOutLosingCardsAnimationCheckbox: true
    property bool animatedCardsCheckbox: true
    property bool reverseFKeysOrderCheckbox: false
    property bool showBlindButtonsCheckbox: true
    property bool showHandChanceMonitorCheckbox: true
    property bool showOwnCardsOnMouseClickCheckbox: false
    property bool disableSplashScreenOnStartupCheckbox: false
    property bool doNotTranslatePokerTermsCheckbox: true

    property bool tableZoomEnabled: true

    // From the networkTab ColumnLayout
    property bool showCountryFlagOnAvatarCheckbox: true
    property bool showNetworkStatusColorOnAvatarCheckbox: true
    property bool focusBetInputOnTurnCheckbox: false
    property bool preventAccidentalCallAfterBigRaiseCheckbox: true
    property bool doNotHideIgnoredPlayerAvatarsCheckbox: false
    property bool showLobbyChatCheckbox: true
}
