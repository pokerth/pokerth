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

#include <net/netpacket.h>
#include <net/netexception.h>
#include <net/socket_msg.h>

#include <core/loghelper.h>

#include <string>

using namespace std;

#define ADD_PADDING(x) ((((x) + 3) >> 2) << 2)

#define NET_TYPE_INIT							0x0001
#define NET_TYPE_INIT_ACK						0x0002
#define NET_TYPE_RETRIEVE_AVATAR				0x0003
#define NET_TYPE_AVATAR_HEADER					0x0004
#define NET_TYPE_AVATAR_FILE					0x0005
#define NET_TYPE_AVATAR_END						0x0006
#define NET_TYPE_UNKNOWN_AVATAR					0x0007
#define NET_TYPE_GAME_LIST_NEW					0x0010
#define NET_TYPE_GAME_LIST_UPDATE				0x0011
#define NET_TYPE_GAME_LIST_PLAYER_JOINED		0x0012
#define NET_TYPE_GAME_LIST_PLAYER_LEFT			0x0013
#define NET_TYPE_GAME_LIST_ADMIN_CHANGED		0x0014
#define NET_TYPE_RETRIEVE_PLAYER_INFO			0x0020
#define NET_TYPE_PLAYER_INFO					0x0021
#define NET_TYPE_UNKNOWN_PLAYER_ID				0x0022
#define NET_TYPE_UNSUBSCRIBE_GAME_LIST			0x0023
#define NET_TYPE_RESUBSCRIBE_GAME_LIST			0x0024
#define NET_TYPE_CREATE_GAME					0x0030
#define NET_TYPE_JOIN_GAME						0x0031
#define NET_TYPE_JOIN_GAME_ACK					0x0032
#define NET_TYPE_JOIN_GAME_FAILED				0x0033
#define NET_TYPE_PLAYER_JOINED					0x0034
#define NET_TYPE_PLAYER_LEFT					0x0035
#define NET_TYPE_GAME_ADMIN_CHANGED				0x0036
#define NET_TYPE_KICK_PLAYER					0x0040
#define NET_TYPE_LEAVE_CURRENT_GAME				0x0041
#define NET_TYPE_START_EVENT					0x0042
#define NET_TYPE_START_EVENT_ACK				0x0043
#define NET_TYPE_GAME_START						0x0050
#define NET_TYPE_HAND_START						0x0051
#define NET_TYPE_PLAYERS_TURN					0x0052
#define NET_TYPE_PLAYERS_ACTION					0x0053
#define NET_TYPE_PLAYERS_ACTION_DONE			0x0054
#define NET_TYPE_PLAYERS_ACTION_REJECTED		0x0055
#define NET_TYPE_DEAL_FLOP_CARDS				0x0060
#define NET_TYPE_DEAL_TURN_CARD					0x0061
#define NET_TYPE_DEAL_RIVER_CARD				0x0062
#define NET_TYPE_ALL_IN_SHOW_CARDS				0x0063
#define NET_TYPE_END_OF_HAND_SHOW_CARDS			0x0064
#define NET_TYPE_END_OF_HAND_HIDE_CARDS			0x0065
#define NET_TYPE_END_OF_GAME					0x0070
#define NET_TYPE_ASK_KICK_PLAYER				0x0071
#define NET_TYPE_ASK_KICK_PLAYER_DENIED			0x0072
#define NET_TYPE_START_KICK_PLAYER_PETITION		0x0073
#define NET_TYPE_VOTE_KICK_PLAYER				0x0074
#define NET_TYPE_VOTE_KICK_PLAYER_ACK			0x0075
#define NET_TYPE_VOTE_KICK_PLAYER_DENIED		0x0076
#define NET_TYPE_KICK_PLAYER_PETITION_UPDATE	0x0077
#define NET_TYPE_END_KICK_PLAYER_PETITION		0x0078

#define NET_TYPE_STATISTICS_CHANGED				0x0080

#define NET_TYPE_REMOVED_FROM_GAME				0x0100
#define NET_TYPE_TIMEOUT_WARNING				0x0101
#define NET_TYPE_RESET_TIMEOUT					0x0102

#define NET_TYPE_SEND_CHAT_TEXT					0x0200
#define NET_TYPE_CHAT_TEXT						0x0201

#define NET_TYPE_ERROR							0x0400

#define NET_GAME_FLAG_PASSWORD_PROTECTED		0x01

#define NET_PLAYER_FLAG_HUMAN					0x01
#define NET_PLAYER_FLAG_HAS_AVATAR				0x02

#define NET_START_FLAG_FILL_WITH_CPU_PLAYERS	0x01

#define NET_PRIVACY_FLAG_SHOW_AVATAR			0x01

// Reasons why join game failed.
#define NET_JOIN_FAILED_GAME_FULL				0x0001
#define NET_JOIN_FAILED_GAME_ALREADY_RUNNING	0x0002
#define NET_JOIN_FAILED_INVALID_PASSWORD		0x0003
#define NET_JOIN_FAILED_OTHER					0xFFFF

// Reasons for being removed from a game.
#define NET_REMOVED_ON_REQUEST					0x0000
#define NET_REMOVED_GAME_FULL					0x0001
#define NET_REMOVED_GAME_ALREADY_RUNNING		0x0002
#define NET_REMOVED_KICKED						0x0003
#define NET_REMOVED_TIMEOUT						0x0004
#define NET_REMOVED_START_FAILED				0x0005
#define NET_REMOVED_OTHER_REASON				0xFFFF

// Reasons why player left a game.
#define NET_LEFT_ON_REQUEST						0x0000
#define NET_LEFT_KICKED							0x0001
#define NET_LEFT_ERROR							0x0002
#define NET_LEFT_OTHER_REASON					0xFFFF

// Reasons why ask kick was denied.
#define NET_ASK_KICK_DENIED_INVALID_STATE		0x0000
#define NET_ASK_KICK_DENIED_TOO_FEW_PLAYERS		0x0001
#define NET_ASK_KICK_DENIED_TEMPORARY			0x0002
#define NET_ASK_KICK_DENIED_OTHER_IN_PROGRESS	0x0003
#define NET_ASK_KICK_DENIED_OTHER_REASON		0xFFFF

// Vote types
#define NET_VOTE_KICK_AGAINST					0x0000
#define NET_VOTE_KICK_IN_FAVOUR					0x0001

// Reasons why vote to kick was denied.
#define NET_VOTE_KICK_DENIED_INVALID_PETITION	0x0000
#define NET_VOTE_KICK_DENIED_ALREADY_VOTED		0x0001
#define NET_VOTE_KICK_DENIED_IMPOSSIBLE			0x0002
#define NET_VOTE_KICK_DENIED_OTHER_REASON		0xFFFF

// Reasons why a petition to kick a player was ended.
#define NET_PETITION_END_ENOUGH_VOTES			0x0000
#define NET_PETITION_END_NOT_ENOUGH_PLAYERS		0x0001
#define NET_PETITION_END_PLAYER_LEFT			0x0002
#define NET_PETITION_END_TIMEOUT				0x0003

// Reasons for timeout warning
#define NET_TIMEOUT_NO_DATA_RECEIVED			0x0000
#define NET_TIMEOUT_INACTIVE_GAME				0x0001
#define NET_TIMEOUT_OTHER_REASON				0xFFFF

// Internal error codes.
#define NET_ERR_RESERVED						0x0000
#define NET_ERR_INIT_VERSION_NOT_SUPPORTED		0x0001
#define NET_ERR_INIT_SERVER_FULL				0x0002
#define NET_ERR_INIT_INVALID_PASSWORD			0x0004
#define NET_ERR_INIT_PLAYER_NAME_IN_USE			0x0005
#define NET_ERR_INIT_INVALID_PLAYER_NAME		0x0006
#define NET_ERR_INIT_SERVER_MAINTENANCE			0x0007
#define NET_ERR_AVATAR_TOO_LARGE				0x0010
#define NET_ERR_AVATAR_WRONG_SIZE				0x0011
#define NET_ERR_AVATAR_UPLOAD_BLOCKED			0x0012
#define NET_ERR_JOIN_GAME_UNKNOWN_GAME			0x0020
#define NET_ERR_GENERAL_INVALID_PACKET			0xFF01
#define NET_ERR_GENERAL_INVALID_STATE			0xFF02
#define NET_ERR_GENERAL_PLAYER_KICKED			0xFF03
#define NET_ERR_GENERAL_PLAYER_BANNED			0xFF04
#define NET_ERR_GENERAL_SESSION_TIMED_OUT		0xFF05
#define NET_ERR_OTHER							0xFFFF

// Statistics types
#define NET_STAT_CUR_PLAYERS_ON_SERVER			0x0001
#define NET_STAT_TOTAL_PLAYERS_EVER_ON_SERVER	0x0002
#define NET_STAT_TOTAL_GAMES_EVER_ON_SERVER		0x0003

#ifdef _MSC_VER
	#pragma pack(push, 1)
	#define GCC_PACKED
#else
	#define GCC_PACKED __attribute__ ((__packed__))
#endif


struct GCC_PACKED NetPacketHeader
{
	u_int16_t			type;
	u_int16_t			length;
};

