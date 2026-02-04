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
#include "darkmodehelper.h"
#include "configfile.h"
#include <QStyleFactory>
#include <QStyle>
#include <QToolTip>
#include <QWidget>

// Store original system palette for "Auto" mode
static QPalette s_systemPalette;
static bool s_systemPaletteStored = false;

void DarkModeHelper::storeSystemPalette()
{
    if (!s_systemPaletteStored) {
        s_systemPalette = QApplication::palette();
        s_systemPaletteStored = true;
    }
}

bool DarkModeHelper::isSystemDarkMode()
{
    // Use stored system palette if available, otherwise current
    QPalette palette = s_systemPaletteStored ? s_systemPalette : QApplication::palette();
    QColor windowColor = palette.color(QPalette::Window);
    return windowColor.lightness() < 128;
}

bool DarkModeHelper::isDarkMode(ConfigFile *config)
{
    if (!config) {
        return isSystemDarkMode();
    }
    
    int darkModeConfig = config->readConfigInt("DarkMode");
    
    switch (darkModeConfig) {
        case 0:  // Light mode forced
            return false;
        case 1:  // Dark mode forced
            return true;
        case 2:  // Auto - follow system
        default:
            return isSystemDarkMode();
    }
}

QString DarkModeHelper::getBackgroundColor(ConfigFile *config)
{
    return isDarkMode(config) ? "#2b2b2b" : "white";
}

QString DarkModeHelper::getTextColor(ConfigFile *config)
{
    return isDarkMode(config) ? "#ffffff" : "rgb(0, 0, 0)";
}

QPalette DarkModeHelper::createDarkPalette()
{
    QPalette darkPalette;
    
    // Window and general backgrounds
    QColor darkBackground(45, 45, 45);       // #2d2d2d
    QColor darkerBackground(30, 30, 30);     // #1e1e1e
    QColor disabledBackground(50, 50, 50);
    
    // Text colors
    QColor textColor(212, 212, 212);         // #d4d4d4
    QColor disabledText(128, 128, 128);
    QColor brightText(255, 255, 255);
    
    // Accent colors
    QColor highlight(42, 130, 218);          // Blue highlight
    QColor highlightedText(255, 255, 255);
    QColor link(86, 156, 214);               // Light blue for links
    
    // Base colors (for text inputs, lists, etc.)
    QColor base(30, 30, 30);                 // Darker than window
    QColor alternateBase(45, 45, 45);
    
    // Button colors
    QColor button(60, 60, 60);
    QColor buttonText(212, 212, 212);
    
    // Set colors for all color groups
    darkPalette.setColor(QPalette::Window, darkBackground);
    darkPalette.setColor(QPalette::WindowText, textColor);
    darkPalette.setColor(QPalette::Base, base);
    darkPalette.setColor(QPalette::AlternateBase, alternateBase);
    darkPalette.setColor(QPalette::ToolTipBase, darkBackground);
    darkPalette.setColor(QPalette::ToolTipText, textColor);
    darkPalette.setColor(QPalette::Text, textColor);
    darkPalette.setColor(QPalette::Button, button);
    darkPalette.setColor(QPalette::ButtonText, buttonText);
    darkPalette.setColor(QPalette::BrightText, brightText);
    darkPalette.setColor(QPalette::Link, link);
    darkPalette.setColor(QPalette::Highlight, highlight);
    darkPalette.setColor(QPalette::HighlightedText, highlightedText);
    
    // Disabled colors
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
    darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledText);
    darkPalette.setColor(QPalette::Disabled, QPalette::Base, disabledBackground);
    darkPalette.setColor(QPalette::Disabled, QPalette::Window, disabledBackground);
    darkPalette.setColor(QPalette::Disabled, QPalette::Button, disabledBackground);
    
    // PlaceholderText for input fields
    darkPalette.setColor(QPalette::PlaceholderText, disabledText);
    
    return darkPalette;
}

QPalette DarkModeHelper::createLightPalette()
{
    // Return the stored system palette for light mode
    // This preserves the native look and feel
    if (s_systemPaletteStored) {
        return s_systemPalette;
    }
    
    // Fallback: create a standard light palette
    QPalette lightPalette;
    
    lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::WindowText, Qt::black);
    lightPalette.setColor(QPalette::Base, Qt::white);
    lightPalette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    lightPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220));
    lightPalette.setColor(QPalette::ToolTipText, Qt::black);
    lightPalette.setColor(QPalette::Text, Qt::black);
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::ButtonText, Qt::black);
    lightPalette.setColor(QPalette::BrightText, Qt::red);
    lightPalette.setColor(QPalette::Link, QColor(0, 0, 255));
    lightPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);
    
    lightPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 120));
    lightPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 120));
    lightPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));
    
    lightPalette.setColor(QPalette::PlaceholderText, QColor(120, 120, 120));
    
    return lightPalette;
}

void DarkModeHelper::applyPalette(ConfigFile *config)
{
    // Store the original system palette before first modification
    storeSystemPalette();
    
    QPalette palette;
    
    if (isDarkMode(config)) {
        palette = createDarkPalette();
    } else {
        palette = createLightPalette();
    }
    
    QApplication::setPalette(palette);
    
    // Also set tooltip palette explicitly
    QToolTip::setPalette(palette);
    
    // Force all existing widgets to update their styles
    for (QWidget *widget : QApplication::allWidgets()) {
        // Re-polish the widget to apply new palette
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        // Also set palette explicitly on the widget
        widget->setPalette(palette);
        // Schedule a repaint
        widget->update();
    }
}
