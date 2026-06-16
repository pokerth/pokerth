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

    property var seasonModel: []      // [{ value, label }]
    property int currentSeason: 0
    property bool alltime: false

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

        // Filterleiste – Saison + All-Time
        Flow {
            Layout.fillWidth: true
            spacing: 16

            Row {
                spacing: 8
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Season:")
                    color: Config.StaticData.palette.secondary.col200
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: Config.Theme.fontSizeBody
                }
                ComboBox {
                    id: seasonCombo
                    width: 160
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
        }

        CommunityRankingView {
            id: view
            Layout.fillWidth: true
            Layout.fillHeight: true

            baseUrl: "https://bbc.pokerth.net"
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
                bbcPage.currentSeason = sel
                seasonCombo.currentIndex = Math.max(0, seasonCombo.indexOfValue(sel))
                // Eingebettete Initialdaten der aktuellen Saison anzeigen.
                rows = jsonAttr(html, "results") || []
            }

            Component.onCompleted: load()
        }
    }
}
