import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// BBC (Best Brainies Cup) Rangliste – https://bbc.pokerth.net/results/ranking
// Filter wie dort: Saison-Auswahl + All-Time. Spalte "Step1" nur in Saison 9/10.
Rectangle {
    id: bbcPage
    objectName: "bbcRankingPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    readonly property bool compact: Config.Responsive.compact

    property var seasonModel: []      // [{ value, label }]
    property int currentSeason: 0
    property bool alltime: false

    // Vom Globus-Toggle gesetzt → Filter (Saison/All-Time) wiederherstellen.
    property var restoreState: null
    function captureState() {
        return { currentSeason: currentSeason, alltime: alltime }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Label {
            text: qsTr("BBC Ranking")
            color: Config.StaticData.palette.secondary.col200
            font.family: Config.StaticData.loadedFont.font.family
            font.pointSize: 14
            font.bold: true
        }

        // Filterleiste – Saison + All-Time + Suche. Im Compact-Modus (Portrait)
        // bricht das Grid auf zwei Spalten um, damit nichts abgeschnitten wird.
        GridLayout {
            Layout.fillWidth: true
            columns: bbcPage.compact ? 2 : 4
            columnSpacing: 16
            rowSpacing: 8

            RowLayout {
                Layout.fillWidth: bbcPage.compact
                spacing: 8
                Label {
                    Layout.alignment: Qt.AlignVCenter
                    text: qsTr("Season:")
                    color: Config.StaticData.palette.secondary.col200
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: Config.Theme.fontSizeBody
                }
                ComboBox {
                    id: seasonCombo
                    Layout.fillWidth: bbcPage.compact
                    Layout.preferredWidth: 160
                    enabled: !bbcPage.alltime && bbcPage.seasonModel.length > 0
                    model: bbcPage.seasonModel
                    textRole: "label"
                    valueRole: "value"
                    onActivated: {
                        bbcPage.currentSeason = currentValue
                        view.applyFilter()
                    }
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

            // Abstandshalter nur im Desktop-Layout (drückt die Suche nach rechts).
            Item { Layout.fillWidth: true; visible: !bbcPage.compact }

            TextField {
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
                    baseUrl: "https://bbc.pokerth.net",
                    nickname: nick,
                    blocks: [
                        { label: qsTr("This season"), key: "season" },
                        { label: qsTr("All-time"),    key: "alltime" }
                    ]
                })
            }

            // "Step1" nur sichtbar in Saison 9/10 (wie auf der Webseite).
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
                    // Gemerkten Filter wiederherstellen und dessen Daten laden.
                    sel = bbcPage.restoreState.currentSeason
                    bbcPage.alltime = bbcPage.restoreState.alltime
                    bbcPage.restoreState = null
                    bbcPage.currentSeason = sel
                    seasonCombo.currentIndex = Math.max(0, seasonCombo.indexOfValue(sel))
                    applyFilter()
                    return
                }
                bbcPage.currentSeason = sel
                seasonCombo.currentIndex = Math.max(0, seasonCombo.indexOfValue(sel))
                // Eingebettete Initialdaten der aktuellen Saison anzeigen.
                rows = jsonAttr(html, "results") || []
            }

            Component.onCompleted: load()
        }
    }
}
