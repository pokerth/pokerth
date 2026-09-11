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
	// Wird bei jedem Schreiben eines Config-Werts erhöht. QML-Bindungen, die
	// generische Werte über readConfigInt()/readConfigString() lesen, referenzieren
	// diese Property, um bei Änderungen sofort (ohne Client-Neustart) neu
	// auszuwerten – analog zum Revisions-Zähler des LobbyHandlers.
	Q_PROPERTY(int configRevision READ configRevision NOTIFY configRevisionChanged)
	// Vom Betriebssystem gemeldeter Hell/Dunkel-Zustand. Nur relevant für die
	// Einstellung "Automatisch" (DarkMode = 2), die ihm folgt – siehe
	// darkmode.h. Ändert der Nutzer das System-Theme im laufenden Betrieb,
	// meldet sich die Property neu und die Oberfläche zieht mit.
	Q_PROPERTY(bool systemDark READ systemDark NOTIFY systemDarkChanged)
	// Wird bei jeder Änderung an den Spieler-Notizen/-Bewertungen erhöht
	// (siehe playerNote()/playerRating()). Bewusst getrennt von
	// configRevision: die Notizen hängen an den Sitzen am Tisch, und jede
	// beliebige andere Einstellung soll deren Bindungen nicht neu auswerten.
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

	// ── Spieler-Notizen und -Bewertungen ─────────────────────────────────
	// Gemeinsamer Speicher mit dem Qt-Widgets-Client: Config-Liste
	// "PlayerTooltips", ein Eintrag je Spieler im Format
	//   Name(!#$%)Notiz(!#$%)Sterne(!#$%)
	// (Trennzeichen und abschließendes Trennzeichen wie in MyAvatarLabel).
	// Die Zerlegung liegt hier in C++ statt in QML, damit beide Clients
	// dasselbe Format schreiben und ein Eintrag nicht durch String-Bastelei
	// in der Oberfläche zerfällt.
	Q_INVOKABLE int playerRating(const QString &playerName) const;
	Q_INVOKABLE QString playerNote(const QString &playerName) const;
	// Notiz und Bewertung eines Spielers in EINEM Schreibvorgang setzen (der
	// Dialog ändert beides gemeinsam). Ein Eintrag ohne Notiz und mit 0
	// Sternen wird entfernt, statt als Leereintrag stehen zu bleiben.
	Q_INVOKABLE void setPlayerNote(const QString &playerName, const QString &note, int rating);
	Q_INVOKABLE void resetToDefaults();
	Q_INVOKABLE QString pickImageFile(const QString &title);

	// Verzeichnisauswahl (Log-Verzeichnis in den Einstellungen). Liefert den
	// gewählten absoluten Pfad; leer bei Abbruch oder wenn die Auswahl kein
	// existierendes lokales Verzeichnis ist.
	Q_INVOKABLE QString pickDirectory(const QString &title, const QString &startDir) const;

	// Avatar-Pfad aus der Config → anzeigbare Bild-URL (file:// bzw. qrc:/ für
	// Alt-Einträge aus dem Ressourcenbundle). Leer, wenn kein Pfad gesetzt ist
	// oder die Datei nicht existiert. Kapselt die URL-Bildung in C++, damit QML
	// nicht selbst Pfade zusammensetzen muss (Windows-Pfade beginnen z. B.
	// nicht mit "/").
	Q_INVOKABLE QUrl avatarDisplayUrl(const QString &path) const;

	// Prüft eine Avatar-Datei mit genau der Funktion, die auch der Upload zum
	// Server benutzt (AvatarManager::OpenAvatarFileForChunkRead): Dateigröße,
	// Format und seit 2.1.8 auch die Bildabmessungen. Nur so bleiben Warnung
	// und Server-Prüfung dieselbe Regel. Ein leerer Pfad gilt als in Ordnung
	// (kein Avatar gewählt ist kein Fehler).
	Q_INVOKABLE bool isAvatarUsable(const QString &path) const;

	// Einmal je Programmlauf true, wenn der eingestellte eigene Avatar von der
	// Engine abgelehnt würde: Die Datei liegt dann zwar lokal vor und ist auch
	// in der eigenen Vorschau zu sehen, andere Spieler bekommen sie aber nicht
	// mehr (der Server lehnt sie beim Ausliefern ab). Betroffen sind vor allem
	// großflächige Grafiken: klein in Kilobyte, aber über der zulässigen
	// Pixelzahl. Die Lobby fragt beim Betreten danach; das Merken verhindert,
	// dass die Warnung nach jeder Rückkehr aus einem Spiel erneut aufgeht.
	Q_INVOKABLE bool takeMyAvatarWarning();

	// Rechnet den eingestellten eigenen Avatar auf ein zulässiges Format
	// herunter (wie beim Import einer zu großen Datei) und trägt die neue
	// Datei als MyAvatar ein. false, wenn nichts zu tun war oder die
	// Umwandlung fehlschlug. Wirksam wird der neue Avatar erst bei der
	// nächsten Anmeldung - der Hash geht beim Verbindungsaufbau raus.
	Q_INVOKABLE bool fixMyAvatar();

	// Für die Über-Seite: Versionsstring (POKERTH_BETA_RELEASE_STRING) sowie
	// die mitgelieferten Texte aus <AppDataDir>/misc/ (leer, wenn nicht gefunden).
	Q_INVOKABLE QString appVersion() const;
	Q_INVOKABLE QString licenseHtml() const;
	Q_INVOKABLE QString thirdPartyLibsText() const;
	Q_INVOKABLE QString changelogText() const;

	// Liste der verfügbaren QML-Stile unter <AppDataDir>/gfx/qml/<table|cards>/*
	// sowie – für importierte Stile – <UserDataDir>/gfx/qml/<...>/*.
	// Jeder Eintrag ist eine Map mit den Schlüsseln:
	//   name, description, maintainer, dir, xml,
	//   preview, previewPortrait  (preview* sind file://-URLs, leer wenn fehlend),
	//   userStyle (true = importiert, liegt im Benutzer-Verzeichnis, löschbar).
	Q_INVOKABLE QVariantList availableTableStyles() const;
	Q_INVOKABLE QVariantList availableCardDeckStyles() const;
	Q_INVOKABLE QVariantList availableCardBackStyles() const;

	// Stil-Import (Pendant zu "addGameTableStyle" & Co. des Widget-Clients):
	// öffnet einen Datei-Dialog für die Stil-XML, prüft die Datei analog zum
	// Widget-Client (XML-Syntax, Stil-Typ, Pflichtfelder, referenzierte
	// Grafiken, Format-Version) und kopiert das komplette Stil-Verzeichnis nach
	// <UserDataDir>/gfx/qml/<category>/<ordnername>/. Ergebnis-Map:
	//   status  "ok" | "warning" | "error" | "cancelled"
	//           (warning = übernommen, aber unvollständig/veraltet – fehlende
	//            Inhalte ersetzt der Client zur Laufzeit durch seine Defaults)
	//   name    Ordnername des importierten Stils (bei ok/warning)
	//   message menschenlesbare Meldung (leer bei ok und cancelled)
	Q_INVOKABLE QVariantMap importTableStyle();
	Q_INVOKABLE QVariantMap importCardDeckStyle();
	Q_INVOKABLE QVariantMap importCardBackStyle();

	// Löscht einen importierten Stil – bewusst nur unterhalb von
	// <UserDataDir>/gfx/qml/, mitgelieferte Stile sind nicht löschbar.
	Q_INVOKABLE bool removeUserStyle(const QString &category, const QString &name);

	// Exportiert einen Stil (mitgeliefert oder importiert) als .zip zum Teilen.
	// Öffnet einen Speichern-Dialog (Vorgabename <name>.zip) und packt den
	// kompletten Stil-Ordner unter einem Wurzelordner <name>/ ins Archiv, sodass
	// er anschließend per import*Style() wieder eingelesen werden kann.
	// Ergebnis-Map: status "ok" | "error" | "cancelled", message (bei error).
	Q_INVOKABLE QVariantMap exportStyle(const QString &category, const QString &name);

	// Liste der mitgelieferten Beispiel-Avatare unter
	// <AppDataDir>/gfx/avatars/default/<people|misc>/*. Diese haben für die
	// Community einen historischen Wert (wie im Widget-Client). Jeder Eintrag
	// ist eine Map mit den Schlüsseln:
	//   name      (Anzeigename, z. B. "No. 1"),
	//   category  ("people" | "misc"),
	//   path      (absoluter Dateipfad, wird so in MyAvatar gespeichert),
	//   url       (file://-URL für die Bildvorschau).
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
	// Scannt <AppDataDir>/gfx/qml/<category>/* und <UserDataDir>/gfx/qml/<category>/*
	// nach Unterordnern, die eine "*<xmlSuffix>"-Datei enthalten, und liefert je
	// Stil eine Beschreibungs-Map. Bei Namensgleichheit gewinnt der mitgelieferte
	// Stil (der Import verhindert solche Duplikate bereits).
	QVariantList scanStyleDir(const QString &category, const QString &xmlSuffix) const;

	// Wurzel-Verzeichnis einer Stil-Kategorie (user=true → Benutzer-Verzeichnis
	// für importierte Stile, sonst mitgelieferte Daten). Ohne Trennzeichen am Ende.
	QString stylesRootPath(bool user, const QString &category) const;

	// Liest eine Textdatei aus <AppDataDir>/misc/ (leer, wenn nicht vorhanden).
	QString readMiscFile(const QString &fileName) const;

	// Basisverzeichnis der Beispiel-Avatare (mit Trennzeichen am Ende). Auf
	// Android liegen sie im Qt-Ressourcenbundle und werden beim ersten Zugriff
	// nach <UserDataDir>/gfx/avatars/default/ kopiert, denn Vorschau (file://)
	// und Engine (std::ifstream beim Avatar-Upload) brauchen echte Dateien.
	QString exampleAvatarsBasePath() const;

	// Übernimmt eine Dateidialog-Auswahl: content://-URIs (Android) und
	// Dateien über dem Engine-Limit (30 KB, MAX_AVATAR_FILE_SIZE) werden als
	// echte Datei unter <UserDataDir>/gfx/avatars/user/ abgelegt – bei Bedarf
	// herunterskaliert –, sonst wird der Pfad unverändert zurückgegeben.
	// Leer bei Abbruch oder Fehler.
	QString importPickedImage(const QString &picked) const;

	// Gemeinsame Implementierung der drei import*Style()-Methoden.
	QVariantMap importStyle(const QString &category, const QString &sectionTag,
							const QString &xmlSuffix, int expectedVersion,
							const QString &lastDirKey, const QString &dialogTitle,
							const QString &wrongTypeMessage);

	boost::shared_ptr<ConfigFile> m_config;
	int m_configRevision = 0;  // hochgezählt bei jedem Schreiben (Live-Reaktivität)
	int m_playerNotesRevision = 0;  // hochgezählt bei jeder Notiz-/Sterne-Änderung
	bool m_myAvatarWarningTaken = false;  // Avatar-Warnung nur einmal je Programmlauf

	// Erhöht m_configRevision und meldet die Änderung → reaktive QML-Bindungen.
	void bumpConfigRevision();
};

#endif // SETTINGSMANAGER_H
