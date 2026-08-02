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
#include "darkmode.h"
#include "game_defs.h"
#include "ziputils.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QDirIterator>
#include <QFileDialog>
#include <QImage>
#include <QTemporaryDir>
#include <QTextStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QUrl>
#include <QVariantMap>
#include <QXmlStreamReader>
#include <core/appimage_utils.h>

SettingsManager::SettingsManager(boost::shared_ptr<ConfigFile> config, QObject *parent)
    : QObject(parent), m_config(config)
{
    // System-Theme-Wechsel im laufenden Betrieb an die Oberfläche melden
    // (wirkt nur bei DarkMode = "Automatisch", siehe darkmode.h).
    if (QStyleHints *hints = QGuiApplication::styleHints()) {
        connect(hints, &QStyleHints::colorSchemeChanged,
                this, &SettingsManager::systemDarkChanged);
    }
}

bool SettingsManager::systemDark() const
{
    return DarkMode::systemPrefersDark();
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

void SettingsManager::bumpConfigRevision()
{
    ++m_configRevision;
    emit configRevisionChanged();
}

void SettingsManager::writeConfigString(const QString &key, const QString &value)
{
    m_config->writeConfigString(key.toStdString(), value.toStdString());
    m_config->writeBuffer();
    bumpConfigRevision();
}

void SettingsManager::writeConfigInt(const QString &key, int value)
{
    m_config->writeConfigInt(key.toStdString(), value);
    m_config->writeBuffer();
    bumpConfigRevision();
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
    bumpConfigRevision();
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
    bumpConfigRevision();
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
    bumpConfigRevision();
}

QString SettingsManager::pickImageFile(const QString &title)
{
    return importPickedImage(QFileDialog::getOpenFileName(
        nullptr,
        title,
        QString(),
        tr("Images (*.png *.jpg *.jpeg *.gif)"),
        nullptr, AppImageUtils::fileDialogOptions()
    ));
}

QString SettingsManager::pickDirectory(const QString &title, const QString &startDir) const
{
    // Startverzeichnis: der übergebene Pfad, sonst das Home-Verzeichnis (der
    // konfigurierte Pfad kann aus einer alten Config stammen und fehlen).
    QString start = startDir;
    if (start.isEmpty() || !QDir(start).exists())
        start = QDir::home().absolutePath();

    const QString picked = QFileDialog::getExistingDirectory(
        nullptr, title, start,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
            | AppImageUtils::fileDialogOptions());

    // Der Aufrufer erwartet einen echten Dateisystempfad (das Log-Verzeichnis
    // wird von der Engine direkt an SQLite weitergereicht), daher nur
    // existierende lokale Verzeichnisse zurückgeben.
    if (picked.isEmpty() || !QDir(picked).exists())
        return QString();
    return QDir(picked).absolutePath();
}

namespace
{
// Obergrenze der Engine für Avatar-Dateien (MAX_AVATAR_FILE_SIZE in
// core/avatarmanager.h, dort nicht ohne schwere Includes erreichbar);
// größere Dateien verwirft der Upload stillschweigend.
const qint64 kMaxAvatarFileSize = 30720;
} // namespace

QString SettingsManager::importPickedImage(const QString &picked) const
{
    if (picked.isEmpty() || !m_config)
        return picked;

    // Lokale Dateien in Engine-tauglicher Größe direkt verwenden (wie der
    // Widget-Client, der Pfade unverändert speichert).
    const bool isContentUri = picked.startsWith(QLatin1String("content:"));
    if (!isContentUri && QFileInfo(picked).size() <= kMaxAvatarFileSize)
        return picked;

    // Zwei Fälle, in denen die Auswahl so nicht verwendbar wäre:
    //  - Android-content://-URIs sind nur für die laufende Sitzung lesbar und
    //    weder von der file://-Vorschau noch von der Engine (std::ifstream
    //    beim Avatar-Upload) zu öffnen.
    //  - Dateien über dem Engine-Limit (typisch für Fotos aus der Galerie).
    // Beides wird in eine echte Datei unter <UserDataDir>/gfx/avatars/user/
    // überführt, bei Bedarf herunterskaliert. Dateiname = MD5 des Inhalts
    // (Namenskonvention des AvatarManagers), so entstehen bei wiederholter
    // Auswahl keine Duplikate.
    QFile src(picked);
    if (!src.open(QIODevice::ReadOnly))
        return QString();
    QByteArray data = src.readAll();
    if (data.isEmpty())
        return QString();

    // Dateiendung anhand des Dateikopfs bestimmen (content://-URIs haben
    // keine; die Engine erkennt den Avatar-Typ an der Endung).
    QString ext;
    if (data.startsWith("\x89PNG"))
        ext = QStringLiteral(".png");
    else if (data.startsWith("\xFF\xD8\xFF"))
        ext = QStringLiteral(".jpg");
    else if (data.startsWith("GIF8"))
        ext = QStringLiteral(".gif");
    else
        return QString();

    if (data.size() > kMaxAvatarFileSize) {
        // Neu kodieren und stufenweise verkleinern, bis die Datei unter das
        // Engine-Limit fällt (Avatare werden ohnehin klein dargestellt).
        // Animierte GIFs über dem Limit werden dabei zum Standbild.
        const QImage img = QImage::fromData(data);
        if (img.isNull())
            return QString();
        const char *format = img.hasAlphaChannel() ? "PNG" : "JPG";
        ext = img.hasAlphaChannel() ? QStringLiteral(".png") : QStringLiteral(".jpg");
        QByteArray scaledData;
        for (const int edge : { 192, 128, 96, 64 }) {
            const QImage scaled = (img.width() > edge || img.height() > edge)
                ? img.scaled(edge, edge, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                : img;
            QByteArray out;
            QBuffer buffer(&out);
            buffer.open(QIODevice::WriteOnly);
            if (!scaled.save(&buffer, format, 85))
                return QString();
            if (out.size() <= kMaxAvatarFileSize) {
                scaledData = out;
                break;
            }
        }
        if (scaledData.isEmpty())
            return QString();
        data = scaledData;
    }

    // UserDataDir endet bereits mit einem Verzeichnis-Trennzeichen.
    const QString dirPath = QString::fromStdString(m_config->readConfigString("UserDataDir"))
                            + "gfx/avatars/user";
    if (!QDir().mkpath(dirPath))
        return QString();

    const QString target = dirPath + "/"
        + QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex())
        + ext;
    if (!QFile::exists(target)) {
        QFile dst(target);
        if (!dst.open(QIODevice::WriteOnly) || dst.write(data) != data.size()) {
            dst.remove();
            return QString();
        }
    }
    return target;
}

QUrl SettingsManager::avatarDisplayUrl(const QString &path) const
{
    if (path.isEmpty())
        return QUrl();
    // Ressourcenpfade (z. B. vom Widget-Client auf Android gespeicherte
    // Beispiel-Avatare, :/android/...).
    if (path.startsWith(QLatin1Char(':')))
        return QUrl(QStringLiteral("qrc") + path);
    if (!QFileInfo::exists(path))
        return QUrl();
    return QUrl::fromLocalFile(path);
}

QString SettingsManager::appVersion() const
{
    return QStringLiteral(POKERTH_BETA_RELEASE_STRING);
}

QString SettingsManager::licenseHtml() const
{
    return readMiscFile(QStringLiteral("agpl.html"));
}

QString SettingsManager::thirdPartyLibsText() const
{
    return readMiscFile(QStringLiteral("third_party_libs.txt"));
}

QString SettingsManager::readMiscFile(const QString &fileName) const
{
    // AppDataDir endet bereits mit einem Verzeichnis-Trennzeichen.
    QFile file(QString::fromStdString(m_config->readConfigString("AppDataDir"))
               + "misc/" + fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QTextStream(&file).readAll();
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

namespace
{
// Format-Versionen der QML-Stil-XMLs. Tisch und Kartenstapel entsprechen den
// vom Widget-Client unterstützten Versionen (POKERTH_GT_/POKERTH_CD_STYLE_
// FILE_VERSION), die Kartenrückseite ist eine eigene Kategorie des QML-Clients.
const int QML_TABLE_STYLE_VERSION = 3;
const int QML_CARD_DECK_STYLE_VERSION = 2;
const int QML_CARD_BACK_STYLE_VERSION = 1;

// Zählt Dateien unterhalb von path, bricht oberhalb von limit früh ab.
int countFilesRecursively(const QString &path, int limit)
{
    QDir dir(path);
    int count = dir.entryList(QDir::Files).size();
    const QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &subDir : subDirs) {
        if (count > limit)
            break;
        count += countFilesRecursively(dir.absoluteFilePath(subDir), limit - count);
    }
    return count;
}

bool copyDirRecursively(const QString &srcPath, const QString &dstPath)
{
    QDir src(srcPath);
    if (!src.exists() || !QDir().mkpath(dstPath))
        return false;
    const QFileInfoList entries =
        src.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        const QString dst = dstPath + "/" + entry.fileName();
        if (entry.isDir()) {
            if (!copyDirRecursively(entry.absoluteFilePath(), dst))
                return false;
        } else if (!QFile::copy(entry.absoluteFilePath(), dst)) {
            return false;
        }
    }
    return true;
}
} // namespace

QString SettingsManager::stylesRootPath(bool user, const QString &category) const
{
    // AppDataDir/UserDataDir enden bereits mit einem Verzeichnis-Trennzeichen.
    const QString base = QString::fromStdString(
        m_config->readConfigString(user ? "UserDataDir" : "AppDataDir"));
    return base + "gfx/qml/" + category;
}

QVariantMap SettingsManager::importTableStyle()
{
    return importStyle("table", "TableStyle", "tablestyle.xml",
                       QML_TABLE_STYLE_VERSION, "LastGameTableStyleDir",
                       tr("Spieltisch-Stil auswählen"),
                       tr("Die ausgewählte Datei ist kein Spieltisch-Stil."));
}

QVariantMap SettingsManager::importCardDeckStyle()
{
    return importStyle("cards", "CardDeck", "deckstyle.xml",
                       QML_CARD_DECK_STYLE_VERSION, "LastCardDeckStyleDir",
                       tr("Kartenstapel-Stil auswählen"),
                       tr("Die ausgewählte Datei ist kein Kartenstapel-Stil."));
}

QVariantMap SettingsManager::importCardBackStyle()
{
    return importStyle("backside", "CardBack", "backsidestyle.xml",
                       QML_CARD_BACK_STYLE_VERSION, "LastCardBackStyleDir",
                       tr("Kartenrückseiten-Stil auswählen"),
                       tr("Die ausgewählte Datei ist kein Kartenrückseiten-Stil."));
}

QVariantMap SettingsManager::importStyle(const QString &category, const QString &sectionTag,
                                         const QString &xmlSuffix, int expectedVersion,
                                         const QString &lastDirKey, const QString &dialogTitle,
                                         const QString &wrongTypeMessage)
{
    QVariantMap result;
    result["status"] = "cancelled";
    if (!m_config)
        return result;

    auto fail = [&result](const QString &message) {
        result["status"] = "error";
        result["message"] = message;
        return result;
    };

    // Datei-Dialog: startet im zuletzt verwendeten Verzeichnis (wie im
    // Widget-Client über die Last*StyleDir-Config-Keys gemerkt). Akzeptiert ein
    // .zip-Archiv ODER – auf dem Desktop – direkt die lose Stil-XML im Ordner.
    QString startDir = QString::fromStdString(
        m_config->readConfigString(lastDirKey.toStdString()));
    if (startDir.isEmpty() || !QDir(startDir).exists())
        startDir = QDir::home().absolutePath();
    const QString picked = QFileDialog::getOpenFileName(
        nullptr, dialogTitle, startDir,
        tr("PokerTH-Stile (*.zip *.xml)"),
        nullptr, AppImageUtils::fileDialogOptions());
    if (picked.isEmpty())
        return result;
    m_config->writeConfigString(lastDirKey.toStdString(),
                                QFileInfo(picked).absolutePath().toStdString());
    m_config->writeBuffer();

    // Ab hier wird ausschließlich mit lokalen Dateien gearbeitet: xmlPath zeigt
    // auf die auszuwertende Stil-XML, styleDir auf deren Ordner. Ein Archiv wird
    // dazu zuerst in ein temporäres Verzeichnis entpackt – das deckt auch den
    // Flatpak-Portal-Fall ab, bei dem nur die gewählte Datei (nicht der Ordner
    // mit den 52 Karten) in die Sandbox gereicht wird. tempDir lebt bis zum
    // Funktionsende und deckt damit das spätere Kopieren mit ab.
    QString xmlPath;
    QDir styleDir;
    QString name;
    QTemporaryDir tempDir;

    // Dateikopf über QFile lesen (bedient auch content:// / Portal-Pfade) und ein
    // ZIP am Magic "PK\x03\x04" erkennen – unabhängig von der Dateiendung.
    QFile pickedFile(picked);
    if (!pickedFile.open(QIODevice::ReadOnly))
        return fail(tr("Die ausgewählte Datei kann nicht gelesen werden."));
    const bool isZip = pickedFile.peek(4).startsWith(QByteArrayLiteral("PK\x03\x04"));

    if (isZip) {
        if (!tempDir.isValid())
            return fail(tr("Es konnte kein temporäres Verzeichnis angelegt werden."));
        const QByteArray zipData = pickedFile.readAll();
        pickedFile.close();
        QString zipError;
        if (!ZipUtils::extractArchive(zipData, tempDir.path(), zipError))
            return fail(zipError);

        // Die Stil-XML im entpackten Baum suchen (Konvention "*<xmlSuffix>").
        QDirIterator it(tempDir.path(), QStringList() << ("*" + xmlSuffix),
                        QDir::Files, QDirIterator::Subdirectories);
        if (!it.hasNext())
            return fail(tr("Das Archiv enthält keine Datei \"%1\".").arg(xmlSuffix));
        xmlPath = it.next();
        styleDir = QFileInfo(xmlPath).dir();
        // Stilname = Ordnername im Archiv. Liegt die XML im Archiv-Wurzelordner
        // (ohne eigenen Ordner), als Fallback den Archiv-Dateinamen verwenden.
        name = QDir::cleanPath(styleDir.absolutePath()) == QDir::cleanPath(tempDir.path())
                   ? QFileInfo(picked).completeBaseName()
                   : styleDir.dirName();
    } else {
        pickedFile.close();
        xmlPath = picked;
        styleDir = QFileInfo(xmlPath).dir();
        name = styleDir.dirName();
    }

    // XML einlesen: <PokerTH><sectionTag><Tag value="..."/>…. Die Werte werden
    // wie im StyleProvider über das value-Attribut transportiert.
    QFile xmlFile(xmlPath);
    if (!xmlFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return fail(tr("Die Stil-Datei kann nicht gelesen werden."));

    QString sectionName;
    QHash<QString, QString> values;
    QXmlStreamReader xml(&xmlFile);
    int depth = 0;
    while (!xml.atEnd()) {
        const auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            ++depth;
            if (depth == 1 && xml.name() != QStringLiteral("PokerTH"))
                return fail(tr("Die ausgewählte Datei ist kein PokerTH-Stil."));
            else if (depth == 2 && sectionName.isEmpty())
                sectionName = xml.name().toString();
            else if (depth == 3)
                values.insert(xml.name().toString(),
                              xml.attributes().value("value").toString().trimmed());
        } else if (token == QXmlStreamReader::EndElement) {
            --depth;
        }
    }
    if (xml.hasError())
        return fail(tr("Die Stil-Datei enthält kein gültiges XML (%1).")
                        .arg(xml.errorString()));
    if (sectionName != sectionTag)
        return fail(wrongTypeMessage);

    // Scan und StyleProvider finden Stile nur über die Namens-Konvention
    // "*<xmlSuffix>" – eine anders benannte Datei wäre nach dem Import unsichtbar.
    if (!xmlPath.endsWith(xmlSuffix, Qt::CaseInsensitive))
        return fail(tr("Der Dateiname der Stil-Datei muss auf \"%1\" enden.").arg(xmlSuffix));

    // Der Stil-Name ist der Ordnername (Config-Keys wie QmlGameTableStyle
    // speichern nur Namen) – Namenskollision mit vorhandenen Stilen ablehnen.
    if (name.isEmpty())
        return fail(tr("Die Stil-Datei muss in einem eigenen Ordner liegen."));
    if (QDir(stylesRootPath(false, category) + "/" + name).exists()
        || QDir(stylesRootPath(true, category) + "/" + name).exists())
        return fail(tr("Ein Stil mit dem Namen \"%1\" ist bereits vorhanden.").arg(name));

    // Prüfungen analog zum Widget-Client (GameTableStyleReader):
    // leftItems = fehlende Pflichtfelder, picsLeft = nicht gefundene Grafiken.
    QStringList requiredFields = { "StyleDescription", "StyleMaintainerName",
                                   "StyleMaintainerEMail", "StyleCreateDate",
                                   "PokerTHStyleFileVersion", "Preview" };
    QStringList fileFields = { "Preview", "PreviewPortrait" };
    if (category == "table") {
        requiredFields << "Table";
        fileFields << "Table" << "DealerPuck" << "SmallBlindPuck" << "BigBlindPuck"
                   << "FoldButton" << "CheckCallButton" << "BetRaiseButton" << "AllInButton";
    } else if (category == "backside") {
        requiredFields << "Backside";
        fileFields << "Backside";
    }

    QStringList leftItems;
    for (const QString &field : requiredFields) {
        if (values.value(field).isEmpty())
            leftItems << field;
    }

    QStringList picsLeft;
    for (const QString &field : fileFields) {
        const QString rel = values.value(field);
        if (!rel.isEmpty() && !QFileInfo::exists(styleDir.absoluteFilePath(rel)))
            picsLeft << field + " = " + rel;
    }

    // Kartenstapel: die 52 Vorderseiten heißen 0.svg..51.svg (Engine-Index).
    // Fehlende Karten sind – anders als beim Tisch – nicht durch Defaults
    // ersetzbar (CardImage baut die Pfade direkt), daher harter Fehler.
    if (category == "cards") {
        QStringList missingCards;
        for (int i = 0; i < 52; ++i) {
            const QString cardFile = QString::number(i) + ".svg";
            if (!QFileInfo::exists(styleDir.absoluteFilePath(cardFile)))
                missingCards << cardFile;
        }
        if (!missingCards.isEmpty())
            return fail(tr("Der Kartenstapel ist unvollständig, es fehlen: %1")
                            .arg(missingCards.join(", ")));
    }
    // Kartenrückseite: besteht aus genau einer Grafik – ohne sie ist der Stil
    // funktionslos, daher ebenfalls harter Fehler.
    if (category == "backside"
        && (values.value("Backside").isEmpty() || !picsLeft.filter("Backside").isEmpty()))
        return fail(tr("Die Kartenrückseiten-Grafik (Backside) fehlt."));

    // Format-Version prüfen (entspricht der Outdated-Warnung des Widget-Clients).
    const QString versionValue = values.value("PokerTHStyleFileVersion");
    const bool outdated = !versionValue.isEmpty() && versionValue.toInt() != expectedVersion;

    // Stil-Verzeichnis komplett ins Benutzer-Verzeichnis kopieren – erst damit
    // taucht er im Namens-basierten Scan (scanStyleDir/StyleProvider) auf und
    // bleibt unabhängig von der Quelle (Download-Ordner, USB-Stick) erhalten.
    // Vorher grob absichern, dass wirklich ein dedizierter Stil-Ordner gewählt
    // wurde und nicht z. B. eine lose in den Download-Ordner entpackte XML –
    // sonst würde der komplette Ordner-Inhalt mitkopiert. Der größte reguläre
    // Stil (Kartenstapel: 52 Karten + XML + Vorschau) bleibt weit darunter.
    const int fileCount = countFilesRecursively(styleDir.absolutePath(), 200);
    if (fileCount > 200)
        return fail(tr("Der Ordner der Stil-Datei enthält ungewöhnlich viele Dateien. "
                       "Bitte den Stil in einen eigenen Ordner legen."));
    const QString targetPath = stylesRootPath(true, category) + "/" + name;
    if (!copyDirRecursively(styleDir.absolutePath(), targetPath)) {
        QDir(targetPath).removeRecursively();
        return fail(tr("Der Stil konnte nicht nach \"%1\" kopiert werden.").arg(targetPath));
    }

    result["status"] = "ok";
    result["name"] = name;
    QStringList problems;
    if (outdated)
        problems << tr("Der Stil hat Format-Version %1, aktuell ist Version %2.")
                        .arg(versionValue).arg(expectedVersion);
    if (!leftItems.isEmpty())
        problems << tr("Fehlende Angaben: %1.").arg(leftItems.join(", "));
    if (!picsLeft.isEmpty())
        problems << tr("Nicht gefundene Grafiken: %1.").arg(picsLeft.join(", "));
    if (!problems.isEmpty()) {
        result["status"] = "warning";
        result["message"] =
            tr("Der Stil \"%1\" wurde übernommen, ist aber unvollständig:").arg(name)
            + "\n" + problems.join("\n") + "\n"
            + tr("Fehlende Inhalte ersetzt der Client durch Standard-Grafiken.");
    }
    return result;
}

bool SettingsManager::removeUserStyle(const QString &category, const QString &name)
{
    static const QStringList kCategories = { "table", "cards", "backside" };
    // Nur echte Stil-Ordnernamen unterhalb des Benutzer-Verzeichnisses zulassen.
    if (!m_config || !kCategories.contains(category) || name.isEmpty()
        || name.contains('/') || name.contains('\\')
        || name == "." || name == "..")
        return false;
    QDir dir(stylesRootPath(true, category) + "/" + name);
    return dir.exists() && dir.removeRecursively();
}

QVariantMap SettingsManager::exportStyle(const QString &category, const QString &name)
{
    QVariantMap result;
    result["status"] = "cancelled";
    static const QStringList kCategories = { "table", "cards", "backside" };
    // Nur echte Stil-Ordnernamen zulassen (keine Pfad-Bestandteile).
    if (!m_config || !kCategories.contains(category) || name.isEmpty()
        || name.contains('/') || name.contains('\\')
        || name == "." || name == "..")
        return result;

    auto fail = [&result](const QString &message) {
        result["status"] = "error";
        result["message"] = message;
        return result;
    };

    // Stil-Ordner suchen – erst importiert (Benutzer-Verzeichnis), dann
    // mitgeliefert; beide sind exportierbar.
    QString styleDir;
    for (const bool userRoot : { true, false }) {
        const QString candidate = stylesRootPath(userRoot, category) + "/" + name;
        if (QDir(candidate).exists()) {
            styleDir = candidate;
            break;
        }
    }
    if (styleDir.isEmpty())
        return fail(tr("Der Stil \"%1\" wurde nicht gefunden.").arg(name));

    // Speichern-Dialog (Vorgabe: <name>.zip im zuletzt genutzten Export-Ordner).
    QString startDir = QString::fromStdString(
        m_config->readConfigString("LastStyleExportDir"));
    if (startDir.isEmpty() || !QDir(startDir).exists())
        startDir = QDir::home().absolutePath();
    const QString target = QFileDialog::getSaveFileName(
        nullptr, tr("Stil exportieren"), startDir + "/" + name + ".zip",
        tr("ZIP-Archive (*.zip)"), nullptr, AppImageUtils::fileDialogOptions());
    if (target.isEmpty())
        return result;
    m_config->writeConfigString("LastStyleExportDir",
                                QFileInfo(target).absolutePath().toStdString());
    m_config->writeBuffer();

    // Archiv im Speicher erzeugen und über QFile schreiben – das bedient auch
    // content:// / Portal-Zielpfade, die kein reguläres FILE* öffnen lassen.
    QString zipError;
    const QByteArray archive = ZipUtils::createArchive(styleDir, name, zipError);
    if (archive.isEmpty())
        return fail(zipError.isEmpty()
                        ? tr("Das Archiv konnte nicht erstellt werden.")
                        : zipError);

    QFile out(target);
    if (!out.open(QIODevice::WriteOnly) || out.write(archive) != archive.size()) {
        out.close();
        QFile::remove(target);
        return fail(tr("Das Archiv konnte nicht nach \"%1\" geschrieben werden.").arg(target));
    }
    out.close();

    result["status"] = "ok";
    result["name"] = name;
    result["path"] = target;
    return result;
}

QString SettingsManager::exampleAvatarsBasePath() const
{
    // AppDataDir endet bereits mit einem Verzeichnis-Trennzeichen.
    const QString base = QString::fromStdString(m_config->readConfigString("AppDataDir"))
                         + "gfx/avatars/default/";
    if (!base.startsWith(QLatin1Char(':')))
        return base;

    // Android: AppDataDir ist ein Qt-Ressourcenpfad (:/android/android-data/).
    // Vorschau (file://-URL) und Engine (std::ifstream beim Avatar-Upload)
    // brauchen aber echte Dateien, deshalb die Beispiel-Avatare einmalig in
    // das Benutzer-Verzeichnis kopieren. UserDataDir endet bereits mit einem
    // Verzeichnis-Trennzeichen.
    const QString target = QString::fromStdString(m_config->readConfigString("UserDataDir"))
                           + "gfx/avatars/default/";
    const QStringList categories = { QStringLiteral("people"), QStringLiteral("misc") };
    for (const QString &category : categories) {
        QDir srcDir(base + category);
        if (!srcDir.exists() || !QDir().mkpath(target + category))
            continue;
        const QStringList files =
            srcDir.entryList(QStringList() << "*.png", QDir::Files);
        for (const QString &file : files) {
            const QString dst = target + category + "/" + file;
            if (!QFile::exists(dst))
                QFile::copy(srcDir.absoluteFilePath(file), dst);
        }
    }
    return target;
}

QVariantList SettingsManager::availableExampleAvatars() const
{
    QVariantList result;
    if (!m_config)
        return result;

    const QString base = exampleAvatarsBasePath();

    // Reihenfolge der Kategorien wie im Widget-Client (selectAvatarDialog).
    const QStringList categories = { QStringLiteral("people"), QStringLiteral("misc") };
    for (const QString &category : categories) {
        QDir dir(base + category);
        if (!dir.exists())
            continue;

        const QStringList files =
            dir.entryList(QStringList() << "*.png", QDir::Files, QDir::Name);
        int i = 0;
        for (const QString &file : files) {
            const QString abs = dir.absoluteFilePath(file);

            QVariantMap entry;
            entry["name"] = QStringLiteral("No. %1").arg(++i);
            entry["category"] = category;
            entry["path"] = abs;
            entry["url"] = QUrl::fromLocalFile(abs).toString();
            result.append(entry);
        }
    }
    return result;
}

QVariantList SettingsManager::scanStyleDir(const QString &category, const QString &xmlSuffix) const
{
    QVariantList result;
    if (!m_config)
        return result;

    // Mitgelieferte Stile zuerst, danach importierte aus dem Benutzer-Verzeichnis.
    QSet<QString> seenNames;
    for (const bool userRoot : { false, true }) {
        QDir baseDir(stylesRootPath(userRoot, category));
        if (!baseDir.exists())
            continue;

        const QFileInfoList styleDirs =
            baseDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &dirInfo : styleDirs) {
            if (seenNames.contains(dirInfo.fileName()))
                continue;
            QDir styleDir(dirInfo.absoluteFilePath());
            const QStringList xmlFiles =
                styleDir.entryList(QStringList() << ("*" + xmlSuffix), QDir::Files, QDir::Name);
            if (xmlFiles.isEmpty())
                continue;
            seenNames.insert(dirInfo.fileName());

            const QString xmlPath = styleDir.absoluteFilePath(xmlFiles.first());

            QVariantMap entry;
            entry["name"] = dirInfo.fileName();
            entry["dir"] = dirInfo.absoluteFilePath();
            entry["xml"] = xmlPath;
            entry["description"] = dirInfo.fileName(); // Fallback bis XML geparst
            entry["maintainer"] = QString();
            entry["userStyle"] = userRoot;

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
    }
    return result;
}
