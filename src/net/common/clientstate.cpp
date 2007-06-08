/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <net/clientstate.h>
#include <net/clientthread.h>
#include <net/clientcontext.h>
#include <net/senderthread.h>
#include <net/receiverhelper.h>
#include <net/netpacket.h>
#include <net/resolverthread.h>
#include <net/clientexception.h>
#include <net/socket_helper.h>
#include <net/socket_msg.h>

#include <game.h>
#include <playerinterface.h>

using namespace std;

#define CLIENT_WAIT_TIMEOUT_MSEC	50
#define CLIENT_CONNECT_TIMEOUT_SEC	10


ClientState::~ClientState()
{
}

//-----------------------------------------------------------------------------

ClientStateInit &
ClientStateInit::Instance()
{
	static ClientStateInit state;
	return state;
}

ClientStateInit::ClientStateInit()
{
}

ClientStateInit::~ClientStateInit()
{
}

int
ClientStateInit::Process(ClientThread &client)
{
	ClientContext &context = client.GetContext();

	if (context.GetServerAddr().empty())
		throw ClientException(ERR_SOCK_SERVERADDR_NOT_SET, 0);

	if (context.GetServerPort() < 1024)
		throw ClientException(ERR_SOCK_INVALID_PORT, 0);

	context.SetSocket(socket(context.GetAddrFamily(), SOCK_STREAM, context.GetProtocol()));
	if (!IS_VALID_SOCKET(context.GetSocket()))
		throw ClientException(ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());

	unsigned long mode = 1;
	if (IOCTLSOCKET(context.GetSocket(), FIONBIO, &mode) == SOCKET_ERROR)
		throw ClientException(ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());

	// The following call is optional - the return value is not checked.
	int nodelay = 1;
	setsockopt(context.GetSocket(), SOL_SOCKET, TCP_NODELAY, (char *)&nodelay, sizeof(nodelay));

	client.SetState(ClientStateStartResolve::Instance());

	return MSG_SOCK_INIT_DONE;
}

//-----------------------------------------------------------------------------

ClientStateStartResolve &
ClientStateStartResolve::Instance()
{
	static ClientStateStartResolve state;
	return state;
}

ClientStateStartResolve::ClientStateStartResolve()
{
}

ClientStateStartResolve::~ClientStateStartResolve()
{
}

int
ClientStateStartResolve::Process(ClientThread &client)
{
	int retVal;

	ClientContext &context = client.GetContext();

	context.GetClientSockaddr()->ss_family = context.GetAddrFamily();

	// Treat the server address as numbers first.
	if (socket_string_to_addr(
		context.GetServerAddr().c_str(),
		context.GetAddrFamily(),
		(struct sockaddr *)context.GetClientSockaddr(),
		context.GetClientSockaddrSize()))
	{
		// Success - but we still need to set the port.
		if (!socket_set_port(context.GetServerPort(), context.GetAddrFamily(), (struct sockaddr *)context.GetClientSockaddr(), context.GetClientSockaddrSize()))
			throw ClientException(ERR_SOCK_SET_PORT_FAILED, 0);

		// No need to resolve - start connecting.
		client.SetState(ClientStateStartConnect::Instance());
		retVal = MSG_SOCK_RESOLVE_DONE;
	}
	else
	{
		// Start name resolution in a separate thread, since it is blocking
		// for up to about 30 seconds.
		std::auto_ptr<ResolverThread> resolver(new ResolverThread);
		resolver->Init(context);
		resolver->Run();

		ClientStateResolving::Instance().SetResolver(resolver.release());
		client.SetState(ClientStateResolving::Instance());

		retVal = MSG_SOCK_INTERNAL_PENDING;
	}

	return retVal;
}

//-----------------------------------------------------------------------------

ClientStateResolving &
ClientStateResolving::Instance()
{
	static ClientStateResolving state;
	return state;
}

ClientStateResolving::ClientStateResolving()
: m_resolver(NULL)
{
}

ClientStateResolving::~ClientStateResolving()
{
	Cleanup();
}

void
ClientStateResolving::SetResolver(ResolverThread *resolver)
{
	Cleanup();
	m_resolver = resolver;
}

