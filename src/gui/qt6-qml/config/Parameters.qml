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
    property bool showOwnCardsOnMouseClickCheckbox: false
    property bool disableSplashScreenOnStartupCheckbox: false
    property bool doNotTranslatePokerTermsCheckbox: true

    property bool tableZoomEnabled: true

    // Sitze von Spielern, die den Tisch verlassen haben (Disconnect, Kick,
    // Verlassen, Ausgeschieden), als unsichtbare Platzhalter im Ring behalten →
    // die verbleibenden Spielerboxen behalten ihre Position. false = Ellipse wird
    // ohne den freien Sitz neu verteilt (bisheriges Verhalten).
    property bool keepEmptySeats: false

    property bool showCommunityContent: true

    // Vorausgewählte Ranglisten-Quelle ("pokerth" | "bbc" | "wec") für Table
    // Info und Player Stats, wenn Community-Inhalte aktiv sind. Auswahl siehe
    // Config.Community.entries.
    property string defaultCommunity: "pokerth"

    // Tooltips auf Icon-Buttons (nur Desktop – auf Touch gibt es kein Hover).
    property bool showTooltips: true

    // From the networkTab ColumnLayout
    property bool showCountryFlagOnAvatarCheckbox: true
    property bool showNetworkStatusColorOnAvatarCheckbox: true
    property bool focusBetInputOnTurnCheckbox: false
    property bool preventAccidentalCallAfterBigRaiseCheckbox: true
    property bool doNotHideIgnoredPlayerAvatarsCheckbox: false
    property bool showLobbyChatCheckbox: true
}
