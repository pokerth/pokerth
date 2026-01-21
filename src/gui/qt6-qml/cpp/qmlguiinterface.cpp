/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "qmlguiinterface.h"
#include "serverconnectionhandler.h"
#include "lobbyhandler.h"
#include "configfile.h"
#include <session.h>
#include <gamedata.h>
#include <QString>

QmlGuiInterface::QmlGuiInterface(ConfigFile *config, ServerConnectionHandler *handler, LobbyHandler *lobbyHandler)
    : m_config(config), m_session(), m_handler(handler), m_lobbyHandler(lobbyHandler)
{
}

QmlGuiInterface::~QmlGuiInterface()
{
}

void QmlGuiInterface::SignalNetClientConnect(int actionID)
{
    if (m_handler) {
        m_handler->onNetClientConnect(actionID);
    }
}

void QmlGuiInterface::SignalNetClientError(int errorID, int osErrorID)
{
    if (m_handler) {
        m_handler->onNetClientError(errorID, osErrorID);
    }
}

void QmlGuiInterface::SignalNetClientLoginShow()
{
    if (m_handler) {
        m_handler->onNetClientLoginShow();
    }
}

void QmlGuiInterface::SignalNetClientGameListNew(unsigned gameId)
{
    if (m_lobbyHandler && m_session) {
        GameInfo gameInfo = m_session->getClientGameInfo(gameId);
        QString gameName = QString::fromStdString(gameInfo.name);
        m_lobbyHandler->onGameListNew(gameId, gameName);
    }
}

void QmlGuiInterface::SignalNetClientGameListRemove(unsigned gameId)
{
    if (m_lobbyHandler) {
        m_lobbyHandler->onGameListRemove(gameId);
    }
}

void QmlGuiInterface::SignalNetClientGameListUpdateMode(unsigned gameId, GameMode mode)
{
    if (m_lobbyHandler) {
        m_lobbyHandler->onGameListUpdateMode(gameId, static_cast<int>(mode));
    }
}

void QmlGuiInterface::SignalNetClientLobbyChatMsg(const std::string &playerName, const std::string &msg)
{
    if (m_lobbyHandler) {
        m_lobbyHandler->onLobbyChatMessage(QString::fromStdString(playerName), QString::fromStdString(msg));
    }
}

void QmlGuiInterface::SignalLobbyPlayerJoined(unsigned playerId, const std::string &nickName)
{
    if (m_lobbyHandler) {
        m_lobbyHandler->onLobbyPlayerJoined(playerId, QString::fromStdString(nickName));
    }
}

void QmlGuiInterface::SignalLobbyPlayerLeft(unsigned playerId)
{
    if (m_lobbyHandler) {
        m_lobbyHandler->onLobbyPlayerLeft(playerId);
    }
}

void QmlGuiInterface::SignalNetClientPlayerJoined(unsigned playerId, const std::string &playerName, bool isGameAdmin)
{
    if (m_lobbyHandler) {
        m_lobbyHandler->updatePlayerName(playerId, QString::fromStdString(playerName), isGameAdmin);
    }
}

void QmlGuiInterface::SignalNetClientPlayerChanged(unsigned playerId, const std::string &newPlayerName)
{
    if (m_lobbyHandler) {
        m_lobbyHandler->updatePlayerName(playerId, QString::fromStdString(newPlayerName), false);
    }
}

void QmlGuiInterface::SignalNetClientSelfJoined(unsigned playerId, const std::string &playerName, bool isGameAdmin)
{
    if (m_lobbyHandler) {
        m_lobbyHandler->setMyPlayerInfo(playerId, QString::fromStdString(playerName));
    }
}
