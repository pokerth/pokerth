/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <net/clientthread.h>
#include <net/clientstate.h>
#include <net/clientcontext.h>
#include <net/senderthread.h>
#include <net/receiverhelper.h>
#include <net/clientexception.h>
#include <net/socket_msg.h>
#include <clientenginefactory.h>
#include <game.h>

#include <boost/lambda/lambda.hpp>
#include <cassert>

using namespace std;


class ClientSenderCallback : public SenderCallback
{
public:
	ClientSenderCallback(ClientThread &client) : m_client(client) {}
	virtual ~ClientSenderCallback() {}

	virtual void SignalNetError(SOCKET sock, int errorID, int osErrorID)
	{
		// For now, we ignore the socket.
		// Just signal the error.
		// We assume that the client thread will be terminated.
		m_client.GetCallback().SignalNetClientError(errorID, osErrorID);
	}

private:
	ClientThread &m_client;
};


ClientThread::ClientThread(GuiInterface &gui)
: m_curState(NULL), m_gui(gui), m_curGameId(1), m_guiPlayerId(0)
{
	m_context.reset(new ClientContext);
	m_senderCallback.reset(new ClientSenderCallback(*this));
	m_sender.reset(new SenderThread(GetSenderCallback()));
	m_receiver.reset(new ReceiverHelper);
}

ClientThread::~ClientThread()
{
}

void
ClientThread::Init(
	const string &serverAddress, unsigned serverPort, bool ipv6, bool sctp, const string &pwd, const string &playerName)
{
	if (IsRunning())
	{
		assert(false);
		return;
	}

	ClientContext &context = GetContext();

	context.SetProtocol(sctp ? SOCKET_IPPROTO_SCTP : 0);
	context.SetAddrFamily(ipv6 ? AF_INET6 : AF_INET);
	context.SetServerAddr(serverAddress);
	context.SetServerPort(serverPort);
	context.SetPassword(pwd);
	context.SetPlayerName(playerName);
}

void
ClientThread::SendKickPlayer(const string &playerName)
{
	boost::shared_ptr<PlayerData> tmpPlayer = GetPlayerDataByName(playerName);
	if (tmpPlayer.get())
	{
		boost::shared_ptr<NetPacket> request(new NetPacketKickPlayer);
		NetPacketKickPlayer::Data requestData;
		requestData.playerId = tmpPlayer->GetUniqueId();
		static_cast<NetPacketKickPlayer *>(request.get())->SetData(requestData);
		GetSender().Send(GetContext().GetSocket(), request);
	}
}

void
ClientThread::SendStartEvent()
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet for the server start event.
	boost::shared_ptr<NetPacket> startEvent(new NetPacketStartEvent);
	// The sender is thread-safe, so just dump the packet.
	GetSender().Send(GetContext().GetSocket(), startEvent);
}

void
ClientThread::SendPlayerAction()
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet containing the current player action.
	boost::shared_ptr<NetPacket> action(new NetPacketPlayersAction);
	NetPacketPlayersAction::Data actionData;
	actionData.gameState = static_cast<GameState>(GetGame()->getCurrentHand()->getActualRound());
	actionData.playerAction = static_cast<PlayerAction>(GetGame()->getPlayerArray()[0]->getMyAction());
	// Only send last bet if not fold/checked.
	if (actionData.playerAction != PLAYER_ACTION_FOLD && actionData.playerAction != PLAYER_ACTION_CHECK)
		actionData.playerBet = GetGame()->getPlayerArray()[0]->getMyLastRelativeSet();
	else
		actionData.playerBet = 0;
	static_cast<NetPacketPlayersAction *>(action.get())->SetData(actionData);
	// The sender is thread-safe, so just dump the packet.
	GetSender().Send(GetContext().GetSocket(), action);
}

void
ClientThread::SendChatMessage(const std::string &msg)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet containing the chat message.
	boost::shared_ptr<NetPacket> chat(new NetPacketSendChatText);
	NetPacketSendChatText::Data chatData;
	chatData.text = msg;
	try
	{
		static_cast<NetPacketSendChatText *>(chat.get())->SetData(chatData);
		// The sender is thread-safe, so just dump the packet.
		GetSender().Send(GetContext().GetSocket(), chat);
	} catch (const NetException &)
	{
	}
}

ClientCallback &
ClientThread::GetCallback()
{
	return m_gui;
}

GuiInterface &
ClientThread::GetGui()
{
	return m_gui;
}

void
ClientThread::Main()
{
	SetState(CLIENT_INITIAL_STATE::Instance());

	GetSender().Run();

	try
	{
		while (!ShouldTerminate())
		{
			int msg = GetState().Process(*this);
			if (msg != MSG_SOCK_INTERNAL_PENDING)
			{
				if (msg <= MSG_SOCK_LIMIT_CONNECT)
					GetCallback().SignalNetClientConnect(msg);
				else
					GetCallback().SignalNetClientGameInfo(msg);

				// Additionally signal the start of the game.
				if (msg == MSG_NET_GAME_CLIENT_START)
				{
					// EngineFactory erstellen
					boost::shared_ptr<EngineFactory> factory(new ClientEngineFactory); // LocalEngine erstellen

					MapPlayerDataList();
					if (GetPlayerDataList().size() != GetStartData().numberOfPlayers)
						throw NetException(ERR_NET_INVALID_PLAYER_COUNT, 0);
					m_game.reset(new Game(&m_gui, factory, GetPlayerDataList(), GetGameData(), GetStartData(), m_curGameId++));
					// Initialize GUI speed.
					GetGui().initGui(GetGameData().guiSpeed);
					// Signal start of game to GUI.
					GetCallback().SignalNetClientGameStart(m_game);
				}
			}
		}
	} catch (const NetException &e)
	{
		GetCallback().SignalNetClientError(e.GetErrorId(), e.GetOsErrorCode());
	}
	GetSender().SignalTermination();
	GetSender().Join(SENDER_THREAD_TERMINATE_TIMEOUT);
}

