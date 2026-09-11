import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// BBC (Best Brainies Cup) ranking – https://bbc.pokerth.net/results/ranking
// Filters as there: season selection + all-time. Column "Step1" only in season 9/10.
Rectangle {
    id: bbcPage
    objectName: "bbcRankingPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    readonly property bool compact: Config.Responsive.compact

    // Initial focus into the search field (filters live) – NOT on mobile devices,
    // that would pull up the on-screen keyboard unasked.
    StackView.onActivated: {
        if (!Config.Responsive.isMobile)
            Qt.callLater(searchField.forceActiveFocus)
    }

    property var seasonModel: []      // [{ value, label }]
    property int currentSeason: 0
    property bool alltime: false

    // Set by the globe toggle → restore the filter (season/all-time).
    property var restoreState: null
    function captureState() {
        return { currentSeason: currentSeason, alltime: alltime }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        AppLabel {
            text: qsTr("BBC Ranking")
            color: Config.StaticData.palette.secondary.col200
            font.pointSize: 14
            font.bold: true
        }

        // Filter bar – season + all-time + search. In compact mode (portrait) the
        // grid wraps to two columns so that nothing is cut off.
        GridLayout {
            Layout.fillWidth: true
            columns: bbcPage.compact ? 2 : 4
            columnSpacing: 16
            rowSpacing: 8

            RankingFilterField {
                id: seasonField
                label: qsTr("Season:")
                model: bbcPage.seasonModel
                comboEnabled: !bbcPage.alltime && bbcPage.seasonModel.length > 0
                comboWidth: 160
                compact: bbcPage.compact
                onActivated: function(value) {
                    bbcPage.currentSeason = value
                    view.applyFilter()
                }
            }

            CheckBox {
                id: alltimeCheck
                text: qsTr("All-Time")
                checked: bbcPage.alltime
                onToggled: {
                    bbcPage.alltime = checked
                    view.applyFilter()
                }
            }

            // Spacer only in the desktop layout (pushes the search to the right).
            Item { Layout.fillWidth: true; visible: !bbcPage.compact }

            TextField {
                id: searchField
                Layout.fillWidth: bbcPage.compact
                Layout.preferredWidth: 180
                Layout.columnSpan: bbcPage.compact ? 2 : 1
                placeholderText: qsTr("Search nickname")
                onTextChanged: view.searchText = text.trim()
            }
        }

        CommunityRankingView {
            id: view
            Layout.fillWidth: true
            Layout.fillHeight: true

            baseUrl: "https://bbc.pokerth.net"

            onPlayerActivated: function(nick) {
                if (nick === "")
                    return
                bbcPage.StackView.view.push("qrc:/components/CommunityPlayerView.qml", {
                    community: "bbc",
                    nickname: nick
                })
            }

            // "Step1" only visible in season 9/10 (as on the website).
            extraColumns: (!bbcPage.alltime
                           && (bbcPage.currentSeason === 9 || bbcPage.currentSeason === 10))
                          ? [{ label: qsTr("Step1"), field: "step1" }] : []
            makeBody: function() {
                return { season: bbcPage.alltime ? 0 : bbcPage.currentSeason }
            }

            onInitialData: function(html) {
                var seasons = jsonAttr(html, "allseasons") || []
                var sel = parseInt(attr(html, "season")) || (seasons.length ? seasons[seasons.length - 1] : 0)
                // Neueste Saison zuerst.
                var m = []
                for (var i = seasons.length - 1; i >= 0; --i)
                    m.push({ value: seasons[i], label: qsTr("Season %1").arg(seasons[i]) })
                bbcPage.seasonModel = m
                if (bbcPage.restoreState) {
                    // Restore the remembered filter and load its data.
                    sel = bbcPage.restoreState.currentSeason
                    bbcPage.alltime = bbcPage.restoreState.alltime
                    bbcPage.restoreState = null
                    bbcPage.currentSeason = sel
                    seasonField.currentIndex = Math.max(0, seasonField.indexOfValue(sel))
                    applyFilter()
                    return
                }
                bbcPage.currentSeason = sel
                seasonField.currentIndex = Math.max(0, seasonField.indexOfValue(sel))
                // Show the embedded initial data of the current season.
                rows = jsonAttr(html, "results") || []
            }

            Component.onCompleted: load()
        }
    }
}