struct GCC_PACKED NetPacketInitData
{
	NetPacketHeader		head;
	u_int16_t			requestedVersionMajor;
	u_int16_t			requestedVersionMinor;
	u_int16_t			passwordLength;
	u_int16_t			playerNameLength;
	u_int16_t			privacyFlags;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketInitAckData
{
	NetPacketHeader		head;
	u_int16_t			latestGameVersion;
	u_int16_t			latestBetaRevision;
	u_int32_t			sessionId;
	u_int32_t			playerId;
};

struct GCC_PACKED NetPacketRetrieveAvatarData
{
	NetPacketHeader		head;
	u_int32_t			requestId;
	unsigned char		avatarMD5[MD5_DATA_SIZE];
};

struct GCC_PACKED NetPacketAvatarHeaderData
{
	NetPacketHeader		head;
	u_int32_t			requestId;
	u_int32_t			avatarFileSize;
	u_int16_t			avatarFileType;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketAvatarFileData
{
	NetPacketHeader		head;
	u_int32_t			requestId;
	u_int16_t			blockSize;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketAvatarEndData
{
	NetPacketHeader		head;
	u_int32_t			requestId;
};

struct GCC_PACKED NetPacketUnknownAvatarData
{
	NetPacketHeader		head;
	u_int32_t			requestId;
};

struct GCC_PACKED GameInfoData
{
	u_int16_t			maxNumberOfPlayers;
	u_int16_t			raiseIntervalMode;
	u_int16_t			raiseSmallBlindInterval;
	u_int16_t			raiseMode;
	u_int16_t			endRaiseMode;
	u_int16_t			numberOfManualBlinds;
	u_int16_t			proposedGuiSpeed;
	u_int16_t			playerActionTimeout;
	u_int32_t			firstSmallBlind;
	u_int32_t			endRaiseSmallBlindValue;
	u_int32_t			startMoney;
};

struct GCC_PACKED NetPacketGameListNewData
{
	NetPacketHeader		head;
	u_int32_t			gameId;
	u_int32_t			adminPlayerId;
	u_int16_t			gameMode;
	u_int16_t			gameNameLength;
	u_int16_t			curNumberOfPlayers;
	u_int16_t			gameFlags;
	GameInfoData		gameData;
};

struct GCC_PACKED NetPacketGameListUpdateData
{
	NetPacketHeader		head;
	u_int32_t			gameId;
	u_int16_t			gameMode;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketGameListPlayerJoinedData
{
	NetPacketHeader		head;
	u_int32_t			gameId;
	u_int32_t			playerId;
};

struct GCC_PACKED NetPacketGameListPlayerLeftData
{
	NetPacketHeader		head;
	u_int32_t			gameId;
	u_int32_t			playerId;
};

struct GCC_PACKED NetPacketGameListAdminChangedData
{
	NetPacketHeader		head;
	u_int32_t			gameId;
	u_int32_t			newAdminPlayerId;
};

struct GCC_PACKED NetPacketRetrievePlayerInfoData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
};

struct GCC_PACKED NetPacketPlayerInfoData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
	u_int16_t			playerFlags;
	u_int16_t			playerNameLength;
	u_int32_t			reserved;
};

struct GCC_PACKED NetPacketUnknownPlayerIdData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
};

struct GCC_PACKED NetPacketUnsubscribeGameListData
{
	NetPacketHeader		head;
	u_int32_t			reserved;
};

struct GCC_PACKED NetPacketResubscribeGameListData
{
	NetPacketHeader		head;
	u_int32_t			reserved;
};

struct GCC_PACKED NetPacketCreateGameData
{
	NetPacketHeader		head;
	u_int16_t			passwordLength;
	u_int16_t			gameNameLength;
	GameInfoData		gameData;
};

struct GCC_PACKED NetPacketJoinGameData
{
	NetPacketHeader		head;
	u_int32_t			gameId;
	u_int16_t			passwordLength;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketJoinGameAckData
{
	NetPacketHeader		head;
	u_int32_t			gameId;
	u_int16_t			playerRights;
	u_int16_t			reserved;
	GameInfoData		gameData;
};

struct GCC_PACKED NetPacketJoinGameFailedData
{
	NetPacketHeader		head;
	u_int16_t			failureReason;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketPlayerJoinedData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
	u_int16_t			playerRights;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketPlayerLeftData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
	u_int16_t			leaveReason;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketGameAdminChangedData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
};

struct GCC_PACKED NetPacketKickPlayerData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
};

struct GCC_PACKED NetPacketLeaveCurrentGameData
{
	NetPacketHeader		head;
	u_int32_t			reserved;
};

struct GCC_PACKED NetPacketStartEventData
{
	NetPacketHeader		head;
	u_int16_t			startFlags;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketStartEventAckData
{
	NetPacketHeader		head;
	u_int32_t			reserved;
};

struct GCC_PACKED NetPacketGameStartData
{
	NetPacketHeader		head;
	u_int32_t			startDealerPlayerId;
	u_int16_t			numberOfPlayers;
	u_int16_t			reserved;
};

struct GCC_PACKED PlayerSlotData
{
	u_int32_t			playerId;
};

struct GCC_PACKED NetPacketHandStartData
{
	NetPacketHeader		head;
	u_int16_t			yourCard1;
	u_int16_t			yourCard2;
	u_int32_t			smallBlind;
};

struct GCC_PACKED NetPacketPlayersTurnData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
	u_int16_t			gameState;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketPlayersActionData
{
	NetPacketHeader		head;
	u_int16_t			gameState;
	u_int16_t			playerAction;
	u_int32_t			playerBet;
};

struct GCC_PACKED NetPacketPlayersActionDoneData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
	u_int16_t			gameState;
	u_int16_t			playerAction;
	u_int32_t			totalPlayerBet;
	u_int32_t			playerMoney;
	u_int32_t			highestSet;
	u_int32_t			minimumRaise;
};

struct GCC_PACKED NetPacketPlayersActionRejectedData
{
	NetPacketHeader		head;
	u_int16_t			gameState;
	u_int16_t			playerAction;
	u_int32_t			playerBet;
	u_int16_t			rejectionReason;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketDealFlopCardsData
{
	NetPacketHeader		head;
	u_int16_t			flopCard1;
	u_int16_t			flopCard2;
	u_int16_t			flopCard3;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketDealTurnCardData
{
	NetPacketHeader		head;
	u_int16_t			turnCard;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketDealRiverCardData
{
	NetPacketHeader		head;
	u_int16_t			riverCard;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketAllInShowCardsData
{
	NetPacketHeader		head;
	u_int16_t			numberOfPlayerCards;
	u_int16_t			reserved;
};

struct GCC_PACKED PlayerCardsData
{
	u_int32_t			playerId;
	u_int16_t			card1;
	u_int16_t			card2;
};

struct GCC_PACKED NetPacketEndOfHandShowCardsData
{
	NetPacketHeader		head;
	u_int16_t			numberOfPlayerResults;
	u_int16_t			reserved;
};

struct GCC_PACKED PlayerResultData
{
	u_int32_t			playerId;
	u_int16_t			card1;
	u_int16_t			card2;
	u_int16_t			bestHandPos1;
	u_int16_t			bestHandPos2;
	u_int16_t			bestHandPos3;
	u_int16_t			bestHandPos4;
	u_int16_t			bestHandPos5;
	u_int16_t			reserved;
	u_int32_t			valueOfCards;
	u_int32_t			moneyWon;
	u_int32_t			playerMoney;
};

struct GCC_PACKED NetPacketEndOfHandHideCardsData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
	u_int32_t			moneyWon;
	u_int32_t			playerMoney;
};

struct GCC_PACKED NetPacketEndOfGameData
{
	NetPacketHeader		head;
	u_int32_t			winnerPlayerId;
};

struct GCC_PACKED NetPacketAskKickPlayerData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
};

struct GCC_PACKED NetPacketAskKickPlayerDeniedData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
	u_int16_t			denyReason;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketStartKickPlayerPetitionData
{
	NetPacketHeader		head;
	u_int32_t			petitionId;
	u_int32_t			proposingPlayerId;
	u_int32_t			kickPlayerId;
	u_int16_t			kickTimeout;
	u_int16_t			numVotesNeededToKick;
};

struct GCC_PACKED NetPacketVoteKickPlayerData
{
	NetPacketHeader		head;
	u_int32_t			petitionId;
	u_int16_t			vote;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketVoteKickPlayerAckData
{
	NetPacketHeader		head;
	u_int32_t			petitionId;
};

struct GCC_PACKED NetPacketVoteKickPlayerDeniedData
{
	NetPacketHeader		head;
	u_int32_t			petitionId;
	u_int16_t			denyReason;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketKickPlayerPetitionUpdateData
{
	NetPacketHeader		head;
	u_int32_t			petitionId;
	u_int16_t			numVotesAgainstKicking;
	u_int16_t			numVotesInFavourOfKicking;
	u_int16_t			numVotesNeededToKick;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketEndKickPlayerPetitionData
{
	NetPacketHeader		head;
	u_int32_t			petitionId;
	u_int16_t			numVotesAgainstKicking;
	u_int16_t			numVotesInFavourOfKicking;
	u_int16_t			voteResult;
	u_int16_t			endReason;
};

struct GCC_PACKED StatisticsData
{
	u_int32_t			statisticsType;
	u_int32_t			statisticsValue;
};

struct GCC_PACKED NetPacketStatisticsChangedData
{
	NetPacketHeader		head;
	u_int16_t			numberOfStatistics;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketRemovedFromGameData
{
	NetPacketHeader		head;
	u_int16_t			removeReason;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketTimeoutWarningData
{
	NetPacketHeader		head;
	u_int16_t			timeoutReason;
	u_int16_t			remainingSeconds;
	u_int32_t			reserved;
};

struct GCC_PACKED NetPacketResetTimeoutData
{
	NetPacketHeader		head;
	u_int32_t			reserved;
};

struct GCC_PACKED NetPacketSendChatTextData
{
	NetPacketHeader		head;
	u_int16_t			textLength;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketChatTextData
{
	NetPacketHeader		head;
	u_int32_t			playerId;
	u_int16_t			textLength;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketErrorData
{
	NetPacketHeader		head;
	u_int16_t			errorReason;
	u_int16_t			reserved;
};

#ifdef _MSC_VER
	#pragma pack(pop)
#endif

// Helper functions

void SetGameInfoData(const GameData &inData, GameInfoData *outData)
{
	assert(outData);

	u_int16_t numManualBlinds = (u_int16_t)inData.manualBlindsList.size();

	outData->maxNumberOfPlayers			= htons(inData.maxNumberOfPlayers);
	outData->raiseIntervalMode			= htons(inData.raiseIntervalMode);
	outData->raiseSmallBlindInterval	= inData.raiseIntervalMode == RAISE_ON_HANDNUMBER
		? htons(inData.raiseSmallBlindEveryHandsValue) : htons(inData.raiseSmallBlindEveryMinutesValue);
	outData->raiseMode					= htons(inData.raiseMode);
	outData->endRaiseMode				= htons(inData.afterManualBlindsMode);
	outData->numberOfManualBlinds		= htons(numManualBlinds);
	outData->proposedGuiSpeed			= htons(inData.guiSpeed);
	outData->playerActionTimeout		= htons(inData.playerActionTimeoutSec);
	outData->endRaiseSmallBlindValue	= htonl(inData.afterMBAlwaysRaiseValue);
	outData->firstSmallBlind			= htonl(inData.firstSmallBlind);
	outData->startMoney					= htonl(inData.startMoney);

	if (numManualBlinds)
	{
		u_int32_t *manualBlindsPtr = (u_int32_t *)((char *)outData + sizeof(GameInfoData));
		list<int>::const_iterator i = inData.manualBlindsList.begin();
		list<int>::const_iterator end = inData.manualBlindsList.end();
		while (i != end)
		{
			*manualBlindsPtr = htonl(*i);
			++manualBlindsPtr;
			++i;
		}
	}
}

void GetGameInfoData(const GameInfoData *inData, GameData &outData)
{
	assert(inData);

	u_int16_t numManualBlinds = ntohs(inData->numberOfManualBlinds);

	outData.maxNumberOfPlayers			= ntohs(inData->maxNumberOfPlayers);
	outData.raiseIntervalMode			= static_cast<RaiseIntervalMode>(ntohs(inData->raiseIntervalMode));
	outData.raiseSmallBlindEveryHandsValue = outData.raiseSmallBlindEveryMinutesValue = 0;
	if (outData.raiseIntervalMode == RAISE_ON_HANDNUMBER)
		outData.raiseSmallBlindEveryHandsValue = ntohs(inData->raiseSmallBlindInterval);
	else
		outData.raiseSmallBlindEveryMinutesValue = ntohs(inData->raiseSmallBlindInterval);
	outData.raiseMode					= static_cast<RaiseMode>(ntohs(inData->raiseMode));
	outData.afterManualBlindsMode		= static_cast<AfterManualBlindsMode>(ntohs(inData->endRaiseMode));
	outData.guiSpeed					= ntohs(inData->proposedGuiSpeed);
	outData.playerActionTimeoutSec		= ntohs(inData->playerActionTimeout);
	outData.firstSmallBlind				= ntohl(inData->firstSmallBlind);
	outData.afterMBAlwaysRaiseValue		= ntohl(inData->endRaiseSmallBlindValue);
	outData.startMoney					= ntohl(inData->startMoney);

	if (numManualBlinds)
	{
		const u_int32_t *manualBlindsPtr = (const u_int32_t *)((const char *)inData + sizeof(GameInfoData));
		for (u_int16_t i = 0; i < numManualBlinds; i++)
		{
			outData.manualBlindsList.push_back(ntohl(*manualBlindsPtr));
			++manualBlindsPtr;
		}
	}
}

void
CheckGameInfoData(const GameInfoData *tmpData)
{
	// Semantic checks
	int maxNumPlayers = ntohs(tmpData->maxNumberOfPlayers);
	int proposedGuiSpeed = ntohs(tmpData->proposedGuiSpeed);
	if (maxNumPlayers < MIN_NUMBER_OF_PLAYERS
		|| maxNumPlayers > MAX_NUMBER_OF_PLAYERS
		|| !ntohs(tmpData->raiseIntervalMode)
		|| !ntohs(tmpData->raiseSmallBlindInterval)
		|| !ntohs(tmpData->raiseMode)
		|| !ntohs(tmpData->endRaiseMode)
		|| proposedGuiSpeed < MIN_GUI_SPEED
		|| proposedGuiSpeed > MAX_GUI_SPEED
		|| !ntohl(tmpData->firstSmallBlind)
		|| !ntohl(tmpData->startMoney))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

boost::shared_ptr<NetPacket>
NetPacket::Create(char *data, unsigned &dataSize)
{
	boost::shared_ptr<NetPacket> tmpPacket;

	// Check minimum requirements.
	if (!data || dataSize < sizeof(NetPacketHeader))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketHeader *tmpHeader = (NetPacketHeader *)data;
	u_int16_t tmpLen = ntohs(tmpHeader->length);

	// Check size restrictions.
	if (tmpLen < sizeof(NetPacketHeader)
		|| tmpLen > MAX_PACKET_SIZE)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	else if (dataSize >= tmpLen)
	{
		// OK - we have a complete packet.
		// Construct a corresponding object.
		try
		{
			switch(ntohs(tmpHeader->type))
			{
				case NET_TYPE_INIT:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketInit);
					break;
				case NET_TYPE_INIT_ACK:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketInitAck);
					break;
				case NET_TYPE_RETRIEVE_AVATAR:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketRetrieveAvatar);
					break;
				case NET_TYPE_AVATAR_HEADER:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketAvatarHeader);
					break;
				case NET_TYPE_AVATAR_FILE:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketAvatarFile);
					break;
				case NET_TYPE_AVATAR_END:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketAvatarEnd);
					break;
				case NET_TYPE_UNKNOWN_AVATAR:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketUnknownAvatar);
					break;
				case NET_TYPE_GAME_LIST_NEW:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketGameListNew);
					break;
				case NET_TYPE_GAME_LIST_UPDATE:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketGameListUpdate);
					break;
				case NET_TYPE_GAME_LIST_PLAYER_JOINED:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketGameListPlayerJoined);
					break;
				case NET_TYPE_GAME_LIST_PLAYER_LEFT:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketGameListPlayerLeft);
					break;
				case NET_TYPE_GAME_LIST_ADMIN_CHANGED:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketGameListAdminChanged);
					break;
				case NET_TYPE_RETRIEVE_PLAYER_INFO:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketRetrievePlayerInfo);
					break;
				case NET_TYPE_PLAYER_INFO:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayerInfo);
					break;
				case NET_TYPE_UNKNOWN_PLAYER_ID:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketUnknownPlayerId);
					break;
				case NET_TYPE_UNSUBSCRIBE_GAME_LIST:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketUnsubscribeGameList);
					break;
				case NET_TYPE_RESUBSCRIBE_GAME_LIST:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketResubscribeGameList);
					break;
				case NET_TYPE_CREATE_GAME:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketCreateGame);
					break;
				case NET_TYPE_JOIN_GAME:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketJoinGame);
					break;
				case NET_TYPE_JOIN_GAME_ACK:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketJoinGameAck);
					break;
				case NET_TYPE_JOIN_GAME_FAILED:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketJoinGameFailed);
					break;
				case NET_TYPE_PLAYER_JOINED:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayerJoined);
					break;
				case NET_TYPE_PLAYER_LEFT:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayerLeft);
					break;
				case NET_TYPE_GAME_ADMIN_CHANGED:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketGameAdminChanged);
					break;
				case NET_TYPE_KICK_PLAYER:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketKickPlayer);
					break;
				case NET_TYPE_LEAVE_CURRENT_GAME:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketLeaveCurrentGame);
					break;
				case NET_TYPE_START_EVENT:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketStartEvent);
					break;
				case NET_TYPE_START_EVENT_ACK:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketStartEventAck);
					break;
				case NET_TYPE_GAME_START:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketGameStart);
					break;
				case NET_TYPE_HAND_START:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketHandStart);
					break;
				case NET_TYPE_PLAYERS_TURN:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayersTurn);
					break;
				case NET_TYPE_PLAYERS_ACTION:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayersAction);
					break;
				case NET_TYPE_PLAYERS_ACTION_DONE:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayersActionDone);
					break;
				case NET_TYPE_PLAYERS_ACTION_REJECTED:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayersActionRejected);
					break;
				case NET_TYPE_DEAL_FLOP_CARDS:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketDealFlopCards);
					break;
				case NET_TYPE_DEAL_TURN_CARD:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketDealTurnCard);
					break;
				case NET_TYPE_DEAL_RIVER_CARD:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketDealRiverCard);
					break;
				case NET_TYPE_ALL_IN_SHOW_CARDS:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketAllInShowCards);
					break;
				case NET_TYPE_END_OF_HAND_SHOW_CARDS:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketEndOfHandShowCards);
					break;
				case NET_TYPE_END_OF_HAND_HIDE_CARDS:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketEndOfHandHideCards);
					break;
				case NET_TYPE_END_OF_GAME:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketEndOfGame);
					break;
				case NET_TYPE_ASK_KICK_PLAYER:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketAskKickPlayer);
					break;
				case NET_TYPE_ASK_KICK_PLAYER_DENIED:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketAskKickPlayerDenied);
					break;
				case NET_TYPE_START_KICK_PLAYER_PETITION:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketStartKickPlayerPetition);
					break;
				case NET_TYPE_VOTE_KICK_PLAYER:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketVoteKickPlayer);
					break;
				case NET_TYPE_VOTE_KICK_PLAYER_ACK:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketVoteKickPlayerAck);
					break;
				case NET_TYPE_VOTE_KICK_PLAYER_DENIED:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketVoteKickPlayerDenied);
					break;
				case NET_TYPE_KICK_PLAYER_PETITION_UPDATE:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketKickPlayerPetitionUpdate);
					break;
				case NET_TYPE_END_KICK_PLAYER_PETITION:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketEndKickPlayerPetition);
					break;
				case NET_TYPE_STATISTICS_CHANGED:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketStatisticsChanged);
					break;
				case NET_TYPE_REMOVED_FROM_GAME:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketRemovedFromGame);
					break;
				case NET_TYPE_TIMEOUT_WARNING:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketTimeoutWarning);
					break;
				case NET_TYPE_RESET_TIMEOUT:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketResetTimeout);
					break;
				case NET_TYPE_SEND_CHAT_TEXT:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketSendChatText);
					break;
				case NET_TYPE_CHAT_TEXT:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketChatText);
					break;
				case NET_TYPE_ERROR:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketError);
					break;
				default:
					throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_TYPE, 0);
			}
			if (tmpPacket.get())
				tmpPacket->SetRawData(tmpHeader);
		} catch (const NetException &)
		{
			tmpPacket.reset();
		}

		// Consume the bytes.
		if (tmpLen < dataSize)
		{
			dataSize -= tmpLen;
			memmove(data, data + tmpLen, dataSize);
		}
		else
			dataSize = 0;
	}
	return tmpPacket;
}

NetPacket::NetPacket(u_int16_t type, u_int16_t initialSize, u_int16_t maxSize)
: m_data(NULL), m_initialSize(initialSize), m_maxSize(maxSize)
{
	assert(initialSize >= sizeof(NetPacketHeader));
	m_data = static_cast<NetPacketHeader *>(malloc(initialSize));
	assert(m_data);
	memset(m_data, 0, initialSize);
	m_data->type = htons(type);
	m_data->length = htons(initialSize);
}

NetPacket::~NetPacket()
{
	if (m_data)
		free(m_data);
}

const NetPacketHeader *
NetPacket::GetRawData() const
{
	assert(m_data);
	return m_data;
}

NetPacketHeader *
NetPacket::GetRawData()
{
	assert(m_data);
	return m_data;
}

void
NetPacket::SetRawData(const NetPacketHeader *p)
{
	if (!p)
		return;
	assert(m_data);

	u_int16_t tmpLen = ntohs(p->length);
	if (ntohs(p->type) != GetType())
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check whether the data is valid.
	Check(p);
	// Resize makes sure m_data is large enough.
	Resize(tmpLen);
	memcpy(m_data, p, tmpLen);
}

u_int16_t
NetPacket::GetType() const
{
	return ntohs(GetRawData()->type);
}

u_int16_t
NetPacket::GetLen() const
{
	return ntohs(GetRawData()->length);
}

const NetPacketInit *
NetPacket::ToNetPacketInit() const
{
	return NULL;
}

const NetPacketInitAck *
NetPacket::ToNetPacketInitAck() const
{
	return NULL;
}

const NetPacketRetrieveAvatar *
NetPacket::ToNetPacketRetrieveAvatar() const
{
	return NULL;
}

const NetPacketAvatarHeader *
NetPacket::ToNetPacketAvatarHeader() const
{
	return NULL;
}

const NetPacketAvatarFile *
NetPacket::ToNetPacketAvatarFile() const
{
	return NULL;
}

const NetPacketAvatarEnd *
NetPacket::ToNetPacketAvatarEnd() const
{
	return NULL;
}

const NetPacketUnknownAvatar *
NetPacket::ToNetPacketUnknownAvatar() const
{
	return NULL;
}

const NetPacketGameListNew *
NetPacket::ToNetPacketGameListNew() const
{
	return NULL;
}

const NetPacketGameListUpdate *
NetPacket::ToNetPacketGameListUpdate() const
{
	return NULL;
}

const NetPacketGameListPlayerJoined *
NetPacket::ToNetPacketGameListPlayerJoined() const
{
	return NULL;
}

const NetPacketGameListPlayerLeft *
NetPacket::ToNetPacketGameListPlayerLeft() const
{
	return NULL;
}

const NetPacketGameListAdminChanged *
NetPacket::ToNetPacketGameListAdminChanged() const
{
	return NULL;
}

const NetPacketRetrievePlayerInfo *
NetPacket::ToNetPacketRetrievePlayerInfo() const
{
	return NULL;
}

const NetPacketPlayerInfo *
NetPacket::ToNetPacketPlayerInfo() const
{
	return NULL;
}

const NetPacketUnknownPlayerId *
NetPacket::ToNetPacketUnknownPlayerId() const
{
	return NULL;
}

const NetPacketUnsubscribeGameList *
NetPacket::ToNetPacketUnsubscribeGameList() const
{
	return NULL;
}

const NetPacketResubscribeGameList *
NetPacket::ToNetPacketResubscribeGameList() const
{
	return NULL;
}

const NetPacketCreateGame *
NetPacket::ToNetPacketCreateGame() const
{
	return NULL;
}

const NetPacketJoinGame *
NetPacket::ToNetPacketJoinGame() const
{
	return NULL;
}

const NetPacketJoinGameAck *
NetPacket::ToNetPacketJoinGameAck() const
{
	return NULL;
}

const NetPacketJoinGameFailed *
NetPacket::ToNetPacketJoinGameFailed() const
{
	return NULL;
}

const NetPacketPlayerJoined *
NetPacket::ToNetPacketPlayerJoined() const
{
	return NULL;
}

const NetPacketPlayerLeft *
NetPacket::ToNetPacketPlayerLeft() const
{
	return NULL;
}

const NetPacketGameAdminChanged *
NetPacket::ToNetPacketGameAdminChanged() const
{
	return NULL;
}

const NetPacketKickPlayer *
NetPacket::ToNetPacketKickPlayer() const
{
	return NULL;
}

const NetPacketLeaveCurrentGame *
NetPacket::ToNetPacketLeaveCurrentGame() const
{
	return NULL;
}

const NetPacketStartEvent *
NetPacket::ToNetPacketStartEvent() const
{
	return NULL;
}

const NetPacketStartEventAck *
NetPacket::ToNetPacketStartEventAck() const
{
	return NULL;
}

const NetPacketGameStart *
NetPacket::ToNetPacketGameStart() const
{
	return NULL;
}

const NetPacketHandStart *
NetPacket::ToNetPacketHandStart() const
{
	return NULL;
}

const NetPacketPlayersTurn *
NetPacket::ToNetPacketPlayersTurn() const
{
	return NULL;
}

const NetPacketPlayersAction *
NetPacket::ToNetPacketPlayersAction() const
{
	return NULL;
}

const NetPacketPlayersActionDone *
NetPacket::ToNetPacketPlayersActionDone() const
{
	return NULL;
}

const NetPacketPlayersActionRejected *
NetPacket::ToNetPacketPlayersActionRejected() const
{
	return NULL;
}

const NetPacketDealFlopCards *
NetPacket::ToNetPacketDealFlopCards() const
{
	return NULL;
}

const NetPacketDealTurnCard *
NetPacket::ToNetPacketDealTurnCard() const
{
	return NULL;
}

const NetPacketDealRiverCard *
NetPacket::ToNetPacketDealRiverCard() const
{
	return NULL;
}

const NetPacketAllInShowCards *
NetPacket::ToNetPacketAllInShowCards() const
{
	return NULL;
}

const NetPacketEndOfHandShowCards *
NetPacket::ToNetPacketEndOfHandShowCards() const
{
	return NULL;
}

const NetPacketEndOfHandHideCards *
NetPacket::ToNetPacketEndOfHandHideCards() const
{
	return NULL;
}

const NetPacketEndOfGame *
NetPacket::ToNetPacketEndOfGame() const
{
	return NULL;
}

const NetPacketAskKickPlayer *
NetPacket::ToNetPacketAskKickPlayer() const
{
	return NULL;
}

const NetPacketAskKickPlayerDenied *
NetPacket::ToNetPacketAskKickPlayerDenied() const
{
	return NULL;
}

const NetPacketStartKickPlayerPetition *
NetPacket::ToNetPacketStartKickPlayerPetition() const
{
	return NULL;
}

const NetPacketVoteKickPlayer *
NetPacket::ToNetPacketVoteKickPlayer() const
{
	return NULL;
}

const NetPacketVoteKickPlayerAck *
NetPacket::ToNetPacketVoteKickPlayerAck() const
{
	return NULL;
}

const NetPacketVoteKickPlayerDenied *
NetPacket::ToNetPacketVoteKickPlayerDenied() const
{
	return NULL;
}

const NetPacketKickPlayerPetitionUpdate *
NetPacket::ToNetPacketKickPlayerPetitionUpdate() const
{
	return NULL;
}

const NetPacketEndKickPlayerPetition *
NetPacket::ToNetPacketEndKickPlayerPetition() const
{
	return NULL;
}

const NetPacketStatisticsChanged *
NetPacket::ToNetPacketStatisticsChanged() const
{
	return NULL;
}

const NetPacketRemovedFromGame *
NetPacket::ToNetPacketRemovedFromGame() const
{
	return NULL;
}

const NetPacketTimeoutWarning *
NetPacket::ToNetPacketTimeoutWarning() const
{
	return NULL;
}

const NetPacketResetTimeout *
NetPacket::ToNetPacketResetTimeout() const
{
	return NULL;
}

const NetPacketSendChatText *
NetPacket::ToNetPacketSendChatText() const
{
	return NULL;
}

const NetPacketChatText *
NetPacket::ToNetPacketChatText() const
{
	return NULL;
}

const NetPacketError *
NetPacket::ToNetPacketError() const
{
	return NULL;
}

void
NetPacket::Resize(u_int16_t newLen)
{
	assert(m_data);
	u_int16_t oldLen = GetLen();
	if (newLen != oldLen)
	{
		if (newLen < sizeof(NetPacketHeader))
			throw NetException(__FILE__, __LINE__, ERR_SOCK_INTERNAL, 0);
		else
		{
			NetPacketHeader *newData = (NetPacketHeader *)malloc(newLen);
			if (!newData)
				throw NetException(__FILE__, __LINE__, ERR_SOCK_INTERNAL, 0);
			else
			{
				// Copy as much data as possible.
				memcpy(newData, m_data, newLen > oldLen ? oldLen : newLen);
				// Initialize new data to 0.
				if (newLen > oldLen)
					memset(((unsigned char *)newData) + oldLen, 0, newLen - oldLen);
				// Set new len.
				newData->length = htons(newLen);
				// Switch over to new data.
				free(m_data);
				m_data = newData;
			}
		}
	}
}

