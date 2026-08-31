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

    // Vorgabe, solange der Nutzer nichts anderes gewählt hat – auf allen
    // Plattformen gleich: der Sockel innerhalb der Box braucht rund um die
    // Boxen keinen Platz und kommt gerade auf kleinen Tischen besser weg.
    readonly property string defaultVariant: "inset"

    // Aktiver Sitz-Stil. Wird – wie Theme.darkMode/effectsEnabled – extern von
    // der ApplicationWindow (Init aus dem Config-Key "QmlSeatStyle") und von
    // den Stil-Einstellungen (Live-Umschaltung) gesetzt; ein Singleton kann die
    // SettingsManager-Context-Property nicht selbst lesen. Leerer Config-Wert
    // bedeutet "Vorgabe" und lässt diesen Default stehen.
    property string variant: defaultVariant

    readonly property bool betInset: variant === "inset"

    // Höhe des Einsatz-Sockels in Basis-Pixeln (vor boxScale): Chip-Icon 15 +
    // Luft. Bewusst knapp gehalten – jeder Pixel hier verkleinert über die
    // Bisektion in GamePage.boxScale den gesamten Tisch.
    readonly property int betStripHeight: 20

    // Platz, den Einsatz + Dealer-/Blind-Puck NEBEN der Box brauchen (Basis-
    // Pixel, vgl. GamePlayerBox.betGroup: 8 px Abstand + Gruppenbreite).
    //   "classic" – Chip-Icon 20 + Betrag (~40) → 8 + 60 = 68.
    //   "inset"   – dort steht nur noch der Puck (32) → 8 + 32 = 40.
    // Die Platz-Bisektion reserviert damit im Stil "classic" ehrlich den Raum,
    // den der Einsatz neben der Box wirklich braucht (bisher pauschal 48), und
    // gibt ihn im Stil "inset" für größere Boxen frei.
    readonly property int betSideOutset: betInset ? 40 : 68

    // Zusatzhöhe, die eine Spielerbox für den Sockel braucht (0 bei "classic").
    // NUR diese Größe geht in die Box-Basismaße ein; alle abgeleiteten Maße
    // (Avatar-/Kartenreihe, Boxbreite) rechnen sie wieder heraus, damit die Box
    // ausschließlich in der HÖHE wächst.
    readonly property int betStripExtra: betInset ? betStripHeight : 0
}