const ClientContext &
ClientThread::GetContext() const
{
	assert(m_context.get());
	return *m_context;
}

ClientContext &
ClientThread::GetContext()
{
	assert(m_context.get());
	return *m_context;
}

ClientState &
ClientThread::GetState()
{
	assert(m_curState);
	return *m_curState;
}

void
ClientThread::SetState(ClientState &newState)
{
	m_curState = &newState;
}

SenderThread &
ClientThread::GetSender()
{
	assert(m_sender.get());
	return *m_sender;
}

ReceiverHelper &
ClientThread::GetReceiver()
{
	assert(m_receiver.get());
	return *m_receiver;
}

const GameData &
ClientThread::GetGameData() const
{
	return m_gameData;
}

void
ClientThread::SetGameData(const GameData &gameData)
{
	m_gameData = gameData;
}

const StartData &
ClientThread::GetStartData() const
{
	return m_startData;
}

void
ClientThread::SetStartData(const StartData &startData)
{
	m_startData = startData;
}

int
ClientThread::GetGuiPlayerId() const
{
	return m_guiPlayerId;
}

void
ClientThread::SetGuiPlayerId(int guiPlayerId)
{
	m_guiPlayerId = guiPlayerId;
}

boost::shared_ptr<Game>
ClientThread::GetGame()
{
	return m_game;
}

ClientSenderCallback &
ClientThread::GetSenderCallback()
{
	assert(m_senderCallback.get());
	return *m_senderCallback;
}

void
ClientThread::AddPlayerData(boost::shared_ptr<PlayerData> playerData)
{
	if (playerData.get() && !playerData->GetName().empty())
	{
		m_playerDataList.push_back(playerData);
		if (playerData->GetUniqueId() == GetGuiPlayerId())
			GetCallback().SignalNetClientSelfJoined(playerData->GetName(), playerData->GetRights());
		else
			GetCallback().SignalNetClientPlayerJoined(playerData->GetName(), playerData->GetRights());
	}
}

void
ClientThread::RemovePlayerData(unsigned playerId)
{
	string playerName;

	PlayerDataList::iterator i = m_playerDataList.begin();
	PlayerDataList::iterator end = m_playerDataList.end();
	while (i != end)
	{
		if ((*i)->GetUniqueId() == playerId)
		{
			playerName = (*i)->GetName();
			m_playerDataList.erase(i);
			break;
		}
		++i;
	}

	if (!playerName.empty())
		GetCallback().SignalNetClientPlayerLeft(playerName);
}

void
ClientThread::MapPlayerDataList()
{
	// Retrieve the GUI player.
	boost::shared_ptr<PlayerData> guiPlayer = GetPlayerDataByUniqueId(GetGuiPlayerId());
	assert(guiPlayer.get());
	int guiPlayerNum = guiPlayer->GetNumber();

	// Create a copy of the player list so that the GUI player
	// is player 0. This is mapped because the GUI depends on it.
	PlayerDataList mappedList;

	PlayerDataList::const_iterator i = m_playerDataList.begin();
	PlayerDataList::const_iterator end = m_playerDataList.end();
	int numPlayers = GetStartData().numberOfPlayers;

	while (i != end)
	{
		boost::shared_ptr<PlayerData> tmpData(new PlayerData(*(*i)));
		int numberDiff = tmpData->GetNumber() - guiPlayerNum;
		if (numberDiff >= 0)
			tmpData->SetNumber(numberDiff);
		else
			tmpData->SetNumber(numPlayers + numberDiff);
		mappedList.push_back(tmpData);
		++i;
	}

	// Sort the list by player number.
	mappedList.sort(*boost::lambda::_1 < *boost::lambda::_2);

	m_playerDataList = mappedList;
}

const PlayerDataList &
ClientThread::GetPlayerDataList() const
{
	return m_playerDataList;
}

boost::shared_ptr<PlayerData>
ClientThread::GetPlayerDataByUniqueId(unsigned id)
{
	boost::shared_ptr<PlayerData> tmpPlayer;

	PlayerDataList::const_iterator i = m_playerDataList.begin();
	PlayerDataList::const_iterator end = m_playerDataList.end();

	while (i != end)
	{
		if ((*i)->GetUniqueId() == id)
		{
			tmpPlayer = *i;
			break;
		}
		++i;
	}
	return tmpPlayer;
}

boost::shared_ptr<PlayerData>
ClientThread::GetPlayerDataByName(const std::string &name)
{
	boost::shared_ptr<PlayerData> tmpPlayer;

	if (!name.empty())
	{
		PlayerDataList::const_iterator i = m_playerDataList.begin();
		PlayerDataList::const_iterator end = m_playerDataList.end();

		while (i != end)
		{
			if ((*i)->GetName() == name)
			{
				tmpPlayer = *i;
				break;
			}
			++i;
		}
	}
	return tmpPlayer;
}

void
ClientThread::RemoveDisconnectedPlayers()
{
	// This should only be called between hands.
	if (m_game.get())
	{
		for (int i = 0; i < m_game->getStartQuantityPlayers(); i++)
		{
			PlayerInterface *tmpPlayer = m_game->getPlayerArray()[i];
			if (tmpPlayer->getMyActiveStatus())
			{
				// If a player is not in the player data list, it was disconnected.
				if (!GetPlayerDataByUniqueId(tmpPlayer->getMyUniqueID()).get())
				{
					tmpPlayer->setMyCash(0);
					tmpPlayer->setMyActiveStatus(false);
				}
			}
		}
	}
}