void
NetPacket::Check(const NetPacketHeader *data) const
{
	// Check the input data.
	assert(data);

	// Check minimum and maximum size.
	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < m_initialSize || dataLen > m_maxSize)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}

	InternalCheck(data);
}

//-----------------------------------------------------------------------------

NetPacketInit::NetPacketInit()
: NetPacket(NET_TYPE_INIT, sizeof(NetPacketInitData), MAX_PACKET_SIZE)
{
	NetPacketInitData *tmpData = (NetPacketInitData *)GetRawData();
	tmpData->requestedVersionMajor = htons(NET_VERSION_MAJOR);
	tmpData->requestedVersionMinor = htons(NET_VERSION_MINOR);
}

NetPacketInit::~NetPacketInit()
{
}

boost::shared_ptr<NetPacket>
NetPacketInit::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketInit);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketInit::SetData(const NetPacketInit::Data &inData)
{
	u_int16_t playerNameLen = (u_int16_t)inData.playerName.length();
	u_int16_t passwordLen = (u_int16_t)inData.password.length();

	// Some basic checks, so we don't use up too much memory.
	// The constructed packet will also be checked.
	if (!playerNameLen || playerNameLen > MAX_NAME_SIZE)
		throw NetException(__FILE__, __LINE__, ERR_NET_INVALID_PLAYER_NAME, 0);
	if (passwordLen > MAX_PASSWORD_SIZE)
		throw NetException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD_STR, 0);

	int avatarSize = inData.showAvatar ? MD5_DATA_SIZE : 0;
	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketInitData)
		+ avatarSize
		+ ADD_PADDING(playerNameLen)
		+ ADD_PADDING(passwordLen)));

	NetPacketInitData *tmpData = (NetPacketInitData *)GetRawData();

	// Set the data.
	tmpData->passwordLength = htons(passwordLen);
	tmpData->playerNameLength = htons(playerNameLen);

	if (inData.showAvatar)
	{
		// Store MD5 sum of avatar.
		tmpData->privacyFlags = htons(NET_PRIVACY_FLAG_SHOW_AVATAR);
		char *avatarPtr = (char *)tmpData + sizeof(NetPacketInitData);
		memcpy(avatarPtr, inData.avatar.data, MD5_DATA_SIZE);
	}

	char *passwordPtr = (char *)tmpData + sizeof(NetPacketInitData) + avatarSize;
	memcpy(passwordPtr, inData.password.c_str(), passwordLen);
	memcpy(passwordPtr + ADD_PADDING(passwordLen), inData.playerName.c_str(), playerNameLen);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketInit::GetData(NetPacketInit::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketInitData *tmpData = (NetPacketInitData *)GetRawData();

	outData.versionMajor = ntohs(tmpData->requestedVersionMajor);
	outData.versionMinor = ntohs(tmpData->requestedVersionMinor);

	outData.showAvatar = ntohs(tmpData->privacyFlags) & NET_PRIVACY_FLAG_SHOW_AVATAR;

	if (outData.showAvatar)
	{
		char *avatarPtr = (char *)tmpData + sizeof(NetPacketInitData);
		memcpy(outData.avatar.data, avatarPtr, MD5_DATA_SIZE);
	}

	int avatarSize = outData.showAvatar ? MD5_DATA_SIZE : 0;
	u_int16_t passwordLen = ntohs(tmpData->passwordLength);
	char *passwordPtr = (char *)tmpData + sizeof(NetPacketInitData) + avatarSize;
	outData.password = string(passwordPtr, passwordLen);
	outData.playerName = string(passwordPtr + ADD_PADDING(passwordLen), ntohs(tmpData->playerNameLength));
}

const NetPacketInit *
NetPacketInit::ToNetPacketInit() const
{
	return this;
}

