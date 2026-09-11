import QtQuick
import QtQuick.Layouts

import "../config" as Config

// One "results" step of the BBC/WEC player page: placement graphic (bars
// green→red or – switchable by tap – a pie chart) next to a table with a
// count and a percentage row per place 1–10, below it a legend mapping colour →
// place (as on the PokerTH player page). `values` = 10 frequencies of one
// step (stats.<block>.places[step]), `barColors` = Config.StaticData.heatColors.
ColumnLayout {
    id: result

    property var values: []
    property var barColors: []
    // Tapping the graphic switches between bars and pie chart
    // (like the bar/pie toggle of the BBC/WEC page).
    property bool showPie: false

    readonly property bool compact: Config.Responsive.compact
    readonly property var placeLabels: ["1st", "2nd", "3rd", "4th", "5th", "6th",
                                        "7th", "8th", "9th", "10th"]
    readonly property int total: {
        var s = 0
        for (var i = 0; i < values.length; ++i)
            s += Number(values[i]) || 0
        return s
    }
    readonly property real maxVal: {
        var m = 1
        for (var i = 0; i < values.length; ++i)
            m = Math.max(m, Number(values[i]) || 0)
        return m
    }

    spacing: 12

    GridLayout {
        Layout.fillWidth: true
        columns: result.compact ? 1 : 2
        columnSpacing: 14
        rowSpacing: 12

        // ── Graphic: bars (green→red, flush, without axes) or pie chart;
        //    switchable by tap. ───────────────────────────────────────────────
        Item {
            id: chartBox
            Layout.preferredWidth: result.compact ? 0 : 210
            Layout.fillWidth: result.compact
            Layout.alignment: Qt.AlignTop
            // Bar height matched to the table next to it (3 rows + header
            // ≈ 93 px + its topMargin) so that both end flush at the bottom.
            Layout.preferredHeight: result.showPie ? 170 : 100

            Row {
                anchors.fill: parent
                spacing: 1
                visible: !result.showPie
                Repeater {
                    model: 10
                    Item {
                        required property int index
                        width: (chartBox.width - 9) / 10   // 9 = spacing(1) * (10-1)
                        height: chartBox.height
                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: Math.max(1, (Number(result.values[index]) || 0)
                                                / result.maxVal * parent.height)
                            color: result.barColors[index % result.barColors.length]
                        }
                    }
                }
            }

            PlacementPieChart {
                anchors.centerIn: parent
                visible: result.showPie
                width: Math.min(parent.width, parent.height)
                height: width
                values: result.values
                colors: result.barColors
            }

            HoverHandler { cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: result.showPie = !result.showPie }
        }

        // ── Table: 1st…10th / count / percentage ───────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 8
            Layout.preferredHeight: tableCol.implicitHeight + 2
            color: Config.StaticData.palette.secondary.col600
            border.color: Config.StaticData.palette.secondary.col500
            border.width: 1
            radius: 4
            clip: true

            Column {
                id: tableCol
                anchors.fill: parent
                anchors.margins: 1

                // Kopf 1.…10.
                RowLayout {
                    width: tableCol.width
                    height: 30
                    spacing: 0
                    Repeater {
                        model: 10
                        AppLabel {
                            required property int index
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            horizontalAlignment: Text.AlignHCenter
                            text: (index + 1) + "."
                            color: Config.StaticData.palette.secondary.col200
                            font.pixelSize: Config.Theme.fontSizeCaption
                            font.bold: true
                        }
                    }
                }
                Rectangle {
                    width: tableCol.width; height: 1
                    color: Config.StaticData.palette.secondary.col500
                }
                // Count
                RowLayout {
                    width: tableCol.width
                    height: 30
                    spacing: 0
                    Repeater {
                        model: 10
                        AppLabel {
                            required property int index
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            horizontalAlignment: Text.AlignHCenter
                            text: "" + (Number(result.values[index]) || 0)
                            color: Config.StaticData.palette.secondary.col100
                            font.pixelSize: Config.Theme.fontSizeCaption
                        }
                    }
                }
                // Prozent
                RowLayout {
                    width: tableCol.width
                    height: 30
                    spacing: 0
                    Repeater {
                        model: 10
                        AppLabel {
                            required property int index
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            horizontalAlignment: Text.AlignHCenter
                            text: result.total > 0
                                  ? Math.round((Number(result.values[index]) || 0)
                                               / result.total * 100) + "%"
                                  : "0%"
                            color: Config.StaticData.palette.secondary.col300
                            font.pixelSize: Config.Theme.fontSizeCaption
                        }
                    }
                }
            }
        }
    }

    // ── Legend: colour → place (as on the PokerTH player page), applies to bars
    //    AND the pie chart. ───────────────────────────────────────────────
    Flow {
        Layout.fillWidth: true
        spacing: 12
        Repeater {
            model: 10
            Row {
                required property int index
                spacing: 5
                Rectangle {
                    width: 12; height: 12; radius: 2
                    anchors.verticalCenter: parent.verticalCenter
                    color: result.barColors[index % result.barColors.length]
                }
                AppLabel {
                    text: result.placeLabels[index]
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                }
            }
        }
    }
}
