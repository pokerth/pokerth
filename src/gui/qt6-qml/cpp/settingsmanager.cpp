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
#include <QDirIterator>
#include <QFileDialog>
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
#include <core/avatarimport.h>

SettingsManager::SettingsManager(boost::shared_ptr<ConfigFile> config, QObject *parent)
	: QObject(parent), m_config(config)
{
	// Report a system theme change during operation to the user interface
	// (only effective with DarkMode = "automatic", see darkmode.h).
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

// ── Player notes and ratings ────────────────────────────────────────────────
// Format of the config list "PlayerTooltips" (shared with the Qt widgets client,
// MyAvatarLabel): name(!#$%)note(!#$%)stars(!#$%)
namespace
{
const QString kPlayerNotesKey = QStringLiteral("PlayerTooltips");
const QString kPlayerNotesSep = QStringLiteral("(!#$%)");

// One list entry split up. Returns false if the line does not match the
// format (too few fields) – such lines are taken over unchanged,
// so that a foreign/broken entry does not disappear silently.
bool splitNoteEntry(const QString &line, QString &name, QString &note, int &rating)
{
	const QStringList f = line.split(kPlayerNotesSep, Qt::KeepEmptyParts);
	if (f.size() < 3)
		return false;
	name = f.at(0);
	note = f.at(1);
	rating = f.at(2).toInt();
	return true;
}

QString joinNoteEntry(const QString &name, const QString &note, int rating)
{
	return name + kPlayerNotesSep + note + kPlayerNotesSep
		   + QString::number(rating) + kPlayerNotesSep;
}
}

int SettingsManager::playerRating(const QString &playerName) const
{
	if (playerName.isEmpty())
		return 0;
	const QStringList lines = readConfigStringList(kPlayerNotesKey);
	for (const QString &line : lines) {
		QString name, note;
		int rating = 0;
		if (splitNoteEntry(line, name, note, rating) && name == playerName)
			return qBound(0, rating, 5);
	}
	return 0;
}

QString SettingsManager::playerNote(const QString &playerName) const
{
	if (playerName.isEmpty())
		return QString();
	const QStringList lines = readConfigStringList(kPlayerNotesKey);
	for (const QString &line : lines) {
		QString name, note;
		int rating = 0;
		if (splitNoteEntry(line, name, note, rating) && name == playerName)
			return note;
	}
	return QString();
}

void SettingsManager::setPlayerNote(const QString &playerName, const QString &note, int rating)
{
	if (playerName.isEmpty())
		return;

	// The separator must not end up in the user text – it would tear the entry
	// into pieces on the next read (here as well as in the Qt widgets client).
	QString cleanNote = note;
	cleanNote.remove(kPlayerNotesSep);
	const int cleanRating = qBound(0, rating, 5);
	const bool empty = cleanNote.isEmpty() && cleanRating == 0;

	QStringList result;
	bool found = false;
	const QStringList lines = readConfigStringList(kPlayerNotesKey);
	for (const QString &line : lines) {
		QString name, oldNote;
		int oldRating = 0;
		if (!splitNoteEntry(line, name, oldNote, oldRating)) {
			result << line;
			continue;
		}
		if (name != playerName) {
			result << joinNoteEntry(name, oldNote, oldRating);
			continue;
		}
		found = true;
		if (!empty)
			result << joinNoteEntry(name, cleanNote, cleanRating);
	}
	if (!found && !empty)
		result << joinNoteEntry(playerName, cleanNote, cleanRating);

	writeConfigStringList(kPlayerNotesKey, result);
	++m_playerNotesRevision;
	emit playerNotesChanged();
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
	// The player notes hang off the same config and are reset afterwards
	// as well – the seat bindings have to be told about that.
	++m_playerNotesRevision;
	emit playerNotesChanged();
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
	// Start directory: the path that was passed, otherwise the home directory (the
	// configured path may come from an old config and be missing).
	QString start = startDir;
	if (start.isEmpty() || !QDir(start).exists())
		start = QDir::home().absolutePath();

	const QString picked = QFileDialog::getExistingDirectory(
							   nullptr, title, start,
							   QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
							   | AppImageUtils::fileDialogOptions());

	// The caller expects a real file system path (the log directory
	// is passed on directly to SQLite by the engine), so only return
	// local directories that exist.
	if (picked.isEmpty() || !QDir(picked).exists())
		return QString();
	return QDir(picked).absolutePath();
}

bool SettingsManager::isAvatarUsable(const QString &path) const
{
	return AvatarImport::isUsable(path);
}

bool SettingsManager::takeMyAvatarWarning()
{
	if (m_myAvatarWarningTaken)
		return false;

	const QString path = myAvatar();
	// A missing path is a different case (no avatar chosen) and is
	// silently accepted as before – a warning is only given when a
	// file is there that nobody gets to see any more.
	if (path.isEmpty() || !QFileInfo::exists(path) || isAvatarUsable(path))
		return false;

	m_myAvatarWarningTaken = true;
	return true;
}

bool SettingsManager::fixMyAvatar()
{
	const QString path = myAvatar();
	if (path.isEmpty() || isAvatarUsable(path))
		return false;

	// importPickedImage() re-encodes the file, scales it down and
	// puts it under <UserDataDir>/gfx/avatars/user/.
	const QString fixed = importPickedImage(path);
	if (fixed.isEmpty() || fixed == path || !isAvatarUsable(fixed))
		return false;

	setMyAvatar(fixed);
	return true;
}

QString SettingsManager::importPickedImage(const QString &picked) const
{
	if (picked.isEmpty() || !m_config)
		return picked;

	// Selection, validation and conversion live in core/avatarimport.h, so that
	// the Qt widgets client does exactly the same.
	return AvatarImport::importImage(
			   picked, QString::fromStdString(m_config->readConfigString("UserDataDir")));
}

QUrl SettingsManager::avatarDisplayUrl(const QString &path) const
{
	if (path.isEmpty())
		return QUrl();
	// Resource paths (e.g. example avatars stored by the widget client on
	// Android, :/android/...).
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

QString SettingsManager::changelogText() const
{
	// misc/ChangeLog is mirrored by CMake from the ChangeLog in the project root
	// directory (see CMakeLists.txt).
	return readMiscFile(QStringLiteral("ChangeLog"));
}

QString SettingsManager::readMiscFile(const QString &fileName) const
{
	// AppDataDir already ends with a directory separator.
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
// Format versions of the QML style XMLs. The table and the card deck correspond to the
// versions supported by the widget client (POKERTH_GT_/POKERTH_CD_STYLE_
// FILE_VERSION), the card back is a category of its own in the QML client.
const int QML_TABLE_STYLE_VERSION = 3;
const int QML_CARD_DECK_STYLE_VERSION = 2;
const int QML_CARD_BACK_STYLE_VERSION = 1;

// Counts files below path, aborting early above limit.
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
	// AppDataDir/UserDataDir already end with a directory separator.
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

	// File dialog: starts in the directory used last (remembered as in the
	// widget client via the Last*StyleDir config keys). It accepts a
	// .zip archive OR – on the desktop – the loose style XML in the folder directly.
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

	// From here on only local files are worked with: xmlPath points
	// to the style XML to be evaluated, styleDir to its folder. An archive is
	// first extracted into a temporary directory for that – this also covers the
	// Flatpak portal case, where only the chosen file (not the folder
	// with the 52 cards) is passed into the sandbox. tempDir lives until the
	// end of the function and thereby covers the later copying as well.
	QString xmlPath;
	QDir styleDir;
	QString name;
	QTemporaryDir tempDir;

	// Read the file header via QFile (this serves content:// / portal paths as well) and
	// recognise a ZIP by the magic "PK\x03\x04" – independently of the file extension.
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

		// Look for the style XML in the extracted tree (the convention "*<xmlSuffix>").
		QDirIterator it(tempDir.path(), QStringList() << ("*" + xmlSuffix),
						QDir::Files, QDirIterator::Subdirectories);
		if (!it.hasNext())
			return fail(tr("Das Archiv enthält keine Datei \"%1\".").arg(xmlSuffix));
		xmlPath = it.next();
		styleDir = QFileInfo(xmlPath).dir();
		// Style name = the folder name in the archive. If the XML lies in the archive root folder
		// (without a folder of its own), use the archive file name as a fallback.
		name = QDir::cleanPath(styleDir.absolutePath()) == QDir::cleanPath(tempDir.path())
			   ? QFileInfo(picked).completeBaseName()
			   : styleDir.dirName();
	} else {
		pickedFile.close();
		xmlPath = picked;
		styleDir = QFileInfo(xmlPath).dir();
		name = styleDir.dirName();
	}

	// Read the XML: <PokerTH><sectionTag><Tag value="..."/>…. The values are
	// transported via the value attribute as in the StyleProvider.
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

	// The scan and the StyleProvider only find styles via the naming convention
	// "*<xmlSuffix>" – a file named differently would be invisible after the import.
	if (!xmlPath.endsWith(xmlSuffix, Qt::CaseInsensitive))
		return fail(tr("Der Dateiname der Stil-Datei muss auf \"%1\" enden.").arg(xmlSuffix));

	// The style name is the folder name (config keys such as QmlGameTableStyle
	// only store names) – reject a name collision with existing styles.
	if (name.isEmpty())
		return fail(tr("Die Stil-Datei muss in einem eigenen Ordner liegen."));
	if (QDir(stylesRootPath(false, category) + "/" + name).exists()
			|| QDir(stylesRootPath(true, category) + "/" + name).exists())
		return fail(tr("Ein Stil mit dem Namen \"%1\" ist bereits vorhanden.").arg(name));

	// Checks analogous to the widget client (GameTableStyleReader):
	// leftItems = missing mandatory fields, picsLeft = graphics that were not found.
	QStringList requiredFields = { "StyleDescription", "StyleMaintainerName",
								   "StyleMaintainerEMail", "StyleCreateDate",
								   "PokerTHStyleFileVersion", "Preview"
								 };
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

	// Card deck: the 52 front sides are named 0.svg..51.svg (the engine index).
	// Missing cards – unlike with the table – cannot be replaced by
	// defaults (CardImage builds the paths directly), hence a hard error.
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
	// Card back: it consists of exactly one graphic – without it the style is
	// non-functional, hence a hard error as well.
	if (category == "backside"
			&& (values.value("Backside").isEmpty() || !picsLeft.filter("Backside").isEmpty()))
		return fail(tr("Die Kartenrückseiten-Grafik (Backside) fehlt."));

	// Check the format version (corresponds to the outdated warning of the widget client).
	const QString versionValue = values.value("PokerTHStyleFileVersion");
	const bool outdated = !versionValue.isEmpty() && versionValue.toInt() != expectedVersion;

	// Copy the style directory completely into the user directory – only then
	// does it show up in the name based scan (scanStyleDir/StyleProvider) and
	// it stays independently of the source (download folder, USB stick).
	// Make sure roughly beforehand that a dedicated style folder really was
	// chosen and not, say, an XML extracted loosely into the download folder –
	// otherwise the complete folder content would be copied along. The largest regular
	// style (card deck: 52 cards + XML + preview) stays well below that.
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
	// Allow only real style folder names below the user directory.
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
	// Allow only real style folder names (no path components).
	if (!m_config || !kCategories.contains(category) || name.isEmpty()
			|| name.contains('/') || name.contains('\\')
			|| name == "." || name == "..")
		return result;

	auto fail = [&result](const QString &message) {
		result["status"] = "error";
		result["message"] = message;
		return result;
	};

	// Look for the style folder – imported ones first (the user directory), then
	// the bundled ones; both can be exported.
	QString styleDir;
	for (const bool userRoot : {
				true, false
			}) {
		const QString candidate = stylesRootPath(userRoot, category) + "/" + name;
		if (QDir(candidate).exists()) {
			styleDir = candidate;
			break;
		}
	}
	if (styleDir.isEmpty())
		return fail(tr("Der Stil \"%1\" wurde nicht gefunden.").arg(name));

	// The save dialog (the default: <name>.zip in the export folder used last).
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

	// Create the archive in memory and write it via QFile – this serves
	// content:// / portal target paths as well, which do not let a regular FILE* be opened.
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
	// AppDataDir already ends with a directory separator.
	const QString base = QString::fromStdString(m_config->readConfigString("AppDataDir"))
						 + "gfx/avatars/default/";
	if (!base.startsWith(QLatin1Char(':')))
		return base;

	// Android: AppDataDir is a Qt resource path (:/android/android-data/).
	// The preview (file:// URL) and the engine (std::ifstream during the avatar upload)
	// need real files though, which is why the example avatars are copied once into
	// the user directory. UserDataDir already ends with a
	// directory separator.
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

	// The order of the categories as in the widget client (selectAvatarDialog).
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

	// Bundled styles first, then the imported ones from the user directory.
	QSet<QString> seenNames;
	for (const bool userRoot : {
				false, true
			}) {
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
			entry["description"] = dirInfo.fileName(); // a fallback until the XML is parsed
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
			// If one orientation is missing, use the other one as a substitute.
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