int
ClientStateResolving::Process(ClientThread &client)
{
	int retVal;

	if (!m_resolver)
		throw ClientException(ERR_SOCK_RESOLVE_FAILED, 0);

	if (m_resolver->Join(CLIENT_WAIT_TIMEOUT_MSEC))
	{
		ClientContext &context = client.GetContext();
		bool success = m_resolver->GetResult(context);
		Cleanup(); // Not required, but better keep things clean.

		if (!success)
			throw ClientException(ERR_SOCK_RESOLVE_FAILED, 0);

		client.SetState(ClientStateStartConnect::Instance());
		retVal = MSG_SOCK_RESOLVE_DONE;
	}
	else
		retVal = MSG_SOCK_INTERNAL_PENDING;

	return retVal;
}


void
ClientStateResolving::Cleanup()
{
	if (m_resolver)
	{
		if (m_resolver->Join(500))
			delete m_resolver;
		// If the resolver does not terminate fast enough, leave it
		// as memory leak.
		m_resolver = NULL;
	}
}

//-----------------------------------------------------------------------------

ClientStateStartConnect &
ClientStateStartConnect::Instance()
{
	static ClientStateStartConnect state;
	return state;
}

ClientStateStartConnect::ClientStateStartConnect()
{
}

ClientStateStartConnect::~ClientStateStartConnect()
{
}

int
ClientStateStartConnect::Process(ClientThread &client)
{
	int retVal;
	ClientContext &context = client.GetContext();

	int connectResult = connect(context.GetSocket(), (struct sockaddr *)context.GetClientSockaddr(), context.GetClientSockaddrSize());

	if (IS_VALID_CONNECT(connectResult))
	{
		client.SetState(ClientStateStartSession::Instance());
		retVal = MSG_SOCK_CONNECT_DONE;
	}
	else
	{
		int errCode = SOCKET_ERRNO();
		if (errCode == SOCKET_ERR_WOULDBLOCK)
		{
			boost::microsec_timer connectTimer;
			connectTimer.start();
			ClientStateConnecting::Instance().SetTimer(connectTimer);
			client.SetState(ClientStateConnecting::Instance());
			retVal = MSG_SOCK_INTERNAL_PENDING;
		}
		else
			throw ClientException(ERR_SOCK_CONNECT_FAILED, SOCKET_ERRNO());
	}

	return retVal;
}

//-----------------------------------------------------------------------------

ClientStateConnecting &
ClientStateConnecting::Instance()
{
	static ClientStateConnecting state;
	return state;
}

ClientStateConnecting::ClientStateConnecting()
{
}

ClientStateConnecting::~ClientStateConnecting()
{
}

void
ClientStateConnecting::SetTimer(const boost::microsec_timer &timer)
{
	m_connectTimer = timer;
}

int
ClientStateConnecting::Process(ClientThread &client)
{
	int retVal;
	ClientContext &context = client.GetContext();

	fd_set writeSet;
	struct timeval timeout;

	FD_ZERO(&writeSet);
	FD_SET(context.GetSocket(), &writeSet);

	timeout.tv_sec  = 0;
	timeout.tv_usec = CLIENT_WAIT_TIMEOUT_MSEC * 1000;
	int selectResult = select(context.GetSocket() + 1, NULL, &writeSet, NULL, &timeout);

	if (selectResult > 0) // success
	{
		// Check whether the connect call succeeded.
		int connectResult = 0;
		socklen_t tmpSize = sizeof(connectResult);
		getsockopt(context.GetSocket(), SOL_SOCKET, SO_ERROR, (char *)&connectResult, &tmpSize);
		if (connectResult != 0)
			throw ClientException(ERR_SOCK_CONNECT_FAILED, connectResult);
		client.SetState(ClientStateStartSession::Instance());
		retVal = MSG_SOCK_CONNECT_DONE;
	}
	else if (selectResult == 0) // timeout
	{
		if (m_connectTimer.elapsed().seconds() >= CLIENT_CONNECT_TIMEOUT_SEC)
			throw ClientException(ERR_SOCK_CONNECT_TIMEOUT, 0);
		else
			retVal = MSG_SOCK_INTERNAL_PENDING;
	}
	else
		throw ClientException(ERR_SOCK_SELECT_FAILED, SOCKET_ERRNO());


	return retVal;
}

//-----------------------------------------------------------------------------

ClientStateStartSession &
ClientStateStartSession::Instance()
{
	static ClientStateStartSession state;
	return state;
}

