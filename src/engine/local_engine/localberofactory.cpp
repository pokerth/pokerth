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

BeRoInterface* LocalBeRoFactory::switchRounds(int currentRound)
{

	BeRoInterface* myBeRo = NULL;

	switch(currentRound) {
		case GAME_STATE_PREFLOP: {
			if(currentRound != myHand->getCurrentBeRo()->getMyBeRoID()) {

				delete myBeRo;
				myBeRo = NULL;

				myBeRo = new LocalBeRoPreflop ( myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind );
			}
		}
		break;
		case GAME_STATE_FLOP: {
			if(currentRound != myHand->getCurrentBeRo()->getMyBeRoID()) {

				delete myBeRo;
				myBeRo = NULL;

				myBeRo = new LocalBeRoFlop( myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind );
			}
		}
		break;
		case GAME_STATE_TURN: {
			if(currentRound != myHand->getCurrentBeRo()->getMyBeRoID()) {

				delete myBeRo;
				myBeRo = NULL;

				myBeRo = new LocalBeRoTurn( myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind );
			}
		}
		break;
		case GAME_STATE_RIVER: {
			if(currentRound != myHand->getCurrentBeRo()->getMyBeRoID()) {

				delete myBeRo;
				myBeRo = NULL;

				myBeRo = new LocalBeRoRiver ( myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind );
			}
		}
		break;
		case GAME_STATE_POST_RIVER: {
			if(currentRound != myHand->getCurrentBeRo()->getMyBeRoID()) {

				delete myBeRo;
				myBeRo = NULL;

				myBeRo = new LocalBeRoPostRiver ( myHand, myID, actualQuantityPlayers, dealerPosition, smallBlind );
			}
		}
		break;
		default: { cout << "BeRoFactory-ERROR" << endl; }
	}



	

	return myBeRo;
}



