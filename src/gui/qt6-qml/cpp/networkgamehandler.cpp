/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "networkgamehandler.h"

#include <session.h>
#include <configfile.h>
#include <gamedata.h>
#include <gui/generic/serverguiwrapper.h>

#include <QFile>
#include <QTextStream>
#include <QDomDocument>
#include <QVariantMap>

#include <list>

NetworkGameHandler::NetworkGameHandler(QObject *parent)
    : QObject(parent)
{
}

void NetworkGameHandler::setConfig(ConfigFile *config)
{
    m_config = config;
    refreshProfiles();
}

NetworkGameHandler::~NetworkGameHandler()
{
}

void NetworkGameHandler::shutdown()
{
    if (m_serverSession)
        m_serverSession->terminateNetworkServer();
    m_serverSession.reset();
    m_serverGui.reset();
    m_session.reset();
}

void NetworkGameHandler::createGame(int maxPlayers, int startCash, int firstSmallBlind,
                                    bool raiseByHands, int raiseEveryHands, int raiseEveryMinutes,
                                    bool doubleBlinds, int playerActionTimeout, int delayBetweenHands)
{
    if (!m_session || !m_config) {
        emit hostingFailed(tr("No session available."));
        return;
    }

    // Persist the chosen settings (the "saved" network game settings).
    m_config->writeConfigInt("NetNumberOfPlayers", maxPlayers);
    m_config->writeConfigInt("NetStartCash", startCash);
    m_config->writeConfigInt("NetFirstSmallBlind", firstSmallBlind);
    m_config->writeConfigInt("NetRaiseBlindsAtHands", raiseByHands ? 1 : 0);
    m_config->writeConfigInt("NetRaiseBlindsAtMinutes", raiseByHands ? 0 : 1);
    m_config->writeConfigInt("NetRaiseSmallBlindEveryHands", raiseEveryHands);
    m_config->writeConfigInt("NetRaiseSmallBlindEveryMinutes", raiseEveryMinutes);
    m_config->writeConfigInt("NetAlwaysDoubleBlinds", doubleBlinds ? 1 : 0);
    m_config->writeConfigInt("NetManualBlindsOrder", doubleBlinds ? 0 : 1);
    m_config->writeConfigInt("NetTimeOutPlayerAction", playerActionTimeout);
    m_config->writeConfigInt("NetDelayBetweenHands", delayBetweenHands);

    // Build the game data (mirrors startWindowImpl::callCreateNetworkGameDialog).
    GameData gameData;
    gameData.maxNumberOfPlayers = maxPlayers;
    gameData.startMoney = startCash;
    gameData.firstSmallBlind = firstSmallBlind;

    if (raiseByHands) {
        gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
        gameData.raiseSmallBlindEveryHandsValue = raiseEveryHands;
    } else {
        gameData.raiseIntervalMode = RAISE_ON_MINUTES;
        gameData.raiseSmallBlindEveryMinutesValue = raiseEveryMinutes;
    }

    if (doubleBlinds) {
        gameData.raiseMode = DOUBLE_BLINDS;
    } else {
        // Manual blinds: use the saved blind list / after-mode (no inline editor).
        gameData.raiseMode = MANUAL_BLINDS_ORDER;
        gameData.manualBlindsList = m_config->readConfigIntList("NetManualBlindsList");
        if (m_config->readConfigInt("NetAfterMBAlwaysRaiseAbout")) {
            gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
            gameData.afterMBAlwaysRaiseValue = m_config->readConfigInt("NetAfterMBAlwaysRaiseValue");
        } else if (m_config->readConfigInt("NetAfterMBStayAtLastBlind")) {
            gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND;
        } else {
            gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS;
        }
    }

    gameData.guiSpeed = m_config->readConfigInt("GameSpeed");
    gameData.delayBetweenHandsSec = delayBetweenHands;
    gameData.playerActionTimeoutSec = playerActionTimeout;

    // Create the embedded server once (a pseudo GUI wrapper + its own Session),
    // re-used for subsequent hosted games.
    if (!m_serverGui) {
        m_serverGui.reset(new ServerGuiWrapper(m_config, m_session->getGui(), m_session->getGui()));
        boost::shared_ptr<Session> serverSession(new Session(m_serverGui.get(), m_config, 0));
        serverSession->init(m_session->getAvatarManager());
        m_serverGui->setSession(serverSession);
        m_serverSession = serverSession;
    }

    // Terminate any running client/server, then host and connect locally.
    m_session->terminateNetworkClient();
    m_serverSession->terminateNetworkServer();

    m_serverSession->startNetworkServer(false);
    m_session->startNetworkClientForLocalServer(gameData);

    emit hostingStarted();
}

// ─── Join a network server ───────────────────────────────────────────────────

