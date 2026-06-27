import QtQuick
import QtQuick.Effects

import "../config" as Config

// Dezenter, weicher Schlagschatten für Panel-Karten (Lobby-Spalten,
// Settings-Boxen). Als Kind mit `anchors.fill: parent` ins Panel legen und
// `radius`/`color` an die Panel-Fläche angleichen.
//
// Es wird ausschließlich die farbige Rundrechteck-Silhouette in eine eigene
// Layer-Textur gerendert – NICHT der Panel-Inhalt – damit scrollende ListViews
// und der Chat scharf bleiben und nicht bei jedem Frame neu in eine Textur
// gezeichnet werden müssen. Über `z: -1` liegt der Schatten hinter dem Inhalt,
// der Halo ragt dank `autoPaddingEnabled` über die Panel-Grenzen hinaus.
Rectangle {
    id: panelShadow

    z: -1
    radius: 5
    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)

    // Feinjustierung pro Verwendung möglich (Default = Theme-Tokens).
    property real shadowOpacity: Config.Theme.panelShadowOpacity
    property real shadowBlur:    Config.Theme.panelShadowBlur
    property real shadowOffset:  Config.Theme.panelShadowOffset

    layer.enabled: Config.Theme.effectsEnabled
    layer.effect: MultiEffect {
        autoPaddingEnabled: true
        shadowEnabled: true
        shadowColor: Config.Theme.colorShadow
        shadowOpacity: panelShadow.shadowOpacity
        shadowBlur: panelShadow.shadowBlur
        shadowVerticalOffset: panelShadow.shadowOffset
        shadowHorizontalOffset: 0
    }
}
