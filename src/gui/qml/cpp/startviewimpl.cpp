#include "startviewimpl.h"
#include "mylistviewitemdata.h"
#include "gamedata.h"

StartViewImpl::StartViewImpl(QObject *parent) : QObject(parent)
{

}

StartViewImpl::~StartViewImpl()
{

}


void StartViewImpl::startLocalGame(GameData &gd)
{
//    // Stop local game.
//	myGuiInterface->getMyW()->stopTimer();


//    Frage 1. Was ist nochmal ServerGuiInterface? Wofür? Was macht es?

//	if (!myServerGuiInterface) {
//		// Create pseudo Gui Wrapper for the server.
//		myServerGuiInterface.reset(new ServerGuiWrapper(myConfig, mySession->getGui(), mySession->getGui(), mySession->getGui()));
//		{
//			boost::shared_ptr<Session> session(new Session(myServerGuiInterface.get(), myConfig, 0));
//			session->init(mySession->getAvatarManager());
//			myServerGuiInterface->setSession(session);
//		}
//	}

//	// Terminate existing network games.
//	mySession->terminateNetworkClient();
//	myServerGuiInterface->getSession()->terminateNetworkServer();



//    Frage 2. Warum an dieser Stelle die StartView-Session an Lobby und StartNetworkDialog?

//  myGameLobbyDialog->setSession(getSession());
//	myStartNetworkGameDialog->setSession(getSession());

//	myServerGuiInterface->getSession()->startNetworkServer(false,true);
//	mySession->startNetworkClientForLocalServer(gd);

//	myGuiInterface->getMyW()->show();
//	myGuiInterface->getMyW()->networkGameModification();

//	assert(getSession());
//	getSession()->sendStartEvent(true);
}

