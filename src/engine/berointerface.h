//
// C++ Interface: berointerface
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BEROINTERFACE_H
#define BEROINTERFACE_H

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class BeRoInterface{
public:

    virtual ~BeRoInterface();
	
	virtual int getMyBeRoID() const =0;

	virtual void setPlayersTurn(int) =0;
	virtual int getPlayersTurn() const =0;
	
	virtual void setHighestSet(int) =0;
	virtual int getHighestSet() const =0;

	virtual void setHighestCardsValue(int theValue) =0;
	virtual void resetFirstRun() =0;
	virtual int getHighestCardsValue() =0;

	virtual void preflopRun() =0;
	virtual void flopRun() =0;
	virtual void turnRun() =0;
	virtual void riverRun() =0;
	virtual void postRiverRun() =0;
	
	virtual void nextPlayer2() =0;

};

#endif
