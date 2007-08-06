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
#include <clientpreflop.h>
#include <clientflop.h>
#include <clientturn.h>
#include <clientriver.h>

ClientBeRoFactory::ClientBeRoFactory(HandInterface* hi, int id, int aP, int dP, int sB)
: myHand(hi), myID(id), actualQuantityPlayers(aP), dealerPosition(dP), smallBlind(sB)
{
}


ClientBeRoFactory::~ClientBeRoFactory()
{
}

std::vector<boost::shared_ptr<BeRoInterface> > ClientBeRoFactory::createBeRo()
{
	std::vector<boost::shared_ptr<BeRoInterface> > myBeRo;

	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new ClientBeRoPreflop(myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind)));

	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new ClientBeRoFlop(myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind)));

	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new ClientBeRoTurn(myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind)));

	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new ClientBeRoRiver(myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind)));

//	myBeRo.push_back(boost::shared_ptr<BeRoInterface>(new ClientBeRoPostRiver(myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind)));

	return myBeRo;
}



BeRoInterface* ClientBeRoFactory::switchRounds(BeRoInterface* currentBeRo, int currentRound) { return NULL;}

BeRoInterface* ClientBeRoFactory::createBeRoPreflop() { return NULL; }
