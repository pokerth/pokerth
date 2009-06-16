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
/* Network client thread. */

#ifndef _CLIENTTHREAD_H_
#define _CLIENTTHREAD_H_

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>

#include <core/thread.h>
#include <guiinterface.h>
#include <serverdata.h>
#include <playerdata.h>
#include <gamedata.h>

class ClientContext;
class ClientState;
class SenderHelper;
class ReceiverHelper;
class DownloaderThread;
class ClientSenderCallback;
class Game;
class NetPacket;
class AvatarManager;
class QtToolsInterface;

class ClientThread : public Thread, public boost::enable_shared_from_this<ClientThread>
{
public:
	ClientThread(GuiInterface &gui, AvatarManager &avatarManager);
	virtual ~ClientThread();

	// Set the parameters. Does not do any error checking.
	// Error checking will be done during connect
	// (i.e. after starting the thread).
	void Init(
		const std::string &serverAddress,
		const std::string &serverListUrl,
		bool useServerList,
		unsigned serverPort,
		bool ipv6,
		bool sctp,
		const std::string &avatarServerAddress,
		const std::string &pwd,
		const std::string &playerName,
		const std::string &avatarFile,
		const std::string &cacheDir);
	virtual void SignalTermination();

	void SendKickPlayer(unsigned playerId);
	void SendLeaveCurrentGame();
	void SendStartEvent(bool fillUpWithCpuPlayers);
	void SendPlayerAction();
	void SendChatMessage(const std::string &msg);
	void SendJoinFirstGame(const std::string &password);
	void SendJoinGame(unsigned gameId, const std::string &password);
	void SendCreateGame(const GameData &gameData, const std::string &name, const std::string &password);
	void SendResetTimeout();
	void SendAskKickPlayer(unsigned playerId);
	void SendVoteKick(bool doKick);

	void StartAsyncRead();
	void HandleRead(const boost::system::error_code& ec, size_t bytesRead);

	void SelectServer(unsigned serverId);
	ServerInfo GetServerInfo(unsigned serverId) const;

	GameInfo GetGameInfo(unsigned gameId) const;
	PlayerInfo GetPlayerInfo(unsigned playerId) const;
	bool GetPlayerIdFromName(const std::string &playerName, unsigned &playerId) const;
	ServerStats GetStatData() const;
	unsigned GetGameId() const;

	ClientCallback &GetCallback();
	GuiInterface &GetGui();
	AvatarManager &GetAvatarManager();

protected:
	typedef std::map<unsigned, GameInfo> GameInfoMap;
	typedef std::list<boost::shared_ptr<NetPacket> > NetPacketList;
	typedef std::map<unsigned, PlayerInfo> PlayerInfoMap;
	typedef std::map<unsigned, boost::shared_ptr<AvatarData> > AvatarDataMap;
	typedef std::map<unsigned, ServerInfo> ServerInfoMap;

	// Main function of the thread.
	virtual void Main();
	void RegisterTimers();
	void CancelTimers();
	void InitGame();

	void SendPacket(boost::shared_ptr<NetPacket> packet);

	bool GetCachedPlayerInfo(unsigned id, PlayerInfo &info) const;
	void RequestPlayerInfo(unsigned id, bool requestAvatar = false);
	void SetPlayerInfo(unsigned id, const PlayerInfo &info);
	void SetUnknownPlayer(unsigned id);
	void SetNewGameAdmin(unsigned id);
	void RetrieveAvatarIfNeeded(unsigned id, const PlayerInfo &info);

	void AddTempAvatarData(unsigned playerId, unsigned avatarSize, AvatarFileType type);
	void StoreInTempAvatarData(unsigned playerId, const std::vector<unsigned char> &data);
	void CompleteTempAvatarData(unsigned playerId);
	void PassAvatarDataToManager(unsigned playerId, boost::shared_ptr<AvatarData> avatarData);
	void SetUnknownAvatar(unsigned playerId);

	void TimerCheckAvatarDownloads(const boost::system::error_code& ec);

	void UnsubscribeLobbyMsg();
	void ResubscribeLobbyMsg();

	const ClientContext &GetContext() const;
	ClientContext &GetContext();
	void CreateContextSession();

	ClientState &GetState();
	void SetState(ClientState &newState);
	boost::asio::deadline_timer &GetStateTimer();

	SenderHelper &GetSender();
	ReceiverHelper &GetReceiver();

	void SetGameId(unsigned id);
	const GameData &GetGameData() const;
	void SetGameData(const GameData &gameData);
	const StartData &GetStartData() const;
	void SetStartData(const StartData &startData);
	unsigned GetGuiPlayerId() const;
	void SetGuiPlayerId(unsigned guiPlayerId);