ClientStateStartSession::ClientStateStartSession()
{
}

ClientStateStartSession::~ClientStateStartSession()
{
}

int
ClientStateStartSession::Process(ClientThread &client)
{
	ClientContext &context = client.GetContext();

	NetPacketJoinGame::Data initData;
	initData.password = context.GetPassword();
	initData.playerName = context.GetPlayerName();
	initData.ptype = PLAYER_TYPE_HUMAN; // TODO

	boost::shared_ptr<NetPacket> packet(new NetPacketJoinGame);
	((NetPacketJoinGame *)packet.get())->SetData(initData);
	
	client.GetSender().Send(context.GetSocket(), packet);

	client.SetState(ClientStateWaitSession::Instance());

	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

AbstractClientStateReceiving::AbstractClientStateReceiving()
{
}

AbstractClientStateReceiving::~AbstractClientStateReceiving()
{
}

int
AbstractClientStateReceiving::Process(ClientThread &client)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	// delegate to receiver helper class
	boost::shared_ptr<NetPacket> tmpPacket = client.GetReceiver().Recv(client.GetContext().GetSocket());

	if (tmpPacket.get())
	{
		if (tmpPacket->ToNetPacketError())
		{
			// Server reported an error.
			NetPacketError::Data errorData;
			tmpPacket->ToNetPacketError()->GetData(errorData);
			// Show the error.
			throw ClientException(errorData.errorCode, 0);
		}
		else if (tmpPacket->ToNetPacketChatText())
		{
			// Chat message - display it in the GUI.
			NetPacketChatText::Data chatData;
			tmpPacket->ToNetPacketChatText()->GetData(chatData);

			boost::shared_ptr<PlayerData> tmpPlayer = client.GetPlayerDataByUniqueId(chatData.playerId);
			if (tmpPlayer.get())
				client.GetCallback().SignalNetClientChatMsg(tmpPlayer->GetName(), chatData.text);
		}
		else if (tmpPacket->ToNetPacketPlayerLeft())
		{
			// A player left the game.
			NetPacketPlayerLeft::Data playerLeftData;
			tmpPacket->ToNetPacketPlayerLeft()->GetData(playerLeftData);

			// Signal to GUI.
			client.RemovePlayerData(playerLeftData.playerId);

			// If the game is running, deactivate player.
			boost::shared_ptr<Game> curGame = client.GetGame();
			if (curGame.get())
			{
				PlayerInterface *tmpPlayer = curGame->getPlayerByUniqueId(playerLeftData.playerId);
				if (!tmpPlayer)
					throw ClientException(ERR_NET_UNKNOWN_PLAYER_ID, 0);

				// Reset his action and his cash.
				tmpPlayer->setMyAction(PLAYER_ACTION_FOLD);
				tmpPlayer->setMyCash(0);
				// Player is now inactive.
				tmpPlayer->setMyActiveStatus(false);

				client.GetGui().refreshAction();
				client.GetGui().refreshCash();
			}
		}
		else
			retVal = InternalProcess(client, tmpPacket);
	}

	return retVal;
}

//-----------------------------------------------------------------------------

ClientStateWaitSession &
ClientStateWaitSession::Instance()
{
	static ClientStateWaitSession state;
	return state;
}

ClientStateWaitSession::ClientStateWaitSession()
{
}

ClientStateWaitSession::~ClientStateWaitSession()
{
}

int
ClientStateWaitSession::InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;
	ClientContext &context = client.GetContext();

	if (packet->ToNetPacketJoinGameAck())
	{
		// Everything is fine - we joined the game.
		// Initialize game configuration.
		NetPacketJoinGameAck::Data joinGameAckData;
		packet->ToNetPacketJoinGameAck()->GetData(joinGameAckData);
		client.SetGameData(joinGameAckData.gameData);
		client.SetGuiPlayerNum(joinGameAckData.yourPlayerNum);

		// TODO: Type Human is fixed here.
		boost::shared_ptr<PlayerData> playerData(
			new PlayerData(joinGameAckData.yourPlayerUniqueId, joinGameAckData.yourPlayerNum, PLAYER_TYPE_HUMAN));
		playerData->SetName(context.GetPlayerName());
		client.AddPlayerData(playerData);

		client.SetState(ClientStateWaitGame::Instance());
		retVal = MSG_SOCK_SESSION_DONE;
	}

	return retVal;
}

