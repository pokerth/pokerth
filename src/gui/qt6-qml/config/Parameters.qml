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

    // Keep seats of players who have left the table (disconnect, kick, leave,
    // knocked out) as invisible placeholders in the ring → the remaining player
    // boxes keep their position (default). false = the ellipse is redistributed
    // without the free seat (the boxes move up).
    property bool keepEmptySeats: true

    property bool showCommunityContent: true

    // Forum news (newspaper icon in the top bar with a counter of unread posts,
    // list + post view). Independent of the community content: the forum is the
    // official PokerTH site, not BBC/WEC.
    property bool showForumNews: true

    // Preselected ranking source ("pokerth" | "bbc" | "wec") for table info and
    // player stats when community content is active. For the selection see
    // Config.Community.entries.
    property string defaultCommunity: "pokerth"

    // Optional admin feature: creators of a BBC step/WEC invite game can
    // suggest matching idle players in the chat of the waiting room
    // (Config.BotSuggest). Only effective with community content enabled.
    // OFF by default: only relevant for the few BBC/WEC admins, the masses
    // are only interested in the community rankings.
    property bool showCommunitySuggest: false

    // Tooltips on icon buttons (desktop only – on touch there is no hover).
    property bool showTooltips: true

    // From the networkTab ColumnLayout
    property bool showCountryFlagOnAvatarCheckbox: true
    property bool showNetworkStatusColorOnAvatarCheckbox: true
    property bool focusBetInputOnTurnCheckbox: false
    property bool preventAccidentalCallAfterBigRaiseCheckbox: true
    property bool doNotHideIgnoredPlayerAvatarsCheckbox: false
    property bool showLobbyChatCheckbox: true
}
