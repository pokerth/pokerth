import QtQuick
import QtQuick.Effects

import "../config" as Config

// Subtle, soft drop shadow for panel cards (lobby columns,
// settings boxes). Place it as a child inside the panel with `anchors.fill: parent`
// and match `radius`/`color` to the panel surface.
//
// Only the coloured rounded-rectangle silhouette is rendered into its own
// layer texture – NOT the panel content – so that scrolling ListViews
// and the chat stay sharp and are not redrawn into a texture on
// every frame. Via `z: -1` the shadow sits behind the content,
// and thanks to `autoPaddingEnabled` the halo extends beyond the panel bounds.
Rectangle {
    id: panelShadow

    z: -1
    radius: 5
    color: Config.Theme.colorPanel

    // Fine-tuning per use case is possible (default = theme tokens).
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
