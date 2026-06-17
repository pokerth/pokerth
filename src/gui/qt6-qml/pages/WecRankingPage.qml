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

        Label {
            text: qsTr("WEC Ranking")
            color: Config.StaticData.palette.secondary.col200
            font.family: Config.StaticData.loadedFont.font.family
            font.pointSize: 14
            font.bold: true
        }

        // Filterleiste – Jahr + Monat + All-Year + All-Time + Suche
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Row {
                spacing: 8
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Year:")
                    color: Config.StaticData.palette.secondary.col200
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: Config.Theme.fontSizeBody
                }
                ComboBox {
                    id: yearCombo
                    width: 110
                    enabled: !wecPage.alltime && wecPage.yearModel.length > 0
                    model: wecPage.yearModel
                    textRole: "label"
                    valueRole: "value"
                    onActivated: {
                        wecPage.currentYear = currentValue
                        view.applyFilter()
                    }
                }
            }

            Row {
                spacing: 8
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Month:")
                    color: Config.StaticData.palette.secondary.col200
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: Config.Theme.fontSizeBody
                }
                ComboBox {
                    id: monthCombo
                    width: 140
                    enabled: !wecPage.alltime && !wecPage.allyear
                    model: wecPage.monthModel
                    textRole: "label"
                    valueRole: "value"
                    onActivated: {
                        wecPage.currentMonth = currentValue
                        view.applyFilter()
                    }
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

            Item { Layout.fillWidth: true }

            TextField {
                Layout.preferredWidth: 180
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
                    baseUrl: "https://wec.pokerth.net",
                    nickname: nick,
                    blocks: [
                        { label: qsTr("This month"), key: "month" },
                        { label: qsTr("This year"),  key: "year" },
                        { label: qsTr("All-time"),   key: "alltime" }
                    ]
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
                    yearCombo.currentIndex = Math.max(0, yearCombo.indexOfValue(wecPage.currentYear))
                    monthCombo.currentIndex = Math.max(0, monthCombo.indexOfValue(wecPage.currentMonth))
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
                yearCombo.currentIndex = Math.max(0, yearCombo.indexOfValue(wecPage.currentYear))
                monthCombo.currentIndex = Math.max(0, monthCombo.indexOfValue(wecPage.currentMonth))
                // Eingebettete Initialdaten anzeigen.
                rows = jsonAttr(html, "stats") || []
            }

            Component.onCompleted: load()
        }
    }
}