	boost::shared_ptr<Game> GetGame();

	ClientSenderCallback &GetSenderCallback();

	QtToolsInterface &GetQtToolsInterface();

	void AddPlayerData(boost::shared_ptr<PlayerData> playerData);
	void RemovePlayerData(unsigned playerId, int removeReason);
	void ClearPlayerDataList();
	void MapPlayerDataList();
	const PlayerDataList &GetPlayerDataList() const;
	boost::shared_ptr<PlayerData> GetPlayerDataByUniqueId(unsigned id);
	boost::shared_ptr<PlayerData> GetPlayerDataByName(const std::string &name);

	void RemoveDisconnectedPlayers();

	void AddServerInfo(unsigned serverId, const ServerInfo &info);
	void ClearServerInfoMap();
	bool GetSelectedServer(unsigned &serverId) const;
	void UseServer(unsigned serverId);

	unsigned GetGameIdByName(const std::string &name) const;
	void AddGameInfo(unsigned gameId, const GameInfo &info);
	void UpdateGameInfoMode(unsigned gameId, GameMode mode);
	void UpdateGameInfoAdmin(unsigned gameId, unsigned adminPlayerId);
	void RemoveGameInfo(unsigned gameId);
	void ModifyGameInfoAddPlayer(unsigned gameId, unsigned playerId);
	void ModifyGameInfoRemovePlayer(unsigned gameId, unsigned playerId);
	void ClearGameInfoMap();

	void StartPetition(unsigned petitionId, unsigned proposingPlayerId, unsigned kickPlayerId, int timeoutSec, int numVotesToKick);
	void UpdatePetition(unsigned petitionId, int numVotesAgainstKicking, int numVotesInFavourOfKicking, int numVotesToKick);
	void EndPetition(unsigned petitionId);

	void UpdateStatData(const ServerStats &stats);

	bool IsSessionEstablished() const;
	void SetSessionEstablished(bool flag);

	bool IsSynchronized() const;

private:

	boost::shared_ptr<boost::asio::io_service> m_ioService;
	boost::shared_ptr<ClientSenderCallback> m_senderCallback;

	NetPacketList m_outPacketList;

	boost::shared_ptr<ClientContext> m_context;
	ClientState *m_curState;
	GuiInterface &m_gui;
	AvatarManager &m_avatarManager;

	boost::shared_ptr<SenderHelper> m_senderHelper;
	boost::shared_ptr<ReceiverHelper> m_receiver;

	boost::shared_ptr<DownloaderThread> m_avatarDownloader;

	GameData m_gameData;
	StartData m_startData;
	PlayerDataList m_playerDataList;

	ServerInfoMap m_serverInfoMap;
	mutable boost::mutex m_serverInfoMapMutex;

	bool m_isServerSelected;
	unsigned m_selectedServerId;
	mutable boost::mutex m_selectServerMutex;

	GameInfoMap m_gameInfoMap;
	mutable boost::mutex m_gameInfoMapMutex;

	boost::shared_ptr<Game> m_game;
	boost::shared_ptr<QtToolsInterface> myQtToolsInterface;

	PlayerInfoMap m_playerInfoMap;
	mutable boost::mutex m_playerInfoMapMutex;
	PlayerIdList m_playerInfoRequestList;
	PlayerIdList m_avatarShouldRequestList;
	PlayerIdList m_avatarHasRequestedList;

	unsigned m_curGameId;
	mutable boost::mutex m_curGameIdMutex;

	unsigned m_curPetitionId;
	mutable boost::mutex m_curPetitionIdMutex;

	AvatarDataMap m_tempAvatarMap;

	unsigned m_curGameNum;
	unsigned m_guiPlayerId;
	bool m_sessionEstablished;

	mutable boost::mutex m_curStatsMutex;
	ServerStats m_curStats;

	boost::asio::deadline_timer m_stateTimer;
	boost::asio::deadline_timer m_avatarTimer;

friend class AbstractClientStateReceiving;
friend class ClientStateInit;
friend class ClientStateStartResolve;
friend class ClientStateResolving;
friend class ClientStateStartServerListDownload;
friend class ClientStateSynchronizingServerList;
friend class ClientStateDownloadingServerList;
friend class ClientStateReadingServerList;
friend class ClientStateWaitChooseServer;
friend class ClientStateStartConnect;
friend class ClientStateConnecting;
friend class ClientStateStartSession;
friend class ClientStateWaitSession;
friend class ClientStateWaitJoin;
friend class ClientStateWaitGame;
friend class ClientStateSynchronizeStart;
friend class ClientStateWaitStart;
friend class ClientStateWaitHand;
friend class ClientStateRunHand;
friend class ClientStateFinal;
friend class ClientSenderCallback;
};

#endif
