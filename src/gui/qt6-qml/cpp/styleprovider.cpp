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

#include "styleprovider.h"
#include "configfile.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QXmlStreamReader>

StyleProvider::StyleProvider(boost::shared_ptr<ConfigFile> config, QObject *parent)
    : QObject(parent), m_config(config)
{
    reload();
}

QString StyleProvider::styleDirPath(const QString &category, const QString &name) const
{
    if (!m_config || name.isEmpty())
        return QString();
    // AppDataDir endet bereits mit einem Verzeichnis-Trennzeichen.
    return QString::fromStdString(m_config->readConfigString("AppDataDir"))
           + "gfx/qml/" + category + "/" + name;
}

void StyleProvider::loadTableStyle()
{
    m_tableBackground.clear();
    m_tableBackgroundAlignment.clear();
    m_tableBackgroundZoom = 1.0;
    m_dealerPuck.clear();
    m_smallBlindPuck.clear();
    m_bigBlindPuck.clear();

    QDir dir(styleDirPath("table", m_tableStyleName));
    if (!dir.exists())
        return;
    const QStringList xmlFiles =
        dir.entryList(QStringList() << "*tablestyle.xml", QDir::Files, QDir::Name);
    if (xmlFiles.isEmpty())
        return;

    auto urlIfExists = [&dir](const QString &rel) -> QString {
        if (rel.isEmpty())
            return QString();
        const QString abs = dir.absoluteFilePath(rel);
        if (!QFileInfo::exists(abs))
            return QString();
        return QUrl::fromLocalFile(abs).toString();
    };

    QFile f(dir.absoluteFilePath(xmlFiles.first()));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QXmlStreamReader xml(&f);
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement)
            continue;
        const QString tag = xml.name().toString();
        const QString value = xml.attributes().value("value").toString();
        if (tag == "Table")
            m_tableBackground = urlIfExists(value);
        else if (tag == "TableBackgroundAlign")
            m_tableBackgroundAlignment = value.toLower().trimmed();
        else if (tag == "TableBackgroundZoom") {
            // Optionaler Crop-/Zoom-Faktor (>= 1.0) für den center-Modus: skaliert
            // das Tischbild über die Minimal-Deckung hinaus → mehr Beschnitt des
            // äußeren Randes, Tisch wirkt größer. Per Daten justierbar (kein Build).
            bool ok = false;
            const double z = value.toDouble(&ok);
            if (ok && z > 0.0)
                m_tableBackgroundZoom = z;
        }
        else if (tag == "DealerPuck")
            m_dealerPuck = urlIfExists(value);
        else if (tag == "SmallBlindPuck")
            m_smallBlindPuck = urlIfExists(value);
        else if (tag == "BigBlindPuck")
            m_bigBlindPuck = urlIfExists(value);
    }
}

void StyleProvider::loadCardDeckStyle()
{
    m_cardDeckDir.clear();

    QDir dir(styleDirPath("cards", m_cardDeckName));
    if (!dir.exists())
        return;
    const QStringList xmlFiles =
        dir.entryList(QStringList() << "*deckstyle.xml", QDir::Files, QDir::Name);
    if (xmlFiles.isEmpty())
        return;

    // Karten-Vorderseiten folgen der festen Namenskonvention 0.svg..51.svg
    // (Engine-Index). Erst wenn mindestens "0.svg" existiert, gilt der
    // Stil als nutzbar und QML baut die Pfade aus cardDeckDir.
    if (QFileInfo::exists(dir.absoluteFilePath("0.svg")))
        m_cardDeckDir = QUrl::fromLocalFile(dir.absolutePath()).toString();
}

void StyleProvider::loadCardBackStyle()
{
    m_cardBack.clear();

    QDir dir(styleDirPath("backside", m_cardBackName));
    if (!dir.exists())
        return;
    const QStringList xmlFiles =
        dir.entryList(QStringList() << "*backsidestyle.xml", QDir::Files, QDir::Name);
    if (xmlFiles.isEmpty())
        return;

    // Genau eine Rückseiten-Grafik je Stil, referenziert über <Backside value=...>.
    QFile f(dir.absoluteFilePath(xmlFiles.first()));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QXmlStreamReader xml(&f);
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement)
            continue;
        if (xml.name().toString() == "Backside") {
            const QString rel = xml.attributes().value("value").toString();
            const QString abs = dir.absoluteFilePath(rel);
            if (!rel.isEmpty() && QFileInfo::exists(abs))
                m_cardBack = QUrl::fromLocalFile(abs).toString();
            break;
        }
    }
}

void StyleProvider::reload()
{
    if (m_config) {
        m_tableStyleName = QString::fromStdString(m_config->readConfigString("QmlGameTableStyle"));
        m_cardDeckName = QString::fromStdString(m_config->readConfigString("QmlCardDeckStyle"));
        m_cardBackName = QString::fromStdString(m_config->readConfigString("QmlCardBackStyle"));
    }
    loadTableStyle();
    loadCardDeckStyle();
    loadCardBackStyle();
    emit changed();
}

void StyleProvider::setTableStyle(const QString &name)
{
    if (name == m_tableStyleName)
        return;
    m_tableStyleName = name;
    if (m_config) {
        m_config->writeConfigString("QmlGameTableStyle", name.toStdString());
        m_config->writeBuffer();
    }
    loadTableStyle();
    emit changed();
}

void StyleProvider::setCardDeckStyle(const QString &name)
{
    if (name == m_cardDeckName)
        return;
    m_cardDeckName = name;
    if (m_config) {
        m_config->writeConfigString("QmlCardDeckStyle", name.toStdString());
        m_config->writeBuffer();
    }
    loadCardDeckStyle();
    emit changed();
}

void StyleProvider::setCardBackStyle(const QString &name)
{
    if (name == m_cardBackName)
        return;
    m_cardBackName = name;
    if (m_config) {
        m_config->writeConfigString("QmlCardBackStyle", name.toStdString());
        m_config->writeBuffer();
    }
    loadCardBackStyle();
    emit changed();
}
