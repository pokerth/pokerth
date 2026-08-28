pragma Singleton
import QtQuick

// Sitz-Stil der Spielerboxen am Tisch – das QML-Gegenstück zu den „Sitz-Packs"
// des Web-Clients (dort html[data-seat="…"]: classic/plate/card/pokerth). Der
// Stil bestimmt vorerst NUR, wo der Einsatz eines Spielers steht; weitere
// Varianten können hier später andocken, ohne die Boxen erneut umzubauen.
//
//   "classic" – Einsatz-Chip AUSSERHALB der Box (links/rechts/über/unter der
//               Box, je nach Sitzposition). Stand bis 08/2026.
//   "inset"   – Einsatz-Chip im Sockel INNERHALB der Box; die Box wächst dafür
//               um betStripHeight in der Höhe.
//
// Der Dealer-/Blind-Puck bleibt in BEIDEN Varianten außerhalb der Box.
//
// Beide Spielerboxen (GamePlayerBox, GamePlayerSelfBox) UND die Platz-
// berechnung der tableZone (GamePage) lesen ausschließlich diese Werte. Ein
// späterer Settings-Schalter (Auswahl wie bei den Tisch-Stilen) muss daher nur
// `variant` schreiben – an den Boxen ist dann nichts mehr zu tun.
QtObject {
    id: root

    // Wie Config.Responsive.isMobile. Bewusst dupliziert statt importiert: die
    // Config-Singletons referenzieren einander nicht (s. Theme.qml, das
    // windowWidth/darkMode ebenfalls spiegelt).
    readonly property bool isMobile: Qt.platform.os === "android" || Qt.platform.os === "ios"

    // Vorgabe der Plattform, solange der Nutzer nichts anderes gewählt hat:
    // Mobile bleibt bei "classic" – dort ist der Platz um die Boxen knapp und
    // das Layout eigens dafür austariert.
    readonly property string platformDefault: isMobile ? "classic" : "inset"

    // Aktiver Sitz-Stil. Wird – wie Theme.darkMode/effectsEnabled – extern von
    // der ApplicationWindow (Init aus dem Config-Key "QmlSeatStyle") und von
    // den Stil-Einstellungen (Live-Umschaltung) gesetzt; ein Singleton kann die
    // SettingsManager-Context-Property nicht selbst lesen. Leerer Config-Wert
    // bedeutet "Vorgabe der Plattform" und lässt diesen Default stehen.
    property string variant: platformDefault

    readonly property bool betInset: variant === "inset"

    // Höhe des Einsatz-Sockels in Basis-Pixeln (vor boxScale): Chip-Icon 15 +
    // Luft. Bewusst knapp gehalten – jeder Pixel hier verkleinert über die
    // Bisektion in GamePage.boxScale den gesamten Tisch.
    readonly property int betStripHeight: 20

    // Zusatzhöhe, die eine Spielerbox für den Sockel braucht (0 bei "classic").
    // NUR diese Größe geht in die Box-Basismaße ein; alle abgeleiteten Maße
    // (Avatar-/Kartenreihe, Boxbreite) rechnen sie wieder heraus, damit die Box
    // ausschließlich in der HÖHE wächst.
    readonly property int betStripExtra: betInset ? betStripHeight : 0
}
