import QtQuick

// Sicherheitsabstände der Systemleisten (Status-/Navigationsleiste, Notch).
//
// Ab Android 15 zeichnen Apps mit targetSdk 35+ ZWINGEND randlos: das
// „Fullscreen“-Flag des App-Themes wird ignoriert, System- und
// Navigationsleiste liegen ÜBER dem Fensterinhalt. Ohne Korrektur säßen die
// Kopfzeile unter der Statusleiste und die Aktionsleiste des Tisches unter der
// Gestenleiste.
//
// `SafeArea` (QtQuick 6.9) meldet, wie viel das Element an jeder Kante noch
// freihalten muss. Kümmert sich die Plattform bereits selbst darum, sind die
// Werte 0 – dann ist diese Datei ein No-op.
//
// Bewusst als eigene Datei, die pokerth.qml über einen Loader zieht: auf
// Qt 6.7 (Android-APK-Variante für Android 8) existiert der Typ `SafeArea`
// nicht. Der Loader scheitert dort still an dieser Datei, die App läuft mit
// Abständen von 0 weiter – ein direkter Zugriff in pokerth.qml wäre dagegen
// ein Ladefehler des gesamten Fensters.
Item {
    id: root

    // Das Element muss die Fläche abdecken, für die die Abstände gelten sollen –
    // der Loader in pokerth.qml füllt dafür das Fenster.
    readonly property real insetTop:    SafeArea.margins.top
    readonly property real insetBottom: SafeArea.margins.bottom
    readonly property real insetLeft:   SafeArea.margins.left
    readonly property real insetRight:  SafeArea.margins.right
}
