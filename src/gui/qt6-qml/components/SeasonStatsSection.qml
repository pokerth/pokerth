import QtQuick
import QtQuick.Layouts

import "../config" as Config

// Season-Stats-Block der Spielerseite – Nachbau der pokerth.net-Grafiken
// (Chart.js/Vue) mit Bordmitteln: Positions-Tabelle, Kreis- und Balken-
// diagramm der Platzierungs-Verteilung. `counts` = Häufigkeit je Platz
// (Index 0 = Platz 1, entspricht bar_stats der API), `percents` = die vom
// Server gelieferten Prozent-Strings (stats[1]). Farben 1:1 wie die Website
// über Config.StaticData.placementColors.
ColumnLayout {
    id: section

    property var counts: []
    property var percents: []

    readonly property bool compact: Config.Responsive.compact
    readonly property var colors: Config.StaticData.placementColors
    readonly property var placeLabels: ["1st", "2nd", "3rd", "4th", "5th", "6th",
                                        "7th", "8th", "9th", "10th"]

    readonly property int total: {
        var s = 0
        for (var i = 0; i < counts.length; ++i)
            s += Number(counts[i]) || 0
        return s
    }

    spacing: 10
    visible: total > 0

    // ── Überschrift ─────────────────────────────────────────────────────────
    AppLabel {
        text: qsTr("Season Stats")
        color: Config.StaticData.palette.secondary.col200
        font.pixelSize: Config.Theme.fontSizeBody
        font.bold: true
    }

    // ── Positions-Tabelle (bei Bedarf horizontal scrollbar) ─────────────────
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: tableCol.implicitHeight + 2
        color: Config.StaticData.palette.secondary.col600
        border.color: Config.StaticData.palette.secondary.col500
        border.width: 1
        radius: 4
        clip: true

        Flickable {
            anchors.fill: parent
            anchors.margins: 1
            contentWidth: Math.max(width, tableCol.implicitWidth)
            contentHeight: height
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalFlick

            Column {
                id: tableCol
                readonly property real labelW: 60
                readonly property real cellW: section.compact ? 42 : 48
                readonly property real rowH: 28

                // Kopfzeile: leer | 1 … 10
                Row {
                    Item { width: tableCol.labelW; height: tableCol.rowH }
                    Repeater {
                        model: 10
                        Item {
                            required property int index
                            width: tableCol.cellW
                            height: tableCol.rowH
                            AppLabel {
                                anchors.centerIn: parent
                                text: index + 1
                                color: Config.StaticData.palette.secondary.col200
                                font.pixelSize: Config.Theme.fontSizeCaption
                                font.bold: true
                            }
                        }
                    }
                }
                Rectangle { width: tableCol.labelW + 10 * tableCol.cellW; height: 1
                            color: Config.StaticData.palette.secondary.col500 }

                // Anzahl-Zeile
                Row {
                    Item {
                        width: tableCol.labelW; height: tableCol.rowH
                        AppLabel {
                            anchors.verticalCenter: parent.verticalCenter
                            x: 8
                            text: qsTr("Games")
                            color: Config.StaticData.palette.secondary.col300
                            font.pixelSize: Config.Theme.fontSizeCaption
                        }
                    }
                    Repeater {
                        model: 10
                        Item {
                            required property int index
                            width: tableCol.cellW
                            height: tableCol.rowH
                            AppLabel {
                                anchors.centerIn: parent
                                text: "" + (Number(section.counts[index]) || 0)
                                color: Config.StaticData.palette.secondary.col100
                                font.pixelSize: Config.Theme.fontSizeCaption
                            }
                        }
                    }
                }

                // Prozent-Zeile
                Row {
                    Item {
                        width: tableCol.labelW; height: tableCol.rowH
                        AppLabel {
                            anchors.verticalCenter: parent.verticalCenter
                            x: 8
                            text: qsTr("Share")
                            color: Config.StaticData.palette.secondary.col300
                            font.pixelSize: Config.Theme.fontSizeCaption
                        }
                    }
                    Repeater {
                        model: 10
                        Item {
                            required property int index
                            width: tableCol.cellW
                            height: tableCol.rowH
                            AppLabel {
                                anchors.centerIn: parent
                                text: section.percents && section.percents[index] !== undefined
                                      ? section.percents[index] : ""
                                color: Config.StaticData.palette.secondary.col300
                                font.pixelSize: Config.Theme.fontSizeCaption
                            }
                        }
                    }
                }
            }
        }
    }

    // ── Legende ─────────────────────────────────────────────────────────────
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
                    color: section.colors[index]
                }
                AppLabel {
                    text: section.placeLabels[index]
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                }
            }
        }
    }

    // ── Kreisdiagramm ───────────────────────────────────────────────────────
    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: section.compact ? 200 : 240
        PlacementPieChart {
            anchors.centerIn: parent
            width: parent.height
            height: parent.height
            values: section.counts
            colors: section.colors
        }
    }

    // ── Balkendiagramm ──────────────────────────────────────────────────────
    Item {
        id: barChart
        Layout.fillWidth: true
        Layout.preferredHeight: section.compact ? 170 : 210

        readonly property int maxVal: {
            var m = 1
            for (var i = 0; i < section.counts.length; ++i)
                m = Math.max(m, Number(section.counts[i]) || 0)
            return m
        }
        readonly property int tickStep: Math.max(1, Math.ceil(maxVal / 6))
        readonly property int topValue: tickStep * Math.ceil(maxVal / tickStep)
        readonly property int tickCount: Math.max(1, topValue / tickStep)

        readonly property real yAxisW: 22
        readonly property real valueRowH: 15
        readonly property real labelRowH: 18
        readonly property real plotTop: valueRowH
        readonly property real plotBottom: height - labelRowH
        readonly property real plotH: plotBottom - plotTop
        readonly property real plotLeft: yAxisW
        readonly property real plotW: width - yAxisW

        // Gitterlinien + Y-Achsenbeschriftung
        Repeater {
            model: barChart.tickCount + 1
            Item {
                required property int index
                readonly property real val: index * barChart.tickStep
                readonly property real yPos: barChart.plotBottom
                                             - (index / barChart.tickCount) * barChart.plotH
                Rectangle {
                    x: barChart.plotLeft
                    y: parent.yPos
                    width: barChart.plotW
                    height: 1
                    color: Config.StaticData.palette.secondary.col500
                    opacity: 0.5
                }
                AppLabel {
                    x: 0
                    width: barChart.yAxisW - 4
                    y: parent.yPos - height / 2
                    horizontalAlignment: Text.AlignRight
                    text: "" + parent.val
                    color: Config.StaticData.palette.secondary.col300
                    font.pixelSize: 10
                }
            }
        }

        // Balken
        Row {
            x: barChart.plotLeft
            y: barChart.plotTop
            width: barChart.plotW
            height: barChart.plotH
            Repeater {
                model: 10
                Item {
                    id: barCell
                    required property int index
                    readonly property int val: Number(section.counts[index]) || 0
                    width: barChart.plotW / 10
                    height: barChart.plotH

                    Rectangle {
                        id: bar
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width * 0.62
                        height: barCell.val > 0
                                ? Math.max(2, barCell.val / barChart.topValue * parent.height)
                                : 0
                        radius: 2
                        color: section.colors[barCell.index]
                    }
                    AppLabel {
                        anchors.bottom: bar.top
                        anchors.bottomMargin: 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: barCell.val > 0
                        text: "" + barCell.val
                        color: Config.StaticData.palette.secondary.col200
                        font.pixelSize: 10
                    }
                }
            }
        }

        // X-Achsenbeschriftung
        Row {
            x: barChart.plotLeft
            y: barChart.plotBottom
            width: barChart.plotW
            height: barChart.labelRowH
            Repeater {
                model: 10
                Item {
                    required property int index
                    width: barChart.plotW / 10
                    height: barChart.labelRowH
                    AppLabel {
                        anchors.centerIn: parent
                        text: section.placeLabels[index]
                        color: Config.StaticData.palette.secondary.col300
                        font.pixelSize: 10
                    }
                }
            }
        }
    }
}
