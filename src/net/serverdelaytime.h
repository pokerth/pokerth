#ifndef SERVERDELAYTIME_H
#define SERVERDELAYTIME_H

#include <game_defs.h>

class ServerDelayTime
{
public:
	ServerDelayTime(const ServerMode mode);

	int getNextGameDelay() const;
	int getDealFlopCardsDelay() const;
	int getDealTurnCardsDelay() const;
	int getDealRiverCardsDelay() const;
	int getDealAddAllInDelay() const;
	int getShowCardsDelay() const;
	int getComputerActionDelay() const;
	int getPlayerTimeoutAddDelay() const;

private:

	int nextGameDelay;
	int dealFlopCardsDelay;
	int dealTurnCardsDelay;
	int dealRiverCardsDelay;
	int dealAddAllInDelay;
	int showCardsDelay;
	int computerActionDelay;
	int playerTimeoutAddDelay;

};

#endif // SERVERDELAYTIME_H
