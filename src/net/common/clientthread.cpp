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
#include <sstream>
#include <cassert>

using namespace std;


class ClientSenderCallback : public SenderCallback
{
public:
	ClientSenderCallback(ClientThread &client) : m_client(client) {}
	virtual ~ClientSenderCallback() {}

	virtual void SignalNetError(SOCKET /*sock*/, int errorID, int osErrorID)
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
: m_curState(NULL), m_gui(gui), m_curGameId(1), m_guiPlayerId(0), m_sessionEstablished(false)
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
ClientThread::SendKickPlayer(unsigned playerId)
{
	boost::shared_ptr<NetPacket> request(new NetPacketKickPlayer);
	NetPacketKickPlayer::Data requestData;
	requestData.playerId = playerId;
	static_cast<NetPacketKickPlayer *>(request.get())->SetData(requestData);
	boost::mutex::scoped_lock lock(m_outPacketListMutex);
	m_outPacketList.push_back(request);
}

void
ClientThread::SendLeaveCurrentGame()
{
	boost::shared_ptr<NetPacket> request(new NetPacketLeaveCurrentGame);
	boost::mutex::scoped_lock lock(m_outPacketListMutex);
	m_outPacketList.push_back(request);
}

void
ClientThread::SendStartEvent(bool fillUpWithCpuPlayers)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet for the server start event.
	boost::shared_ptr<NetPacket> start(new NetPacketStartEvent);
	NetPacketStartEvent::Data startData;
	startData.fillUpWithCpuPlayers = fillUpWithCpuPlayers;
	try
	{
		static_cast<NetPacketStartEvent *>(start.get())->SetData(startData);
		boost::mutex::scoped_lock lock(m_outPacketListMutex);
		m_outPacketList.push_back(start);
	} catch (const NetException &)
	{
	}
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
	try
	{
		static_cast<NetPacketPlayersAction *>(action.get())->SetData(actionData);
		// Just dump the packet.
		boost::mutex::scoped_lock lock(m_outPacketListMutex);
		m_outPacketList.push_back(action);
	} catch (const NetException &)
	{
	}
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
		// Just dump the packet.
		boost::mutex::scoped_lock lock(m_outPacketListMutex);
		m_outPacketList.push_back(chat);
	} catch (const NetException &)
	{
	}
}

void
ClientThread::SendJoinFirstGame(const std::string &password)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet to request joining a game.
	boost::shared_ptr<NetPacket> join(new NetPacketJoinGame);
	NetPacketJoinGame::Data joinData;
	joinData.gameId = 0;
	joinData.password = password;
	try
	{
		static_cast<NetPacketJoinGame *>(join.get())->SetData(joinData);
		boost::mutex::scoped_lock lock(m_outPacketListMutex);
		m_outPacketList.push_back(join);
	} catch (const NetException &)
	{
		// TODO
	}
}

void
ClientThread::SendJoinGame(unsigned gameId, const std::string &password)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet to request joining a game.
	boost::shared_ptr<NetPacket> join(new NetPacketJoinGame);
	NetPacketJoinGame::Data joinData;
	joinData.password = password;
	joinData.gameId = gameId;
	try
	{
		static_cast<NetPacketJoinGame *>(join.get())->SetData(joinData);
		boost::mutex::scoped_lock lock(m_outPacketListMutex);
		m_outPacketList.push_back(join);
	} catch (const NetException &)
	{
		// TODO
	}
}

void
ClientThread::SendCreateGame(const GameData &gameData, const std::string &name, const std::string &password)
{
	// Warning: This function is called in the context of the GUI thread.
	// Create a network packet to request creating a new game.
	boost::shared_ptr<NetPacket> create(new NetPacketCreateGame);
	NetPacketCreateGame::Data createData;
	createData.gameData = gameData;
	createData.gameName = name;
	createData.password = password;
	try
	{
		static_cast<NetPacketCreateGame *>(create.get())->SetData(createData);
		boost::mutex::scoped_lock lock(m_outPacketListMutex);
		m_outPacketList.push_back(create);
	} catch (const NetException &)
	{
		// TODO
	}
}

