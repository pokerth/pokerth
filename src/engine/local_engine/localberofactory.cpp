//
// C++ Implementation: localberofactory
//
// Description:
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "localberofactory.h"

LocalBeRoFactory::LocalBeRoFactory (HandInterface* hi, int id, int aP, int dP, int sB)
		: BeRoFactoryInterface() , myHand(hi), myID(id), actualQuantityPlayers(aP), dealerPosition(dP), smallBlind(sB)
{}


LocalBeRoFactory::~LocalBeRoFactory()
{}

BeRoInterface* LocalBeRoFactory::switchRounds()
{

	BeRoInterface* myBeRo = NULL;

	myBeRo = new LocalBeRoPreflop ( myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind );

	return myBeRo;
}



