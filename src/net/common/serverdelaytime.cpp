#include "serverdelaytime.h"

//#define POKERTH_SERVER_TEST

#ifdef POKERTH_SERVER_TEST
#define SERVER_DELAY_NEXT_GAME_SEC				0
#define SERVER_DEAL_FLOP_CARDS_DELAY_SEC		0
#define SERVER_DEAL_TURN_CARD_DELAY_SEC			0
#define SERVER_DEAL_RIVER_CARD_DELAY_SEC		0
#define SERVER_DEAL_ADD_ALL_IN_DELAY_SEC		0
#define SERVER_SHOW_CARDS_DELAY_SEC				0
#define SERVER_COMPUTER_ACTION_DELAY_SEC		0
#define SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC		1
#else
#define SERVER_DELAY_NEXT_GAME_SEC              10
#define SERVER_DEAL_FLOP_CARDS_DELAY_SEC		5
#define SERVER_DEAL_TURN_CARD_DELAY_SEC			2
#define SERVER_DEAL_RIVER_CARD_DELAY_SEC		2
#define SERVER_DEAL_ADD_ALL_IN_DELAY_SEC		2
#define SERVER_SHOW_CARDS_DELAY_SEC				2
#define SERVER_COMPUTER_ACTION_DELAY_SEC		2
#define SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC		2
#endif

ServerDelayTime::ServerDelayTime(const ServerMode mode)
{
	if(mode == SERVER_MODE_LAN_LOCAL) {
		nextGameDelay = 0;
		dealFlopCardsDelay = 0;
		dealTurnCardsDelay = 0;
		dealRiverCardsDelay = 0;
		dealAddAllInDelay = 0;
		showCardsDelay = 0;
		computerActionDelay = 0;
		playerTimeoutAddDelay = 0;
	} else {
		nextGameDelay = SERVER_DELAY_NEXT_GAME_SEC;
		dealFlopCardsDelay = SERVER_DEAL_FLOP_CARDS_DELAY_SEC;
		dealTurnCardsDelay = SERVER_DEAL_TURN_CARD_DELAY_SEC;
		dealRiverCardsDelay = SERVER_DEAL_RIVER_CARD_DELAY_SEC;
		dealAddAllInDelay = SERVER_DEAL_ADD_ALL_IN_DELAY_SEC;
		showCardsDelay = SERVER_SHOW_CARDS_DELAY_SEC;
		computerActionDelay = SERVER_COMPUTER_ACTION_DELAY_SEC;
		playerTimeoutAddDelay = SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC;
	}

}

int ServerDelayTime::getNextGameDelay() const
{
	return nextGameDelay;
}

int ServerDelayTime::getDealFlopCardsDelay() const
{
	return dealFlopCardsDelay;
}

int ServerDelayTime::getDealTurnCardsDelay() const
{
	return dealTurnCardsDelay;
}

int ServerDelayTime::getDealRiverCardsDelay() const
{
	return dealRiverCardsDelay;
}

int ServerDelayTime::getDealAddAllInDelay() const
{
	return dealAddAllInDelay;
}

int ServerDelayTime::getShowCardsDelay() const
{
	return showCardsDelay;
}

int ServerDelayTime::getComputerActionDelay() const
{
	return computerActionDelay;
}

int ServerDelayTime::getPlayerTimeoutAddDelay() const
{
	return playerTimeoutAddDelay;
}