void
NetPacketInit::InternalCheck(const NetPacketHeader* data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketInitData *tmpData = (NetPacketInitData *)data;
	int passwordLength = ntohs(tmpData->passwordLength);
	int playerNameLength = ntohs(tmpData->playerNameLength);
	int avatarSize = ntohs(tmpData->privacyFlags) & NET_PRIVACY_FLAG_SHOW_AVATAR ? MD5_DATA_SIZE : 0;
	// Generous checking of dynamic packet size -
	// larger packets are allowed.
	// This is because the version number is in this packet,
	// and later versions might provide larger packets.
	if (dataLen <
		sizeof(NetPacketInitData)
		+ avatarSize
		+ ADD_PADDING(passwordLength)
		+ ADD_PADDING(playerNameLength))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check string sizes.
	if (passwordLength > MAX_PASSWORD_SIZE
		|| !playerNameLength
		|| playerNameLength > MAX_NAME_SIZE)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check name string.
	char *namePtr = (char *)tmpData + sizeof(NetPacketInitData) + avatarSize + ADD_PADDING(passwordLength);
	if (namePtr[0] == 0)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketInitAck::NetPacketInitAck()
: NetPacket(NET_TYPE_INIT_ACK, sizeof(NetPacketInitAckData), sizeof(NetPacketInitAckData))
{
}

NetPacketInitAck::~NetPacketInitAck()
{
}

boost::shared_ptr<NetPacket>
NetPacketInitAck::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketInitAck);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketInitAck::SetData(const NetPacketInitAck::Data &inData)
{
	NetPacketInitAckData *tmpData = (NetPacketInitAckData *)GetRawData();

	tmpData->latestGameVersion		= htons(inData.latestGameVersion);
	tmpData->latestBetaRevision		= htons(inData.latestBetaRevision);
	tmpData->sessionId				= htonl(inData.sessionId);
	tmpData->playerId				= htonl(inData.playerId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketInitAck::GetData(NetPacketInitAck::Data &outData) const
{
	NetPacketInitAckData *tmpData = (NetPacketInitAckData *)GetRawData();

	outData.latestGameVersion		= ntohs(tmpData->latestGameVersion);
	outData.latestBetaRevision		= ntohs(tmpData->latestBetaRevision);
	outData.sessionId				= ntohl(tmpData->sessionId);
	outData.playerId				= ntohl(tmpData->playerId);
}

const NetPacketInitAck *
NetPacketInitAck::ToNetPacketInitAck() const
{
	return this;
}

void
NetPacketInitAck::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketRetrieveAvatar::NetPacketRetrieveAvatar()
: NetPacket(NET_TYPE_RETRIEVE_AVATAR, sizeof(NetPacketRetrieveAvatarData), sizeof(NetPacketRetrieveAvatarData))
{
}

NetPacketRetrieveAvatar::~NetPacketRetrieveAvatar()
{
}

boost::shared_ptr<NetPacket>
NetPacketRetrieveAvatar::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketRetrieveAvatar);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketRetrieveAvatar::SetData(const NetPacketRetrieveAvatar::Data &inData)
{
	NetPacketRetrieveAvatarData *tmpData = (NetPacketRetrieveAvatarData *)GetRawData();

	tmpData->requestId				= htonl(inData.requestId);
	memcpy(tmpData->avatarMD5, inData.avatar.data, MD5_DATA_SIZE);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketRetrieveAvatar::GetData(NetPacketRetrieveAvatar::Data &outData) const
{
	NetPacketRetrieveAvatarData *tmpData = (NetPacketRetrieveAvatarData *)GetRawData();

	outData.requestId				= ntohl(tmpData->requestId);
	memcpy(outData.avatar.data, tmpData->avatarMD5, MD5_DATA_SIZE);
}

const NetPacketRetrieveAvatar *
NetPacketRetrieveAvatar::ToNetPacketRetrieveAvatar() const
{
	return this;
}

void
NetPacketRetrieveAvatar::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketAvatarHeader::NetPacketAvatarHeader()
: NetPacket(NET_TYPE_AVATAR_HEADER, sizeof(NetPacketAvatarHeaderData), sizeof(NetPacketAvatarHeaderData))
{
}

NetPacketAvatarHeader::~NetPacketAvatarHeader()
{
}

boost::shared_ptr<NetPacket>
NetPacketAvatarHeader::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketAvatarHeader);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketAvatarHeader::SetData(const NetPacketAvatarHeader::Data &inData)
{
	NetPacketAvatarHeaderData *tmpData = (NetPacketAvatarHeaderData *)GetRawData();

	tmpData->requestId				= htonl(inData.requestId);
	tmpData->avatarFileSize			= htonl(inData.avatarFileSize);
	tmpData->avatarFileType			= htons(inData.avatarFileType);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketAvatarHeader::GetData(NetPacketAvatarHeader::Data &outData) const
{
	NetPacketAvatarHeaderData *tmpData = (NetPacketAvatarHeaderData *)GetRawData();

	outData.requestId				= ntohl(tmpData->requestId);
	outData.avatarFileSize			= ntohl(tmpData->avatarFileSize);
	outData.avatarFileType			= static_cast<AvatarFileType>(ntohs(tmpData->avatarFileType));
}

const NetPacketAvatarHeader *
NetPacketAvatarHeader::ToNetPacketAvatarHeader() const
{
	return this;
}

void
NetPacketAvatarHeader::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketAvatarFile::NetPacketAvatarFile()
: NetPacket(NET_TYPE_AVATAR_FILE, sizeof(NetPacketAvatarFileData), MAX_PACKET_SIZE)
{
}

NetPacketAvatarFile::~NetPacketAvatarFile()
{
}

boost::shared_ptr<NetPacket>
NetPacketAvatarFile::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketAvatarFile);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketAvatarFile::SetData(const NetPacketAvatarFile::Data &inData)
{
	int fileDataSize = static_cast<int>(inData.fileData.size());

	if (fileDataSize > MAX_FILE_DATA_SIZE)
		throw NetException(__FILE__, __LINE__, ERR_NET_BUF_INVALID_SIZE, 0);

	Resize((u_int16_t)
		(sizeof(NetPacketAvatarFileData)
		+ ADD_PADDING(fileDataSize)));

	NetPacketAvatarFileData *tmpData = (NetPacketAvatarFileData *)GetRawData();

	tmpData->requestId				= htonl(inData.requestId);
	tmpData->blockSize				= htons(fileDataSize);

	char *avatarDataPtr = (char *)tmpData + sizeof(NetPacketAvatarFileData);
	memcpy(avatarDataPtr, &inData.fileData[0], fileDataSize);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketAvatarFile::GetData(NetPacketAvatarFile::Data &outData) const
{
	NetPacketAvatarFileData *tmpData = (NetPacketAvatarFileData *)GetRawData();

	outData.requestId				= ntohl(tmpData->requestId);
	int fileDataSize				= ntohs(tmpData->blockSize);

	char *avatarDataPtr = (char *)tmpData + sizeof(NetPacketAvatarFileData);
	outData.fileData.resize(fileDataSize);
	memcpy(&outData.fileData[0], avatarDataPtr, fileDataSize);
}

const NetPacketAvatarFile *
NetPacketAvatarFile::ToNetPacketAvatarFile() const
{
	return this;
}

void
NetPacketAvatarFile::InternalCheck(const NetPacketHeader *data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketAvatarFileData *tmpData = (NetPacketAvatarFileData *)data;
	int fileDataSize = ntohs(tmpData->blockSize);

	// Exact checking of dynamic packet size.
	if (dataLen !=
		sizeof(NetPacketAvatarFileData)
		+ ADD_PADDING(fileDataSize))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	if (!fileDataSize || fileDataSize > MAX_FILE_DATA_SIZE)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketAvatarEnd::NetPacketAvatarEnd()
: NetPacket(NET_TYPE_AVATAR_END, sizeof(NetPacketAvatarEndData), sizeof(NetPacketAvatarEndData))
{
}

NetPacketAvatarEnd::~NetPacketAvatarEnd()
{
}

boost::shared_ptr<NetPacket>
NetPacketAvatarEnd::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketAvatarEnd);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketAvatarEnd::SetData(const NetPacketAvatarEnd::Data &inData)
{
	NetPacketAvatarEndData *tmpData = (NetPacketAvatarEndData *)GetRawData();

	tmpData->requestId				= htonl(inData.requestId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketAvatarEnd::GetData(NetPacketAvatarEnd::Data &outData) const
{
	NetPacketAvatarEndData *tmpData = (NetPacketAvatarEndData *)GetRawData();

	outData.requestId				= ntohl(tmpData->requestId);
}

const NetPacketAvatarEnd *
NetPacketAvatarEnd::ToNetPacketAvatarEnd() const
{
	return this;
}

void
NetPacketAvatarEnd::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketUnknownAvatar::NetPacketUnknownAvatar()
: NetPacket(NET_TYPE_UNKNOWN_AVATAR, sizeof(NetPacketUnknownAvatarData), sizeof(NetPacketUnknownAvatarData))
{
}

NetPacketUnknownAvatar::~NetPacketUnknownAvatar()
{
}

boost::shared_ptr<NetPacket>
NetPacketUnknownAvatar::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketUnknownAvatar);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketUnknownAvatar::SetData(const NetPacketUnknownAvatar::Data &inData)
{
	NetPacketUnknownAvatarData *tmpData = (NetPacketUnknownAvatarData *)GetRawData();

	tmpData->requestId				= htonl(inData.requestId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketUnknownAvatar::GetData(NetPacketUnknownAvatar::Data &outData) const
{
	NetPacketUnknownAvatarData *tmpData = (NetPacketUnknownAvatarData *)GetRawData();

	outData.requestId				= ntohl(tmpData->requestId);
}

const NetPacketUnknownAvatar *
NetPacketUnknownAvatar::ToNetPacketUnknownAvatar() const
{
	return this;
}

void
NetPacketUnknownAvatar::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketGameListNew::NetPacketGameListNew()
: NetPacket(NET_TYPE_GAME_LIST_NEW, sizeof(NetPacketGameListNewData), MAX_PACKET_SIZE)
{
}

NetPacketGameListNew::~NetPacketGameListNew()
{
}

boost::shared_ptr<NetPacket>
NetPacketGameListNew::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketGameListNew);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketGameListNew::SetData(const NetPacketGameListNew::Data &inData)
{
	u_int16_t gameNameLen = (u_int16_t)inData.gameInfo.name.length();
	u_int16_t curNumPlayers = (u_int16_t)inData.gameInfo.players.size();
	u_int16_t numManualBlinds = (u_int16_t)inData.gameInfo.data.manualBlindsList.size();

	int manualBlindsSize = numManualBlinds * sizeof(u_int32_t);

	// Some basic checks, so we don't use up too much memory.
	// The constructed packet will also be checked.
	if (!gameNameLen || gameNameLen > MAX_NAME_SIZE)
		throw NetException(__FILE__, __LINE__, ERR_NET_INVALID_GAME_NAME, 0);
	if (numManualBlinds > MAX_NUM_MANUAL_BLINDS)
		throw NetException(__FILE__, __LINE__, ERR_NET_TOO_MANY_MANUAL_BLINDS, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketGameListNewData)
		+ manualBlindsSize
		+ ADD_PADDING(gameNameLen)
		+ curNumPlayers * sizeof(unsigned)
		));

	NetPacketGameListNewData *tmpData = (NetPacketGameListNewData *)GetRawData();

	// Set the data.
	tmpData->gameId					= htonl(inData.gameId);
	tmpData->adminPlayerId			= htonl(inData.gameInfo.adminPlayerId);
	tmpData->gameMode				= htons(inData.gameInfo.mode);
	tmpData->gameNameLength			= htons(gameNameLen);
	tmpData->curNumberOfPlayers		= htons(curNumPlayers);
	tmpData->gameFlags				= htons(inData.gameInfo.isPasswordProtected ? NET_GAME_FLAG_PASSWORD_PROTECTED : 0);

	SetGameInfoData(inData.gameInfo.data, &tmpData->gameData);

	char *gameNamePtr = (char *)tmpData + sizeof(NetPacketGameListNewData) + manualBlindsSize;
	memcpy(gameNamePtr, inData.gameInfo.name.c_str(), gameNameLen);

	PlayerIdList::const_iterator i = inData.gameInfo.players.begin();
	PlayerIdList::const_iterator end = inData.gameInfo.players.end();

	// Copy the player list to continous memory
	unsigned *tmpPlayer =
		(unsigned *)(gameNamePtr + ADD_PADDING(gameNameLen));
	while (i != end)
	{
		*tmpPlayer = htonl(*i);
		++tmpPlayer;
		++i;
	}

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketGameListNew::GetData(NetPacketGameListNew::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketGameListNewData *tmpData = (NetPacketGameListNewData *)GetRawData();

	u_int16_t numManualBlinds = ntohs(tmpData->gameData.numberOfManualBlinds);
	int manualBlindsSize = numManualBlinds * sizeof(u_int32_t);

	outData.gameId								= ntohl(tmpData->gameId);
	outData.gameInfo.adminPlayerId				= ntohl(tmpData->adminPlayerId);
	outData.gameInfo.mode						= static_cast<GameMode>(ntohs(tmpData->gameMode));
	u_int16_t gameNameLen						= ntohs(tmpData->gameNameLength);
	u_int16_t curNumPlayers						= ntohs(tmpData->curNumberOfPlayers);
	outData.gameInfo.isPasswordProtected		= ntohs(tmpData->gameFlags) == NET_GAME_FLAG_PASSWORD_PROTECTED;

	GetGameInfoData(&tmpData->gameData, outData.gameInfo.data);

	char *gameNamePtr = (char *)tmpData + sizeof(NetPacketGameListNewData) + manualBlindsSize;
	outData.gameInfo.name = string(gameNamePtr, gameNameLen);

	unsigned *tmpPlayer = (unsigned *)(gameNamePtr + ADD_PADDING(gameNameLen));

	// Store all available players.
	for (int i = 0; i < curNumPlayers; i++)
	{
		outData.gameInfo.players.push_back(ntohl(*tmpPlayer));
		++tmpPlayer;
	}
}

const NetPacketGameListNew *
NetPacketGameListNew::ToNetPacketGameListNew() const
{
	return this;
}

void
NetPacketGameListNew::InternalCheck(const NetPacketHeader* data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketGameListNewData *tmpData = (NetPacketGameListNewData *)data;
	int gameNameLength = ntohs(tmpData->gameNameLength);
	int curNumPlayers = ntohs(tmpData->curNumberOfPlayers);
	u_int16_t numManualBlinds = ntohs(tmpData->gameData.numberOfManualBlinds);
	int manualBlindsLength = numManualBlinds * sizeof(u_int32_t);
	// Exact checking of dynamic packet size.
	if (dataLen !=
		sizeof(NetPacketGameListNewData)
		+ manualBlindsLength
		+ ADD_PADDING(gameNameLength)
		+ curNumPlayers * sizeof(unsigned))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check manual blind list size
	if (numManualBlinds > MAX_NUM_MANUAL_BLINDS)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check string size.
	if (!gameNameLength
		|| gameNameLength > MAX_NAME_SIZE)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check name string.
	char *namePtr = (char *)tmpData + sizeof(NetPacketGameListNewData) + manualBlindsLength;
	if (namePtr[0] == 0)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketGameListUpdate::NetPacketGameListUpdate()
: NetPacket(NET_TYPE_GAME_LIST_UPDATE, sizeof(NetPacketGameListUpdateData), sizeof(NetPacketGameListUpdateData))
{
}

NetPacketGameListUpdate::~NetPacketGameListUpdate()
{
}

boost::shared_ptr<NetPacket>
NetPacketGameListUpdate::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketGameListUpdate);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketGameListUpdate::SetData(const NetPacketGameListUpdate::Data &inData)
{
	NetPacketGameListUpdateData *tmpData = (NetPacketGameListUpdateData *)GetRawData();

	// Set the data.
	tmpData->gameId			= htonl(inData.gameId);
	tmpData->gameMode		= htons(inData.gameMode);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketGameListUpdate::GetData(NetPacketGameListUpdate::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketGameListUpdateData *tmpData = (NetPacketGameListUpdateData *)GetRawData();

	outData.gameId			= ntohl(tmpData->gameId);
	outData.gameMode		= static_cast<GameMode>(ntohs(tmpData->gameMode));
}

const NetPacketGameListUpdate *
NetPacketGameListUpdate::ToNetPacketGameListUpdate() const
{
	return this;
}

void
NetPacketGameListUpdate::InternalCheck(const NetPacketHeader*) const
{
}

//-----------------------------------------------------------------------------

NetPacketGameListPlayerJoined::NetPacketGameListPlayerJoined()
: NetPacket(NET_TYPE_GAME_LIST_PLAYER_JOINED, sizeof(NetPacketGameListPlayerJoinedData), sizeof(NetPacketGameListPlayerJoinedData))
{
}

NetPacketGameListPlayerJoined::~NetPacketGameListPlayerJoined()
{
}

boost::shared_ptr<NetPacket>
NetPacketGameListPlayerJoined::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketGameListPlayerJoined);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketGameListPlayerJoined::SetData(const NetPacketGameListPlayerJoined::Data &inData)
{
	NetPacketGameListPlayerJoinedData *tmpData = (NetPacketGameListPlayerJoinedData *)GetRawData();

	// Set the data.
	tmpData->gameId			= htonl(inData.gameId);
	tmpData->playerId		= htonl(inData.playerId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketGameListPlayerJoined::GetData(NetPacketGameListPlayerJoined::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketGameListPlayerJoinedData *tmpData = (NetPacketGameListPlayerJoinedData *)GetRawData();

	outData.gameId			= ntohl(tmpData->gameId);
	outData.playerId		= ntohl(tmpData->playerId);
}

const NetPacketGameListPlayerJoined *
NetPacketGameListPlayerJoined::ToNetPacketGameListPlayerJoined() const
{
	return this;
}

void
NetPacketGameListPlayerJoined::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketGameListPlayerLeft::NetPacketGameListPlayerLeft()
: NetPacket(NET_TYPE_GAME_LIST_PLAYER_LEFT, sizeof(NetPacketGameListPlayerLeftData), sizeof(NetPacketGameListPlayerLeftData))
{
}

NetPacketGameListPlayerLeft::~NetPacketGameListPlayerLeft()
{
}

boost::shared_ptr<NetPacket>
NetPacketGameListPlayerLeft::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketGameListPlayerLeft);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketGameListPlayerLeft::SetData(const NetPacketGameListPlayerLeft::Data &inData)
{
	NetPacketGameListPlayerLeftData *tmpData = (NetPacketGameListPlayerLeftData *)GetRawData();

	// Set the data.
	tmpData->gameId			= htonl(inData.gameId);
	tmpData->playerId		= htonl(inData.playerId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketGameListPlayerLeft::GetData(NetPacketGameListPlayerLeft::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketGameListPlayerLeftData *tmpData = (NetPacketGameListPlayerLeftData *)GetRawData();

	outData.gameId			= ntohl(tmpData->gameId);
	outData.playerId		= ntohl(tmpData->playerId);
}

const NetPacketGameListPlayerLeft *
NetPacketGameListPlayerLeft::ToNetPacketGameListPlayerLeft() const
{
	return this;
}

void
NetPacketGameListPlayerLeft::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketGameListAdminChanged::NetPacketGameListAdminChanged()
: NetPacket(NET_TYPE_GAME_LIST_ADMIN_CHANGED, sizeof(NetPacketGameListAdminChangedData), sizeof(NetPacketGameListAdminChangedData))
{
}

NetPacketGameListAdminChanged::~NetPacketGameListAdminChanged()
{
}

boost::shared_ptr<NetPacket>
NetPacketGameListAdminChanged::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketGameListAdminChanged);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketGameListAdminChanged::SetData(const NetPacketGameListAdminChanged::Data &inData)
{
	NetPacketGameListAdminChangedData *tmpData = (NetPacketGameListAdminChangedData *)GetRawData();

	// Set the data.
	tmpData->gameId				= htonl(inData.gameId);
	tmpData->newAdminPlayerId	= htonl(inData.newAdminplayerId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketGameListAdminChanged::GetData(NetPacketGameListAdminChanged::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketGameListAdminChangedData *tmpData = (NetPacketGameListAdminChangedData *)GetRawData();

	outData.gameId				= ntohl(tmpData->gameId);
	outData.newAdminplayerId	= ntohl(tmpData->newAdminPlayerId);
}

const NetPacketGameListAdminChanged *
NetPacketGameListAdminChanged::ToNetPacketGameListAdminChanged() const
{
	return this;
}

void
NetPacketGameListAdminChanged::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketRetrievePlayerInfo::NetPacketRetrievePlayerInfo()
: NetPacket(NET_TYPE_RETRIEVE_PLAYER_INFO, sizeof(NetPacketRetrievePlayerInfoData), sizeof(NetPacketRetrievePlayerInfoData))
{
}

NetPacketRetrievePlayerInfo::~NetPacketRetrievePlayerInfo()
{
}

boost::shared_ptr<NetPacket>
NetPacketRetrievePlayerInfo::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketRetrievePlayerInfo);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketRetrievePlayerInfo::SetData(const NetPacketRetrievePlayerInfo::Data &inData)
{
	NetPacketRetrievePlayerInfoData *tmpData = (NetPacketRetrievePlayerInfoData *)GetRawData();

	// Set the data.
	tmpData->playerId		= htonl(inData.playerId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketRetrievePlayerInfo::GetData(NetPacketRetrievePlayerInfo::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketRetrievePlayerInfoData *tmpData = (NetPacketRetrievePlayerInfoData *)GetRawData();

	outData.playerId		= ntohl(tmpData->playerId);
}

const NetPacketRetrievePlayerInfo *
NetPacketRetrievePlayerInfo::ToNetPacketRetrievePlayerInfo() const
{
	return this;
}

void
NetPacketRetrievePlayerInfo::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketPlayerInfo::NetPacketPlayerInfo()
: NetPacket(NET_TYPE_PLAYER_INFO, sizeof(NetPacketPlayerInfoData), MAX_PACKET_SIZE)
{
}

NetPacketPlayerInfo::~NetPacketPlayerInfo()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayerInfo::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayerInfo);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketPlayerInfo::SetData(const NetPacketPlayerInfo::Data &inData)
{
	u_int16_t playerNameLen = (u_int16_t)inData.playerInfo.playerName.length();

	if (!playerNameLen || playerNameLen > MAX_NAME_SIZE)
		throw NetException(__FILE__, __LINE__, ERR_NET_INVALID_PLAYER_NAME, 0);

	int avatarSize = inData.playerInfo.hasAvatar ? MD5_DATA_SIZE : 0;
	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketPlayerInfoData) + avatarSize + ADD_PADDING(playerNameLen)));

	NetPacketPlayerInfoData *tmpData = (NetPacketPlayerInfoData *)GetRawData();

	// Set the data.
	tmpData->playerId			= htonl(inData.playerId);
	tmpData->playerNameLength	= htons(playerNameLen);

	u_int16_t tmpPlayerFlags = inData.playerInfo.ptype == PLAYER_TYPE_HUMAN ? NET_PLAYER_FLAG_HUMAN : 0;
	if (inData.playerInfo.hasAvatar)
	{
		tmpPlayerFlags |= NET_PLAYER_FLAG_HAS_AVATAR;
		char *avatarPtr = (char *)tmpData + sizeof(NetPacketPlayerInfoData);
		memcpy(avatarPtr, inData.playerInfo.avatar.data, MD5_DATA_SIZE);
	}
	tmpData->playerFlags		= htons(tmpPlayerFlags);

	char *namePtr = (char *)tmpData + sizeof(NetPacketPlayerInfoData) + avatarSize;
	memcpy(namePtr, inData.playerInfo.playerName.c_str(), playerNameLen);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketPlayerInfo::GetData(NetPacketPlayerInfo::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketPlayerInfoData *tmpData = (NetPacketPlayerInfoData *)GetRawData();

	outData.playerId = ntohl(tmpData->playerId);
	u_int16_t tmpPlayerFlags = ntohs(tmpData->playerFlags);
	outData.playerInfo.ptype = (tmpPlayerFlags & NET_PLAYER_FLAG_HUMAN) ? PLAYER_TYPE_HUMAN : PLAYER_TYPE_COMPUTER;
	outData.playerInfo.hasAvatar = (tmpPlayerFlags & NET_PLAYER_FLAG_HAS_AVATAR) ? true : false;

	if (outData.playerInfo.hasAvatar)
	{
		char *avatarPtr = (char *)tmpData + sizeof(NetPacketPlayerInfoData);
		memcpy(outData.playerInfo.avatar.data, avatarPtr, MD5_DATA_SIZE);
	}

	int avatarSize = outData.playerInfo.hasAvatar ? MD5_DATA_SIZE : 0;

	char *namePtr = (char *)tmpData + avatarSize + sizeof(NetPacketPlayerInfoData);
	outData.playerInfo.playerName = string(namePtr, ntohs(tmpData->playerNameLength));
}

const NetPacketPlayerInfo *
NetPacketPlayerInfo::ToNetPacketPlayerInfo() const
{
	return this;
}

void
NetPacketPlayerInfo::InternalCheck(const NetPacketHeader* data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketPlayerInfoData *tmpData = (NetPacketPlayerInfoData *)data;
	int playerNameLength = ntohs(tmpData->playerNameLength);
	int avatarSize = (ntohs(tmpData->playerFlags) & NET_PLAYER_FLAG_HAS_AVATAR) ? MD5_DATA_SIZE : 0;
	// Exact checking this time.
	if (dataLen !=
		sizeof(NetPacketPlayerInfoData)
		+ avatarSize
		+ ADD_PADDING(playerNameLength))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check string sizes.
	if (!playerNameLength
		|| playerNameLength > MAX_NAME_SIZE)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketUnknownPlayerId::NetPacketUnknownPlayerId()
: NetPacket(NET_TYPE_UNKNOWN_PLAYER_ID, sizeof(NetPacketUnknownPlayerIdData), sizeof(NetPacketUnknownPlayerIdData))
{
}

NetPacketUnknownPlayerId::~NetPacketUnknownPlayerId()
{
}

boost::shared_ptr<NetPacket>
NetPacketUnknownPlayerId::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketUnknownPlayerId);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketUnknownPlayerId::SetData(const NetPacketUnknownPlayerId::Data &inData)
{
	NetPacketUnknownPlayerIdData *tmpData = (NetPacketUnknownPlayerIdData *)GetRawData();

	// Set the data.
	tmpData->playerId		= htonl(inData.playerId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketUnknownPlayerId::GetData(NetPacketUnknownPlayerId::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketUnknownPlayerIdData *tmpData = (NetPacketUnknownPlayerIdData *)GetRawData();

	outData.playerId		= ntohl(tmpData->playerId);
}

const NetPacketUnknownPlayerId *
NetPacketUnknownPlayerId::ToNetPacketUnknownPlayerId() const
{
	return this;
}

void
NetPacketUnknownPlayerId::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketUnsubscribeGameList::NetPacketUnsubscribeGameList()
: NetPacket(NET_TYPE_UNSUBSCRIBE_GAME_LIST, sizeof(NetPacketUnsubscribeGameListData), sizeof(NetPacketUnsubscribeGameListData))
{
}

NetPacketUnsubscribeGameList::~NetPacketUnsubscribeGameList()
{
}

boost::shared_ptr<NetPacket>
NetPacketUnsubscribeGameList::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketUnsubscribeGameList);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

const NetPacketUnsubscribeGameList *
NetPacketUnsubscribeGameList::ToNetPacketUnsubscribeGameList() const
{
	return this;
}

void
NetPacketUnsubscribeGameList::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketResubscribeGameList::NetPacketResubscribeGameList()
: NetPacket(NET_TYPE_RESUBSCRIBE_GAME_LIST, sizeof(NetPacketResubscribeGameListData), sizeof(NetPacketResubscribeGameListData))
{
}

NetPacketResubscribeGameList::~NetPacketResubscribeGameList()
{
}

boost::shared_ptr<NetPacket>
NetPacketResubscribeGameList::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketResubscribeGameList);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

const NetPacketResubscribeGameList *
NetPacketResubscribeGameList::ToNetPacketResubscribeGameList() const
{
	return this;
}

void
NetPacketResubscribeGameList::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketCreateGame::NetPacketCreateGame()
: NetPacket(NET_TYPE_CREATE_GAME, sizeof(NetPacketCreateGameData), MAX_PACKET_SIZE)
{
}

NetPacketCreateGame::~NetPacketCreateGame()
{
}

boost::shared_ptr<NetPacket>
NetPacketCreateGame::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketCreateGame);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketCreateGame::SetData(const NetPacketCreateGame::Data &inData)
{
	u_int16_t gameNameLen = (u_int16_t)inData.gameName.length();
	u_int16_t passwordLen = (u_int16_t)inData.password.length();
	u_int16_t numManualBlinds = (u_int16_t)inData.gameData.manualBlindsList.size();

	// Some basic checks, so we don't use up too much memory.
	// The constructed packet will also be checked.
	if (!gameNameLen || gameNameLen > MAX_NAME_SIZE)
		throw NetException(__FILE__, __LINE__, ERR_NET_INVALID_PLAYER_NAME, 0);
	if (passwordLen > MAX_PASSWORD_SIZE)
		throw NetException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD_STR, 0);
	if (numManualBlinds > MAX_NUM_MANUAL_BLINDS)
		throw NetException(__FILE__, __LINE__, ERR_NET_TOO_MANY_MANUAL_BLINDS, 0);

	int manualBlindsSize = numManualBlinds * sizeof(u_int32_t);
	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketCreateGameData) + manualBlindsSize + ADD_PADDING(gameNameLen) + ADD_PADDING(passwordLen)));

	NetPacketCreateGameData *tmpData = (NetPacketCreateGameData *)GetRawData();

	// Set the data.
	tmpData->passwordLength				= htons(passwordLen);
	tmpData->gameNameLength				= htons(gameNameLen);

	SetGameInfoData(inData.gameData, &tmpData->gameData);

	char *passwordPtr = (char *)tmpData + sizeof(NetPacketCreateGameData) + manualBlindsSize;
	memcpy(passwordPtr, inData.password.c_str(), passwordLen);
	memcpy(passwordPtr + ADD_PADDING(passwordLen), inData.gameName.c_str(), gameNameLen);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketCreateGame::GetData(NetPacketCreateGame::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketCreateGameData *tmpData = (NetPacketCreateGameData *)GetRawData();

	u_int16_t numManualBlinds = ntohs(tmpData->gameData.numberOfManualBlinds);
	int manualBlindsSize = numManualBlinds * sizeof(u_int32_t);

	GetGameInfoData(&tmpData->gameData, outData.gameData);

	u_int16_t passwordLen = ntohs(tmpData->passwordLength);
	char *passwordPtr = (char *)tmpData + manualBlindsSize + sizeof(NetPacketCreateGameData);
	outData.password = string(passwordPtr, passwordLen);
	outData.gameName = string(passwordPtr + ADD_PADDING(passwordLen), ntohs(tmpData->gameNameLength));
}

const NetPacketCreateGame *
NetPacketCreateGame::ToNetPacketCreateGame() const
{
	return this;
}

void
NetPacketCreateGame::InternalCheck(const NetPacketHeader* data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketCreateGameData *tmpData = (NetPacketCreateGameData *)data;
	int passwordLength = ntohs(tmpData->passwordLength);
	int gameNameLength = ntohs(tmpData->gameNameLength);
	int numManualBlinds = ntohs(tmpData->gameData.numberOfManualBlinds);
	int manualBlindsLength = numManualBlinds * sizeof(u_int32_t);
	// Exact checking of dynamic packet size.
	if (dataLen <
		sizeof(NetPacketCreateGameData)
		+ manualBlindsLength
		+ ADD_PADDING(passwordLength)
		+ ADD_PADDING(gameNameLength))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check manual blind list size
	if (numManualBlinds > MAX_NUM_MANUAL_BLINDS)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check string sizes.
	if (passwordLength > MAX_PASSWORD_SIZE
		|| !gameNameLength
		|| gameNameLength > MAX_NAME_SIZE)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check name string.
	char *namePtr = (char *)tmpData + sizeof(NetPacketCreateGameData) + manualBlindsLength + ADD_PADDING(passwordLength);
	if (namePtr[0] == 0)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	CheckGameInfoData(&tmpData->gameData);
}

//-----------------------------------------------------------------------------

NetPacketJoinGame::NetPacketJoinGame()
: NetPacket(NET_TYPE_JOIN_GAME, sizeof(NetPacketJoinGameData), MAX_PACKET_SIZE)
{
}

NetPacketJoinGame::~NetPacketJoinGame()
{
}

boost::shared_ptr<NetPacket>
NetPacketJoinGame::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketJoinGame);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketJoinGame::SetData(const NetPacketJoinGame::Data &inData)
{
	u_int16_t passwordLen = (u_int16_t)inData.password.length();

	// Some basic checks, so we don't use up too much memory.
	// The constructed packet will also be checked.
	if (passwordLen > MAX_PASSWORD_SIZE)
		throw NetException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD_STR, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketJoinGameData) + ADD_PADDING(passwordLen)));

	NetPacketJoinGameData *tmpData = (NetPacketJoinGameData *)GetRawData();

	// Set the data.
	tmpData->gameId					= htonl(inData.gameId);
	tmpData->passwordLength			= htons(passwordLen);

	char *passwordPtr = (char *)tmpData + sizeof(NetPacketJoinGameData);
	memcpy(passwordPtr, inData.password.c_str(), passwordLen);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketJoinGame::GetData(NetPacketJoinGame::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketJoinGameData *tmpData = (NetPacketJoinGameData *)GetRawData();

	outData.gameId					= ntohl(tmpData->gameId);

	u_int16_t passwordLen = ntohs(tmpData->passwordLength);
	char *passwordPtr = (char *)tmpData + sizeof(NetPacketJoinGameData);
	outData.password = string(passwordPtr, passwordLen);
}

const NetPacketJoinGame *
NetPacketJoinGame::ToNetPacketJoinGame() const
{
	return this;
}

void
NetPacketJoinGame::InternalCheck(const NetPacketHeader* data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketJoinGameData *tmpData = (NetPacketJoinGameData *)data;
	int passwordLength = ntohs(tmpData->passwordLength);
	// Exact checking of dynamic packet size.
	if (dataLen <
		sizeof(NetPacketJoinGameData)
		+ ADD_PADDING(passwordLength))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check string size.
	if (passwordLength > MAX_PASSWORD_SIZE)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketJoinGameAck::NetPacketJoinGameAck()
: NetPacket(NET_TYPE_JOIN_GAME_ACK, sizeof(NetPacketJoinGameAckData), MAX_PACKET_SIZE)
{
}

NetPacketJoinGameAck::~NetPacketJoinGameAck()
{
}

boost::shared_ptr<NetPacket>
NetPacketJoinGameAck::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketJoinGameAck);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketJoinGameAck::SetData(const NetPacketJoinGameAck::Data &inData)
{
	u_int16_t numManualBlinds = (u_int16_t)inData.gameData.manualBlindsList.size();

	// Some basic checks, so we don't use up too much memory.
	// The constructed packet will also be checked.
	if (numManualBlinds > MAX_NUM_MANUAL_BLINDS)
		throw NetException(__FILE__, __LINE__, ERR_NET_TOO_MANY_MANUAL_BLINDS, 0);

	int manualBlindsSize = numManualBlinds * sizeof(u_int32_t);
	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketJoinGameAckData) + manualBlindsSize));

	NetPacketJoinGameAckData *tmpData = (NetPacketJoinGameAckData *)GetRawData();

	// Set the data.
	tmpData->gameId						= htonl(inData.gameId);
	tmpData->playerRights				= htons(inData.prights);

	SetGameInfoData(inData.gameData, &tmpData->gameData);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketJoinGameAck::GetData(NetPacketJoinGameAck::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketJoinGameAckData *tmpData = (NetPacketJoinGameAckData *)GetRawData();

	outData.gameId								= ntohl(tmpData->gameId);
	outData.prights								= static_cast<PlayerRights>(ntohs(tmpData->playerRights));

	GetGameInfoData(&tmpData->gameData, outData.gameData);
}

