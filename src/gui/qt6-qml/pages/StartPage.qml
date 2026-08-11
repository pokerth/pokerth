import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

Rectangle {
    id: startPage
    // An den sichtbaren Bereich (StackView unterhalb der Topbar) binden, nicht
    // an das ganze Fenster – sonst ist die Box vertikal nicht zentriert.
    width: mainStackView.width
    height: mainStackView.height
    color: "transparent"

    Image {
        id: preLoaderBackground
        anchors.fill: parent
        source: "../resources/startWindowBackground.png"
        fillMode: Image.PreserveAspectCrop
    }

    // Innenabstände der Box – identisch zum Login-Dialog (Config.Theme.margin).
    readonly property real hPad: Config.Theme.margin
    readonly property real vPad: Config.Theme.margin
    readonly property real innerSpacing: Config.Theme.spacing
    // Abstand zwischen Logo-Block und Button-Raster (Column-spacing).
    readonly property real contentSpacing: 20

    // ── Höhenbudget: die Fußzeile bleibt immer sichtbar ───────────────────
    // Unten ist fest der Platz der Fußzeile reserviert; die Box weicht in
    // dieser Reihenfolge aus: kleineres Logo → flachere Buttons → zweispaltig.
    // Alle Vergleichshöhen werden aus Tokens berechnet und NICHT aus den
    // gemessenen Höhen von Header/Box – sonst entstünde eine Binding-Schleife
    // (kleineres Logo → mehr Platz → größeres Logo → …).
    readonly property real footerReserve: Config.Theme.startFooterReserve
    readonly property real minButtonHeight: 36
    readonly property real boxBudget: height - Config.Theme.margin * 2 - footerReserve

    readonly property int buttonCount: Config.Parameters.showCommunityContent ? 6 : 5

    // Nicht schrumpfender Anteil der Box: Innenabstände + Zeilenabstände.
    function chromeHeight(rows) {
        return vPad * 2 + contentSpacing + (rows - 1) * innerSpacing
    }
    // Kleinstmögliche Box-Höhe (Logo und Buttons am Anschlag) – Grundlage für
    // den Umschaltpunkt auf zwei Spalten und für die Sichtbarkeit der Fußzeile.
    function minBoxHeight(rows) {
        return chromeHeight(rows)
               + Config.Theme.brandHeaderHeight(Config.Theme.brandLogoSizeMin)
               + rows * minButtonHeight
    }

    // ── Zweispaltiger Button-Modus ────────────────────────────────────────
    // Reicht der Platz auch mit kleinstem Logo und flachen Buttons nicht, werden
    // die Buttons zweispaltig angeordnet statt vertikal zu scrollen –
    // vorausgesetzt, die breitere Box passt horizontal.
    readonly property real twoColumnBoxWidth: 620
    readonly property bool twoColumns:
        minBoxHeight(buttonCount) > boxBudget
        && width >= twoColumnBoxWidth + Config.Theme.margin * 2
    readonly property int buttonRows: twoColumns ? Math.ceil(buttonCount / 2) : buttonCount

    // Buttons bleiben auf Touch-Größe, solange das Logo den Platz ausgleichen
    // kann; erst wenn dieses am Minimum ist, werden sie flacher.
    readonly property real buttonHeight:
        Math.max(minButtonHeight,
                 Math.min(Config.Theme.touchTarget,
                          (boxBudget - chromeHeight(buttonRows)
                           - Config.Theme.brandHeaderHeight(Config.Theme.brandLogoSizeMin))
                          / buttonRows))
    // Logo bekommt, was nach Buttons und Abständen übrig bleibt – gedeckelt auf
    // die reguläre Größe (Config.Theme.brandLogoSize, wie im Login-Dialog).
    readonly property real logoSize:
        Math.max(Config.Theme.brandLogoSizeMin,
                 Math.min(Config.Theme.brandLogoSize,
                          Config.Theme.brandHeaderLogoForHeight(
                              boxBudget - chromeHeight(buttonRows)
                              - buttonRows * buttonHeight)))

    Flickable {
        id: startScroll
        anchors.fill: parent
        // Unterer Rand bleibt für die Fußzeile frei – die Box zentriert sich im
        // verbleibenden Bereich, statt die Fußzeile zu überdecken.
        anchors.bottomMargin: startPage.footerReserve
        contentWidth: width
        contentHeight: startContent.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Item {
            id: startContent
            width: startScroll.width
            // Mindesthöhe = Viewport → Box bleibt vertikal zentriert, solange sie
            // passt; sonst kann gescrollt werden.
            implicitHeight: Math.max(startScroll.height,
                                     startPageMainButtonsBox.height + Config.Theme.margin * 2)

            // ── Overlay-Box: enthält Logo + Navigations-Buttons ──────────────
            Rectangle {
                id: startPageMainButtonsBox
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(startContent.width - Config.Theme.margin * 2,
                                startPage.twoColumns ? startPage.twoColumnBoxWidth
                                                     : Config.Theme.brandBoxWidth)
                // Höhe folgt dem Inhalt (Logo + Buttons) inkl. oben/unten gleichem
                // Innenabstand – so bleibt die Box bei beliebig vielen Buttons
                // vertikal zentriert, statt unten herauszulaufen.
                height: startBoxContent.implicitHeight + startPage.vPad * 2
                color: "transparent"

                // Dunkler Hintergrund – immer dunkel damit der Kontrast zum Feuer-
                // Hintergrund stimmt, unabhängig vom Hell/Dunkel-Theme.
                Rectangle {
                    anchors.fill: parent
                    color: "#1d222b"
                    opacity: 0.88
                    radius: 5
                }

                Column {
                    id: startBoxContent
                    // Icon fix am oberen Rand der Box positioniert (Config.Theme.margin)
                    // – identisch zum Login-Dialog. Die Box selbst ist im Fenster
                    // vertikal zentriert.
                    anchors {
                        left: parent.left; right: parent.right; top: parent.top
                        leftMargin: startPage.hPad
                        rightMargin: startPage.hPad
                        topMargin: startPage.vPad
                    }
                    spacing: startPage.contentSpacing

                    // ── PokerTH-Logo + Kartensymbole ─────────────────────────
                    BrandHeader {
                        id: brandHeader
                        anchors.horizontalCenter: parent.horizontalCenter
                        logoSize: startPage.logoSize
                    }

                    // ── Navigations-Buttons ───────────────────────────────────
                    // Gleiche preferredWidth auf allen Buttons → im zweispaltigen
                    // Modus bekommen beide Spalten exakt dieselbe Breite.
                    GridLayout {
                        id: startPageMainButtons
                        width: parent.width
                        columns: startPage.twoColumns ? 2 : 1
                        columnSpacing: startPage.innerSpacing
                        rowSpacing: startPage.innerSpacing

                        CustomButton {
                            text: qsTr("Internetspiel")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: startPage.buttonHeight
                            onClicked: mainStackView.push("ServerConnectionDialog.qml")
                        }

                        CustomButton {
                            text: qsTr("Lokales Spiel starten")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: startPage.buttonHeight
                            onClicked: mainStackView.push("LocalGamePage.qml")
                        }

                        CustomButton {
                            text: qsTr("Netzwerkspiel erstellen")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: startPage.buttonHeight
                            onClicked: mainStackView.push("NetworkGameCreatePage.qml")
                        }

                        CustomButton {
                            text: qsTr("Netzwerkspiel beitreten")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: startPage.buttonHeight
                            onClicked: mainStackView.push("NetworkGameEnterPage.qml")
                        }

                        CustomButton {
                            text: qsTr("Community / Ranking")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: startPage.buttonHeight
                            visible: Config.Parameters.showCommunityContent
                            // Über denselben Toggle wie der Globus → gemerkter
                            // Ranking-Stand wird wiederhergestellt.
                            onClicked: mainWindow.toggleTopBarSection(
                                "pages/CommunityRankingPage.qml",
                                mainWindow.rankingSectionPages, true)
                        }

                        CustomButton {
                            text: qsTr("Logs")
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: startPage.buttonHeight
                            onClicked: mainStackView.push("LogsPage.qml")
                        }
                    }
                }
            }
        }
    }

    // ── Fußzeile: Community-Links + Lizenz/Quelle ─────────────────────────
    // Sitzt im unten reservierten Streifen (footerReserve), den die Flickable
    // freilässt – sie überdeckt die Box also nie. Ausgeblendet wird sie nur
    // noch, wenn die Box selbst mit kleinstem Logo und flachen Buttons nicht
    // mehr passt: dann wird gescrollt und der Streifen zusätzlich gebraucht.
    StartFooter {
        id: startFooter
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: implicitHeight
        visible: startPage.boxBudget >= startPage.minBoxHeight(startPage.buttonRows)
    }
}
