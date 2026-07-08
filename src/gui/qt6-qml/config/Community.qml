pragma Singleton
import QtQuick

// Zentrale Registry der Ranglisten-Quellen (PokerTH / BBC / WEC): Reihenfolge im
// Umschalter, Basis-URLs und das Routing zur passenden Player-Page. Bündelt, was
// vorher in CommunitySwitch, Bbc-/WecRankingPage und CommunityPlayerView
// mehrfach dupliziert war. Die Anzeige-Namen sind Eigennamen und werden nicht
// übersetzt (wie bisher im CommunitySwitch).
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

    // Player-Page der Quelle: PokerTH hat eine eigene Seite, BBC/WEC teilen sich
    // die CommunityPlayerView (die ihre Basis-URL + Stat-Blöcke aus `community`
    // selbst ableitet).
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