GameInfo
ClientThread::GetGameInfo(unsigned gameId) const
{
	GameInfo tmpInfo;
	boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
	GameInfoMap::const_iterator pos = m_gameInfoMap.find(gameId);
	if (pos != m_gameInfoMap.end())
	{
		tmpInfo = pos->second;
	}
	return tmpInfo;
}

PlayerInfo
ClientThread::GetPlayerInfo(unsigned playerId) const
{
	PlayerInfo info;
	if (!GetCachedPlayerInfo(playerId, info))
	{
		ostringstream name;
		name << "#" << playerId;

		info.playerName = name.str();
	}
	return info;
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
					if (GetPlayerDataList().size() != (unsigned)GetStartData().numberOfPlayers)
						throw NetException(ERR_NET_INVALID_PLAYER_COUNT, 0);
					m_game.reset(new Game(&m_gui, factory, GetPlayerDataList(), GetGameData(), GetStartData(), m_curGameId++));
					// Initialize GUI speed.
					GetGui().initGui(GetGameData().guiSpeed);
					// Signal start of game to GUI.
					GetCallback().SignalNetClientGameStart(m_game);
				}
			}
			if (IsSessionEstablished())
				SendPacketLoop();
		}
	} catch (const NetException &e)
	{
		GetCallback().SignalNetClientError(e.GetErrorId(), e.GetOsErrorCode());
	}
	GetSender().SignalTermination();
	GetSender().Join(SENDER_THREAD_TERMINATE_TIMEOUT);
}

void
ClientThread::AddPacket(boost::shared_ptr<NetPacket> packet)
{
	boost::mutex::scoped_lock lock(m_outPacketListMutex);
	m_outPacketList.push_back(packet);
}

void
ClientThread::SendPacketLoop()
{
	boost::mutex::scoped_lock lock(m_outPacketListMutex);

	if (!m_outPacketList.empty())
	{
		NetPacketList::iterator i = m_outPacketList.begin();
		NetPacketList::iterator end = m_outPacketList.end();

		while (i != end)
		{
			GetSender().Send(GetContext().GetSocket(), *i);
			++i;
		}
		m_outPacketList.clear();
	}
}

bool
ClientThread::GetCachedPlayerInfo(unsigned id, PlayerInfo &info) const
{
	bool retVal = false;

	boost::mutex::scoped_lock lock(m_playerInfoMapMutex);
	PlayerInfoMap::const_iterator pos = m_playerInfoMap.find(id);
	if (pos != m_playerInfoMap.end())
	{
		info = pos->second;
		retVal = true;
	}
	return retVal;
}

void
ClientThread::RequestPlayerInfo(unsigned id)
{
	if (find(m_playerInfoRequestList.begin(), m_playerInfoRequestList.end(), id) == m_playerInfoRequestList.end())
	{
		boost::shared_ptr<NetPacket> req(new NetPacketRetrievePlayerInfo);
		NetPacketRetrievePlayerInfo::Data reqData;
		reqData.playerId = id;
		static_cast<NetPacketRetrievePlayerInfo *>(req.get())->SetData(reqData);
		GetSender().Send(GetContext().GetSocket(), req);

		m_playerInfoRequestList.push_back(id);
	}
}