void NetworkGameHandler::joinGame(const QString &address, int port, bool ipv6, bool sctp)
{
    if (!m_session) {
        emit joinFailed(tr("No session available."));
        return;
    }
    const QString addr = address.trimmed();
    if (addr.isEmpty()) {
        emit joinFailed(tr("Please enter a server address."));
        return;
    }

    // Mirror startWindowImpl::callJoinNetworkGameDialog: stop any running
    // client/server, then connect to the remote server (auto-joins its first
    // game → wait room via the existing connect/lobby signals).
    m_session->terminateNetworkClient();
    if (m_serverSession)
        m_serverSession->terminateNetworkServer();

    m_session->startNetworkClient(addr.toStdString(),
                                  static_cast<unsigned>(port), ipv6, sctp);
    emit joinStarted();
}

int NetworkGameHandler::defaultPort() const
{
    if (!m_config)
        return 7234;
    const int p = m_config->readConfigInt("ServerPort");
    return p > 0 ? p : 7234;
}

// ─── Server profiles (<UserDataDir>/serverprofiles.xml) ──────────────────────

QString NetworkGameHandler::serverProfilesPath() const
{
    if (!m_config)
        return QString();
    QString dir = QString::fromStdString(m_config->readConfigString("UserDataDir"));
    if (dir.isEmpty())
        return QString();
    if (!dir.endsWith('/') && !dir.endsWith('\\'))
        dir += '/';
    return dir + "serverprofiles.xml";
}

void NetworkGameHandler::refreshProfiles()
{
    QVariantList list;
    const QString path = serverProfilesPath();
    if (!path.isEmpty()) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QDomDocument doc;
            if (doc.setContent(&file)) {
                // Iterate over every child element regardless of its tag name:
                // the legacy (Qt-Widgets) format uses the profile name as the
                // element tag, so we must not filter on a fixed "Profile" tag.
                QDomElement profiles = doc.documentElement().firstChildElement("ServerProfiles");
                for (QDomElement e = profiles.firstChildElement(); !e.isNull();
                     e = e.nextSiblingElement()) {
                    QVariantMap m;
                    m["name"]    = e.attribute("Name");
                    m["address"] = e.attribute("Address");
                    m["port"]    = e.attribute("Port").toInt();
                    m["ipv6"]    = e.attribute("IsIpv6").toInt() != 0;
                    m["sctp"]    = e.attribute("IsSctp").toInt() != 0;
                    list.append(m);
                }
            }
            file.close();
        }
    }
    m_serverProfiles = list;
    emit serverProfilesChanged();
}

void NetworkGameHandler::saveProfile(const QString &name, const QString &address,
                                     int port, bool ipv6, bool sctp)
{
    const QString path = serverProfilesPath();
    const QString trimmedName = name.trimmed();
    if (path.isEmpty() || trimmedName.isEmpty())
        return;

    QDomDocument doc;
    QFile in(path);
    if (in.open(QIODevice::ReadOnly)) {
        doc.setContent(&in);
        in.close();
    }
    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        root = doc.createElement("PokerTH");
        doc.appendChild(root);
    }
    QDomElement profiles = root.firstChildElement("ServerProfiles");
    if (profiles.isNull()) {
        profiles = doc.createElement("ServerProfiles");
        root.appendChild(profiles);
    }
    // Replace an existing profile with the same name. Match by the Name
    // attribute independent of the element tag, so profiles written by the
    // Qt-Widgets client (and legacy files) are recognized too.
    QDomElement e = profiles.firstChildElement();
    while (!e.isNull()) {
        QDomElement next = e.nextSiblingElement();
        if (e.attribute("Name") == trimmedName)
            profiles.removeChild(e);
        e = next;
    }
    // Element tag = profile name, matching the legacy serverprofiles.xml format
    // shared with the Qt-Widgets client. The UI validator keeps the name a
    // valid XML element name; bail out defensively if it is not.
    QDomElement p = doc.createElement(trimmedName);
    if (p.isNull())
        return;
    p.setAttribute("Name", trimmedName);
    p.setAttribute("Address", address.trimmed());
    p.setAttribute("Port", QString::number(port));
    p.setAttribute("IsIpv6", ipv6 ? 1 : 0);
    p.setAttribute("IsSctp", sctp ? 1 : 0);
    profiles.appendChild(p);

    QFile out(path);
    if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&out);
        stream << doc.toString();
        out.close();
    }
    refreshProfiles();
}

void NetworkGameHandler::deleteProfile(const QString &name)
{
    const QString path = serverProfilesPath();
    if (path.isEmpty())
        return;
    QDomDocument doc;
    QFile in(path);
    if (!in.open(QIODevice::ReadOnly))
        return;
    const auto parseResult = doc.setContent(&in);
    in.close();
    if (!parseResult)   // ParseResult / bool – contextual conversion
        return;

    QDomElement profiles = doc.documentElement().firstChildElement("ServerProfiles");
    QDomElement e = profiles.firstChildElement();
    while (!e.isNull()) {
        QDomElement next = e.nextSiblingElement();
        if (e.attribute("Name") == name)
            profiles.removeChild(e);
        e = next;
    }
    QFile out(path);
    if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&out);
        stream << doc.toString();
        out.close();
    }
    refreshProfiles();
}
