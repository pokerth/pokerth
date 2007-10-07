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

#include <boost/shared_ptr.hpp>
#include <list>

#include "game_defs.h"

class PlayerInterface;

typedef boost::shared_ptr<std::list<boost::shared_ptr<PlayerInterface> > > PlayerList;
typedef std::list<boost::shared_ptr<PlayerInterface> >::iterator PlayerListIterator;

class BeRoInterface{
public:

	virtual ~BeRoInterface();
	
	virtual GameState getMyBeRoID() const =0;

	virtual void setPlayersTurn(int) =0;
	virtual int getPlayersTurn() const =0;

	virtual void setCurrentPlayersTurnId(unsigned) =0;
	virtual unsigned getCurrentPlayersTurnId() const =0;

	virtual void setCurrentPlayersTurnIt(PlayerListIterator) =0;
	virtual PlayerListIterator getCurrentPlayersTurnIt() const =0;

	virtual void setSmallBlindPositionId(unsigned) =0;
	virtual unsigned getSmallBlindPositionId() const =0;

	virtual void setBigBlindPositionId(unsigned) =0;
	virtual unsigned getBigBlindPositionId() const =0;

	virtual void setHighestSet(int) =0;
	virtual int getHighestSet() const =0;

	virtual void setHighestCardsValue(int theValue) =0;
	virtual int getHighestCardsValue() const =0;

	virtual void setLastActionPlayer( int theValue ) =0;
	virtual int getLastActionPlayer() const =0;

	virtual void setMinimumRaise (int) =0;
	virtual int getMinimumRaise() const =0;

	virtual bool getFirstRound() const =0;

	virtual void skipFirstRunGui() =0;

	virtual void nextPlayer() =0;
	virtual void run() =0;

	virtual void postRiverRun() =0;

};

#endif
