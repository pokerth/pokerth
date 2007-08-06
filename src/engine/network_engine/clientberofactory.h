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

#include <vector>

#include <boost/shared_ptr.hpp>

#include <berofactoryinterface.h>
#include <handinterface.h>

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class ClientBeRoFactory : public BeRoFactoryInterface
{
public:
    ClientBeRoFactory(HandInterface* hi, int id, int aP, int dP, int sB);

    ~ClientBeRoFactory();

	BeRoInterface* switchRounds(BeRoInterface*, int);

	BeRoInterface* createBeRoPreflop();

	std::vector<boost::shared_ptr<BeRoInterface> > createBeRo();

};

#endif
