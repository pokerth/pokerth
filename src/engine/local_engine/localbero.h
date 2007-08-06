//
// C++ Interface: localbero
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef LOCALBERO_H
#define LOCALBERO_H

#include "berointerface.h"

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class LocalBeRo : public BeRoInterface{
public:
    LocalBeRo();

    ~LocalBeRo();

	int getMyBeRoID() const { return myBeRoID; }
	
	int getHighestCardsValue() const {return 0;}
	void setHighestCardsValue(int theValue) {}

	void run() {}
	//only until bero refactoring is over
	void preflopRun() {}
	void flopRun() {}
	void riverRun() {}
	void turnRun() {}
	void postRiverRun() {}

	void resetFirstRun() {}
	
protected:

	int myBeRoID;

};

#endif
