//
// C++ Implementation: clientberofactory
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "clientberofactory.h"
#include "berointerface.h"

ClientBeRoFactory::ClientBeRoFactory(HandInterface* hi, int id, int aP, int dP, int sB)
 : BeRoFactoryInterface()
{
}


ClientBeRoFactory::~ClientBeRoFactory()
{
}

std::vector<boost::shared_ptr<BeRoInterface> > ClientBeRoFactory::createBeRo()
{
	std::vector<boost::shared_ptr<BeRoInterface> > bettingRounds;
//         bettingRounds.push_back(boost::shared_ptr<BeRoInterface>(new
// LocalBeRoPreflop(...)));
//         bettingRounds.push_back(boost::shared_ptr<BeRoInterface>(new
// LocalBeRoFlop(...)));
// 
//         ...
	return bettingRounds;
}



BeRoInterface* ClientBeRoFactory::switchRounds(BeRoInterface* currentBeRo, int currentRound) { return NULL;}

BeRoInterface* ClientBeRoFactory::createBeRoPreflop() { return NULL; }
