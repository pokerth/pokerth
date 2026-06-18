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
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QVariantMap>
#include <QXmlStreamReader>
#include <core/appimage_utils.h>

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

QStringList SettingsManager::readConfigStringList(const QString &key) const
{
    QStringList result;
    for (const auto& s : m_config->readConfigStringList(key.toStdString()))
        result << QString::fromStdString(s);
    return result;
}

void SettingsManager::writeConfigStringList(const QString &key, const QStringList &list)
{
    std::list<std::string> stdList;
    for (const auto& s : list)
        stdList.push_back(s.toStdString());
    m_config->writeConfigStringList(key.toStdString(), stdList);
    m_config->writeBuffer();
}

QList<int> SettingsManager::readConfigIntList(const QString &key) const
{
    QList<int> result;
    for (int v : m_config->readConfigIntList(key.toStdString()))
        result << v;
    return result;
}

void SettingsManager::writeConfigIntList(const QString &key, const QList<int> &list)
{
    std::list<int> stdList(list.begin(), list.end());
    m_config->writeConfigIntList(key.toStdString(), stdList);
    m_config->writeBuffer();
}

void SettingsManager::saveConfig()
{
    m_config->writeBuffer();
}

void SettingsManager::resetToDefaults()
{
    m_config->resetToDefaults();
    emit languageChanged();
    emit styleChanged();
    emit soundEnabledChanged();
    emit disableSplashScreenChanged();
    emit myNameChanged();
    emit myAvatarChanged();
}

QString SettingsManager::pickImageFile(const QString &title)
{
    return QFileDialog::getOpenFileName(
        nullptr,
        title,
        QString(),
        tr("Images (*.png *.jpg *.jpeg *.gif *.bmp)"),
        nullptr, AppImageUtils::fileDialogOptions()
    );
}

QVariantList SettingsManager::availableTableStyles() const
{
    return scanStyleDir("table", "tablestyle.xml");
}

QVariantList SettingsManager::availableCardDeckStyles() const
{
    return scanStyleDir("cards", "deckstyle.xml");
}

QVariantList SettingsManager::availableCardBackStyles() const
{
    return scanStyleDir("backside", "backsidestyle.xml");
}

QVariantList SettingsManager::scanStyleDir(const QString &category, const QString &xmlSuffix) const
{
    QVariantList result;
    if (!m_config)
        return result;

    // AppDataDir endet bereits mit einem Verzeichnis-Trennzeichen.
    const QString base = QString::fromStdString(m_config->readConfigString("AppDataDir"))
                         + "gfx/qml/" + category;
    QDir baseDir(base);
    if (!baseDir.exists())
        return result;

    const QFileInfoList styleDirs =
        baseDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &dirInfo : styleDirs) {
        QDir styleDir(dirInfo.absoluteFilePath());
        const QStringList xmlFiles =
            styleDir.entryList(QStringList() << ("*" + xmlSuffix), QDir::Files, QDir::Name);
        if (xmlFiles.isEmpty())
            continue;

        const QString xmlPath = styleDir.absoluteFilePath(xmlFiles.first());

        QVariantMap entry;
        entry["name"] = dirInfo.fileName();
        entry["dir"] = dirInfo.absoluteFilePath();
        entry["xml"] = xmlPath;
        entry["description"] = dirInfo.fileName(); // Fallback bis XML geparst
        entry["maintainer"] = QString();

        QString previewRel, previewPortraitRel;
        QFile xmlFile(xmlPath);
        if (xmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QXmlStreamReader xml(&xmlFile);
            while (!xml.atEnd()) {
                if (xml.readNext() != QXmlStreamReader::StartElement)
                    continue;
                const QString tag = xml.name().toString();
                const QString value = xml.attributes().value("value").toString();
                if (tag == "StyleDescription" && !value.isEmpty())
                    entry["description"] = value;
                else if (tag == "StyleMaintainerName")
                    entry["maintainer"] = value;
                else if (tag == "Preview")
                    previewRel = value;
                else if (tag == "PreviewPortrait")
                    previewPortraitRel = value;
            }
        }

        auto toUrl = [&styleDir](const QString &rel) -> QString {
            if (rel.isEmpty())
                return QString();
            const QString abs = styleDir.absoluteFilePath(rel);
            if (!QFileInfo::exists(abs))
                return QString();
            return QUrl::fromLocalFile(abs).toString();
        };

        QString preview = toUrl(previewRel);
        QString previewPortrait = toUrl(previewPortraitRel);
        // Fehlt eine Orientierung, die jeweils andere als Ersatz verwenden.
        if (preview.isEmpty())
            preview = previewPortrait;
        if (previewPortrait.isEmpty())
            previewPortrait = preview;
        entry["preview"] = preview;
        entry["previewPortrait"] = previewPortrait;

        result.append(entry);
    }
    return result;
}
