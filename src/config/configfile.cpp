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

#include "configfile.h"
#include <qttoolsinterface.h>
#include <core/loghelper.h>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QTextStream>
#include <QDir>              // QDir::mkpath – zuverlaessiges Anlegen der App-Verzeichnisse
#include <QStandardPaths>    // iOS: beschreibbares AppDataLocation statt Container-Root

#define MODUS 0711

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#endif

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <set>
#include <algorithm>
#include <cerrno>

#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

namespace
{
// Decode a filesystem path that is stored in the platform's local 8-bit
// encoding into a QString. On Windows getenv("AppData") / _getcwd() /
// GetTempPath() return the path in the active ANSI code page (e.g. CP1250 for a
// Hungarian Windows), so it MUST be decoded with fromLocal8Bit – NOT with
// fromUtf8 (which is what QString::fromStdString does). Using fromUtf8 here
// corrupts any non-ASCII character in the user profile path (e.g. "ő"/"ű"),
// after which the config file path no longer matches the real directory and
// config.xml can neither be read nor written.
QString pathToQString(const std::string &path)
{
	return QString::fromLocal8Bit(path.c_str());
}
}

ConfigFile::ConfigFile(char *argv0, bool readonly) : noWriteAccess(readonly)
{

	myArgv0 = argv0;

	myQtToolsInterface = CreateQtToolsWrapper();

	myConfigState = OK;

	// !!!! Revisionsnummer der Configdefaults !!!!!
	// 109: ServerTlsCertFile / ServerTlsKeyFile hinzugekommen.
	// 110: FlipsideStyle / FlipsideStyleFile hinzugekommen (Widget-Kartenrückseiten-Stile).
	configRev = 110;

	// standard defaults
	logOnOffDefault = "1";

	// Pfad und Dateinamen setzen
#ifdef _WIN32
	const int MaxPathSize = 1024;
	const char *appDataPath = getenv("AppData");
	if (appDataPath && appDataPath[0] != 0)
	{
		configFileName = appDataPath;
	}
	else
	{
		// %AppData% not set -> fall back to the current working directory.
		char curDir[MaxPathSize + 1];
		curDir[0] = 0;
		_getcwd(curDir, MaxPathSize);
		curDir[MaxPathSize] = 0;
		configFileName = curDir;
	}

	// Verify the chosen base directory is actually writable. This must be done
	// for %AppData% too, not only for the current-working-directory fallback:
	// a roaming/redirected (Group Policy) AppData on an unreachable network
	// share, restrictive ACLs or a full disk would otherwise leave us writing
	// into a directory that silently rejects every write. Fall back to %TEMP%
	// in that case so the client at least stays usable for the session.
	{
		ofstream tmpFile;
		const string tmpFilePath = configFileName + "\\pokerth_test.tmp";
		tmpFile.open(tmpFilePath.c_str());
		if (tmpFile)
		{
			// Erfolgreich, Verzeichnis beschreibbar. Datei wieder loeschen.
			tmpFile.close();
			remove(tmpFilePath.c_str());
		}
		else
		{
			// Fehlgeschlagen, Verzeichnis nicht beschreibbar
			char tmpDir[MaxPathSize + 1];
			tmpDir[0] = 0;
			GetTempPathA(MaxPathSize, tmpDir);
			tmpDir[MaxPathSize] = 0;
			LOG_ERROR("Config directory '" << configFileName
				<< "' is not writable, falling back to temp directory '" << tmpDir << "'");
			configFileName = tmpDir;
		}
	}
	// define app-dir
	configFileName += "\\pokerth\\";
	////define log-dir
	logDir = configFileName;
	logDir += "log-files\\";
	////define data-dir
	dataDir = configFileName;
	dataDir += "data\\";
	////define cache-dir
	cacheDir = configFileName;
	cacheDir += "cache\\";

	// create directories on first start of app. _mkdir() returns -1 with
	// errno==EEXIST if the directory already exists, which is fine; any other
	// failure means we will not be able to store the config and is worth
	// surfacing in the log instead of failing silently.
	if (_mkdir(configFileName.c_str()) != 0 && errno != EEXIST)
		LOG_ERROR("Could not create config directory '" << configFileName << "' (errno " << errno << ")");
	_mkdir(logDir.c_str());
	_mkdir(dataDir.c_str());
	_mkdir(cacheDir.c_str());

#elif defined(Q_OS_IOS)
	// iOS: Der Sandbox-Container-Root ($HOME) ist nicht zuverlaessig beschreibbar –
	// ein "mkdir($HOME/.pokerth)" schlaegt still fehl. In der Folge existieren
	// weder Log- noch Cache-Verzeichnis, was sich als Absturz in Log::init()
	// (opendir==NULL) bzw. als Fehler 25 (ERR_SOCK_TRANSFER_OPEN_FAILED) beim
	// Serverlisten-Download aeussert. Deshalb das von Apple vorgesehene,
	// garantiert beschreibbare Application-Support-Verzeichnis nutzen. Es enthaelt
	// bereits den App-Namen (QGuiApplication::setApplicationName/OrganizationName
	// werden vor der ConfigFile-Konstruktion gesetzt), daher kein eigenes
	// ".pokerth" mehr anhaengen.
	{
		const QString iosBase =
			QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
		if (!iosBase.isEmpty())
		{
			configFileName = iosBase.toStdString() + "/";
			////define log-dir
			// Bewusst NICHT unter Application Support, sondern unter Documents:
			// nur Documents ist – zusammen mit UIFileSharingEnabled und
			// LSSupportsOpeningDocumentsInPlace in der Info.plist – aus der
			// Dateien-App und aus dem Finder erreichbar. So kommt man jederzeit
			// an pokerth-debug.log und die Spiel-Logs, ohne den App-Container
			// ueber Xcode herunterladen zu muessen.
			const QString iosDocs =
				QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
			logDir = iosDocs.isEmpty() ? configFileName : (iosDocs.toStdString() + "/");
			logDir += "log-files/";
			////define data-dir
			dataDir = configFileName;
			dataDir += "data/";
			////define cache-dir
			cacheDir = configFileName;
			cacheDir += "cache/";
			// Verzeichnisse zuverlaessig anlegen. QDir::mkpath legt fehlende
			// Elternpfade mit an und ist idempotent (kein Fehler, wenn schon da).
			QDir().mkpath(pathToQString(configFileName));
			QDir().mkpath(pathToQString(logDir));
			QDir().mkpath(pathToQString(dataDir));
			QDir().mkpath(pathToQString(cacheDir));
		}
	}
#else
	// define app-dir
	const char *homePath = getenv("XDG_CONFIG_HOME");
	if (homePath == NULL)
	{
		homePath = getenv("HOME");
	}
	if (homePath)
	{
		configFileName = homePath;
#ifndef ANDROID
		configFileName += "/.pokerth/";
#endif
		////define log-dir
		logDir = configFileName;
		logDir += "log-files/";
		////define data-dir
		dataDir = configFileName;
		dataDir += "data/";
		////define cache-dir
		cacheDir = configFileName;
		cacheDir += "cache/";
		// create directories on first start of app. mkdir() returns -1 with
		// errno==EEXIST if the directory already exists, which is fine; any
		// other failure means we will not be able to store config/cache (and
		// the server list download later fails as "Network error (25)"), so it
		// is worth surfacing in the log instead of failing silently.
		if (mkdir(configFileName.c_str(), MODUS) != 0 && errno != EEXIST)
			LOG_ERROR("Could not create config directory '" << configFileName << "' (errno " << errno << ")");
		mkdir(logDir.c_str(), MODUS);
		mkdir(dataDir.c_str(), MODUS);
		if (mkdir(cacheDir.c_str(), MODUS) != 0 && errno != EEXIST)
			LOG_ERROR("Could not create cache directory '" << cacheDir << "' (errno " << errno << ")");
	}
#endif

	ostringstream tempIntToString;
	tempIntToString << configRev;
	configList.push_back(ConfigInfo("ConfigRevision", CONFIG_TYPE_INT, tempIntToString.str()));
#ifdef ANDROID
	configList.push_back(ConfigInfo("AppDataDir", CONFIG_TYPE_STRING, ":/android/android-data/"));
#else
	configList.push_back(ConfigInfo("AppDataDir", CONFIG_TYPE_STRING, myQtToolsInterface->getDataPathStdString(myArgv0)));
#endif
	configList.push_back(ConfigInfo("Language", CONFIG_TYPE_INT, myQtToolsInterface->getDefaultLanguage()));
	configList.push_back(ConfigInfo("ShowLeftToolBox", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowCountryFlagInAvatar", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowPingStateInAvatar", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowRightToolBox", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowFadeOutCardsAnimation", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowFlipCardsAnimation", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowBlindButtons", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowPotPercentButtons", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowCardsChanceMonitor", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("DontTranslateInternationalPokerStringsFromStyle", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("DisableSplashScreenOnStartup", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AccidentallyCallBlocker", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("DontHideAvatarsOfIgnored", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("DisableEmojiReactions", CONFIG_TYPE_INT, "0"));
	// Chat-Übersetzung anbieten (Globus-Symbol neben eingehenden Nachrichten).
	// Standardmäßig aktiv; es verlässt nichts den Client, bis der Nutzer das
	// Symbol einer konkreten Zeile antippt (siehe ChatTranslator /
	// docs/third_party_services.md).
	configList.push_back(ConfigInfo("AllowChatTranslation", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowChatTimestamp", CONFIG_TYPE_INT, "1")); // Zeitstempel [HH:mm:ss] vor Chatzeilen
	configList.push_back(ConfigInfo("DarkMode", CONFIG_TYPE_INT, "2")); // 0=Light, 1=Dark, 2=Auto/System
	configList.push_back(ConfigInfo("AntiPeekMode", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AlternateFKeysUserActionMode", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("EnableBetInputFocusSwitch", CONFIG_TYPE_INT, "0"));
	// Widget-Client, Kartenrückseite: entweder die flipside.png des gewählten
	// Kartenstapels (FlipsideTux, Default), ein mitgelieferter Rückseiten-Stil
	// (FlipsideStyle + FlipsideStyleFile, voller XML-Pfad wie bei Current*Style)
	// oder eine beliebige eigene Bilddatei (FlipsideOwn + FlipsideOwnFile).
	configList.push_back(ConfigInfo("FlipsideTux", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("FlipsideStyle", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("FlipsideStyleFile", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("FlipsideOwn", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("FlipsideOwnFile", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("GameTableStylesList", CONFIG_TYPE_STRING_LIST, "GameTableStyles"));
	configList.push_back(ConfigInfo("CurrentGameTableStyle", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("CardDeckStylesList", CONFIG_TYPE_STRING_LIST, "CardDeckStyles"));
	configList.push_back(ConfigInfo("PlayerTooltips", CONFIG_TYPE_STRING_LIST, "PlayerTooltips"));
	configList.push_back(ConfigInfo("CurrentCardDeckStyle", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("LastGameTableStyleDir", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("LastCardDeckStyleDir", CONFIG_TYPE_STRING, ""));
	// QML-Client: Kartenrückseiten sind dort eine eigene Stil-Kategorie.
	configList.push_back(ConfigInfo("LastCardBackStyleDir", CONFIG_TYPE_STRING, ""));
	// QML-Client: Name des ausgewählten Stil-Unterordners in
	// <AppDataDir>/gfx/qml/<table|cards>/<name>. Eigene Keys, damit der
	// Widget-Client (Current*Style speichert volle XML-Pfade) unberührt bleibt.
	configList.push_back(ConfigInfo("QmlGameTableStyle", CONFIG_TYPE_STRING, "default"));
	configList.push_back(ConfigInfo("QmlCardDeckStyle", CONFIG_TYPE_STRING, "default"));
	configList.push_back(ConfigInfo("QmlCardBackStyle", CONFIG_TYPE_STRING, "default"));
	// QML-Client: dekorative Render-Effekte (Schlagschatten/Glow/Blur) global
	// abschaltbar für schwache/passiv gekühlte Systeme bzw. Software-Rendering.
	// 0 = Effekte an (Default), 1 = reduziert. Siehe config/Theme.qml.
	configList.push_back(ConfigInfo("QmlReduceEffects", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("PlaySoundEffects", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("SoundVolume", CONFIG_TYPE_INT, "8"));
	configList.push_back(ConfigInfo("PlayGameActions", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("PlayLobbyChatNotification", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("PlayNetworkGameNotification", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("PlayBlindRaiseNotification", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("NumberOfPlayers", CONFIG_TYPE_INT, "10"));
	configList.push_back(ConfigInfo("StartCash", CONFIG_TYPE_INT, "5000"));
	configList.push_back(ConfigInfo("FirstSmallBlind", CONFIG_TYPE_INT, "10"));
	configList.push_back(ConfigInfo("RaiseBlindsAtHands", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("RaiseBlindsAtMinutes", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("RaiseSmallBlindEveryHands", CONFIG_TYPE_INT, "8"));
	configList.push_back(ConfigInfo("RaiseSmallBlindEveryMinutes", CONFIG_TYPE_INT, "5"));
	configList.push_back(ConfigInfo("AlwaysDoubleBlinds", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ManualBlindsOrder", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ManualBlindsList", CONFIG_TYPE_INT_LIST, "Blind"));
	configList.push_back(ConfigInfo("AfterMBAlwaysDoubleBlinds", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("AfterMBAlwaysRaiseAbout", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AfterMBAlwaysRaiseValue", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AfterMBStayAtLastBlind", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("GameSpeed", CONFIG_TYPE_INT, "4"));
	configList.push_back(ConfigInfo("PauseBetweenHands", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ShowGameSettingsDialogOnNewGame", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("NetNumberOfPlayers", CONFIG_TYPE_INT, "10"));
	configList.push_back(ConfigInfo("NetStartCash", CONFIG_TYPE_INT, "3000"));
	configList.push_back(ConfigInfo("NetFirstSmallBlind", CONFIG_TYPE_INT, "10"));
	configList.push_back(ConfigInfo("NetRaiseBlindsAtHands", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("NetRaiseBlindsAtMinutes", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("NetRaiseSmallBlindEveryHands", CONFIG_TYPE_INT, "8"));
	configList.push_back(ConfigInfo("NetRaiseSmallBlindEveryMinutes", CONFIG_TYPE_INT, "5"));
	configList.push_back(ConfigInfo("NetAlwaysDoubleBlinds", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("NetManualBlindsOrder", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("NetManualBlindsList", CONFIG_TYPE_INT_LIST, "NetBlind"));
	configList.push_back(ConfigInfo("NetAfterMBAlwaysDoubleBlinds", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("NetAfterMBAlwaysRaiseAbout", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("NetAfterMBAlwaysRaiseValue", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("NetAfterMBStayAtLastBlind", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("NetGameSpeed", CONFIG_TYPE_INT, "4"));
	configList.push_back(ConfigInfo("NetDelayBetweenHands", CONFIG_TYPE_INT, "7"));
	configList.push_back(ConfigInfo("NetTimeOutPlayerAction", CONFIG_TYPE_INT, "20"));
	configList.push_back(ConfigInfo("NetAutoLeaveGameAfterFinish", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ServerUseIpv6", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerUseSctp", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerUseTls", CONFIG_TYPE_INT, "0"));
	// Absolute paths to the TLS material the server presents. The default points
	// outside the source tree on purpose: the private key must not live in the
	// repository, and in the docker setup /etc/tls is mounted into the
	// container. Clearing an entry selects the historic location instead, the
	// tls/ folder next to bin/ - see GetServerTlsCertFile().
	configList.push_back(ConfigInfo("ServerTlsCertFile", CONFIG_TYPE_STRING, "/etc/tls/server.crt"));
	configList.push_back(ConfigInfo("ServerTlsKeyFile", CONFIG_TYPE_STRING, "/etc/tls/server.key"));
	configList.push_back(ConfigInfo("ServerUseWebSocket", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerUseWebSocketTls", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerPort", CONFIG_TYPE_INT, "7234"));
	configList.push_back(ConfigInfo("ServerWebSocketPort", CONFIG_TYPE_INT, "7233"));
	configList.push_back(ConfigInfo("ServerWebSocketResource", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ServerWebSocketOrigin", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("DiscordChatWebhookUrl", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ServerUsePutAvatars", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ServerPutAvatarsAddress", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ServerPutAvatarsUser", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ServerPutAvatarsPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ServerBruteForceProtection", CONFIG_TYPE_INT, "1"));
	// Comma separated addresses excluded from the login rate limit, for hosts
	// that legitimately funnel many players through one address (e.g. the web
	// client proxy). Empty by default - only set this for hosts you control.
	configList.push_back(ConfigInfo("ServerBruteForceProtectionExempt", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("InternetServerConfigMode", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("InternetServerListAddress", CONFIG_TYPE_STRING, "pokerth.net/serverlist.xml.z"));
	configList.push_back(ConfigInfo("InternetServerAddress", CONFIG_TYPE_STRING, "pokerth.6dns.org"));
	configList.push_back(ConfigInfo("InternetServerPort", CONFIG_TYPE_INT, "7234"));
	configList.push_back(ConfigInfo("InternetServerUseIpv6", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("InternetServerUseSctp", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("InternetServerUseTls", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("UseAvatarServer", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AvatarServerAddress", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("UseInternetGamePassword", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("InternetGamePassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("InternetGameType", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("InternetGameName", CONFIG_TYPE_STRING, "My Online Game"));
	configList.push_back(ConfigInfo("InternetGameAllowSpectators", CONFIG_TYPE_INT, "1"));
	// Community-Inhalte (BBC/WEC-Turnier-Vorlagen im Create-Dialog) und das
	// optionale Admin-„Suggest"-Feature. Default wie im QML-Client: Inhalte an,
	// Suggest aus (nur für wenige BBC/WEC-Admins relevant).
	configList.push_back(ConfigInfo("ShowCommunityContent", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("ShowCommunitySuggest", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("UseLobbyChat", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("UseAdminIRC", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AdminIRCServerAddress", CONFIG_TYPE_STRING, "chat.freenode.net"));
	configList.push_back(ConfigInfo("AdminIRCServerPort", CONFIG_TYPE_INT, "6667"));
	configList.push_back(ConfigInfo("AdminIRCChannel", CONFIG_TYPE_STRING, "#test"));
	configList.push_back(ConfigInfo("AdminIRCChannelPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("AdminIRCServerUseIpv6", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("AdminIRCServerNick", CONFIG_TYPE_INT, "PokerTH_Admin"));
	configList.push_back(ConfigInfo("UseLobbyIRC", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("LobbyIRCServerAddress", CONFIG_TYPE_STRING, "chat.freenode.net"));
	configList.push_back(ConfigInfo("LobbyIRCServerPort", CONFIG_TYPE_INT, "6667"));
	configList.push_back(ConfigInfo("LobbyIRCChannel", CONFIG_TYPE_STRING, "#pokerth-lobby"));
	configList.push_back(ConfigInfo("LobbyIRCChannelPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("LobbyIRCServerUseIpv6", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("LobbyIRCServerNick", CONFIG_TYPE_INT, "PokerTH_Lobby"));
	configList.push_back(ConfigInfo("UseChatCleaner", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ChatCleanerHostAddress", CONFIG_TYPE_STRING, "localhost"));
	configList.push_back(ConfigInfo("ChatCleanerPort", CONFIG_TYPE_INT, "4327"));
	configList.push_back(ConfigInfo("ChatCleanerClientAuth", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ChatCleanerServerAuth", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("ChatCleanerUseIpv6", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("MyName", CONFIG_TYPE_STRING, "Human Player"));
	configList.push_back(ConfigInfo("MyAvatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("MyRememberedNameDuringGuestLogin", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent1Name", CONFIG_TYPE_STRING, "Player 1"));
	configList.push_back(ConfigInfo("Opponent1Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent2Name", CONFIG_TYPE_STRING, "Player 2"));
	configList.push_back(ConfigInfo("Opponent2Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent3Name", CONFIG_TYPE_STRING, "Player 3"));
	configList.push_back(ConfigInfo("Opponent3Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent4Name", CONFIG_TYPE_STRING, "Player 4"));
	configList.push_back(ConfigInfo("Opponent4Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent5Name", CONFIG_TYPE_STRING, "Player 5"));
	configList.push_back(ConfigInfo("Opponent5Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent6Name", CONFIG_TYPE_STRING, "Player 6"));
	configList.push_back(ConfigInfo("Opponent6Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent7Name", CONFIG_TYPE_STRING, "Player 7"));
	configList.push_back(ConfigInfo("Opponent7Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent8Name", CONFIG_TYPE_STRING, "Player 8"));
	configList.push_back(ConfigInfo("Opponent8Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("Opponent9Name", CONFIG_TYPE_STRING, "Player 9"));
	configList.push_back(ConfigInfo("Opponent9Avatar", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("LogOnOff", CONFIG_TYPE_INT, logOnOffDefault));
	configList.push_back(ConfigInfo("LogDir", CONFIG_TYPE_STRING, logDir));
	configList.push_back(ConfigInfo("LogStoreDuration", CONFIG_TYPE_INT, "2"));
	configList.push_back(ConfigInfo("LogInterval", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("UserDataDir", CONFIG_TYPE_STRING, dataDir));
	configList.push_back(ConfigInfo("CacheDir", CONFIG_TYPE_STRING, cacheDir));
	configList.push_back(ConfigInfo("CLA_NoWriteAccess", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("DisableBackToLobbyWarning", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("DlgGameLobbyGameListSortingSection", CONFIG_TYPE_INT, "2"));
	configList.push_back(ConfigInfo("DlgGameLobbyGameListSortingOrder", CONFIG_TYPE_INT, "1"));
	configList.push_back(ConfigInfo("DlgGameLobbyGameListFilterIndex", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("DlgGameLobbyNickListSortFilterIndex", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("GameTableFullScreenSave", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("GameTableHeightSave", CONFIG_TYPE_INT, "621"));
	configList.push_back(ConfigInfo("GameTableWidthSave", CONFIG_TYPE_INT, "1024"));
	configList.push_back(ConfigInfo("InternetLoginMode", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("InternetLoginPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("InternetSavePassword", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("IfInfoMessageShowList", CONFIG_TYPE_STRING_LIST, "Msg"));
	configList.push_back(ConfigInfo("PlayerIgnoreList", CONFIG_TYPE_STRING_LIST, "Player"));
	configList.push_back(ConfigInfo("DBServerAddress", CONFIG_TYPE_STRING, "127.0.0.1"));
	configList.push_back(ConfigInfo("DBServerUser", CONFIG_TYPE_STRING, "pokerth"));
	configList.push_back(ConfigInfo("DBServerPassword", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("DBServerDatabaseName", CONFIG_TYPE_STRING, "pokerth"));
	configList.push_back(ConfigInfo("DBServerEncryptionKey", CONFIG_TYPE_STRING, ""));
	configList.push_back(ConfigInfo("GameNameBadWordList", CONFIG_TYPE_STRING_LIST, "Regex"));
	configList.push_back(ConfigInfo("ServerRestrictGuestLogin", CONFIG_TYPE_INT, "0"));
	configList.push_back(ConfigInfo("ServerLimitRankNum", CONFIG_TYPE_INT, "4"));
	configList.push_back(ConfigInfo("ServerLimitRankPeriod", CONFIG_TYPE_INT, "60"));
	configList.push_back(ConfigInfo("AndroidUiScalePercent", CONFIG_TYPE_INT, "0")); // 0 = auto (fit to screen), >0 = manual override

	// fill tempList firstTime
	configBufferList = configList;

	// 	cout << configTempList[3].name << " " << configTempList[10].defaultValue << endl;

	if (!noWriteAccess)
	{
		configFileName += "config.xml";

		QDomDocument xmlDoc;

		QString qPath = pathToQString(configFileName);
		QFile file(qPath);
		if (!file.open(QIODevice::ReadOnly) || !xmlDoc.setContent(&file))
		{
			file.close();
			myConfigState = NONEXISTING;
			updateConfig(myConfigState);
		}
		else
		{
			file.close();

			// Check if config revision and AppDataDir is ok. Otherwise --> update()
			int tempRevision = 0;
			string tempAppDataPath("");

			QDomElement confRevision = xmlDoc.documentElement().firstChildElement("Configuration").firstChildElement("ConfigRevision");
			if (!confRevision.isNull())
			{
				// confRevision->QueryIntAttribute("value", &tempRevision );
				tempRevision = confRevision.attribute("value").toInt();
			}

			QDomElement confAppDataPath = xmlDoc.documentElement().firstChildElement("Configuration").firstChildElement("AppDataDir");

			if (!confAppDataPath.isNull())
			{
				// const char *tmpStr = confAppDataPath.attribute("value");
				// if (tmpStr) tempAppDataPath = tmpStr;
				QString tempAppDataPath = confAppDataPath.attribute("value");
				// if appdatapath changes directly update it here not in UpdateConfig()
#ifdef ANDROID
				if (tempAppDataPath != ":/android/android-data/")
				{
					confAppDataPath.setAttribute("value", ":/android/android-data/");
#else
				if (tempAppDataPath != QString::fromStdString(myQtToolsInterface->getDataPathStdString(myArgv0)))
				{
					confAppDataPath.setAttribute("value", QString::fromStdString(myQtToolsInterface->getDataPathStdString(myArgv0)));
#endif
					writeConfigDocument(xmlDoc.toString());
				}
			}
			if (tempRevision < configRev)
			{
				myConfigState = OLD;
				updateConfig(myConfigState);
			}
		}

		fillBuffer();
		checkAndCorrectBuffer();
	}
}

ConfigFile::~ConfigFile()
{
	delete myQtToolsInterface;
	myQtToolsInterface = 0;
}

bool ConfigFile::writeConfigDocument(const QString &xmlContent) const
{
	QFile file(pathToQString(configFileName));
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		LOG_ERROR("Could not open config file for writing: '" << configFileName
			<< "' (" << file.errorString().toStdString() << ")");
		return false;
	}
	QTextStream stream(&file);
	stream << xmlContent;
	file.close();
	return true;
}

void ConfigFile::fillBuffer()
{

    boost::recursive_mutex::scoped_lock lock(m_configMutex);

    QDomDocument xmlDoc;
    QFile file(pathToQString(configFileName));
    if (file.open(QIODevice::ReadOnly) && xmlDoc.setContent(&file))
	{
		file.close();

		for (size_t i = 0; i < configBufferList.size(); i++)
		{

			QDomElement conf = xmlDoc.documentElement().firstChildElement("Configuration").firstChildElement(QString::fromStdString(configList[i].name));
			if (!conf.isNull())
			{

				QString tmpStr1 = conf.attribute("value", "");
				configBufferList[i].defaultValue = tmpStr1.toStdString();
				QString tmpStr2 = conf.attribute("type");
				if (tmpStr2 != "")
				{
					if (tmpStr2 == "list")
					{

						list<std::string> tempStringList2;

					QDomElement confList = xmlDoc.documentElement().firstChildElement("Configuration").firstChildElement(QString::fromStdString(configList[i].name));

						for (QDomElement n = confList.firstChildElement(); !n.isNull(); n = n.nextSiblingElement())
						{
							tempStringList2.push_back(n.attribute("value").toStdString());
						}

						configBufferList[i].defaultListValue = tempStringList2;
					}
				}
			}
			else
			{
				LOG_ERROR("Could not find the root element in the config file!");
			}

			// cout << configBufferList[i].name << " " << configBufferList[i].defaultValue << endl;
		}
	}
	file.close();
}

void ConfigFile::checkAndCorrectBuffer()
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);
	// For now, only the player names are checked.
	checkAndCorrectPlayerNames();
}

void ConfigFile::checkAndCorrectPlayerNames()
{
	// Verify that the player names are uniquely set.
	set<string> playerNames;
	playerNames.insert(readConfigString("MyName"));
	for (int i = 1; i <= 9; i++)
	{
		ostringstream opponentVar;
		opponentVar << "Opponent" << i << "Name";
		playerNames.insert(readConfigString(opponentVar.str()));
	}
	if (playerNames.size() < 10 || playerNames.find("") != playerNames.end())
	{
		// The set contains less than 10 players or an empty player name.
		// Reset to default player names.
		writeConfigString("MyName", "Human Player");
		for (int i = 1; i <= 9; i++)
		{
			ostringstream opponentVar;
			ostringstream opponentName;
			opponentVar << "Opponent" << i << "Name";
			opponentName << "Player " << i;
			writeConfigString(opponentVar.str(), opponentName.str());
		}
	}
}

void ConfigFile::writeBuffer() const
{

	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	// write buffer to disc if enabled
	if (!noWriteAccess)
	{
		// Merge the buffer into the existing document instead of rebuilding it
		// from scratch: elements this binary does not know (written by another
		// client flavour or a newer version) must survive every write.
		QDomDocument xmlDoc;
		QDomElement config;

		QFile file(pathToQString(configFileName));
		if (file.open(QIODevice::ReadOnly) && xmlDoc.setContent(&file))
		{
			config = xmlDoc.documentElement().firstChildElement("Configuration");
		}
		file.close();

		if (config.isNull())
		{
			// no usable config on disc --> create a fresh document
			xmlDoc = QDomDocument();
			QDomProcessingInstruction xmlVers = xmlDoc.createProcessingInstruction("xml", "version=\"1.0\" encoding='utf-8'");
			xmlDoc.appendChild(xmlVers);

			QDomElement root = xmlDoc.createElement("PokerTH");
			xmlDoc.appendChild(root);

			config = xmlDoc.createElement("Configuration");
			root.appendChild(config);
		}

		size_t i;

		for (i = 0; i < configBufferList.size(); i++)
		{

			QDomElement tmpElement = config.firstChildElement(QString::fromStdString(configBufferList[i].name));
			if (tmpElement.isNull())
			{
				tmpElement = xmlDoc.createElement(QString::fromStdString(configBufferList[i].name));
				config.appendChild(tmpElement);
			}
			tmpElement.setAttribute("value", QString::fromStdString(configBufferList[i].defaultValue));

			if (configBufferList[i].type == CONFIG_TYPE_INT_LIST || configBufferList[i].type == CONFIG_TYPE_STRING_LIST)
			{

				tmpElement.setAttribute("type", "list");
				while (!tmpElement.firstChild().isNull())
					tmpElement.removeChild(tmpElement.firstChild());

				list<string> tempList = configBufferList[i].defaultListValue;
				list<string>::iterator it;
				for (it = tempList.begin(); it != tempList.end(); ++it)
				{

					QDomElement tmpSubElement = xmlDoc.createElement(QString::fromStdString(configBufferList[i].defaultValue));
					tmpElement.appendChild(tmpSubElement);
					tmpSubElement.setAttribute("value", QString::fromStdString(*it));
				}
			}
		}

		writeConfigDocument(xmlDoc.toString());
	}
}

void ConfigFile::updateConfig(ConfigState myConfigState)
{

	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;

	if (myConfigState == NONEXISTING)
	{
		// configBufferList still holds the defaults at this point, so
		// writeBuffer() creates a fresh document with all default values.
		writeBuffer();
	}

	if (myConfigState == OLD)
	{

		// Update the existing document in place: bump the revision, refresh
		// AppDataDir, apply version hacks and append newly introduced options
		// with their defaults. Everything else - including elements unknown to
		// this binary (other client flavour / newer version) - is left
		// untouched for maximum compatibility.
		QDomDocument xmlDoc;
		QFile file(pathToQString(configFileName));
		if (file.open(QIODevice::ReadOnly) && xmlDoc.setContent(&file))
		{
			file.close();

			QDomElement config = xmlDoc.documentElement().firstChildElement("Configuration");
			if (config.isNull())
			{
				LOG_ERROR("Cannot update config file: no Configuration element found.");
				return;
			}

			// set the value of an element, creating the element if missing
			auto setElementValue = [&xmlDoc, &config](const QString &name, const QString &value) {
				QDomElement el = config.firstChildElement(name);
				if (el.isNull())
				{
					el = xmlDoc.createElement(name);
					config.appendChild(el);
				}
				el.setAttribute("value", value);
			};

			// change configRev and AppDataPath
			setElementValue("ConfigRevision", QString::number(configRev));
			setElementValue("AppDataDir", QString::fromStdString(myQtToolsInterface->stringToUtf8(myQtToolsInterface->getDataPathStdString(myArgv0))));

			///////// VERSION HACK SECTION ///////////////////////
			// this is the right place for special version depending config hacks:
			// 0.9.1 - log interval needs to be set to 1 instead of 0
			if (configRev >= 95 && configRev <= 98)
			{ // this means 0.9.1 or 0.9.2 or 1.0
				setElementValue("LogInterval", "1");
			}

			if (configRev == 98)
			{ // this means 1.0
				setElementValue("CurrentCardDeckStyle", "");
			}
			///////// VERSION HACK SECTION ///////////////////////

			for (i = 0; i < configList.size(); i++)
			{

				// if element is already there --> keep the saved values (and list content) as they are
				if (!config.firstChildElement(QString::fromStdString(configList[i].name)).isNull())
					continue;

				QDomElement tmpElement = xmlDoc.createElement(QString::fromStdString(configList[i].name));
				config.appendChild(tmpElement);
				tmpElement.setAttribute("value", QString::fromStdString(configList[i].defaultValue));

				if (configList[i].type == CONFIG_TYPE_INT_LIST || configList[i].type == CONFIG_TYPE_STRING_LIST)
				{

					tmpElement.setAttribute("type", "list");
					list<string> tempList = configList[i].defaultListValue;
					list<string>::iterator it;
					for (it = tempList.begin(); it != tempList.end(); ++it)
					{

						QDomElement tmpSubElement = xmlDoc.createElement(QString::fromStdString(configList[i].defaultValue));
						tmpElement.appendChild(tmpSubElement);
						tmpSubElement.setAttribute("value", QString::fromStdString(*it));
					}
				}
			}
			writeConfigDocument(xmlDoc.toString());
		}
		else
		{
			LOG_ERROR("Cannot update config file: Unable to load configuration.");
		}
	}
}

ConfigState ConfigFile::getConfigState() const
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);
	return myConfigState;
}

string ConfigFile::readConfigString(string varName) const
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	string tempString("");

	for (i = 0; i < configBufferList.size(); i++)
	{

		if (configBufferList[i].name == varName)
		{
			tempString = configBufferList[i].defaultValue;
		}
	}
	return tempString;
}

int ConfigFile::readConfigInt(string varName) const
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	string tempString("");
	int tempInt = 0;

	for (i = 0; i < configBufferList.size(); i++)
	{

		if (configBufferList[i].name == varName)
		{
			tempString = configBufferList[i].defaultValue;
		}
	}

	istringstream isst;
	isst.str(tempString);
	isst >> tempInt;

	return tempInt;
}

list<int> ConfigFile::readConfigIntList(string varName) const
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	list<string> tempStringList;
	list<int> tempIntList;

	for (i = 0; i < configBufferList.size(); i++)
	{

		if (configBufferList[i].name == varName)
		{
			tempStringList = configBufferList[i].defaultListValue;
		}
	}

	istringstream isst;
	int tempInt;
	list<string>::iterator it;
	for (it = tempStringList.begin(); it != tempStringList.end(); ++it)
	{

		isst.str(*it);
		isst >> tempInt;
		tempIntList.push_back(tempInt);
		isst.str("");
		isst.clear();
	}

	return tempIntList;
}

list<string> ConfigFile::readConfigStringList(string varName) const
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	list<string> tempStringList;

	for (i = 0; i < configBufferList.size(); i++)
	{

		if (configBufferList[i].name == varName)
		{
			tempStringList = configBufferList[i].defaultListValue;
		}
	}

	return tempStringList;
}

void ConfigFile::writeConfigInt(string varName, int varCont)
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	ostringstream intToString;

	for (i = 0; i < configBufferList.size(); i++)
	{

		if (configBufferList[i].name == varName)
		{
			intToString << varCont;
			configBufferList[i].defaultValue = intToString.str();
		}
	}
}

void ConfigFile::writeConfigIntList(string varName, list<int> varCont)
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	ostringstream intToString;
	list<string> stringList;

	for (i = 0; i < configBufferList.size(); i++)
	{

		if (configBufferList[i].name == varName)
		{
			list<int>::iterator it;
			for (it = varCont.begin(); it != varCont.end(); ++it)
			{

				intToString << (*it);
				stringList.push_back(intToString.str());
				intToString.str("");
				intToString.clear();
			}

			configBufferList[i].defaultListValue = stringList;
		}
	}
}

void ConfigFile::writeConfigString(string varName, string varCont)
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	for (i = 0; i < configBufferList.size(); i++)
	{
		if (configBufferList[i].name == varName)
		{
			configBufferList[i].defaultValue = varCont;
		}
	}
}

void ConfigFile::writeConfigStringList(string varName, list<string> varCont)
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);

	size_t i;
	for (i = 0; i < configBufferList.size(); i++)
	{

		if (configBufferList[i].name == varName)
		{
			configBufferList[i].defaultListValue = varCont;
		}
	}
}

void ConfigFile::deleteConfigFile()
{
	remove(configFileName.c_str());
}

void ConfigFile::resetToDefaults()
{
	boost::recursive_mutex::scoped_lock lock(m_configMutex);
	configBufferList = configList;
	writeBuffer();
}
