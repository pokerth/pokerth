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
#include <QUrl>
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
	// Incremented on every write of a config value. QML bindings that read
	// generic values via readConfigInt()/readConfigString() reference
	// this property in order to re-evaluate immediately on changes (without a
	// client restart) – analogous to the revision counter of the LobbyHandler.
	Q_PROPERTY(int configRevision READ configRevision NOTIFY configRevisionChanged)
	// The light/dark state reported by the operating system. Only relevant for the
	// setting "automatic" (DarkMode = 2), which follows it – see
	// darkmode.h. If the user changes the system theme during operation,
	// the property reports anew and the user interface follows.
	Q_PROPERTY(bool systemDark READ systemDark NOTIFY systemDarkChanged)
	// Incremented on every change to the player notes/ratings
	// (see playerNote()/playerRating()). Deliberately separate from
	// configRevision: the notes hang off the seats at the table, and any
	// other setting should not make their bindings re-evaluate.
	Q_PROPERTY(int playerNotesRevision READ playerNotesRevision NOTIFY playerNotesChanged)

public:
	explicit SettingsManager(boost::shared_ptr<ConfigFile> config, QObject *parent = nullptr);

	// Property getters
	QString language() const;
	QString style() const;
	bool soundEnabled() const;
	bool disableSplashScreen() const;
	QString myName() const;
	QString myAvatar() const;
	int configRevision() const
	{
		return m_configRevision;
	}
	int playerNotesRevision() const
	{
		return m_playerNotesRevision;
	}
	bool systemDark() const;

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

	// ── Player notes and ratings ─────────────────────────────────────────
	// Shared storage with the Qt widgets client: the config list
	// "PlayerTooltips", one entry per player in the format
	//   name(!#$%)note(!#$%)stars(!#$%)
	// (the separator and the trailing separator as in MyAvatarLabel).
	// The splitting lives here in C++ instead of in QML, so that both clients
	// write the same format and an entry does not fall apart through string
	// tinkering in the user interface.
	Q_INVOKABLE int playerRating(const QString &playerName) const;
	Q_INVOKABLE QString playerNote(const QString &playerName) const;
	// Set the note and the rating of a player in ONE write operation (the
	// dialog changes both together). An entry without a note and with 0
	// stars is removed instead of staying as an empty entry.
	Q_INVOKABLE void setPlayerNote(const QString &playerName, const QString &note, int rating);
	Q_INVOKABLE void resetToDefaults();
	Q_INVOKABLE QString pickImageFile(const QString &title);

	// Directory selection (the log directory in the settings). It returns the
	// chosen absolute path; empty on cancel or if the selection is no
	// existing local directory.
	Q_INVOKABLE QString pickDirectory(const QString &title, const QString &startDir) const;

	// Avatar path from the config → a displayable image URL (file:// or qrc:/ for
	// old entries from the resource bundle). Empty if no path is set
	// or the file does not exist. It encapsulates building the URL in C++, so that QML
	// does not have to assemble paths itself (Windows paths, for instance, do not
	// start with "/").
	Q_INVOKABLE QUrl avatarDisplayUrl(const QString &path) const;

	// Checks an avatar file with exactly the function that the upload to the
	// server uses as well (AvatarManager::OpenAvatarFileForChunkRead): the file size,
	// the format and, since 2.1.8, the image dimensions too. Only that way do the warning
	// and the server check stay the same rule. An empty path counts as fine
	// (no avatar chosen is not an error).
	Q_INVOKABLE bool isAvatarUsable(const QString &path) const;

	// True once per program run if the configured own avatar would be rejected by the
	// engine: the file is then present locally and can also be seen
	// in your own preview, but other players do not get it
	// any more (the server rejects it when serving it). Affected are above all
	// large flat graphics: small in kilobytes, but above the permitted
	// pixel count. The lobby asks for it when entering; remembering it prevents
	// the warning from opening again after every return from a game.
	Q_INVOKABLE bool takeMyAvatarWarning();

	// Scales the configured own avatar down to a permitted format
	// (as when importing a file that is too large) and enters the new
	// file as MyAvatar. false if there was nothing to do or the
	// conversion failed. The new avatar only takes effect at the
	// next login - the hash goes out while the connection is established.
	Q_INVOKABLE bool fixMyAvatar();

	// For the about page: the version string (POKERTH_BETA_RELEASE_STRING) as well as
	// the bundled texts from <AppDataDir>/misc/ (empty if not found).
	Q_INVOKABLE QString appVersion() const;
	Q_INVOKABLE QString licenseHtml() const;
	Q_INVOKABLE QString thirdPartyLibsText() const;
	Q_INVOKABLE QString changelogText() const;

	// List of the available QML styles under <AppDataDir>/gfx/qml/<table|cards>/*
	// as well as – for imported styles – <UserDataDir>/gfx/qml/<...>/*.
	// Every entry is a map with the keys:
	//   name, description, maintainer, dir, xml,
	//   preview, previewPortrait  (preview* are file:// URLs, empty if missing),
	//   userStyle (true = imported, lies in the user directory, can be deleted).
	Q_INVOKABLE QVariantList availableTableStyles() const;
	Q_INVOKABLE QVariantList availableCardDeckStyles() const;
	Q_INVOKABLE QVariantList availableCardBackStyles() const;

	// Style import (the counterpart to "addGameTableStyle" & co. of the widget client):
	// it opens a file dialog for the style XML, checks the file analogously to the
	// widget client (XML syntax, style type, mandatory fields, referenced
	// graphics, format version) and copies the complete style directory to
	// <UserDataDir>/gfx/qml/<category>/<folder name>/. The result map:
	//   status  "ok" | "warning" | "error" | "cancelled"
	//           (warning = adopted, but incomplete/outdated – missing
	//            content is replaced by the client at runtime with its defaults)
	//   name    the folder name of the imported style (with ok/warning)
	//   message a human readable message (empty with ok and cancelled)
	Q_INVOKABLE QVariantMap importTableStyle();
	Q_INVOKABLE QVariantMap importCardDeckStyle();
	Q_INVOKABLE QVariantMap importCardBackStyle();

	// Deletes an imported style – deliberately only below
	// <UserDataDir>/gfx/qml/, bundled styles cannot be deleted.
	Q_INVOKABLE bool removeUserStyle(const QString &category, const QString &name);

	// Exports a style (bundled or imported) as a .zip for sharing.
	// It opens a save dialog (the default name <name>.zip) and packs the
	// complete style folder under a root folder <name>/ into the archive, so that
	// it can be read in again afterwards via import*Style().
	// The result map: status "ok" | "error" | "cancelled", message (on error).
	Q_INVOKABLE QVariantMap exportStyle(const QString &category, const QString &name);

	// List of the bundled example avatars under
	// <AppDataDir>/gfx/avatars/default/<people|misc>/*. These have historical
	// value for the community (as in the widget client). Every entry
	// is a map with the keys:
	//   name      (the display name, e.g. "No. 1"),
	//   category  ("people" | "misc"),
	//   path      (the absolute file path, stored like this in MyAvatar),
	//   url       (the file:// URL for the image preview).
	Q_INVOKABLE QVariantList availableExampleAvatars() const;