void
ClientThread::SetPlayerInfo(unsigned id, const PlayerInfo &info)
{
	{
		boost::mutex::scoped_lock lock(m_playerInfoMapMutex);
		m_playerInfoMap[id] = info;
	}
	// Update player data for current game.
	boost::shared_ptr<PlayerData> playerData = GetPlayerDataByUniqueId(id);
	if (playerData.get())
	{
		playerData->SetName(info.playerName);
		playerData->SetType(info.ptype);
	}

	GetCallback().SignalNetClientPlayerChanged(id, info.playerName);
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

unsigned
ClientThread::GetGuiPlayerId() const
{
	return m_guiPlayerId;
}

void
ClientThread::SetGuiPlayerId(unsigned guiPlayerId)
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
			GetCallback().SignalNetClientSelfJoined(playerData->GetUniqueId(), playerData->GetName(), playerData->GetRights());
		else
			GetCallback().SignalNetClientPlayerJoined(playerData->GetUniqueId(), playerData->GetName(), playerData->GetRights());
	}
}

void
ClientThread::RemovePlayerData(unsigned playerId)
{
	boost::shared_ptr<PlayerData> tmpData;

	PlayerDataList::iterator i = m_playerDataList.begin();
	PlayerDataList::iterator end = m_playerDataList.end();
	while (i != end)
	{
		if ((*i)->GetUniqueId() == playerId)
		{
			tmpData = *i;
			m_playerDataList.erase(i);
			break;
		}
		++i;
	}

	if (tmpData.get())
	{
		// Remove player from gui.
		GetCallback().SignalNetClientPlayerLeft(tmpData->GetUniqueId(), tmpData->GetName());
	}
}

void
ClientThread::ClearPlayerDataList()
{
	m_playerDataList.clear();
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
			boost::shared_ptr<PlayerInterface> tmpPlayer = m_game->getPlayerArray()[i];
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

unsigned
ClientThread::GetGameIdByName(const std::string &name) const
{
	// Find the game.
	boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
	GameInfoMap::const_iterator i = m_gameInfoMap.begin();
	GameInfoMap::const_iterator end = m_gameInfoMap.end();
	while (i != end)
	{
		if (i->second.name == name)
			break;
		++i;
	}

	if (i == end)
		throw NetException(ERR_NET_UNKNOWN_GAME, 0);
	return i->first;
}

void
ClientThread::AddGameInfo(unsigned gameId, const GameInfo &info)
{
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		m_gameInfoMap.insert(GameInfoMap::value_type(gameId, info));
	}
	GetCallback().SignalNetClientGameListNew(gameId);
}

void
ClientThread::RemoveGameInfo(unsigned gameId)
{
	string name;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		GameInfoMap::iterator pos = m_gameInfoMap.find(gameId);
		if (pos != m_gameInfoMap.end())
		{
			name = pos->second.name;
			m_gameInfoMap.erase(pos);
		}
	}
	if (!name.empty())
		GetCallback().SignalNetClientGameListRemove(gameId);
}

void
ClientThread::ModifyGameInfoAddPlayer(unsigned gameId, unsigned playerId)
{
	bool playerAdded = false;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		GameInfoMap::iterator pos = m_gameInfoMap.find(gameId);
		if (pos != m_gameInfoMap.end())
		{
			pos->second.players.push_back(playerId);
			playerAdded = true;
		}
	}
	if (playerAdded)
		GetCallback().SignalNetClientGameListPlayerJoined(gameId, playerId);
}

void
ClientThread::ModifyGameInfoRemovePlayer(unsigned gameId, unsigned playerId)
{
	bool playerRemoved = false;
	{
		boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
		GameInfoMap::iterator pos = m_gameInfoMap.find(gameId);
		if (pos != m_gameInfoMap.end())
		{
			pos->second.players.remove(playerId);
			playerRemoved = true;
		}
	}
	if (playerRemoved)
		GetCallback().SignalNetClientGameListPlayerLeft(gameId, playerId);
}

void
ClientThread::ClearGameInfoMap()
{
	boost::mutex::scoped_lock lock(m_gameInfoMapMutex);
	m_gameInfoMap.clear();
}

bool
ClientThread::IsSessionEstablished() const
{
	return m_sessionEstablished;
}

void
ClientThread::SetSessionEstablished(bool flag)
{
	m_sessionEstablished = flag;
}