const NetPacketJoinGameAck *
NetPacketJoinGameAck::ToNetPacketJoinGameAck() const
{
	return this;
}

void
NetPacketJoinGameAck::InternalCheck(const NetPacketHeader* data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketJoinGameAckData *tmpData = (NetPacketJoinGameAckData *)data;
	int numManualBlinds = ntohs(tmpData->gameData.numberOfManualBlinds);
	int manualBlindsLength = numManualBlinds * sizeof(u_int32_t);
	// Exact checking of dynamic packet size.
	if (dataLen <
		sizeof(NetPacketJoinGameAckData)
		+ manualBlindsLength)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}

	// Check manual blind list size
	if (numManualBlinds > MAX_NUM_MANUAL_BLINDS)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Semantic checks
	CheckGameInfoData(&tmpData->gameData);
}

//-----------------------------------------------------------------------------

NetPacketJoinGameFailed::NetPacketJoinGameFailed()
: NetPacket(NET_TYPE_JOIN_GAME_FAILED, sizeof(NetPacketJoinGameFailedData), sizeof(NetPacketJoinGameFailedData))
{
}

NetPacketJoinGameFailed::~NetPacketJoinGameFailed()
{
}

boost::shared_ptr<NetPacket>
NetPacketJoinGameFailed::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketJoinGameFailed);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketJoinGameFailed::SetData(const NetPacketJoinGameFailed::Data &inData)
{
	NetPacketJoinGameFailedData *tmpData = (NetPacketJoinGameFailedData *)GetRawData();

	switch (inData.failureCode)
	{
	// Join Game Errors.
		case NTF_NET_JOIN_GAME_FULL :
			tmpData->failureReason = htons(NET_JOIN_FAILED_GAME_FULL);
			break;
		case NTF_NET_JOIN_ALREADY_RUNNING :
			tmpData->failureReason = htons(NET_JOIN_FAILED_GAME_ALREADY_RUNNING);
			break;
		case NTF_NET_JOIN_INVALID_PASSWORD :
			tmpData->failureReason = htons(NET_JOIN_FAILED_INVALID_PASSWORD);
			break;
		default :
			tmpData->failureReason = htons(NET_JOIN_FAILED_OTHER);
			break;
	}

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketJoinGameFailed::GetData(NetPacketJoinGameFailed::Data &outData) const
{
	NetPacketJoinGameFailedData *tmpData = (NetPacketJoinGameFailedData *)GetRawData();

	switch (ntohs(tmpData->failureReason))
	{
	// Join Game Errors.
		case NET_JOIN_FAILED_GAME_FULL :
			outData.failureCode = NTF_NET_JOIN_GAME_FULL;
			break;
		case NET_JOIN_FAILED_GAME_ALREADY_RUNNING :
			outData.failureCode = NTF_NET_JOIN_ALREADY_RUNNING;
			break;
		case NET_JOIN_FAILED_INVALID_PASSWORD :
			outData.failureCode = NTF_NET_JOIN_INVALID_PASSWORD;
			break;
		default :
			outData.failureCode = NTF_NET_INTERNAL;
			break;
	}
}

const NetPacketJoinGameFailed *
NetPacketJoinGameFailed::ToNetPacketJoinGameFailed() const
{
	return this;
}

void
NetPacketJoinGameFailed::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketPlayerJoined::NetPacketPlayerJoined()
: NetPacket(NET_TYPE_PLAYER_JOINED, sizeof(NetPacketPlayerJoinedData), sizeof(NetPacketPlayerJoinedData))
{
}

NetPacketPlayerJoined::~NetPacketPlayerJoined()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayerJoined::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayerJoined);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketPlayerJoined::SetData(const NetPacketPlayerJoined::Data &inData)
{
	NetPacketPlayerJoinedData *tmpData = (NetPacketPlayerJoinedData *)GetRawData();

	// Set the data.
	tmpData->playerId			= htonl(inData.playerId);
	tmpData->playerRights		= htons(inData.prights);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketPlayerJoined::GetData(NetPacketPlayerJoined::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketPlayerJoinedData *tmpData = (NetPacketPlayerJoinedData *)GetRawData();

	outData.playerId = ntohl(tmpData->playerId);
	outData.prights = static_cast<PlayerRights>(ntohs(tmpData->playerRights));
}

const NetPacketPlayerJoined *
NetPacketPlayerJoined::ToNetPacketPlayerJoined() const
{
	return this;
}

void
NetPacketPlayerJoined::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketPlayerLeft::NetPacketPlayerLeft()
: NetPacket(NET_TYPE_PLAYER_LEFT, sizeof(NetPacketPlayerLeftData), sizeof(NetPacketPlayerLeftData))
{
}

NetPacketPlayerLeft::~NetPacketPlayerLeft()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayerLeft::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayerLeft);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketPlayerLeft::SetData(const NetPacketPlayerLeft::Data &inData)
{
	NetPacketPlayerLeftData *tmpData = (NetPacketPlayerLeftData *)GetRawData();

	// Set the data.
	tmpData->playerId		= htonl(inData.playerId);
	tmpData->leaveReason	= htons(inData.removeReason);

	switch(inData.removeReason)
	{
		case NTF_NET_REMOVED_ON_REQUEST :
			tmpData->leaveReason = htons(NET_LEFT_ON_REQUEST);
			break;
		case NTF_NET_REMOVED_KICKED :
			tmpData->leaveReason = htons(NET_LEFT_KICKED);
			break;
		case NTF_NET_INTERNAL :
			tmpData->leaveReason = htons(NET_LEFT_ERROR);
			break;
		default :
			tmpData->leaveReason = htons(NET_LEFT_OTHER_REASON);
			break;
	}

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketPlayerLeft::GetData(NetPacketPlayerLeft::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketPlayerLeftData *tmpData = (NetPacketPlayerLeftData *)GetRawData();

	outData.playerId		= ntohl(tmpData->playerId);

	switch(ntohs(tmpData->leaveReason))
	{
		case NET_LEFT_ON_REQUEST :
			outData.removeReason = NTF_NET_REMOVED_ON_REQUEST;
			break;
		case NET_LEFT_KICKED :
			outData.removeReason = NTF_NET_REMOVED_KICKED;
			break;
		default :
			outData.removeReason = NTF_NET_INTERNAL;
			break;
	}
}

const NetPacketPlayerLeft *
NetPacketPlayerLeft::ToNetPacketPlayerLeft() const
{
	return this;
}

void
NetPacketPlayerLeft::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}


//-----------------------------------------------------------------------------

NetPacketGameAdminChanged::NetPacketGameAdminChanged()
: NetPacket(NET_TYPE_GAME_ADMIN_CHANGED, sizeof(NetPacketGameAdminChangedData), sizeof(NetPacketGameAdminChangedData))
{
}

NetPacketGameAdminChanged::~NetPacketGameAdminChanged()
{
}

boost::shared_ptr<NetPacket>
NetPacketGameAdminChanged::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketGameAdminChanged);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketGameAdminChanged::SetData(const NetPacketGameAdminChanged::Data &inData)
{
	NetPacketGameAdminChangedData *tmpData = (NetPacketGameAdminChangedData *)GetRawData();

	// Set the data.
	tmpData->playerId = htonl(inData.playerId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketGameAdminChanged::GetData(NetPacketGameAdminChanged::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketGameAdminChangedData *tmpData = (NetPacketGameAdminChangedData *)GetRawData();

	outData.playerId = ntohl(tmpData->playerId);
}

const NetPacketGameAdminChanged *
NetPacketGameAdminChanged::ToNetPacketGameAdminChanged() const
{
	return this;
}

void
NetPacketGameAdminChanged::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketKickPlayer::NetPacketKickPlayer()
: NetPacket(NET_TYPE_KICK_PLAYER, sizeof(NetPacketKickPlayerData), sizeof(NetPacketKickPlayerData))
{
}

NetPacketKickPlayer::~NetPacketKickPlayer()
{
}

boost::shared_ptr<NetPacket>
NetPacketKickPlayer::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketKickPlayer);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketKickPlayer::SetData(const NetPacketKickPlayer::Data &inData)
{
	NetPacketKickPlayerData *tmpData = (NetPacketKickPlayerData *)GetRawData();

	// Set the data.
	tmpData->playerId = htonl(inData.playerId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketKickPlayer::GetData(NetPacketKickPlayer::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketKickPlayerData *tmpData = (NetPacketKickPlayerData *)GetRawData();

	outData.playerId = ntohl(tmpData->playerId);
}

const NetPacketKickPlayer *
NetPacketKickPlayer::ToNetPacketKickPlayer() const
{
	return this;
}

void
NetPacketKickPlayer::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketLeaveCurrentGame::NetPacketLeaveCurrentGame()
: NetPacket(NET_TYPE_LEAVE_CURRENT_GAME, sizeof(NetPacketLeaveCurrentGameData), sizeof(NetPacketLeaveCurrentGameData))
{
}

NetPacketLeaveCurrentGame::~NetPacketLeaveCurrentGame()
{
}

boost::shared_ptr<NetPacket>
NetPacketLeaveCurrentGame::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketLeaveCurrentGame);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

const NetPacketLeaveCurrentGame *
NetPacketLeaveCurrentGame::ToNetPacketLeaveCurrentGame() const
{
	return this;
}

void
NetPacketLeaveCurrentGame::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketStartEvent::NetPacketStartEvent()
: NetPacket(NET_TYPE_START_EVENT, sizeof(NetPacketStartEventData), sizeof(NetPacketStartEventData))
{
}

NetPacketStartEvent::~NetPacketStartEvent()
{
}

boost::shared_ptr<NetPacket>
NetPacketStartEvent::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketStartEvent);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketStartEvent::SetData(const NetPacketStartEvent::Data &inData)
{
	NetPacketStartEventData *tmpData = (NetPacketStartEventData *)GetRawData();

	// Set the data.
	tmpData->startFlags = htons(inData.fillUpWithCpuPlayers ? NET_START_FLAG_FILL_WITH_CPU_PLAYERS : 0);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketStartEvent::GetData(NetPacketStartEvent::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketStartEventData *tmpData = (NetPacketStartEventData *)GetRawData();

	outData.fillUpWithCpuPlayers = (ntohs(tmpData->startFlags) & NET_START_FLAG_FILL_WITH_CPU_PLAYERS);
}

const NetPacketStartEvent *
NetPacketStartEvent::ToNetPacketStartEvent() const
{
	return this;
}

void
NetPacketStartEvent::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketStartEventAck::NetPacketStartEventAck()
: NetPacket(NET_TYPE_START_EVENT_ACK, sizeof(NetPacketStartEventAckData), sizeof(NetPacketStartEventAckData))
{
}

NetPacketStartEventAck::~NetPacketStartEventAck()
{
}

boost::shared_ptr<NetPacket>
NetPacketStartEventAck::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketStartEventAck);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

const NetPacketStartEventAck *
NetPacketStartEventAck::ToNetPacketStartEventAck() const
{
	return this;
}

void
NetPacketStartEventAck::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketGameStart::NetPacketGameStart()
: NetPacket(NET_TYPE_GAME_START, sizeof(NetPacketGameStartData), MAX_PACKET_SIZE)
{
}

NetPacketGameStart::~NetPacketGameStart()
{
}

boost::shared_ptr<NetPacket>
NetPacketGameStart::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketGameStart);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketGameStart::SetData(const NetPacketGameStart::Data &inData)
{
	u_int16_t numPlayers = (u_int16_t)inData.playerSlots.size();

	// Basic checking.
	if (numPlayers < MIN_NUMBER_OF_PLAYERS || numPlayers > MAX_NUMBER_OF_PLAYERS || numPlayers != inData.startData.numberOfPlayers)
	{
		// This seems to occur too often.
		// TODO needs fix.
		LOG_VERBOSE("Invalid number of players. Slots: " << numPlayers << ", Players: " << inData.startData.numberOfPlayers << ".");
		throw NetException(__FILE__, __LINE__, ERR_NET_INVALID_PLAYER_COUNT, 0);
	}

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketGameStartData) + ADD_PADDING(numPlayers * sizeof(PlayerSlotData))));

	NetPacketGameStartData *tmpData = (NetPacketGameStartData *)GetRawData();

	tmpData->startDealerPlayerId	= htonl(inData.startData.startDealerPlayerId);
	tmpData->numberOfPlayers		= htons(numPlayers);

	PlayerSlotList::const_iterator i = inData.playerSlots.begin();
	PlayerSlotList::const_iterator end = inData.playerSlots.end();

	// Copy the player slot data to continous memory
	PlayerSlotData *curPlayerSlotData =
		(PlayerSlotData *)((char *)tmpData + sizeof(NetPacketGameStartData));
	while (i != end)
	{
		curPlayerSlotData->playerId		= htonl((*i).playerId);
		++curPlayerSlotData;
		++i;
	}

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketGameStart::GetData(NetPacketGameStart::Data &outData) const
{
	NetPacketGameStartData *tmpData = (NetPacketGameStartData *)GetRawData();

	outData.startData.startDealerPlayerId	= ntohl(tmpData->startDealerPlayerId);
	u_int16_t numPlayers = ntohs(tmpData->numberOfPlayers);
	outData.startData.numberOfPlayers = numPlayers;

	PlayerSlotData *curPlayerSlotData =
		(PlayerSlotData *)((char *)tmpData + sizeof(NetPacketGameStartData));

	// Store all available player slots.
	for (int i = 0; i < numPlayers; i++)
	{
		PlayerSlot tmpPlayerSlot;
		tmpPlayerSlot.playerId	= ntohl(curPlayerSlotData->playerId);

		outData.playerSlots.push_back(tmpPlayerSlot);
		++curPlayerSlotData;
	}
}

const NetPacketGameStart *
NetPacketGameStart::ToNetPacketGameStart() const
{
	return this;
}

void
NetPacketGameStart::InternalCheck(const NetPacketHeader* data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketGameStartData *tmpData = (NetPacketGameStartData *)data;
	int numPlayers = ntohs(tmpData->numberOfPlayers);
	// Check exact packet size.
	if (dataLen !=
		sizeof(NetPacketGameStartData)
		+ numPlayers * sizeof(PlayerSlotData))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Semantic checks not needed.
}

//-----------------------------------------------------------------------------

NetPacketHandStart::NetPacketHandStart()
: NetPacket(NET_TYPE_HAND_START, sizeof(NetPacketHandStartData), sizeof(NetPacketHandStartData))
{
}

NetPacketHandStart::~NetPacketHandStart()
{
}