signals:
	void languageChanged();
	void styleChanged();
	void soundEnabledChanged();
	void disableSplashScreenChanged();
	void myNameChanged();
	void myAvatarChanged();
	void configRevisionChanged();
	void systemDarkChanged();
	void playerNotesChanged();

private:
	// Scans <AppDataDir>/gfx/qml/<category>/* and <UserDataDir>/gfx/qml/<category>/*
	// for subfolders that contain a "*<xmlSuffix>" file and returns a
	// description map per style. On a name collision the bundled style
	// wins (the import prevents such duplicates already).
	QVariantList scanStyleDir(const QString &category, const QString &xmlSuffix) const;

	// The root directory of a style category (user=true → the user directory
	// for imported styles, otherwise the bundled data). Without a trailing separator.
	QString stylesRootPath(bool user, const QString &category) const;

	// Reads a text file from <AppDataDir>/misc/ (empty if it is not present).
	QString readMiscFile(const QString &fileName) const;

	// The base directory of the example avatars (with a trailing separator). On
	// Android they live in the Qt resource bundle and are copied on the first access
	// to <UserDataDir>/gfx/avatars/default/, because the preview (file://)
	// and the engine (std::ifstream during the avatar upload) need real files.
	QString exampleAvatarsBasePath() const;

	// Adopts a file dialog selection: content:// URIs (Android) and
	// files above the engine limit (30 KB, MAX_AVATAR_FILE_SIZE) are stored as
	// a real file under <UserDataDir>/gfx/avatars/user/ – scaled down if
	// needed –, otherwise the path is returned unchanged.
	// Empty on cancel or error.
	QString importPickedImage(const QString &picked) const;

	// The shared implementation of the three import*Style() methods.
	QVariantMap importStyle(const QString &category, const QString &sectionTag,
							const QString &xmlSuffix, int expectedVersion,
							const QString &lastDirKey, const QString &dialogTitle,
							const QString &wrongTypeMessage);

	boost::shared_ptr<ConfigFile> m_config;
	int m_configRevision = 0;  // incremented on every write (live reactivity)
	int m_playerNotesRevision = 0;  // incremented on every note/star change
	bool m_myAvatarWarningTaken = false;  // the avatar warning only once per program run

	// Increments m_configRevision and reports the change → reactive QML bindings.
	void bumpConfigRevision();
};

#endif // SETTINGSMANAGER_H
