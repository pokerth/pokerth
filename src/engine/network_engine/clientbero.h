//
// C++ Interface: clientbero
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CLIENTBERO_H
#define CLIENTBERO_H

#include <vector>

#include <boost/shared_ptr.hpp>

#include "berointerface.h"


/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class ClientBeRo : public BeRoInterface{
public:
    ClientBeRo();

    ~ClientBeRo();

	int getMyBeRoID() const { return myBeRoID; }

	void run() {}

protected:

	int myBeRoID;

};

#endif