boost::shared_ptr<NetPacket>
NetPacketHandStart::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketHandStart);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketHandStart::SetData(const NetPacketHandStart::Data &inData)
{
	NetPacketHandStartData *tmpData = (NetPacketHandStartData *)GetRawData();

	tmpData->yourCard1		= htons(inData.yourCards[0]);
	tmpData->yourCard2		= htons(inData.yourCards[1]);
	tmpData->smallBlind		= htonl(inData.smallBlind);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketHandStart::GetData(NetPacketHandStart::Data &outData) const
{
	NetPacketHandStartData *tmpData = (NetPacketHandStartData *)GetRawData();

	outData.yourCards[0]		= ntohs(tmpData->yourCard1);
	outData.yourCards[1]		= ntohs(tmpData->yourCard2);
	outData.smallBlind			= ntohl(tmpData->smallBlind);
}

const NetPacketHandStart *
NetPacketHandStart::ToNetPacketHandStart() const
{
	return this;
}

void
NetPacketHandStart::InternalCheck(const NetPacketHeader* data) const
{
	NetPacketHandStartData *tmpData = (NetPacketHandStartData *)data;
	if (ntohs(tmpData->yourCard1) > 51 || ntohs(tmpData->yourCard2) > 51)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketPlayersTurn::NetPacketPlayersTurn()
: NetPacket(NET_TYPE_PLAYERS_TURN, sizeof(NetPacketPlayersTurnData), sizeof(NetPacketPlayersTurnData))
{
}

NetPacketPlayersTurn::~NetPacketPlayersTurn()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayersTurn::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayersTurn);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketPlayersTurn::SetData(const NetPacketPlayersTurn::Data &inData)
{
	NetPacketPlayersTurnData *tmpData = (NetPacketPlayersTurnData *)GetRawData();

	tmpData->playerId	= htonl(inData.playerId);
	tmpData->gameState	= htons(inData.gameState);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketPlayersTurn::GetData(NetPacketPlayersTurn::Data &outData) const
{
	NetPacketPlayersTurnData *tmpData = (NetPacketPlayersTurnData *)GetRawData();

	outData.playerId	= ntohl(tmpData->playerId);
	outData.gameState	= static_cast<GameState>(ntohs(tmpData->gameState));
}

const NetPacketPlayersTurn *
NetPacketPlayersTurn::ToNetPacketPlayersTurn() const
{
	return this;
}

void
NetPacketPlayersTurn::InternalCheck(const NetPacketHeader* data) const
{
	// Check whether the state is valid.
	NetPacketPlayersTurnData *tmpData = (NetPacketPlayersTurnData *)data;
	if (ntohs(tmpData->gameState) > GAME_STATE_RIVER)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketPlayersAction::NetPacketPlayersAction()
: NetPacket(NET_TYPE_PLAYERS_ACTION, sizeof(NetPacketPlayersActionData), sizeof(NetPacketPlayersActionData))
{
}

NetPacketPlayersAction::~NetPacketPlayersAction()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayersAction::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayersAction);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketPlayersAction::SetData(const NetPacketPlayersAction::Data &inData)
{
	NetPacketPlayersActionData *tmpData = (NetPacketPlayersActionData *)GetRawData();

	tmpData->gameState		= htons(inData.gameState);
	tmpData->playerAction	= htons(inData.playerAction);
	tmpData->playerBet		= htonl(inData.playerBet);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketPlayersAction::GetData(NetPacketPlayersAction::Data &outData) const
{
	NetPacketPlayersActionData *tmpData = (NetPacketPlayersActionData *)GetRawData();

	outData.gameState		= static_cast<GameState>(ntohs(tmpData->gameState));
	outData.playerAction	= static_cast<PlayerAction>(ntohs(tmpData->playerAction));
	outData.playerBet		= ntohl(tmpData->playerBet);
}

const NetPacketPlayersAction *
NetPacketPlayersAction::ToNetPacketPlayersAction() const
{
	return this;
}

void
NetPacketPlayersAction::InternalCheck(const NetPacketHeader* data) const
{
	NetPacketPlayersActionData *tmpData = (NetPacketPlayersActionData *)data;
	// Check whether the state is valid.
	if (ntohs(tmpData->gameState) > GAME_STATE_RIVER)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check whether the player action is valid.
	if (ntohs(tmpData->playerAction) > PLAYER_ACTION_ALLIN)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Semantic checks are done by the server process.
}

//-----------------------------------------------------------------------------

NetPacketPlayersActionDone::NetPacketPlayersActionDone()
: NetPacket(NET_TYPE_PLAYERS_ACTION_DONE, sizeof(NetPacketPlayersActionDoneData), sizeof(NetPacketPlayersActionDoneData))
{
}

NetPacketPlayersActionDone::~NetPacketPlayersActionDone()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayersActionDone::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayersActionDone);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketPlayersActionDone::SetData(const NetPacketPlayersActionDone::Data &inData)
{
	NetPacketPlayersActionDoneData *tmpData = (NetPacketPlayersActionDoneData *)GetRawData();

	tmpData->playerId		= htonl(inData.playerId);
	tmpData->gameState		= htons(inData.gameState);
	tmpData->playerAction	= htons(inData.playerAction);
	tmpData->totalPlayerBet	= htonl(inData.totalPlayerBet);
	tmpData->playerMoney	= htonl(inData.playerMoney);
	tmpData->highestSet		= htonl(inData.highestSet);
	tmpData->minimumRaise	= htonl(inData.minimumRaise);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketPlayersActionDone::GetData(NetPacketPlayersActionDone::Data &outData) const
{
	NetPacketPlayersActionDoneData *tmpData = (NetPacketPlayersActionDoneData *)GetRawData();

	outData.playerId		= ntohl(tmpData->playerId);
	outData.gameState		= static_cast<GameState>(ntohs(tmpData->gameState));
	outData.playerAction	= static_cast<PlayerAction>(ntohs(tmpData->playerAction));
	outData.totalPlayerBet	= ntohl(tmpData->totalPlayerBet);
	outData.playerMoney		= ntohl(tmpData->playerMoney);
	outData.highestSet		= ntohl(tmpData->highestSet);
	outData.minimumRaise	= ntohl(tmpData->minimumRaise);
}

const NetPacketPlayersActionDone *
NetPacketPlayersActionDone::ToNetPacketPlayersActionDone() const
{
	return this;
}

void
NetPacketPlayersActionDone::InternalCheck(const NetPacketHeader* data) const
{
	NetPacketPlayersActionDoneData *tmpData = (NetPacketPlayersActionDoneData *)data;
	// Check whether the state is valid.
	int gameState = ntohs(tmpData->gameState);
	if (gameState > GAME_STATE_RIVER
		&& gameState != GAME_STATE_PREFLOP_SMALL_BLIND
		&& gameState != GAME_STATE_PREFLOP_BIG_BLIND)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check whether the player action is valid.
	if (ntohs(tmpData->playerAction) > PLAYER_ACTION_ALLIN)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketPlayersActionRejected::NetPacketPlayersActionRejected()
: NetPacket(NET_TYPE_PLAYERS_ACTION_REJECTED, sizeof(NetPacketPlayersActionRejectedData), sizeof(NetPacketPlayersActionRejectedData))
{
}

NetPacketPlayersActionRejected::~NetPacketPlayersActionRejected()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayersActionRejected::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayersActionRejected);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketPlayersActionRejected::SetData(const NetPacketPlayersActionRejected::Data &inData)
{
	NetPacketPlayersActionRejectedData *tmpData = (NetPacketPlayersActionRejectedData *)GetRawData();

	tmpData->gameState			= htons(inData.gameState);
	tmpData->playerAction		= htons(inData.playerAction);
	tmpData->playerBet			= htonl(inData.playerBet);
	tmpData->rejectionReason	= htons(inData.rejectionReason);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketPlayersActionRejected::GetData(NetPacketPlayersActionRejected::Data &outData) const
{
	NetPacketPlayersActionRejectedData *tmpData = (NetPacketPlayersActionRejectedData *)GetRawData();

	outData.gameState		= static_cast<GameState>(ntohs(tmpData->gameState));
	outData.playerAction	= static_cast<PlayerAction>(ntohs(tmpData->playerAction));
	outData.playerBet		= ntohl(tmpData->playerBet);
	outData.rejectionReason	= static_cast<PlayerActionCode>(ntohs(tmpData->rejectionReason));
}

const NetPacketPlayersActionRejected *
NetPacketPlayersActionRejected::ToNetPacketPlayersActionRejected() const
{
	return this;
}

void
NetPacketPlayersActionRejected::InternalCheck(const NetPacketHeader* data) const
{
	// Check whether the state is valid.
	NetPacketPlayersActionRejectedData *tmpData = (NetPacketPlayersActionRejectedData *)data;
	if (ntohs(tmpData->gameState) > GAME_STATE_RIVER)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check whether the player action is valid.
	if (ntohs(tmpData->playerAction) > PLAYER_ACTION_ALLIN)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	if (!ntohs(tmpData->rejectionReason))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketDealFlopCards::NetPacketDealFlopCards()
: NetPacket(NET_TYPE_DEAL_FLOP_CARDS, sizeof(NetPacketDealFlopCardsData), sizeof(NetPacketDealFlopCardsData))
{
}

NetPacketDealFlopCards::~NetPacketDealFlopCards()
{
}

boost::shared_ptr<NetPacket>
NetPacketDealFlopCards::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketDealFlopCards);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketDealFlopCards::SetData(const NetPacketDealFlopCards::Data &inData)
{
	NetPacketDealFlopCardsData *tmpData = (NetPacketDealFlopCardsData *)GetRawData();

	tmpData->flopCard1		= htons(inData.flopCards[0]);
	tmpData->flopCard2		= htons(inData.flopCards[1]);
	tmpData->flopCard3		= htons(inData.flopCards[2]);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketDealFlopCards::GetData(NetPacketDealFlopCards::Data &outData) const
{
	NetPacketDealFlopCardsData *tmpData = (NetPacketDealFlopCardsData *)GetRawData();

	outData.flopCards[0]		= ntohs(tmpData->flopCard1);
	outData.flopCards[1]		= ntohs(tmpData->flopCard2);
	outData.flopCards[2]		= ntohs(tmpData->flopCard3);
}

const NetPacketDealFlopCards *
NetPacketDealFlopCards::ToNetPacketDealFlopCards() const
{
	return this;
}

void
NetPacketDealFlopCards::InternalCheck(const NetPacketHeader* data) const
{
	NetPacketDealFlopCardsData *tmpData = (NetPacketDealFlopCardsData *)data;
	if (ntohs(tmpData->flopCard1) > 51 || ntohs(tmpData->flopCard2) > 51 || ntohs(tmpData->flopCard3) > 51)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketDealTurnCard::NetPacketDealTurnCard()
: NetPacket(NET_TYPE_DEAL_TURN_CARD, sizeof(NetPacketDealTurnCardData), sizeof(NetPacketDealTurnCardData))
{
}

NetPacketDealTurnCard::~NetPacketDealTurnCard()
{
}

boost::shared_ptr<NetPacket>
NetPacketDealTurnCard::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketDealTurnCard);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketDealTurnCard::SetData(const NetPacketDealTurnCard::Data &inData)
{
	NetPacketDealTurnCardData *tmpData = (NetPacketDealTurnCardData *)GetRawData();

	tmpData->turnCard			= htons(inData.turnCard);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketDealTurnCard::GetData(NetPacketDealTurnCard::Data &outData) const
{
	NetPacketDealTurnCardData *tmpData = (NetPacketDealTurnCardData *)GetRawData();

	outData.turnCard			= ntohs(tmpData->turnCard);
}

const NetPacketDealTurnCard *
NetPacketDealTurnCard::ToNetPacketDealTurnCard() const
{
	return this;
}

void
NetPacketDealTurnCard::InternalCheck(const NetPacketHeader* data) const
{
	NetPacketDealTurnCardData *tmpData = (NetPacketDealTurnCardData *)data;
	if (ntohs(tmpData->turnCard) > 51)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketDealRiverCard::NetPacketDealRiverCard()
: NetPacket(NET_TYPE_DEAL_RIVER_CARD, sizeof(NetPacketDealRiverCardData), sizeof(NetPacketDealRiverCardData))
{
}

NetPacketDealRiverCard::~NetPacketDealRiverCard()
{
}

boost::shared_ptr<NetPacket>
NetPacketDealRiverCard::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketDealRiverCard);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketDealRiverCard::SetData(const NetPacketDealRiverCard::Data &inData)
{
	NetPacketDealRiverCardData *tmpData = (NetPacketDealRiverCardData *)GetRawData();

	tmpData->riverCard			= htons(inData.riverCard);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketDealRiverCard::GetData(NetPacketDealRiverCard::Data &outData) const
{
	NetPacketDealRiverCardData *tmpData = (NetPacketDealRiverCardData *)GetRawData();

	outData.riverCard			= ntohs(tmpData->riverCard);
}

const NetPacketDealRiverCard *
NetPacketDealRiverCard::ToNetPacketDealRiverCard() const
{
	return this;
}

void
NetPacketDealRiverCard::InternalCheck(const NetPacketHeader* data) const
{
	NetPacketDealRiverCardData *tmpData = (NetPacketDealRiverCardData *)data;
	if (ntohs(tmpData->riverCard) > 51)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketAllInShowCards::NetPacketAllInShowCards()
: NetPacket(NET_TYPE_ALL_IN_SHOW_CARDS, sizeof(NetPacketAllInShowCardsData), MAX_PACKET_SIZE)
{
}

NetPacketAllInShowCards::~NetPacketAllInShowCards()
{
}

boost::shared_ptr<NetPacket>
NetPacketAllInShowCards::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketAllInShowCards);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketAllInShowCards::SetData(const NetPacketAllInShowCards::Data &inData)
{
	u_int16_t numPlayerCards = (u_int16_t)inData.playerCards.size();

	if (!numPlayerCards || numPlayerCards > MAX_NUM_PLAYER_CARDS)
		throw NetException(__FILE__, __LINE__, ERR_NET_INVALID_PLAYER_CARDS, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketAllInShowCardsData) + numPlayerCards * sizeof(PlayerCardsData)));

	NetPacketAllInShowCardsData *tmpData = (NetPacketAllInShowCardsData *)GetRawData();

	tmpData->numberOfPlayerCards		= htons(numPlayerCards);

	PlayerCardsList::const_iterator i = inData.playerCards.begin();
	PlayerCardsList::const_iterator end = inData.playerCards.end();

	// Copy the player cards data to continous memory
	PlayerCardsData *curPlayerCardsData =
		(PlayerCardsData *)((char *)tmpData + sizeof(NetPacketAllInShowCardsData));
	while (i != end)
	{
		curPlayerCardsData->playerId		= htonl((*i).playerId);
		curPlayerCardsData->card1			= htons((*i).cards[0]);
		curPlayerCardsData->card2			= htons((*i).cards[1]);
		++curPlayerCardsData;
		++i;
	}

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketAllInShowCards::GetData(NetPacketAllInShowCards::Data &outData) const
{
	NetPacketAllInShowCardsData *tmpData = (NetPacketAllInShowCardsData *)GetRawData();

	outData.playerCards.clear();

	u_int16_t numPlayerCards = ntohs(tmpData->numberOfPlayerCards);
	PlayerCardsData *curPlayerCardsData =
		(PlayerCardsData *)((char *)tmpData + sizeof(NetPacketAllInShowCardsData));

	// Store all available player cards.
	for (int i = 0; i < numPlayerCards; i++)
	{
		PlayerCards tmpPlayerCards;
		tmpPlayerCards.playerId		= ntohl(curPlayerCardsData->playerId);
		tmpPlayerCards.cards[0]		= ntohs(curPlayerCardsData->card1);
		tmpPlayerCards.cards[1]		= ntohs(curPlayerCardsData->card2);

		outData.playerCards.push_back(tmpPlayerCards);
		++curPlayerCardsData;
	}
}

const NetPacketAllInShowCards *
NetPacketAllInShowCards::ToNetPacketAllInShowCards() const
{
	return this;
}

void
NetPacketAllInShowCards::InternalCheck(const NetPacketHeader* data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketAllInShowCardsData *tmpData = (NetPacketAllInShowCardsData *)data;
	int numPlayerCards = ntohs(tmpData->numberOfPlayerCards);

	if (dataLen !=
		sizeof(NetPacketAllInShowCardsData)
		+ numPlayerCards * sizeof(PlayerCardsData))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	if (ntohs(tmpData->numberOfPlayerCards) > MAX_NUMBER_OF_PLAYERS)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Semantic checks not needed, this packet is sent by the server.
}

//-----------------------------------------------------------------------------

NetPacketEndOfHandShowCards::NetPacketEndOfHandShowCards()
: NetPacket(NET_TYPE_END_OF_HAND_SHOW_CARDS, sizeof(NetPacketEndOfHandShowCardsData), MAX_PACKET_SIZE)
{
}

NetPacketEndOfHandShowCards::~NetPacketEndOfHandShowCards()
{
}

boost::shared_ptr<NetPacket>
NetPacketEndOfHandShowCards::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketEndOfHandShowCards);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketEndOfHandShowCards::SetData(const NetPacketEndOfHandShowCards::Data &inData)
{
	u_int16_t numPlayerResults = (u_int16_t)inData.playerResults.size();

	if (!numPlayerResults || numPlayerResults > MAX_NUM_PLAYER_RESULTS)
		throw NetException(__FILE__, __LINE__, ERR_NET_INVALID_PLAYER_RESULTS, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketEndOfHandShowCardsData) + numPlayerResults * sizeof(PlayerResultData)));

	NetPacketEndOfHandShowCardsData *tmpData = (NetPacketEndOfHandShowCardsData *)GetRawData();

	tmpData->numberOfPlayerResults		= htons(numPlayerResults);

	PlayerResultList::const_iterator i = inData.playerResults.begin();
	PlayerResultList::const_iterator end = inData.playerResults.end();

	// Copy the player result data to continous memory
	PlayerResultData *curPlayerResultData =
		(PlayerResultData *)((char *)tmpData + sizeof(NetPacketEndOfHandShowCardsData));
	while (i != end)
	{
		curPlayerResultData->playerId		= htonl((*i).playerId);
		curPlayerResultData->card1			= htons((*i).cards[0]);
		curPlayerResultData->card2			= htons((*i).cards[1]);
		curPlayerResultData->bestHandPos1	= htons((*i).bestHandPos[0]);
		curPlayerResultData->bestHandPos2	= htons((*i).bestHandPos[1]);
		curPlayerResultData->bestHandPos3	= htons((*i).bestHandPos[2]);
		curPlayerResultData->bestHandPos4	= htons((*i).bestHandPos[3]);
		curPlayerResultData->bestHandPos5	= htons((*i).bestHandPos[4]);
		curPlayerResultData->valueOfCards	= htonl((*i).valueOfCards);
		curPlayerResultData->moneyWon		= htonl((*i).moneyWon);
		curPlayerResultData->playerMoney	= htonl((*i).playerMoney);
		++curPlayerResultData;
		++i;
	}

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketEndOfHandShowCards::GetData(NetPacketEndOfHandShowCards::Data &outData) const
{
	NetPacketEndOfHandShowCardsData *tmpData = (NetPacketEndOfHandShowCardsData *)GetRawData();

	outData.playerResults.clear();

	u_int16_t numPlayerResults = ntohs(tmpData->numberOfPlayerResults);
	PlayerResultData *curPlayerResultData =
		(PlayerResultData *)((char *)tmpData + sizeof(NetPacketEndOfHandShowCardsData));

	// Store all available player results.
	for (int i = 0; i < numPlayerResults; i++)
	{
		PlayerResult tmpPlayerResult;
		tmpPlayerResult.playerId		= ntohl(curPlayerResultData->playerId);
		tmpPlayerResult.cards[0]		= ntohs(curPlayerResultData->card1);
		tmpPlayerResult.cards[1]		= ntohs(curPlayerResultData->card2);
		tmpPlayerResult.bestHandPos[0]	= ntohs(curPlayerResultData->bestHandPos1);
		tmpPlayerResult.bestHandPos[1]	= ntohs(curPlayerResultData->bestHandPos2);
		tmpPlayerResult.bestHandPos[2]	= ntohs(curPlayerResultData->bestHandPos3);
		tmpPlayerResult.bestHandPos[3]	= ntohs(curPlayerResultData->bestHandPos4);
		tmpPlayerResult.bestHandPos[4]	= ntohs(curPlayerResultData->bestHandPos5);
		tmpPlayerResult.valueOfCards	= ntohl(curPlayerResultData->valueOfCards);
		tmpPlayerResult.moneyWon		= ntohl(curPlayerResultData->moneyWon);
		tmpPlayerResult.playerMoney		= ntohl(curPlayerResultData->playerMoney);

		outData.playerResults.push_back(tmpPlayerResult);
		++curPlayerResultData;
	}
}

const NetPacketEndOfHandShowCards *
NetPacketEndOfHandShowCards::ToNetPacketEndOfHandShowCards() const
{
	return this;
}

void
NetPacketEndOfHandShowCards::InternalCheck(const NetPacketHeader* data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketEndOfHandShowCardsData *tmpData = (NetPacketEndOfHandShowCardsData *)data;
	int numPlayerResults = ntohs(tmpData->numberOfPlayerResults);

	if (dataLen !=
		sizeof(NetPacketEndOfHandShowCardsData)
		+ numPlayerResults * sizeof(PlayerResultData))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	if (ntohs(tmpData->numberOfPlayerResults) > MAX_NUMBER_OF_PLAYERS)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Semantic checks not needed, this packet is sent by the server.
}

//-----------------------------------------------------------------------------

NetPacketEndOfHandHideCards::NetPacketEndOfHandHideCards()
: NetPacket(NET_TYPE_END_OF_HAND_HIDE_CARDS, sizeof(NetPacketEndOfHandHideCardsData), sizeof(NetPacketEndOfHandHideCardsData))
{
}

NetPacketEndOfHandHideCards::~NetPacketEndOfHandHideCards()
{
}

boost::shared_ptr<NetPacket>
NetPacketEndOfHandHideCards::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketEndOfHandHideCards);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketEndOfHandHideCards::SetData(const NetPacketEndOfHandHideCards::Data &inData)
{
	NetPacketEndOfHandHideCardsData *tmpData = (NetPacketEndOfHandHideCardsData *)GetRawData();

	tmpData->playerId			= htonl(inData.playerId);
	tmpData->moneyWon			= htonl(inData.moneyWon);
	tmpData->playerMoney		= htonl(inData.playerMoney);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketEndOfHandHideCards::GetData(NetPacketEndOfHandHideCards::Data &outData) const
{
	NetPacketEndOfHandHideCardsData *tmpData = (NetPacketEndOfHandHideCardsData *)GetRawData();

	outData.playerId			= ntohl(tmpData->playerId);
	outData.moneyWon			= ntohl(tmpData->moneyWon);
	outData.playerMoney			= ntohl(tmpData->playerMoney);
}

const NetPacketEndOfHandHideCards *
NetPacketEndOfHandHideCards::ToNetPacketEndOfHandHideCards() const
{
	return this;
}

void
NetPacketEndOfHandHideCards::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketEndOfGame::NetPacketEndOfGame()
: NetPacket(NET_TYPE_END_OF_GAME, sizeof(NetPacketEndOfGameData), sizeof(NetPacketEndOfGameData))
{
}

NetPacketEndOfGame::~NetPacketEndOfGame()
{
}

boost::shared_ptr<NetPacket>
NetPacketEndOfGame::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketEndOfGame);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketEndOfGame::SetData(const NetPacketEndOfGame::Data &inData)
{
	NetPacketEndOfGameData *tmpData = (NetPacketEndOfGameData *)GetRawData();

	tmpData->winnerPlayerId	= htonl(inData.winnerPlayerId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketEndOfGame::GetData(NetPacketEndOfGame::Data &outData) const
{
	NetPacketEndOfGameData *tmpData = (NetPacketEndOfGameData *)GetRawData();

	outData.winnerPlayerId	= ntohl(tmpData->winnerPlayerId);
}

const NetPacketEndOfGame *
NetPacketEndOfGame::ToNetPacketEndOfGame() const
{
	return this;
}

void
NetPacketEndOfGame::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketAskKickPlayer::NetPacketAskKickPlayer()
: NetPacket(NET_TYPE_ASK_KICK_PLAYER, sizeof(NetPacketAskKickPlayerData), sizeof(NetPacketAskKickPlayerData))
{
}

NetPacketAskKickPlayer::~NetPacketAskKickPlayer()
{
}

boost::shared_ptr<NetPacket>
NetPacketAskKickPlayer::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketAskKickPlayer);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketAskKickPlayer::SetData(const NetPacketAskKickPlayer::Data &inData)
{
	NetPacketAskKickPlayerData *tmpData = (NetPacketAskKickPlayerData *)GetRawData();

	tmpData->playerId	= htonl(inData.playerId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketAskKickPlayer::GetData(NetPacketAskKickPlayer::Data &outData) const
{
	NetPacketAskKickPlayerData *tmpData = (NetPacketAskKickPlayerData *)GetRawData();

	outData.playerId	= ntohl(tmpData->playerId);
}

const NetPacketAskKickPlayer *
NetPacketAskKickPlayer::ToNetPacketAskKickPlayer() const
{
	return this;
}

void
NetPacketAskKickPlayer::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketAskKickPlayerDenied::NetPacketAskKickPlayerDenied()
: NetPacket(NET_TYPE_ASK_KICK_PLAYER_DENIED, sizeof(NetPacketAskKickPlayerDeniedData), sizeof(NetPacketAskKickPlayerDeniedData))
{
}

NetPacketAskKickPlayerDenied::~NetPacketAskKickPlayerDenied()
{
}

boost::shared_ptr<NetPacket>
NetPacketAskKickPlayerDenied::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketAskKickPlayerDenied);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketAskKickPlayerDenied::SetData(const NetPacketAskKickPlayerDenied::Data &inData)
{
	NetPacketAskKickPlayerDeniedData *tmpData = (NetPacketAskKickPlayerDeniedData *)GetRawData();

	tmpData->playerId	= htonl(inData.playerId);
	switch (inData.denyReason)
	{
		case KICK_DENIED_INVALID_STATE:
			tmpData->denyReason = htons(NET_ASK_KICK_DENIED_INVALID_STATE);
			break;
		case KICK_DENIED_TOO_FEW_PLAYERS:
			tmpData->denyReason = htons(NET_ASK_KICK_DENIED_TOO_FEW_PLAYERS);
			break;
		case KICK_DENIED_TEMPORARY:
			tmpData->denyReason = htons(NET_ASK_KICK_DENIED_TEMPORARY);
			break;
		case KICK_DENIED_OTHER_IN_PROGRESS:
			tmpData->denyReason = htons(NET_ASK_KICK_DENIED_OTHER_IN_PROGRESS);
			break;
		default:
			tmpData->denyReason = htons(NET_ASK_KICK_DENIED_OTHER_REASON);
			break;
	}

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketAskKickPlayerDenied::GetData(NetPacketAskKickPlayerDenied::Data &outData) const
{
	NetPacketAskKickPlayerDeniedData *tmpData = (NetPacketAskKickPlayerDeniedData *)GetRawData();

	outData.playerId	= ntohl(tmpData->playerId);
	switch (ntohs(tmpData->denyReason))
	{
		case NET_ASK_KICK_DENIED_INVALID_STATE:
			outData.denyReason = KICK_DENIED_INVALID_STATE;
			break;
		case NET_ASK_KICK_DENIED_TOO_FEW_PLAYERS:
			outData.denyReason = KICK_DENIED_TOO_FEW_PLAYERS;
			break;
		case NET_ASK_KICK_DENIED_TEMPORARY:
			outData.denyReason = KICK_DENIED_TEMPORARY;
			break;
		case NET_ASK_KICK_DENIED_OTHER_IN_PROGRESS:
			outData.denyReason = KICK_DENIED_OTHER_IN_PROGRESS;
			break;
		default:
			outData.denyReason = KICK_DENIED_OTHER_REASON;
			break;
	}
}

const NetPacketAskKickPlayerDenied *
NetPacketAskKickPlayerDenied::ToNetPacketAskKickPlayerDenied() const
{
	return this;
}

void
NetPacketAskKickPlayerDenied::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketStartKickPlayerPetition::NetPacketStartKickPlayerPetition()
: NetPacket(NET_TYPE_START_KICK_PLAYER_PETITION, sizeof(NetPacketStartKickPlayerPetitionData), sizeof(NetPacketStartKickPlayerPetitionData))
{
}

NetPacketStartKickPlayerPetition::~NetPacketStartKickPlayerPetition()
{
}

boost::shared_ptr<NetPacket>
NetPacketStartKickPlayerPetition::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketStartKickPlayerPetition);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketStartKickPlayerPetition::SetData(const NetPacketStartKickPlayerPetition::Data &inData)
{
	NetPacketStartKickPlayerPetitionData *tmpData = (NetPacketStartKickPlayerPetitionData *)GetRawData();

	tmpData->petitionId				= htonl(inData.petitionId);
	tmpData->proposingPlayerId		= htonl(inData.proposingPlayerId);
	tmpData->kickPlayerId			= htonl(inData.kickPlayerId);
	tmpData->kickTimeout			= htons(inData.kickTimeoutSec);
	tmpData->numVotesNeededToKick	= htons(inData.numVotesNeededToKick);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketStartKickPlayerPetition::GetData(NetPacketStartKickPlayerPetition::Data &outData) const
{
	NetPacketStartKickPlayerPetitionData *tmpData = (NetPacketStartKickPlayerPetitionData *)GetRawData();

	outData.petitionId				= ntohl(tmpData->petitionId);
	outData.proposingPlayerId		= ntohl(tmpData->proposingPlayerId);
	outData.kickPlayerId			= ntohl(tmpData->kickPlayerId);
	outData.kickTimeoutSec			= ntohs(tmpData->kickTimeout);
	outData.numVotesNeededToKick	= ntohs(tmpData->numVotesNeededToKick);
}

const NetPacketStartKickPlayerPetition *
NetPacketStartKickPlayerPetition::ToNetPacketStartKickPlayerPetition() const
{
	return this;
}

void
NetPacketStartKickPlayerPetition::InternalCheck(const NetPacketHeader *data) const
{
	NetPacketStartKickPlayerPetitionData *tmpData = (NetPacketStartKickPlayerPetitionData *)data;

	if (ntohs(tmpData->numVotesNeededToKick) > MAX_NUMBER_OF_PLAYERS - 1)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
}

//-----------------------------------------------------------------------------

NetPacketVoteKickPlayer::NetPacketVoteKickPlayer()
: NetPacket(NET_TYPE_VOTE_KICK_PLAYER, sizeof(NetPacketVoteKickPlayerData), sizeof(NetPacketVoteKickPlayerData))
{
}

NetPacketVoteKickPlayer::~NetPacketVoteKickPlayer()
{
}

boost::shared_ptr<NetPacket>
NetPacketVoteKickPlayer::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketVoteKickPlayer);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketVoteKickPlayer::SetData(const NetPacketVoteKickPlayer::Data &inData)
{
	NetPacketVoteKickPlayerData *tmpData = (NetPacketVoteKickPlayerData *)GetRawData();

	tmpData->petitionId	= htonl(inData.petitionId);
	tmpData->vote = htons(static_cast<u_int16_t>(inData.vote));

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketVoteKickPlayer::GetData(NetPacketVoteKickPlayer::Data &outData) const
{
	NetPacketVoteKickPlayerData *tmpData = (NetPacketVoteKickPlayerData *)GetRawData();

	outData.petitionId	= ntohl(tmpData->petitionId);
	if (ntohs(tmpData->vote) == NET_VOTE_KICK_IN_FAVOUR)
		outData.vote = KICK_VOTE_IN_FAVOUR;
	else
		outData.vote = KICK_VOTE_AGAINST;
}

const NetPacketVoteKickPlayer *
NetPacketVoteKickPlayer::ToNetPacketVoteKickPlayer() const
{
	return this;
}

void
NetPacketVoteKickPlayer::InternalCheck(const NetPacketHeader *data) const
{
	NetPacketVoteKickPlayerData *tmpData = (NetPacketVoteKickPlayerData *)data;
	u_int16_t tmpVote = ntohs(tmpData->vote);
	if (tmpVote != 0 && tmpVote != 1)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
}

//-----------------------------------------------------------------------------

NetPacketVoteKickPlayerAck::NetPacketVoteKickPlayerAck()
: NetPacket(NET_TYPE_VOTE_KICK_PLAYER_ACK, sizeof(NetPacketVoteKickPlayerAckData), sizeof(NetPacketVoteKickPlayerAckData))
{
}

NetPacketVoteKickPlayerAck::~NetPacketVoteKickPlayerAck()
{
}

boost::shared_ptr<NetPacket>
NetPacketVoteKickPlayerAck::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketVoteKickPlayerAck);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketVoteKickPlayerAck::SetData(const NetPacketVoteKickPlayerAck::Data &inData)
{
	NetPacketVoteKickPlayerAckData *tmpData = (NetPacketVoteKickPlayerAckData *)GetRawData();

	tmpData->petitionId	= htonl(inData.petitionId);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketVoteKickPlayerAck::GetData(NetPacketVoteKickPlayerAck::Data &outData) const
{
	NetPacketVoteKickPlayerAckData *tmpData = (NetPacketVoteKickPlayerAckData *)GetRawData();

	outData.petitionId	= ntohl(tmpData->petitionId);
}

const NetPacketVoteKickPlayerAck *
NetPacketVoteKickPlayerAck::ToNetPacketVoteKickPlayerAck() const
{
	return this;
}

void
NetPacketVoteKickPlayerAck::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketVoteKickPlayerDenied::NetPacketVoteKickPlayerDenied()
: NetPacket(NET_TYPE_VOTE_KICK_PLAYER_DENIED, sizeof(NetPacketVoteKickPlayerDeniedData), sizeof(NetPacketVoteKickPlayerDeniedData))
{
}

NetPacketVoteKickPlayerDenied::~NetPacketVoteKickPlayerDenied()
{
}

boost::shared_ptr<NetPacket>
NetPacketVoteKickPlayerDenied::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketVoteKickPlayerDenied);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketVoteKickPlayerDenied::SetData(const NetPacketVoteKickPlayerDenied::Data &inData)
{
	NetPacketVoteKickPlayerDeniedData *tmpData = (NetPacketVoteKickPlayerDeniedData *)GetRawData();

	tmpData->petitionId	= htonl(inData.petitionId);
	switch (inData.denyReason)
	{
		case VOTE_DENIED_INVALID_PETITION:
			tmpData->denyReason = htons(NET_VOTE_KICK_DENIED_INVALID_PETITION);
			break;
		case VOTE_DENIED_ALREADY_VOTED:
			tmpData->denyReason = htons(NET_VOTE_KICK_DENIED_ALREADY_VOTED);
			break;
		case VOTE_DENIED_IMPOSSIBLE:
			tmpData->denyReason = htons(NET_VOTE_KICK_DENIED_IMPOSSIBLE);
			break;
		default:
			tmpData->denyReason = htons(NET_VOTE_KICK_DENIED_OTHER_REASON);
			break;
	}

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketVoteKickPlayerDenied::GetData(NetPacketVoteKickPlayerDenied::Data &outData) const
{
	NetPacketVoteKickPlayerDeniedData *tmpData = (NetPacketVoteKickPlayerDeniedData *)GetRawData();

	outData.petitionId	= ntohl(tmpData->petitionId);
	switch (ntohs(tmpData->denyReason))
	{
		case NET_VOTE_KICK_DENIED_INVALID_PETITION:
			outData.denyReason = VOTE_DENIED_INVALID_PETITION;
			break;
		case NET_VOTE_KICK_DENIED_ALREADY_VOTED:
			outData.denyReason = VOTE_DENIED_ALREADY_VOTED;
			break;
		case NET_VOTE_KICK_DENIED_IMPOSSIBLE:
			outData.denyReason = VOTE_DENIED_IMPOSSIBLE;
			break;
		default:
			outData.denyReason = VOTE_DENIED_OTHER_REASON;
			break;
	}
}

const NetPacketVoteKickPlayerDenied *
NetPacketVoteKickPlayerDenied::ToNetPacketVoteKickPlayerDenied() const
{
	return this;
}

void
NetPacketVoteKickPlayerDenied::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketKickPlayerPetitionUpdate::NetPacketKickPlayerPetitionUpdate()
: NetPacket(NET_TYPE_KICK_PLAYER_PETITION_UPDATE, sizeof(NetPacketKickPlayerPetitionUpdateData), sizeof(NetPacketKickPlayerPetitionUpdateData))
{
}

NetPacketKickPlayerPetitionUpdate::~NetPacketKickPlayerPetitionUpdate()
{
}

boost::shared_ptr<NetPacket>
NetPacketKickPlayerPetitionUpdate::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketKickPlayerPetitionUpdate);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketKickPlayerPetitionUpdate::SetData(const NetPacketKickPlayerPetitionUpdate::Data &inData)
{
	NetPacketKickPlayerPetitionUpdateData *tmpData = (NetPacketKickPlayerPetitionUpdateData *)GetRawData();

	tmpData->petitionId					= htonl(inData.petitionId);
	tmpData->numVotesAgainstKicking		= htons(inData.numVotesAgainstKicking);
	tmpData->numVotesInFavourOfKicking	= htons(inData.numVotesInFavourOfKicking);
	tmpData->numVotesNeededToKick		= htons(inData.numVotesNeededToKick);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketKickPlayerPetitionUpdate::GetData(NetPacketKickPlayerPetitionUpdate::Data &outData) const
{
	NetPacketKickPlayerPetitionUpdateData *tmpData = (NetPacketKickPlayerPetitionUpdateData *)GetRawData();

	outData.petitionId					= ntohl(tmpData->petitionId);
	outData.numVotesAgainstKicking		= ntohs(tmpData->numVotesAgainstKicking);
	outData.numVotesInFavourOfKicking	= ntohs(tmpData->numVotesInFavourOfKicking);
	outData.numVotesNeededToKick		= ntohs(tmpData->numVotesNeededToKick);
}

const NetPacketKickPlayerPetitionUpdate *
NetPacketKickPlayerPetitionUpdate::ToNetPacketKickPlayerPetitionUpdate() const
{
	return this;
}

void
NetPacketKickPlayerPetitionUpdate::InternalCheck(const NetPacketHeader *data) const
{
	NetPacketKickPlayerPetitionUpdateData *tmpData = (NetPacketKickPlayerPetitionUpdateData *)data;

	if (ntohs(tmpData->numVotesAgainstKicking) > MAX_NUMBER_OF_PLAYERS)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	if (ntohs(tmpData->numVotesInFavourOfKicking) > MAX_NUMBER_OF_PLAYERS)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	if (ntohs(tmpData->numVotesNeededToKick) > MAX_NUMBER_OF_PLAYERS)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
}

//-----------------------------------------------------------------------------

NetPacketEndKickPlayerPetition::NetPacketEndKickPlayerPetition()
: NetPacket(NET_TYPE_END_KICK_PLAYER_PETITION, sizeof(NetPacketEndKickPlayerPetitionData), sizeof(NetPacketEndKickPlayerPetitionData))
{
}

NetPacketEndKickPlayerPetition::~NetPacketEndKickPlayerPetition()
{
}

boost::shared_ptr<NetPacket>
NetPacketEndKickPlayerPetition::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketEndKickPlayerPetition);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketEndKickPlayerPetition::SetData(const NetPacketEndKickPlayerPetition::Data &inData)
{
	NetPacketEndKickPlayerPetitionData *tmpData = (NetPacketEndKickPlayerPetitionData *)GetRawData();

	tmpData->petitionId					= htonl(inData.petitionId);
	tmpData->numVotesAgainstKicking		= htons(inData.numVotesAgainstKicking);
	tmpData->numVotesInFavourOfKicking	= htons(inData.numVotesInFavourOfKicking);
	tmpData->voteResult					= htons(inData.playerKicked ? 1 : 0);

	switch (inData.endReason)
	{
		case PETITION_END_ENOUGH_VOTES:
			tmpData->endReason = htons(NET_PETITION_END_ENOUGH_VOTES);
			break;
		case PETITION_END_NOT_ENOUGH_PLAYERS:
			tmpData->endReason = htons(NET_PETITION_END_NOT_ENOUGH_PLAYERS);
			break;
		case PETITION_END_PLAYER_LEFT:
			tmpData->endReason = htons(NET_PETITION_END_PLAYER_LEFT);
			break;
		case PETITION_END_TIMEOUT:
			tmpData->endReason = htons(NET_PETITION_END_TIMEOUT);
			break;
		default:
			throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketEndKickPlayerPetition::GetData(NetPacketEndKickPlayerPetition::Data &outData) const
{
	NetPacketEndKickPlayerPetitionData *tmpData = (NetPacketEndKickPlayerPetitionData *)GetRawData();

	outData.petitionId					= ntohl(tmpData->petitionId);
	outData.numVotesAgainstKicking		= ntohs(tmpData->numVotesAgainstKicking);
	outData.numVotesInFavourOfKicking	= ntohs(tmpData->numVotesInFavourOfKicking);
	outData.playerKicked				= ntohs(tmpData->voteResult) == 1;

	switch (ntohs(tmpData->endReason))
	{
		case NET_PETITION_END_ENOUGH_VOTES:
			outData.endReason = PETITION_END_ENOUGH_VOTES;
			break;
		case NET_PETITION_END_NOT_ENOUGH_PLAYERS:
			outData.endReason = PETITION_END_NOT_ENOUGH_PLAYERS;
			break;
		case NET_PETITION_END_PLAYER_LEFT:
			outData.endReason = PETITION_END_PLAYER_LEFT;
			break;
		case NET_PETITION_END_TIMEOUT:
			outData.endReason = PETITION_END_TIMEOUT;
			break;
		default:
			throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

const NetPacketEndKickPlayerPetition *
NetPacketEndKickPlayerPetition::ToNetPacketEndKickPlayerPetition() const
{
	return this;
}

void
NetPacketEndKickPlayerPetition::InternalCheck(const NetPacketHeader *data) const
{
	NetPacketEndKickPlayerPetitionData *tmpData = (NetPacketEndKickPlayerPetitionData *)data;

	if (ntohs(tmpData->numVotesAgainstKicking) > MAX_NUMBER_OF_PLAYERS)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	if (ntohs(tmpData->numVotesInFavourOfKicking) > MAX_NUMBER_OF_PLAYERS)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	u_int16_t tmpVoteResult = ntohs(tmpData->voteResult);
	if (tmpVoteResult != 0 && tmpVoteResult != 1)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
}

//-----------------------------------------------------------------------------

NetPacketStatisticsChanged::NetPacketStatisticsChanged()
: NetPacket(NET_TYPE_STATISTICS_CHANGED, sizeof(NetPacketStatisticsChangedData), MAX_PACKET_SIZE)
{
}

NetPacketStatisticsChanged::~NetPacketStatisticsChanged()
{
}

boost::shared_ptr<NetPacket>
NetPacketStatisticsChanged::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketStatisticsChanged);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketStatisticsChanged::SetData(const NetPacketStatisticsChanged::Data &inData)
{
	// Only send number of players.
//	u_int16_t numValues = 0;
	u_int16_t numValues = 1;
/*	if (inData.stats.numberOfPlayersOnServer)
		++numValues;
	if (inData.stats.totalPlayersEverLoggedIn)
		++numValues;
	if (inData.stats.totalGamesEverCreated)
		++numValues;*/

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketStatisticsChangedData) + numValues * sizeof(StatisticsData)));

	NetPacketStatisticsChangedData *tmpData = (NetPacketStatisticsChangedData *)GetRawData();

	tmpData->numberOfStatistics			= htons(numValues);

	StatisticsData *curStatisticsData =
		(StatisticsData *)((char *)tmpData + sizeof(NetPacketStatisticsChangedData));

//	if (inData.stats.numberOfPlayersOnServer)
//	{
		curStatisticsData->statisticsType	= htonl(NET_STAT_CUR_PLAYERS_ON_SERVER);
		curStatisticsData->statisticsValue	= htonl(inData.stats.numberOfPlayersOnServer);
		++curStatisticsData;
//	}
/*	if (inData.stats.totalPlayersEverLoggedIn)
	{
		curStatisticsData->statisticsType	= htonl(NET_STAT_TOTAL_PLAYERS_EVER_ON_SERVER);
		curStatisticsData->statisticsValue	= htonl(inData.stats.totalPlayersEverLoggedIn);
		++curStatisticsData;
	}
	if (inData.stats.totalGamesEverCreated)
	{
		curStatisticsData->statisticsType	= htonl(NET_STAT_TOTAL_GAMES_EVER_ON_SERVER);
		curStatisticsData->statisticsValue	= htonl(inData.stats.totalGamesEverCreated);
	}*/

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketStatisticsChanged::GetData(NetPacketStatisticsChanged::Data &outData) const
{
	NetPacketStatisticsChangedData *tmpData = (NetPacketStatisticsChangedData *)GetRawData();

	u_int16_t numStatistics = ntohs(tmpData->numberOfStatistics);
	StatisticsData *curStatisticsData =
		(StatisticsData *)((char *)tmpData + sizeof(NetPacketStatisticsChangedData));

	for (int i = 0; i < numStatistics; i++)
	{
		switch(ntohl(curStatisticsData->statisticsType))
		{
			case NET_STAT_CUR_PLAYERS_ON_SERVER:
				outData.stats.numberOfPlayersOnServer = ntohl(curStatisticsData->statisticsValue);
				break;
			case NET_STAT_TOTAL_PLAYERS_EVER_ON_SERVER:
				outData.stats.totalPlayersEverLoggedIn = ntohl(curStatisticsData->statisticsValue);
				break;
			case NET_STAT_TOTAL_GAMES_EVER_ON_SERVER:
				outData.stats.totalGamesEverCreated = ntohl(curStatisticsData->statisticsValue);
				break;
		}
	}
}

const NetPacketStatisticsChanged *
NetPacketStatisticsChanged::ToNetPacketStatisticsChanged() const
{
	return this;
}

void
NetPacketStatisticsChanged::InternalCheck(const NetPacketHeader *data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketStatisticsChangedData *tmpData = (NetPacketStatisticsChangedData *)data;
	int numStatistics = ntohs(tmpData->numberOfStatistics);

	if (dataLen !=
		sizeof(NetPacketStatisticsChangedData)
		+ numStatistics * sizeof(StatisticsData))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	if (!numStatistics)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Semantic checks not needed, this packet is sent by the server.
}

//-----------------------------------------------------------------------------

NetPacketRemovedFromGame::NetPacketRemovedFromGame()
: NetPacket(NET_TYPE_REMOVED_FROM_GAME, sizeof(NetPacketRemovedFromGameData), sizeof(NetPacketRemovedFromGameData))
{
}

NetPacketRemovedFromGame::~NetPacketRemovedFromGame()
{
}

boost::shared_ptr<NetPacket>
NetPacketRemovedFromGame::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketRemovedFromGame);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketRemovedFromGame::SetData(const NetPacketRemovedFromGame::Data &inData)
{
	NetPacketRemovedFromGameData *tmpData = (NetPacketRemovedFromGameData *)GetRawData();

	switch (inData.removeReason)
	{
		case NTF_NET_REMOVED_ON_REQUEST :
			tmpData->removeReason = htons(NET_REMOVED_ON_REQUEST);
			break;
		case NTF_NET_REMOVED_GAME_FULL :
			tmpData->removeReason = htons(NET_REMOVED_GAME_FULL);
			break;
		case NTF_NET_REMOVED_ALREADY_RUNNING :
			tmpData->removeReason = htons(NET_REMOVED_GAME_ALREADY_RUNNING);
			break;
		case NTF_NET_REMOVED_KICKED :
			tmpData->removeReason = htons(NET_REMOVED_KICKED);
			break;
		case NTF_NET_REMOVED_TIMEOUT :
			tmpData->removeReason = htons(NET_REMOVED_TIMEOUT);
			break;
		case NTF_NET_REMOVED_START_FAILED :
			tmpData->removeReason = htons(NET_REMOVED_START_FAILED);
			break;
		default :
			tmpData->removeReason = htons(NET_REMOVED_OTHER_REASON);
			break;
	}

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketRemovedFromGame::GetData(NetPacketRemovedFromGame::Data &outData) const
{
	NetPacketRemovedFromGameData *tmpData = (NetPacketRemovedFromGameData *)GetRawData();

	switch (ntohs(tmpData->removeReason))
	{
	// Removed Errors.
		case NET_REMOVED_ON_REQUEST :
			outData.removeReason = NTF_NET_REMOVED_ON_REQUEST;
			break;
		case NET_REMOVED_GAME_FULL :
			outData.removeReason = NTF_NET_REMOVED_GAME_FULL;
			break;
		case NET_REMOVED_GAME_ALREADY_RUNNING :
			outData.removeReason = NTF_NET_REMOVED_ALREADY_RUNNING;
			break;
		case NET_REMOVED_KICKED :
			outData.removeReason = NTF_NET_REMOVED_KICKED;
			break;
		case NET_REMOVED_TIMEOUT :
			outData.removeReason = NTF_NET_REMOVED_TIMEOUT;
			break;
		case NET_REMOVED_START_FAILED :
			outData.removeReason = NTF_NET_REMOVED_START_FAILED;
			break;
		default :
			outData.removeReason = NTF_NET_INTERNAL;
			break;
	}
}

const NetPacketRemovedFromGame *
NetPacketRemovedFromGame::ToNetPacketRemovedFromGame() const
{
	return this;
}

void
NetPacketRemovedFromGame::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketTimeoutWarning::NetPacketTimeoutWarning()
: NetPacket(NET_TYPE_TIMEOUT_WARNING, sizeof(NetPacketTimeoutWarningData), sizeof(NetPacketTimeoutWarningData))
{
}

NetPacketTimeoutWarning::~NetPacketTimeoutWarning()
{
}

boost::shared_ptr<NetPacket>
NetPacketTimeoutWarning::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketTimeoutWarning);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketTimeoutWarning::SetData(const NetPacketTimeoutWarning::Data &inData)
{
	NetPacketTimeoutWarningData *tmpData = (NetPacketTimeoutWarningData *)GetRawData();

	switch (inData.timeoutReason)
	{
		case NETWORK_TIMEOUT_GENERIC:
			tmpData->timeoutReason = htons(NET_TIMEOUT_NO_DATA_RECEIVED);
			break;
		case NETWORK_TIMEOUT_GAME_ADMIN_IDLE:
			tmpData->timeoutReason = htons(NET_TIMEOUT_INACTIVE_GAME);
			break;
		default :
			tmpData->timeoutReason = htons(NET_TIMEOUT_OTHER_REASON);
			break;
	}
	tmpData->remainingSeconds = htons(inData.remainingSeconds);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketTimeoutWarning::GetData(NetPacketTimeoutWarning::Data &outData) const
{
	NetPacketTimeoutWarningData *tmpData = (NetPacketTimeoutWarningData *)GetRawData();

	switch (ntohs(tmpData->timeoutReason))
	{
	// Removed Errors.
		case NET_TIMEOUT_INACTIVE_GAME :
			outData.timeoutReason = NETWORK_TIMEOUT_GAME_ADMIN_IDLE;
			break;
		default :
			outData.timeoutReason = NETWORK_TIMEOUT_GENERIC;
			break;
	}
	outData.remainingSeconds = ntohs(tmpData->remainingSeconds);
}

const NetPacketTimeoutWarning *
NetPacketTimeoutWarning::ToNetPacketTimeoutWarning() const
{
	return this;
}

void
NetPacketTimeoutWarning::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketResetTimeout::NetPacketResetTimeout()
: NetPacket(NET_TYPE_RESET_TIMEOUT, sizeof(NetPacketResetTimeoutData), sizeof(NetPacketResetTimeoutData))
{
}

NetPacketResetTimeout::~NetPacketResetTimeout()
{
}

boost::shared_ptr<NetPacket>
NetPacketResetTimeout::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketResetTimeout);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

const NetPacketResetTimeout *
NetPacketResetTimeout::ToNetPacketResetTimeout() const
{
	return this;
}

void
NetPacketResetTimeout::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

NetPacketSendChatText::NetPacketSendChatText()
: NetPacket(NET_TYPE_SEND_CHAT_TEXT, sizeof(NetPacketSendChatTextData), MAX_PACKET_SIZE)
{
}

NetPacketSendChatText::~NetPacketSendChatText()
{
}

boost::shared_ptr<NetPacket>
NetPacketSendChatText::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketSendChatText);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketSendChatText::SetData(const NetPacketSendChatText::Data &inData)
{
	u_int16_t textLen = (u_int16_t)inData.text.length();

	if (!textLen || textLen > MAX_CHAT_TEXT_SIZE)
		throw NetException(__FILE__, __LINE__, ERR_NET_INVALID_CHAT_TEXT, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketSendChatTextData) + ADD_PADDING(textLen)));

	NetPacketSendChatTextData *tmpData = (NetPacketSendChatTextData *)GetRawData();

	// Set the data.
	tmpData->textLength = htons(textLen);
	char *textPtr = (char *)tmpData + sizeof(NetPacketSendChatTextData);
	memcpy(textPtr, inData.text.c_str(), textLen);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketSendChatText::GetData(NetPacketSendChatText::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketSendChatTextData *tmpData = (NetPacketSendChatTextData *)GetRawData();

	char *textPtr = (char *)tmpData + sizeof(NetPacketSendChatTextData);
	outData.text = string(textPtr, ntohs(tmpData->textLength));
}

const NetPacketSendChatText *
NetPacketSendChatText::ToNetPacketSendChatText() const
{
	return this;
}

void
NetPacketSendChatText::InternalCheck(const NetPacketHeader* data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketSendChatTextData *tmpData = (NetPacketSendChatTextData *)data;
	int textLength = ntohs(tmpData->textLength);
	// Check exact packet length.
	if (dataLen !=
		sizeof(NetPacketSendChatTextData)
		+ ADD_PADDING(textLength))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check string size.
	if (!textLength
		|| textLength > MAX_CHAT_TEXT_SIZE)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check text string.
	char *textPtr = (char *)tmpData + sizeof(NetPacketSendChatTextData);
	if (textPtr[0] == 0)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketChatText::NetPacketChatText()
: NetPacket(NET_TYPE_CHAT_TEXT, sizeof(NetPacketChatTextData), MAX_PACKET_SIZE)
{
}

NetPacketChatText::~NetPacketChatText()
{
}

boost::shared_ptr<NetPacket>
NetPacketChatText::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketChatText);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketChatText::SetData(const NetPacketChatText::Data &inData)
{
	u_int16_t textLen = (u_int16_t)inData.text.length();

	if (!textLen || textLen > MAX_CHAT_TEXT_SIZE)
		throw NetException(__FILE__, __LINE__, ERR_NET_INVALID_CHAT_TEXT, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketChatTextData) + ADD_PADDING(textLen)));

	NetPacketChatTextData *tmpData = (NetPacketChatTextData *)GetRawData();

	// Set the data.
	tmpData->playerId = htonl(inData.playerId);
	tmpData->textLength = htons(textLen);
	char *textPtr = (char *)tmpData + sizeof(NetPacketChatTextData);
	memcpy(textPtr, inData.text.c_str(), textLen);

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketChatText::GetData(NetPacketChatText::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketChatTextData *tmpData = (NetPacketChatTextData *)GetRawData();

	outData.playerId = ntohl(tmpData->playerId);
	char *textPtr = (char *)tmpData + sizeof(NetPacketChatTextData);
	outData.text = string(textPtr, ntohs(tmpData->textLength));
}

const NetPacketChatText *
NetPacketChatText::ToNetPacketChatText() const
{
	return this;
}

void
NetPacketChatText::InternalCheck(const NetPacketHeader* data) const
{
	u_int16_t dataLen = ntohs(data->length);
	NetPacketChatTextData *tmpData = (NetPacketChatTextData *)data;
	int textLength = ntohs(tmpData->textLength);
	// Check exact packet length.
	if (dataLen !=
		sizeof(NetPacketChatTextData)
		+ ADD_PADDING(textLength))
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check string size.
	if (!textLength
		|| textLength > MAX_CHAT_TEXT_SIZE)
	{
		throw NetException(__FILE__, __LINE__, ERR_SOCK_INVALID_PACKET, 0);
	}
	// No more checks required - this packet is sent by the server.
}

//-----------------------------------------------------------------------------

NetPacketError::NetPacketError()
: NetPacket(NET_TYPE_ERROR, sizeof(NetPacketErrorData), sizeof(NetPacketErrorData))
{
}

NetPacketError::~NetPacketError()
{
}

boost::shared_ptr<NetPacket>
NetPacketError::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketError);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketError::SetData(const NetPacketError::Data &inData)
{
	NetPacketErrorData *tmpData = (NetPacketErrorData *)GetRawData();

	switch (inData.errorCode)
	{
	// Init Errors.
		case ERR_NET_VERSION_NOT_SUPPORTED :
			tmpData->errorReason = htons(NET_ERR_INIT_VERSION_NOT_SUPPORTED);
			break;
		case ERR_NET_SERVER_FULL :
			tmpData->errorReason = htons(NET_ERR_INIT_SERVER_FULL);
			break;
		case ERR_NET_INVALID_PASSWORD :
			tmpData->errorReason = htons(NET_ERR_INIT_INVALID_PASSWORD);
			break;
		case ERR_NET_PLAYER_NAME_IN_USE :
			tmpData->errorReason = htons(NET_ERR_INIT_PLAYER_NAME_IN_USE);
			break;
		case ERR_NET_INVALID_PLAYER_NAME :
			tmpData->errorReason = htons(NET_ERR_INIT_INVALID_PLAYER_NAME);
			break;
		case ERR_NET_SERVER_MAINTENANCE :
			tmpData->errorReason = htons(NET_ERR_INIT_SERVER_MAINTENANCE);
			break;
		case ERR_NET_AVATAR_TOO_LARGE :
			tmpData->errorReason = htons(NET_ERR_AVATAR_TOO_LARGE);
			break;
		case ERR_NET_WRONG_AVATAR_SIZE :
			tmpData->errorReason = htons(NET_ERR_AVATAR_WRONG_SIZE);
			break;
		case ERR_NET_AVATAR_UPLOAD_BLOCKED :
			tmpData->errorReason = htons(NET_ERR_AVATAR_UPLOAD_BLOCKED);
			break;
		case ERR_NET_UNKNOWN_GAME :
			tmpData->errorReason = htons(NET_ERR_JOIN_GAME_UNKNOWN_GAME);
			break;

		// General Errors.
		case ERR_SOCK_INVALID_PACKET :
			tmpData->errorReason = htons(NET_ERR_GENERAL_INVALID_PACKET);
			break;
		case ERR_SOCK_INVALID_STATE :
			tmpData->errorReason = htons(NET_ERR_GENERAL_INVALID_STATE);
			break;
		case ERR_NET_PLAYER_KICKED :
			tmpData->errorReason = htons(NET_ERR_GENERAL_PLAYER_KICKED);
			break;
		case ERR_NET_PLAYER_BANNED :
			tmpData->errorReason = htons(NET_ERR_GENERAL_PLAYER_BANNED);
			break;
		case ERR_NET_SESSION_TIMED_OUT :
			tmpData->errorReason = htons(NET_ERR_GENERAL_SESSION_TIMED_OUT);
			break;
		default :
			tmpData->errorReason = htons(NET_ERR_OTHER);
			break;
	}

	// Check the packet - just in case.
	Check(GetRawData());
}

void
NetPacketError::GetData(NetPacketError::Data &outData) const
{
	NetPacketErrorData *tmpData = (NetPacketErrorData *)GetRawData();

	switch (ntohs(tmpData->errorReason))
	{
	// Init Errors.
		case NET_ERR_INIT_VERSION_NOT_SUPPORTED :
			outData.errorCode = ERR_NET_VERSION_NOT_SUPPORTED;
			break;
		case NET_ERR_INIT_SERVER_FULL :
			outData.errorCode = ERR_NET_SERVER_FULL;
			break;
		case NET_ERR_INIT_INVALID_PASSWORD :
			outData.errorCode = ERR_NET_INVALID_PASSWORD;
			break;
		case NET_ERR_INIT_PLAYER_NAME_IN_USE :
			outData.errorCode = ERR_NET_PLAYER_NAME_IN_USE;
			break;
		case NET_ERR_INIT_INVALID_PLAYER_NAME :
			outData.errorCode = ERR_NET_INVALID_PLAYER_NAME;
			break;
		case NET_ERR_INIT_SERVER_MAINTENANCE :
			outData.errorCode = ERR_NET_SERVER_MAINTENANCE;
			break;
		case NET_ERR_AVATAR_TOO_LARGE :
			outData.errorCode = ERR_NET_AVATAR_TOO_LARGE;
			break;
		case NET_ERR_AVATAR_WRONG_SIZE :
			outData.errorCode = ERR_NET_WRONG_AVATAR_SIZE;
			break;
		case NET_ERR_AVATAR_UPLOAD_BLOCKED :
			outData.errorCode = ERR_NET_AVATAR_UPLOAD_BLOCKED;
			break;
		case NET_ERR_JOIN_GAME_UNKNOWN_GAME :
			outData.errorCode = ERR_NET_UNKNOWN_GAME;
			break;

	// General Errors.
		case NET_ERR_GENERAL_INVALID_PACKET :
			outData.errorCode = ERR_SOCK_INVALID_PACKET;
			break;
		case NET_ERR_GENERAL_INVALID_STATE :
			outData.errorCode = ERR_SOCK_INVALID_STATE;
			break;
		case NET_ERR_GENERAL_PLAYER_KICKED :
			outData.errorCode = ERR_NET_PLAYER_KICKED;
			break;
		case NET_ERR_GENERAL_PLAYER_BANNED :
			outData.errorCode = ERR_NET_PLAYER_BANNED;
			break;
		case NET_ERR_GENERAL_SESSION_TIMED_OUT :
			outData.errorCode = ERR_NET_SESSION_TIMED_OUT;
			break;
		default :
			outData.errorCode = ERR_SOCK_INTERNAL;
			break;
	}
}

const NetPacketError *
NetPacketError::ToNetPacketError() const
{
	return this;
}

void
NetPacketError::InternalCheck(const NetPacketHeader*) const
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

