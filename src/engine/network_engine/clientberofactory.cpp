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

ClientBeRoFactory::ClientBeRoFactory(HandInterface* hi, int id, int aP, int dP, int sB)
 : BeRoFactoryInterface()
{
}


ClientBeRoFactory::~ClientBeRoFactory()
{
}


BeRoInterface* ClientBeRoFactory::switchRounds(int currentRound) { return NULL;}
