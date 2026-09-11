import QtQuick

// Safe area insets of the system bars (status/navigation bar, notch).
//
// From Android 15 on, apps with targetSdk 35+ MUST draw edge to edge: the
// "fullscreen" flag of the app theme is ignored, the system and navigation
// bar lie ABOVE the window content. Without a correction the header would sit
// under the status bar and the action bar of the table under the gesture
// bar.
//
// `SafeArea` (QtQuick 6.9) reports how much the element still has to keep
// free at each edge. If the platform already takes care of it itself, the
// values are 0 – then this file is a no-op.
//
// Deliberately a file of its own, which pokerth.qml pulls in via a Loader: on
// Qt 6.7 (the Android APK variant for Android 8) the type `SafeArea` does not
// exist. The Loader fails silently on this file there, the app keeps running
// with insets of 0 – a direct access in pokerth.qml, by contrast, would be
// a load error of the whole window.
Item {
    id: root

    // The element has to cover the area the insets should apply to –
    // the Loader in pokerth.qml fills the window for that.
    readonly property real insetTop:    SafeArea.margins.top
    readonly property real insetBottom: SafeArea.margins.bottom
    readonly property real insetLeft:   SafeArea.margins.left
    readonly property real insetRight:  SafeArea.margins.right
}
