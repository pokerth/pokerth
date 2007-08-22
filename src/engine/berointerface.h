//
// C++ Interface: berointerface
//
// Description: Betting rounds interface
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BEROINTERFACE_H
#define BEROINTERFACE_H

#include "game_defs.h"

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class BeRoInterface{
public:

	virtual ~BeRoInterface();
	
	virtual GameState getMyBeRoID() const =0;


	virtual void setPlayersTurn(int) =0;
	virtual int getPlayersTurn() const =0;
	
	virtual void setHighestSet(int) =0;
	virtual int getHighestSet() const =0;

	virtual void setHighestCardsValue(int theValue) =0;
	virtual int getHighestCardsValue() const =0;

	virtual void setLastActionPlayer( int theValue ) =0;
	virtual int getLastActionPlayer() const =0;

	virtual void resetFirstRun() =0;

	virtual void nextPlayer() =0;
	virtual void run() =0;

	virtual void postRiverRun() =0;

};

#endif
