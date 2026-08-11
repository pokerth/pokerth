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
    // die verbleibenden Spielerboxen behalten ihre Position (Standard). false =
    // Ellipse wird ohne den freien Sitz neu verteilt (Boxen rücken nach).
    property bool keepEmptySeats: true

    property bool showCommunityContent: true

    // Forum-Neuigkeiten (Zeitungs-Icon in der Topbar mit Zähler ungelesener
    // Beiträge, Liste + Beitragsansicht). Unabhängig von den Community-
    // Inhalten: das Forum ist die offizielle PokerTH-Seite, nicht BBC/WEC.
    property bool showForumNews: true

    // Vorausgewählte Ranglisten-Quelle ("pokerth" | "bbc" | "wec") für Table
    // Info und Player Stats, wenn Community-Inhalte aktiv sind. Auswahl siehe
    // Config.Community.entries.
    property string defaultCommunity: "pokerth"

    // Optionales Admin-Feature: Ersteller eines BBC-Step-/WEC-Invite-Spiels
    // können im Warteraum passende idle Spieler in den Chat vorschlagen
    // (Config.BotSuggest). Nur wirksam bei aktivierten Community-Inhalten.
    // Standardmäßig AUS: nur für die wenigen BBC/WEC-Admins relevant, die
    // Masse interessiert sich nur für die Community-Ranglisten.
    property bool showCommunitySuggest: false

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
