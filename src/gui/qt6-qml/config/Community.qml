pragma Singleton
import QtQuick

// Central registry of the ranking sources (PokerTH / BBC / WEC): order in the
// switch, base URLs and the routing to the matching player page. Bundles what
// used to be duplicated in CommunitySwitch, Bbc-/WecRankingPage and
// CommunityPlayerView. The display names are proper names and are not
// translated (as before in the CommunitySwitch).
QtObject {
    readonly property var entries: [
        { label: "PokerTH", key: "pokerth" },
        { label: "BBC",     key: "bbc" },
        { label: "WEC",     key: "wec" }
    ]

    function has(community) {
        for (var i = 0; i < entries.length; ++i)
            if (entries[i].key === community)
                return true
        return false
    }

    function baseUrlFor(community) {
        return community === "bbc" ? "https://bbc.pokerth.net"
                                   : "https://wec.pokerth.net"
    }

    // Player page of the source: PokerTH has a page of its own, BBC/WEC share
    // the CommunityPlayerView (which derives its base URL + stat blocks from
    // `community` itself).
    function playerPageUrl(community) {
        return community === "pokerth"
               ? "qrc:/pages/PokerthPlayerPage.qml"
               : "qrc:/components/CommunityPlayerView.qml"
    }
    function playerPageProps(community, nick) {
        return community === "pokerth" ? { username: nick }
                                       : { community: community, nickname: nick }
    }
}
