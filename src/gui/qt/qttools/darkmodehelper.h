/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#ifndef DARKMODEHELPER_H
#define DARKMODEHELPER_H

#include <QApplication>
#include <QPalette>
#include <QColor>

class ConfigFile;

/**
 * @brief Helper class for dark mode detection and configuration
 * 
 * DarkMode config values:
 *   0 = Light (force light mode)
 *   1 = Dark (force dark mode)
 *   2 = Auto (follow system theme)
 */
class DarkModeHelper
{
public:
    /**
     * @brief Store the original system palette (call once at startup before any modifications)
     */
    static void storeSystemPalette();
    
    /**
     * @brief Determines if dark mode should be used based on config and system theme
     * @param config Pointer to ConfigFile (can be nullptr for system-only detection)
     * @return true if dark mode should be used, false otherwise
     */
    static bool isDarkMode(ConfigFile *config);
    
    /**
     * @brief Detects if the system is using a dark theme
     * @return true if system uses dark theme, false otherwise
     */
    static bool isSystemDarkMode();
    
    /**
     * @brief Gets the appropriate background color for dark/light mode
     * @param config Pointer to ConfigFile
     * @return Background color string (hex format)
     */
    static QString getBackgroundColor(ConfigFile *config);
    
    /**
     * @brief Gets the appropriate text color for dark/light mode
     * @param config Pointer to ConfigFile
     * @return Text color string (hex or rgb format)
     */
    static QString getTextColor(ConfigFile *config);
    
    /**
     * @brief Applies the appropriate palette to the application based on config
     * @param config Pointer to ConfigFile
     */
    static void applyPalette(ConfigFile *config);
    
    /**
     * @brief Creates a dark mode palette
     * @return QPalette configured for dark mode
     */
    static QPalette createDarkPalette();
    
    /**
     * @brief Creates a light mode palette
     * @return QPalette configured for light mode
     */
    static QPalette createLightPalette();
};

#endif // DARKMODEHELPER_H