//-----------------------------------------------------------------------------

ClientStateWaitGame &
ClientStateWaitGame::Instance()
{
	static ClientStateWaitGame state;
	return state;
}

ClientStateWaitGame::ClientStateWaitGame()
{
}

ClientStateWaitGame::~ClientStateWaitGame()
{
}

int
ClientStateWaitGame::InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;
	ClientContext &context = client.GetContext();

	if (packet->ToNetPacketGameStart())
	{
		// Start the network game as client.
		NetPacketGameStart::Data gameStartData;
		packet->ToNetPacketGameStart()->GetData(gameStartData);

		client.SetStartData(gameStartData.startData);

		client.SetState(ClientStateWaitHand::Instance());
		retVal = MSG_NET_GAME_CLIENT_START;
	}
	else if (packet->ToNetPacketPlayerJoined())
	{
		// Another player joined the network game.
		NetPacketPlayerJoined::Data netPlayerData;
		packet->ToNetPacketPlayerJoined()->GetData(netPlayerData);

		boost::shared_ptr<PlayerData> playerData(
			new PlayerData(netPlayerData.playerId, netPlayerData.playerNumber, netPlayerData.ptype));
		playerData->SetName(netPlayerData.playerName);
		client.AddPlayerData(playerData);
	}
	// TODO: handle error packet (kicked from server)

	return retVal;
}

//-----------------------------------------------------------------------------

ClientStateWaitHand &
ClientStateWaitHand::Instance()
{
	static ClientStateWaitHand state;
	return state;
}

ClientStateWaitHand::ClientStateWaitHand()
{
}

ClientStateWaitHand::~ClientStateWaitHand()
{
}

int
ClientStateWaitHand::InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;
	ClientContext &context = client.GetContext();

	if (packet->ToNetPacketHandStart())
	{
		// Hand was started.
		// These are the cards. Good luck.
		NetPacketHandStart::Data tmpData;
		packet->ToNetPacketHandStart()->GetData(tmpData);
		int myCards[2];
		myCards[0] = (int)tmpData.yourCards[0];
		myCards[1] = (int)tmpData.yourCards[1];
		client.GetGame()->getPlayerArray()[0]->setMyCards(myCards);
		client.GetGame()->initHand();
		client.GetGame()->startHand();
		client.GetGui().dealHoleCards();
		client.GetGui().refreshGameLabels(GAME_STATE_PREFLOP);
		client.SetState(ClientStateRunHand::Instance());

		retVal = MSG_NET_GAME_CLIENT_HAND_START;
	}

	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

ClientStateRunHand &
ClientStateRunHand::Instance()
{
	static ClientStateRunHand state;
	return state;
}

ClientStateRunHand::ClientStateRunHand()
{
}

ClientStateRunHand::~ClientStateRunHand()
{
}

