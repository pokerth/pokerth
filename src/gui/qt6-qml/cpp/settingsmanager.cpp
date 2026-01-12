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
 *****************************************************************************/

#include "settingsmanager.h"
#include "configfile.h"

SettingsManager::SettingsManager(boost::shared_ptr<ConfigFile> config, QObject *parent)
    : QObject(parent), m_config(config)
{
}

QString SettingsManager::language() const
{
    return QString::fromStdString(m_config->readConfigString("Language"));
}

QString SettingsManager::style() const
{
    return QString::fromStdString(m_config->readConfigString("Style"));
}

bool SettingsManager::soundEnabled() const
{
    return m_config->readConfigInt("PlaySoundEffects") != 0;
}

bool SettingsManager::disableSplashScreen() const
{
    return m_config->readConfigInt("DisableSplashScreenOnStartup") != 0;
}

QString SettingsManager::myName() const
{
    return QString::fromStdString(m_config->readConfigString("MyName"));
}

QString SettingsManager::myAvatar() const
{
    return QString::fromStdString(m_config->readConfigString("MyAvatar"));
}

void SettingsManager::setLanguage(const QString &lang)
{
    if (language() != lang) {
        m_config->writeConfigString("Language", lang.toStdString());
        m_config->writeBuffer();
        emit languageChanged();
    }
}

void SettingsManager::setStyle(const QString &style)
{
    if (this->style() != style) {
        m_config->writeConfigString("Style", style.toStdString());
        m_config->writeBuffer();
        emit styleChanged();
    }
}

void SettingsManager::setSoundEnabled(bool enabled)
{
    if (soundEnabled() != enabled) {
        m_config->writeConfigInt("PlaySoundEffects", enabled ? 1 : 0);
        m_config->writeBuffer();
        emit soundEnabledChanged();
    }
}

void SettingsManager::setDisableSplashScreen(bool disabled)
{
    if (disableSplashScreen() != disabled) {
        m_config->writeConfigInt("DisableSplashScreenOnStartup", disabled ? 1 : 0);
        m_config->writeBuffer();
        emit disableSplashScreenChanged();
    }
}

void SettingsManager::setMyName(const QString &name)
{
    if (myName() != name) {
        m_config->writeConfigString("MyName", name.toStdString());
        m_config->writeBuffer();
        emit myNameChanged();
    }
}

void SettingsManager::setMyAvatar(const QString &avatar)
{
    if (myAvatar() != avatar) {
        m_config->writeConfigString("MyAvatar", avatar.toStdString());
        m_config->writeBuffer();
        emit myAvatarChanged();
    }
}

QString SettingsManager::readConfigString(const QString &key) const
{
    return QString::fromStdString(m_config->readConfigString(key.toStdString()));
}

int SettingsManager::readConfigInt(const QString &key) const
{
    return m_config->readConfigInt(key.toStdString());
}

void SettingsManager::writeConfigString(const QString &key, const QString &value)
{
    m_config->writeConfigString(key.toStdString(), value.toStdString());
    m_config->writeBuffer();
}

void SettingsManager::writeConfigInt(const QString &key, int value)
{
    m_config->writeConfigInt(key.toStdString(), value);
    m_config->writeBuffer();
}

void SettingsManager::saveConfig()
{
    m_config->writeBuffer();
}
