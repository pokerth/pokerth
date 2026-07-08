import QtQuick
import QtQuick.Layouts

import "../config" as Config

// Ein „Results"-Step der BBC/WEC-Spielerseite: Platzierungs-Grafik (Balken
// grün→rot bzw. – per Tap umschaltbar – Kreisdiagramm) neben einer Tabelle mit
// Anzahl- und Prozent-Zeile je Platz 1–10, darunter eine Legende, die Farbe →
// Platz zuordnet (wie auf der PokerTH-Spielerseite). `values` = 10 Häufigkeiten
// eines Steps (stats.<block>.places[step]), `barColors` = Config.StaticData.heatColors.
ColumnLayout {
    id: result

    property var values: []
    property var barColors: []
    // Tippen auf die Grafik schaltet zwischen Balken und Kreisdiagramm um
    // (wie das Bar/Pie-Toggle der BBC/WEC-Seite).
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

        // ── Grafik: Balken (grün→rot, bündig, ohne Achsen) oder Kreisdiagramm;
        //    per Tap umschaltbar. ─────────────────────────────────────────────
        Item {
            id: chartBox
            Layout.preferredWidth: result.compact ? 0 : 210
            Layout.fillWidth: result.compact
            Layout.alignment: Qt.AlignTop
            // Balkenhöhe an die Tabelle daneben angeglichen (3 Zeilen + Kopf
            // ≈ 93 px + deren topMargin), damit beide unten bündig abschließen.
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

        // ── Tabelle: 1.…10. / Anzahl / Prozent ──────────────────────────────
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
                // Anzahl
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

    // ── Legende: Farbe → Platz (wie PokerTH-Spielerseite), gilt für Balken
    //    UND Kreisdiagramm. ────────────────────────────────────────────────
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
