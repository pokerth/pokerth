#ifndef DARKMODE_H
#define DARKMODE_H

#include <QGuiApplication>
#include <QStyleHints>

/* Bedeutung des Config-Werts "DarkMode" – identisch zum Widgets-Client
 * (DarkModeHelper::isDarkMode): 0 = Hell, 1 = Dunkel, 2 = Automatisch.
 * „Automatisch" folgt dem System; früher hat der QML-Client den Wert 2 wie
 * „Dunkel" behandelt, sodass die Einstellung auf einem hell eingestellten
 * Windows/macOS wirkungslos blieb.
 *
 * Einzige Wahrheit für C++ (SettingsManager, LobbyHandler) UND – über
 * SettingsManager.systemDark – für die QML-Singletons StaticData/Theme.
 */
namespace DarkMode {

enum Setting { Light = 0, Dark = 1, Auto = 2 };

// Vom System gemeldeter Modus. Meldet die Plattform nichts (Qt::ColorScheme::
// Unknown, z. B. auf schlichten X11-Setups), bleibt es beim dunklen Standard
// der Oberfläche.
inline bool systemPrefersDark()
{
    const QStyleHints *hints = QGuiApplication::styleHints();
    if (!hints)
        return true;
    return hints->colorScheme() != Qt::ColorScheme::Light;
}

// Config-Wert -> effektiver Modus.
inline bool resolve(int settingValue)
{
    switch (settingValue) {
    case Light: return false;
    case Dark:  return true;
    case Auto:
    default:    return systemPrefersDark();
    }
}

} // namespace DarkMode

#endif // DARKMODE_H
