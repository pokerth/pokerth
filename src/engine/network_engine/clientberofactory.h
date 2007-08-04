//
// C++ Interface: clientberofactory
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CLIENTBEROFACTORY_H
#define CLIENTBEROFACTORY_H

#include <berofactoryinterface.h>
#include <handinterface.h>

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class ClientBeRoFactory : public BeRoFactoryInterface
{
public:
    ClientBeRoFactory(HandInterface* hi);

    ~ClientBeRoFactory();

    BeRoInterface* switchRounds();

};

#endif
