#ifndef DARKMODE_H
#define DARKMODE_H

#include <QGuiApplication>
#include <QStyleHints>

/* Meaning of the config value "DarkMode" – identical to the widgets client
 * (DarkModeHelper::isDarkMode): 0 = light, 1 = dark, 2 = automatic.
 * "Automatic" follows the system; the QML client used to treat the value 2
 * like "dark", so that the setting had no effect on a Windows/macOS set to
 * a light theme.
 *
 * Single source of truth for C++ (SettingsManager, LobbyHandler) AND – via
 * SettingsManager.systemDark – for the QML singletons StaticData/Theme.
 */
namespace DarkMode
{

enum Setting { Light = 0, Dark = 1, Auto = 2 };

// Mode reported by the system. If the platform reports nothing (Qt::ColorScheme::
// Unknown, e.g. on plain X11 setups), the dark default of the user interface
// stays in effect.
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
	case Light:
		return false;
	case Dark:
		return true;
	case Auto:
	default:
		return systemPrefersDark();
	}
}

} // namespace DarkMode

#endif // DARKMODE_H
