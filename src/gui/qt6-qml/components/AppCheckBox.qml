import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

// CheckBox, deren Beschriftung UMBRICHT statt rechts abgeschnitten zu werden.
// Basis für alle Optionen der Einstellungsseiten (direkt oder über
// ConfigCheckBox).
//
// Der Style-Default (Universal/CheckBox.qml) legt seinen contentItem als
// einzeiligen Text an: passt die Beschriftung nicht in die Spaltenbreite, ist
// der Rest schlicht weg. Im schmalen Portrait (Android-Phone, ~360 dp) trifft
// das fast jede längere Option.
//
// Zwei Dinge gehören dafür zusammen – deshalb stehen sie hier einmal statt an
// jeder Verwendungsstelle:
//   • wrapMode: Text.Wrap – erlaubt den Umbruch überhaupt.
//   • Layout.fillWidth    – erzwingt ihn auch. Der implicitWidth einer CheckBox
//     bleibt die UNumbrochene Textbreite; ohne fillWidth legt der ColumnLayout
//     die Box in genau dieser Breite an, sie ragt über die Spalte hinaus und
//     der ScrollView (clip: true) schneidet sie ab – der Umbruch käme nie zum
//     Tragen. Genau das war der Grund für die abgeschnittenen Zeilen.
// Eine Breiten-Schwelle braucht es nicht: reicht der Platz, bricht Text.Wrap
// nicht um, und die Box verhält sich exakt wie zuvor.
CheckBox {
    id: control

    Layout.fillWidth: true

    contentItem: Text {
        text: control.text
        wrapMode: Text.Wrap
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
        // font durchreichen: der Style-Default setzt font: control.font. Fehlt
        // das, fällt dieser Text auf die Standard-Schriftgröße zurück (gemessen:
        // 12 statt 13) und die umbrechenden Labels erscheinen größer als alle
        // anderen.
        font: control.font
        // Universal.foreground statt palette.windowText: palette folgt der
        // SYSTEM-Palette und nicht dem Universal.theme, das pokerth.qml aus dem
        // DarkMode-Setting ableitet. Auf iOS/iPadOS ist windowText schwarz →
        // schwarze Schrift auf dunklem Grund. Universal.foreground ist exakt
        // das, was der Style-Default nutzt – umbrechende und einzeilige Labels
        // sehen damit identisch aus.
        color: control.Universal.foreground
    }
}