int
ClientStateRunHand::InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;
	ClientContext &context = client.GetContext();

	if (packet.get())
	{
		boost::shared_ptr<Game> curGame = client.GetGame();
		if (packet->ToNetPacketPlayersActionDone())
		{
			NetPacketPlayersActionDone::Data actionDoneData;
			packet->ToNetPacketPlayersActionDone()->GetData(actionDoneData);
			PlayerInterface *tmpPlayer = curGame->getPlayerByUniqueId(actionDoneData.playerId);
			if (!tmpPlayer)
				throw ClientException(ERR_NET_UNKNOWN_PLAYER_ID, 0);

			if (actionDoneData.gameState == GAME_STATE_PREFLOP_SMALL_BLIND)
				tmpPlayer->setMyButton(BUTTON_SMALL_BLIND);
			else if (actionDoneData.gameState == GAME_STATE_PREFLOP_BIG_BLIND)
				tmpPlayer->setMyButton(BUTTON_BIG_BLIND);
			else // no blind -> log
			{
				if (actionDoneData.playerAction)
				{
					assert(actionDoneData.totalPlayerBet >= (unsigned)tmpPlayer->getMySet());
					client.GetGui().logPlayerActionMsg(
						tmpPlayer->getMyName(),
						actionDoneData.playerAction,
						actionDoneData.totalPlayerBet - tmpPlayer->getMySet());
				}
			}

			tmpPlayer->setMyAction(actionDoneData.playerAction);
			tmpPlayer->setMySetAbsolute(actionDoneData.totalPlayerBet);
			tmpPlayer->setMyCash(actionDoneData.playerMoney);
			curGame->getCurrentHand()->getBoard()->collectSets();

			// Update highest set
			if (tmpPlayer->getMySet() > GetHighestSet(*curGame))
				SetHighestSet(*curGame, tmpPlayer->getMySet());

			// Stop the timeout for the player.
			client.GetGui().stopTimeoutAnimation(tmpPlayer->getMyID());

			// Unmark last player in GUI.
			client.GetGui().refreshGroupbox(tmpPlayer->getMyID(), 3);

			// Refresh GUI
			client.GetGui().refreshPot();
			client.GetGui().refreshSet();
			client.GetGui().refreshAction();
			client.GetGui().refreshCash();
			client.GetGui().refreshButton();
		}
		else if (packet->ToNetPacketPlayersTurn())
		{
			NetPacketPlayersTurn::Data turnData;
			packet->ToNetPacketPlayersTurn()->GetData(turnData);
			PlayerInterface *tmpPlayer = curGame->getPlayerByUniqueId(turnData.playerId);
			if (!tmpPlayer)
				throw ClientException(ERR_NET_UNKNOWN_PLAYER_ID, 0);

			// Set round.
			if (curGame->getCurrentHand()->getActualRound() != turnData.gameState)
			{
				ResetPlayerActions(*curGame);
				curGame->getCurrentHand()->setActualRound(turnData.gameState);
				// Refresh actions.
				client.GetGui().refreshAction();
			}

			// Next player's turn.
			SetPlayersTurn(*curGame, tmpPlayer->getMyID());

			// Mark current player in GUI.
			int guiStatus = 2;
			if (!tmpPlayer->getMyActiveStatus())
				guiStatus = 0;
			else if (tmpPlayer->getMyAction() == PLAYER_ACTION_FOLD)
				guiStatus = 1;
			client.GetGui().refreshGroupbox(tmpPlayer->getMyID(), guiStatus);

			// Start displaying the timeout for the player.
			client.GetGui().startTimeoutAnimation(tmpPlayer->getMyID(), client.GetGameData().playerActionTimeoutSec);

			if (tmpPlayer->getMyID() == 0) // Is this the GUI player?
				client.GetGui().meInAction();
		}
		else if (packet->ToNetPacketDealFlopCards())
		{
			NetPacketDealFlopCards::Data cardsData;
			packet->ToNetPacketDealFlopCards()->GetData(cardsData);
			int tmpCards[5];
			for (int num = 0; num < 3; num++)
				tmpCards[num] = static_cast<int>(cardsData.flopCards[num]);
			tmpCards[3] = tmpCards[4] = 0;
			curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
			curGame->getCurrentHand()->getBoard()->collectPot();

			client.GetGui().logDealBoardCardsMsg(GAME_STATE_FLOP, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
			client.GetGui().refreshGameLabels(GAME_STATE_FLOP);
			client.GetGui().refreshPot();
			client.GetGui().refreshSet();
			client.GetGui().dealFlopCards();
		}
		else if (packet->ToNetPacketDealTurnCard())
		{
			NetPacketDealTurnCard::Data cardsData;
			packet->ToNetPacketDealTurnCard()->GetData(cardsData);
			int tmpCards[5];
			curGame->getCurrentHand()->getBoard()->getMyCards(tmpCards);
			tmpCards[3] = static_cast<int>(cardsData.turnCard);
			curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
			curGame->getCurrentHand()->getBoard()->collectPot();

			client.GetGui().logDealBoardCardsMsg(GAME_STATE_TURN, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
			client.GetGui().refreshGameLabels(GAME_STATE_TURN);
			client.GetGui().refreshPot();
			client.GetGui().refreshSet();
			client.GetGui().dealTurnCard();
		}
		else if (packet->ToNetPacketDealRiverCard())
		{
			NetPacketDealRiverCard::Data cardsData;
			packet->ToNetPacketDealRiverCard()->GetData(cardsData);
			int tmpCards[5];
			curGame->getCurrentHand()->getBoard()->getMyCards(tmpCards);
			tmpCards[4] = static_cast<int>(cardsData.riverCard);
			curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
			curGame->getCurrentHand()->getBoard()->collectPot();

			client.GetGui().logDealBoardCardsMsg(GAME_STATE_RIVER, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
			client.GetGui().refreshGameLabels(GAME_STATE_RIVER);
			client.GetGui().refreshPot();
			client.GetGui().refreshSet();
			client.GetGui().dealRiverCard();
		}
		else if (packet->ToNetPacketAllInShowCards())
		{
			NetPacketAllInShowCards::Data allInData;
			packet->ToNetPacketAllInShowCards()->GetData(allInData);

			NetPacketAllInShowCards::PlayerCardsList::const_iterator i
				= allInData.playerCards.begin();
			NetPacketAllInShowCards::PlayerCardsList::const_iterator end
				= allInData.playerCards.end();

			while (i != end)
			{
				PlayerInterface *tmpPlayer = curGame->getPlayerByUniqueId((*i).playerId);
				if (!tmpPlayer)
					throw ClientException(ERR_NET_UNKNOWN_PLAYER_ID, 0);

				int tmpCards[2];
				tmpCards[0] = static_cast<int>((*i).cards[0]);
				tmpCards[1] = static_cast<int>((*i).cards[1]);
				tmpPlayer->setMyCards(tmpCards);
				++i;
			}
			client.GetGui().flipHolecardsAllIn();
		}
		else if (packet->ToNetPacketEndOfHandHideCards())
		{
			// End of Hand, but keep cards hidden.
			NetPacketEndOfHandHideCards::Data endHandData;
			packet->ToNetPacketEndOfHandHideCards()->GetData(endHandData);

			PlayerInterface *tmpPlayer = curGame->getPlayerByUniqueId(endHandData.playerId);
			if (!tmpPlayer)
				throw ClientException(ERR_NET_UNKNOWN_PLAYER_ID, 0);

			tmpPlayer->setMyCash(endHandData.playerMoney);
			// TODO use moneyWon
			client.GetGui().postRiverRunAnimation1();

			// Reset player actions
			for (int i = 0; i < MAX_NUMBER_OF_PLAYERS; i++)
				curGame->getPlayerArray()[i]->setMySetNull();
			// Wait for next Hand.
			client.SetState(ClientStateWaitHand::Instance());
			retVal = MSG_NET_GAME_SERVER_HAND_END;
		}
		else if (packet->ToNetPacketEndOfHandShowCards())
		{
			// End of Hand, show cards.
			NetPacketEndOfHandShowCards::Data endHandData;
			packet->ToNetPacketEndOfHandShowCards()->GetData(endHandData);

			NetPacketEndOfHandShowCards::PlayerResultList::const_iterator i
				= endHandData.playerResults.begin();
			NetPacketEndOfHandShowCards::PlayerResultList::const_iterator end
				= endHandData.playerResults.end();

			int highestValueOfCards = 0;
			while (i != end)
			{
				PlayerInterface *tmpPlayer = curGame->getPlayerByUniqueId((*i).playerId);
				if (!tmpPlayer)
					throw ClientException(ERR_NET_UNKNOWN_PLAYER_ID, 0);

				int tmpCards[2];
				tmpCards[0] = static_cast<int>((*i).cards[0]);
				tmpCards[1] = static_cast<int>((*i).cards[1]);
				tmpPlayer->setMyCards(tmpCards);
				for (int num = 0; num < 5; num++)
					tmpPlayer->getMyBestHandPosition()[num] = (*i).bestHandPos[num];
				tmpPlayer->setMyCardsValueInt((*i).valueOfCards);
				if (tmpPlayer->getMyCardsValueInt() > highestValueOfCards)
					highestValueOfCards = tmpPlayer->getMyCardsValueInt();
				tmpPlayer->setMyCash((*i).playerMoney);
				// TODO use moneyWon
				++i;
			}
			client.GetGame()->getCurrentHand()->getRiver()->setHighestCardsValue(highestValueOfCards);
			client.GetGui().postRiverRunAnimation1();

			// Reset player sets
			for (int i = 0; i < MAX_NUMBER_OF_PLAYERS; i++)
				curGame->getPlayerArray()[i]->setMySetNull();
			// Wait for next Hand.
			client.SetState(ClientStateWaitHand::Instance());
			retVal = MSG_NET_GAME_SERVER_HAND_END;
		}
	}

	return retVal;
}


int
ClientStateRunHand::GetHighestSet(Game &curGame)
{
	int highestSet = 0;
	// TODO: no switch needed here if game states are polymorphic
	switch(curGame.getCurrentHand()->getActualRound()) {
		case GAME_STATE_PREFLOP: {
			highestSet = curGame.getCurrentHand()->getPreflop()->getHighestSet();
		} break;
		case GAME_STATE_FLOP: {
			highestSet = curGame.getCurrentHand()->getFlop()->getHighestSet();
		} break;
		case GAME_STATE_TURN: {
			highestSet = curGame.getCurrentHand()->getTurn()->getHighestSet();
		} break;
		case GAME_STATE_RIVER: {
			highestSet = curGame.getCurrentHand()->getRiver()->getHighestSet();
		} break;
		default: {
			// 
		}
	}
	return highestSet;
}

void
ClientStateRunHand::SetHighestSet(Game &curGame, int highestSet)
{
	// TODO: no switch needed here if game states are polymorphic
	switch(curGame.getCurrentHand()->getActualRound()) {
		case GAME_STATE_PREFLOP: {
			curGame.getCurrentHand()->getPreflop()->setHighestSet(highestSet);
		} break;
		case GAME_STATE_FLOP: {
			curGame.getCurrentHand()->getFlop()->setHighestSet(highestSet);
		} break;
		case GAME_STATE_TURN: {
			curGame.getCurrentHand()->getTurn()->setHighestSet(highestSet);
		} break;
		case GAME_STATE_RIVER: {
			curGame.getCurrentHand()->getRiver()->setHighestSet(highestSet);
		} break;
		default: {
			// 
		}
	}
}

int
ClientStateRunHand::GetPlayersTurn(Game &curGame)
{
	int playersTurn = 0;
	// TODO: no switch needed here if game states are polymorphic
	switch(curGame.getCurrentHand()->getActualRound()) {
		case GAME_STATE_PREFLOP: {
			playersTurn = curGame.getCurrentHand()->getPreflop()->getPlayersTurn();
		} break;
		case GAME_STATE_FLOP: {
			playersTurn = curGame.getCurrentHand()->getFlop()->getPlayersTurn();
		} break;
		case GAME_STATE_TURN: {
			playersTurn = curGame.getCurrentHand()->getTurn()->getPlayersTurn();
		} break;
		case GAME_STATE_RIVER: {
			playersTurn = curGame.getCurrentHand()->getRiver()->getPlayersTurn();
		} break;
		default: {
			// 
		}
	}
	return playersTurn;
}

void
ClientStateRunHand::SetPlayersTurn(Game &curGame, int playersTurn)
{
	// TODO: no switch needed here if game states are polymorphic
	switch(curGame.getCurrentHand()->getActualRound()) {
		case GAME_STATE_PREFLOP: {
			curGame.getCurrentHand()->getPreflop()->setPlayersTurn(playersTurn);
		} break;
		case GAME_STATE_FLOP: {
			curGame.getCurrentHand()->getFlop()->setPlayersTurn(playersTurn);
		} break;
		case GAME_STATE_TURN: {
			curGame.getCurrentHand()->getTurn()->setPlayersTurn(playersTurn);
		} break;
		case GAME_STATE_RIVER: {
			curGame.getCurrentHand()->getRiver()->setPlayersTurn(playersTurn);
		} break;
		default: {
			// 
		}
	}
}

void
ClientStateRunHand::ResetPlayerActions(Game &curGame)
{
	// Reset player actions
	for (int i = 0; i < MAX_NUMBER_OF_PLAYERS; i++)
	{
		int action = curGame.getPlayerArray()[i]->getMyAction();
		if (action != 1 && action != 6)
			curGame.getPlayerArray()[i]->setMyAction(0);
		curGame.getPlayerArray()[i]->setMySetNull();
	}
}

//-----------------------------------------------------------------------------

ClientStateFinal &
ClientStateFinal::Instance()
{
	static ClientStateFinal state;
	return state;
}

ClientStateFinal::ClientStateFinal()
{
}

ClientStateFinal::~ClientStateFinal()
{
}

int
ClientStateFinal::Process(ClientThread &client)
{
	Thread::Msleep(10);

	return MSG_SOCK_INTERNAL_PENDING;
}
