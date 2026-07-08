import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// WEC (World Elite Cup) Rangliste – https://wec.pokerth.net/results/ranking
// Filter wie dort: Jahr + Monat, plus All-Year und All-Time.
Rectangle {
    id: wecPage
    objectName: "wecRankingPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    readonly property bool compact: Config.Responsive.compact

    property var yearModel: []        // [{ value, label }]
    property var monthModel: [
        { value: 1,  label: qsTr("January") },   { value: 2,  label: qsTr("February") },
        { value: 3,  label: qsTr("March") },     { value: 4,  label: qsTr("April") },
        { value: 5,  label: qsTr("May") },       { value: 6,  label: qsTr("June") },
        { value: 7,  label: qsTr("July") },      { value: 8,  label: qsTr("August") },
        { value: 9,  label: qsTr("September") }, { value: 10, label: qsTr("October") },
        { value: 11, label: qsTr("November") },  { value: 12, label: qsTr("December") }
    ]
    property int currentYear: 0
    property int currentMonth: 0
    property bool allyear: false
    property bool alltime: false

    // Vom Globus-Toggle gesetzt → Filter (Jahr/Monat/All-Year/All-Time) wiederherstellen.
    property var restoreState: null
    function captureState() {
        return { currentYear: currentYear, currentMonth: currentMonth,
                 allyear: allyear, alltime: alltime }
    }

    Component.onCompleted: {
        // Jahres-Liste aufbauen: aktuelles Jahr … 2012 (wie auf der Webseite).
        var now = new Date().getFullYear()
        var m = []
        for (var y = now; y >= 2012; --y)
            m.push({ value: y, label: "" + y })
        yearModel = m
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        AppLabel {
            text: qsTr("WEC Ranking")
            color: Config.StaticData.palette.secondary.col200
            font.pointSize: 14
            font.bold: true
        }

        // Filterleiste – Jahr + Monat + All-Year + All-Time + Suche. Im Compact-
        // Modus (Portrait) bricht das Grid auf zwei Spalten um (Jahr|Monat,
        // All-Year|All-Time, Suche), damit nichts abgeschnitten wird.
        GridLayout {
            Layout.fillWidth: true
            columns: wecPage.compact ? 2 : 6
            columnSpacing: 16
            rowSpacing: 8

            RankingFilterField {
                id: yearField
                label: qsTr("Year:")
                model: wecPage.yearModel
                comboEnabled: !wecPage.alltime && wecPage.yearModel.length > 0
                comboWidth: 110
                compact: wecPage.compact
                onActivated: function(value) {
                    wecPage.currentYear = value
                    view.applyFilter()
                }
            }

            RankingFilterField {
                id: monthField
                label: qsTr("Month:")
                model: wecPage.monthModel
                comboEnabled: !wecPage.alltime && !wecPage.allyear
                comboWidth: 140
                compact: wecPage.compact
                onActivated: function(value) {
                    wecPage.currentMonth = value
                    view.applyFilter()
                }
            }

            CheckBox {
                id: allyearCheck
                text: qsTr("All-Year")
                checked: wecPage.allyear
                enabled: !wecPage.alltime
                onToggled: {
                    wecPage.allyear = checked
                    view.applyFilter()
                }
            }

            CheckBox {
                id: alltimeCheck
                text: qsTr("All-Time")
                checked: wecPage.alltime
                onToggled: {
                    wecPage.alltime = checked
                    view.applyFilter()
                }
            }

            // Abstandshalter nur im Desktop-Layout (drückt die Suche nach rechts).
            Item { Layout.fillWidth: true; visible: !wecPage.compact }

            TextField {
                Layout.fillWidth: wecPage.compact
                Layout.preferredWidth: 180
                Layout.columnSpan: wecPage.compact ? 2 : 1
                placeholderText: qsTr("Search nickname")
                onTextChanged: view.searchText = text.trim()
            }
        }

        CommunityRankingView {
            id: view
            Layout.fillWidth: true
            Layout.fillHeight: true

            baseUrl: "https://wec.pokerth.net"

            onPlayerActivated: function(nick) {
                if (nick === "")
                    return
                wecPage.StackView.view.push("qrc:/components/CommunityPlayerView.qml", {
                    community: "wec",
                    nickname: nick
                })
            }

            makeBody: function() {
                return {
                    year:  wecPage.alltime ? 0 : wecPage.currentYear,
                    month: (wecPage.alltime || wecPage.allyear) ? 0 : wecPage.currentMonth
                }
            }

            onInitialData: function(html) {
                if (wecPage.restoreState) {
                    // Gemerkten Filter wiederherstellen und dessen Daten laden.
                    var r = wecPage.restoreState
                    wecPage.restoreState = null
                    wecPage.currentYear = r.currentYear
                    wecPage.currentMonth = r.currentMonth
                    wecPage.allyear = r.allyear
                    wecPage.alltime = r.alltime
                    yearField.currentIndex = Math.max(0, yearField.indexOfValue(wecPage.currentYear))
                    monthField.currentIndex = Math.max(0, monthField.indexOfValue(wecPage.currentMonth))
                    applyFilter()
                    return
                }
                var y = parseInt(attr(html, "stats_year"))
                var mo = parseInt(attr(html, "stats_month"))
                if (!isNaN(y))
                    wecPage.currentYear = y
                if (!isNaN(mo)) {
                    wecPage.currentMonth = mo
                    wecPage.allyear = false
                } else {
                    wecPage.allyear = true        // Jahr ohne Monat → ganzes Jahr
                }
                yearField.currentIndex = Math.max(0, yearField.indexOfValue(wecPage.currentYear))
                monthField.currentIndex = Math.max(0, monthField.indexOfValue(wecPage.currentMonth))
                // Eingebettete Initialdaten anzeigen.
                rows = jsonAttr(html, "stats") || []
            }

            Component.onCompleted: load()
        }
    }
}
