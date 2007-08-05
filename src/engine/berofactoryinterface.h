//
// C++ Interface: berofactoryinterface
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BEROFACTORYINTERFACE_H
#define BEROFACTORYINTERFACE_H

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/

#include <berointerface.h>

class BeRoFactoryInterface{
public:

    virtual ~BeRoFactoryInterface();

    virtual BeRoInterface* switchRounds(int) = 0;

};

#endif
