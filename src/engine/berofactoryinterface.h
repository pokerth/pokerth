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

#include <vector>

#include <boost/shared_ptr.hpp>

#include <berointerface.h>

class BeRoFactoryInterface{
public:

    virtual ~BeRoFactoryInterface();

	virtual BeRoInterface* switchRounds(BeRoInterface*, int) = 0;

	virtual BeRoInterface* createBeRoPreflop() =0;

	virtual std::vector<boost::shared_ptr<BeRoInterface> > createBeRo() =0;

};

#endif
