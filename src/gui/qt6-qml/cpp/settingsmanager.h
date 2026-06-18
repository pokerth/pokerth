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

#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <boost/shared_ptr.hpp>

class ConfigFile;

class SettingsManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString style READ style WRITE setStyle NOTIFY styleChanged)
    Q_PROPERTY(bool soundEnabled READ soundEnabled WRITE setSoundEnabled NOTIFY soundEnabledChanged)
    Q_PROPERTY(bool disableSplashScreen READ disableSplashScreen WRITE setDisableSplashScreen NOTIFY disableSplashScreenChanged)
    Q_PROPERTY(QString myName READ myName WRITE setMyName NOTIFY myNameChanged)
    Q_PROPERTY(QString myAvatar READ myAvatar WRITE setMyAvatar NOTIFY myAvatarChanged)

public:
    explicit SettingsManager(boost::shared_ptr<ConfigFile> config, QObject *parent = nullptr);

    // Property getters
    QString language() const;
    QString style() const;
    bool soundEnabled() const;
    bool disableSplashScreen() const;
    QString myName() const;
    QString myAvatar() const;

    // Property setters
    void setLanguage(const QString &lang);
    void setStyle(const QString &style);
    void setSoundEnabled(bool enabled);
    void setDisableSplashScreen(bool disabled);
    void setMyName(const QString &name);
    void setMyAvatar(const QString &avatar);

    // Generic config access
    Q_INVOKABLE QString readConfigString(const QString &key) const;
    Q_INVOKABLE int readConfigInt(const QString &key) const;
    Q_INVOKABLE void writeConfigString(const QString &key, const QString &value);
    Q_INVOKABLE void writeConfigInt(const QString &key, int value);
    Q_INVOKABLE QStringList readConfigStringList(const QString &key) const;
    Q_INVOKABLE void writeConfigStringList(const QString &key, const QStringList &list);
    Q_INVOKABLE QList<int> readConfigIntList(const QString &key) const;
    Q_INVOKABLE void writeConfigIntList(const QString &key, const QList<int> &list);
    Q_INVOKABLE void saveConfig();
    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE QString pickImageFile(const QString &title);

    // Liste der verfügbaren QML-Stile unter <AppDataDir>/gfx/qml/<table|cards>/*.
    // Jeder Eintrag ist eine Map mit den Schlüsseln:
    //   name, description, maintainer, dir, xml,
    //   preview, previewPortrait  (preview* sind file://-URLs, leer wenn fehlend).
    Q_INVOKABLE QVariantList availableTableStyles() const;
    Q_INVOKABLE QVariantList availableCardDeckStyles() const;
    Q_INVOKABLE QVariantList availableCardBackStyles() const;

signals:
    void languageChanged();
    void styleChanged();
    void soundEnabledChanged();
    void disableSplashScreenChanged();
    void myNameChanged();
    void myAvatarChanged();

private:
    // Scannt <AppDataDir>/gfx/qml/<category>/* nach Unterordnern, die eine
    // "*<xmlSuffix>"-Datei enthalten, und liefert je Stil eine Beschreibungs-Map.
    QVariantList scanStyleDir(const QString &category, const QString &xmlSuffix) const;

    boost::shared_ptr<ConfigFile> m_config;
};

#endif // SETTINGSMANAGER_H
