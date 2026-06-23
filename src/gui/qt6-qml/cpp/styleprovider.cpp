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
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
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
    m_foldButton.clear();
    m_checkCallButton.clear();
    m_betRaiseButton.clear();
    m_allInButton.clear();
    m_foldButtonTextColor.clear();
    m_checkCallButtonTextColor.clear();
    m_betRaiseButtonTextColor.clear();
    m_allInButtonTextColor.clear();

    // Chat-/Log-Box-Farben: gebündelte Dunkel-Defaults (entsprechen der
    // bisherigen Dunkel-Palette des Clients). Ein Tisch-Theme darf sie via
    // <ChatLog*>-Tags überschreiben; fehlt ein Tag, bleibt der Default.
    m_chatLogBackground    = QStringLiteral("#1d222b");
    m_chatLogSurface       = QStringLiteral("#394150");
    m_chatLogBorder        = QStringLiteral("#576378");
    m_chatLogText          = QStringLiteral("#eff1f5");
    m_chatLogTextSecondary = QStringLiteral("#cdd3e0");
    m_chatLogTextMuted     = QStringLiteral("#7787a3");

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

    // Relative SVG-Pfade + evtl. explizite Schriftfarben sammeln; die effektive
    // Textfarbe wird nach dem Parsen bestimmt (Override > style-weit > aus SVG).
    QString foldRel, callRel, raiseRel, allInRel;
    QString styleWideTextColor;
    QString foldTextColor, callTextColor, raiseTextColor, allInTextColor;

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
        else if (tag == "FoldButton") {
            foldRel = value;
            m_foldButton = urlIfExists(value);
        }
        else if (tag == "CheckCallButton") {
            callRel = value;
            m_checkCallButton = urlIfExists(value);
        }
        else if (tag == "BetRaiseButton") {
            raiseRel = value;
            m_betRaiseButton = urlIfExists(value);
        }
        else if (tag == "AllInButton") {
            allInRel = value;
            m_allInButton = urlIfExists(value);
        }
        else if (tag == "ActionButtonTextColor")
            styleWideTextColor = value.trimmed();
        else if (tag == "FoldButtonTextColor")
            foldTextColor = value.trimmed();
        else if (tag == "CheckCallButtonTextColor")
            callTextColor = value.trimmed();
        else if (tag == "BetRaiseButtonTextColor")
            raiseTextColor = value.trimmed();
        else if (tag == "AllInButtonTextColor")
            allInTextColor = value.trimmed();
        // Chat-/Log-Box-Farben (optional je Stil): nur gültige, nicht-leere
        // Werte übernehmen, sonst bleibt der Dunkel-Default erhalten.
        else if (tag == "ChatLogBackground") {
            const QString v = value.trimmed();
            if (!v.isEmpty()) m_chatLogBackground = v;
        }
        else if (tag == "ChatLogSurface") {
            const QString v = value.trimmed();
            if (!v.isEmpty()) m_chatLogSurface = v;
        }
        else if (tag == "ChatLogBorder") {
            const QString v = value.trimmed();
            if (!v.isEmpty()) m_chatLogBorder = v;
        }
        else if (tag == "ChatLogText") {
            const QString v = value.trimmed();
            if (!v.isEmpty()) m_chatLogText = v;
        }
        else if (tag == "ChatLogTextSecondary") {
            const QString v = value.trimmed();
            if (!v.isEmpty()) m_chatLogTextSecondary = v;
        }
        else if (tag == "ChatLogTextMuted") {
            const QString v = value.trimmed();
            if (!v.isEmpty()) m_chatLogTextMuted = v;
        }
    }

    // Effektive Schriftfarbe je Button bestimmen: explizite Theme-Angabe
    // (per Button oder style-weit) hat Vorrang, sonst automatisch aus der
    // Button-Helligkeit – so steht die Schrift immer im Kontrast zum Button.
    auto effectiveTextColor = [&](const QString &override, const QString &rel) -> QString {
        if (!override.isEmpty())
            return override;
        if (!styleWideTextColor.isEmpty())
            return styleWideTextColor;
        if (rel.isEmpty())
            return QString();
        const QString abs = dir.absoluteFilePath(rel);
        if (!QFileInfo::exists(abs))
            return QString();
        return contrastTextColor(abs);
    };
    m_foldButtonTextColor = effectiveTextColor(foldTextColor, foldRel);
    m_checkCallButtonTextColor = effectiveTextColor(callTextColor, callRel);
    m_betRaiseButtonTextColor = effectiveTextColor(raiseTextColor, raiseRel);
    m_allInButtonTextColor = effectiveTextColor(allInTextColor, allInRel);
}

QString StyleProvider::contrastTextColor(const QString &svgAbsPath) const
{
    QFile f(svgAbsPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QStringLiteral("#FFFFFF");
    const QString svg = QString::fromUtf8(f.readAll());

    // Repräsentative Hintergrundfarbe aus den Gradient-Stops mitteln. Der helle
    // Gloss-Streifen (rgba(255,255,255,…)) und der stroke bleiben außen vor.
    static const QRegularExpression reStop(
        QStringLiteral("stop-color\\s*=\\s*\"(#[0-9a-fA-F]{3,8})\""));
    int r = 0, g = 0, b = 0, n = 0;
    auto it = reStop.globalMatch(svg);
    while (it.hasNext()) {
        const QColor c(it.next().captured(1));
        if (c.isValid()) { r += c.red(); g += c.green(); b += c.blue(); ++n; }
    }
    if (n == 0) {
        // Kein Gradient → erstes solides fill="#…" als Notnagel.
        static const QRegularExpression reFill(
            QStringLiteral("fill\\s*=\\s*\"(#[0-9a-fA-F]{3,8})\""));
        const auto m = reFill.match(svg);
        if (m.hasMatch()) {
            const QColor c(m.captured(1));
            if (c.isValid()) { r = c.red(); g = c.green(); b = c.blue(); n = 1; }
        }
    }
    if (n == 0)
        return QStringLiteral("#FFFFFF");

    // Wahrgenommene Helligkeit (sRGB-gewichtet, 0..1). Hell → dunkle Schrift.
    const double rr = r / (255.0 * n);
    const double gg = g / (255.0 * n);
    const double bb = b / (255.0 * n);
    const double luminance = 0.2126 * rr + 0.7152 * gg + 0.0722 * bb;
    return luminance > 0.6 ? QStringLiteral("#1A1A1A") : QStringLiteral("#FFFFFF");
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
