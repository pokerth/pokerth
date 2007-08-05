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

using namespace std;

LocalBeRoFactory::LocalBeRoFactory (HandInterface* hi, int id, int aP, int dP, int sB)
		: BeRoFactoryInterface() , myHand(hi), myID(id), actualQuantityPlayers(aP), dealerPosition(dP), smallBlind(sB)
{}


LocalBeRoFactory::~LocalBeRoFactory()
{}

BeRoInterface* LocalBeRoFactory::createBeRoPreflop() {

	BeRoInterface* preflopBeRo = new LocalBeRoPreflop ( myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind );

	return preflopBeRo;

}

BeRoInterface* LocalBeRoFactory::switchRounds(BeRoInterface* currentBeRo, int currentRound)
{
	if(currentRound != currentBeRo->getMyBeRoID()) {

		delete currentBeRo;
		currentBeRo = NULL;

		switch(currentRound) {
			case GAME_STATE_PREFLOP: {

				currentBeRo = new LocalBeRoPreflop ( myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind );
			}
			break;
			case GAME_STATE_FLOP: {
	
				currentBeRo = new LocalBeRoFlop( myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind );
			}
			break;
			case GAME_STATE_TURN: {
	
				currentBeRo = new LocalBeRoTurn( myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind );

			}
			break;
			case GAME_STATE_RIVER: {
	
				currentBeRo = new LocalBeRoRiver ( myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind );

			}
			break;
			case GAME_STATE_POST_RIVER: {
	
				currentBeRo = new LocalBeRoPostRiver ( myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind );

			}
			break;
			default: { cout << "BeRoFactory-ERROR" << endl; }
		}

	}

	return currentBeRo;
}



