/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer LMay   *
 *   f.thauer@web.de   *
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
#include "startnetworkgamedialogimpl.h"
#include "startwindowimpl.h"
#include "session.h"
#include "configfile.h"
#include "chattools.h"
#include <net/socket_msg.h>

startNetworkGameDialogImpl::startNetworkGameDialogImpl(startWindowImpl *parent, ConfigFile *config)
      : QDialog(parent), myW(NULL), myStartWindow(parent), keyUpDownChatCounter(0), myPlayerId(0), isAdmin(false), myConfig(config), myChat(NULL)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif	
	setupUi(this);

	myChat = new ChatTools(lineEdit_ChatInput, myConfig, 1, textBrowser_ChatDisplay, treeWidget );

	lineEdit_ChatInput->installEventFilter(this);

	connect( pushButton_cancel, SIGNAL( clicked() ), this, SLOT( cancel() ) );
	connect( pushButton_startGame, SIGNAL( clicked() ), this, SLOT( startGame() ) );
	connect( pushButton_Kick, SIGNAL( clicked() ), this, SLOT( kickPlayer() ) );
	connect( treeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem*) ), this, SLOT( playerSelected(QTreeWidgetItem*, QTreeWidgetItem*) ) );
	connect( lineEdit_ChatInput, SIGNAL( returnPressed () ), myChat, SLOT( sendMessage() ) );
	connect( lineEdit_ChatInput, SIGNAL( textChanged (QString) ), myChat, SLOT( checkInputLength(QString) ) );
	connect( lineEdit_ChatInput, SIGNAL( textEdited (QString) ), myChat, SLOT( setChatTextEdited() ) );

	clearDialog();
}

void startNetworkGameDialogImpl::exec() {

	QDialog::exec();
}

void startNetworkGameDialogImpl::startGame() {
	assert(mySession);
	mySession->sendStartEvent(checkBox_fillUpWithComputerOpponents->isChecked());
}

void startNetworkGameDialogImpl::cancel() {
	
}

void startNetworkGameDialogImpl::refresh(int actionID) {

	if (actionID == MSG_NET_GAME_CLIENT_START)
	{
		QTimer::singleShot(500, this, SLOT(accept()));
	}
}

void startNetworkGameDialogImpl::joinedNetworkGame(unsigned playerId, QString playerName, int rights) {

	myPlayerId = playerId;
	isAdmin = rights == PLAYER_RIGHTS_ADMIN;
	addConnectedPlayer(playerId, playerName, rights);
}

void startNetworkGameDialogImpl::addConnectedPlayer(unsigned playerId, QString playerName, int /*rights*/) {

	// TODO mark admin
	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget, 0);
	item->setData(0, Qt::UserRole, playerId);
	item->setData(0, Qt::DisplayRole, playerName);

	GameInfo info = mySession->getClientGameInfo(1);

	if(this->isVisible() && myConfig->readConfigInt("PlayNetworkGameNotification")) {
		if(treeWidget->topLevelItemCount() < info.data.maxNumberOfPlayers) {
			myW->getMySDLPlayer()->playSound("playerconnected", 0);
		}
		else {
			myW->getMySDLPlayer()->playSound("onlinegameready", 0);
		}
	}

	checkPlayerQuantity();
}

void startNetworkGameDialogImpl::updatePlayer(unsigned playerId, QString newPlayerName)
{
	QTreeWidgetItemIterator it(treeWidget);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == playerId)
		{
			(*it)->setData(0, Qt::DisplayRole, newPlayerName);
			break;
		}
		++it;
	}
}

void startNetworkGameDialogImpl::removePlayer(unsigned playerId, QString) {

	QTreeWidgetItemIterator it(treeWidget);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == playerId)
		{
			treeWidget->takeTopLevelItem(treeWidget->indexOfTopLevelItem(*it));
			break;
		}
		++it;
	}

	checkPlayerQuantity();
}

void startNetworkGameDialogImpl::newGameAdmin(unsigned playerId, QString)
{
	if (myPlayerId == playerId)
	{
		isAdmin = true;
		checkPlayerQuantity();
	}
}

void startNetworkGameDialogImpl::gameCreated(unsigned /*gameId*/)
{
	GameInfo info = mySession->getClientGameInfo(1);
	label_maxPlayerNumber->setText(QString::number(info.data.maxNumberOfPlayers));
}

void startNetworkGameDialogImpl::playerSelected(QTreeWidgetItem* item, QTreeWidgetItem*) {

	if (item)
		pushButton_Kick->setEnabled(isAdmin);
}

void startNetworkGameDialogImpl::kickPlayer() {

	
	QTreeWidgetItem *item = treeWidget->currentItem();
	if (item)
	{
		QString playerName = item->text(0);
		if(playerName == QString::fromUtf8(myConfig->readConfigString("MyName").c_str())) {
			{ QMessageBox::warning(this, tr("Server Error"),
					tr("You should not kick yourself from this game!"),
					QMessageBox::Close); }
		}
		else {
			assert(mySession);
			mySession->kickPlayer(item->data(0, Qt::UserRole).toUInt());
		}
	}
	pushButton_Kick->setEnabled(false);
}

void startNetworkGameDialogImpl::checkPlayerQuantity() {

	if(isAdmin){
		pushButton_Kick->show();
		pushButton_startGame->show();
		checkBox_fillUpWithComputerOpponents->show();
		
		if (treeWidget->topLevelItemCount() >= 2) {
			pushButton_startGame->setEnabled(true);
		}
		else {
			pushButton_startGame->setEnabled(false);
		}
	}
}

void startNetworkGameDialogImpl::clearDialog()
{
	pushButton_Kick->hide();
	pushButton_startGame->hide();
	pushButton_Kick->setEnabled(false);
	pushButton_startGame->setEnabled(false);
	treeWidget->clear();
	myChat->clearChat();
	checkBox_fillUpWithComputerOpponents->hide();
	label_maxPlayerNumber->setText(QString::number(0));

	myPlayerId = 0;
}

void startNetworkGameDialogImpl::setSession(boost::shared_ptr<Session> session)
{
	mySession = session;
	myChat->setSession(mySession);
}

void startNetworkGameDialogImpl::keyPressEvent ( QKeyEvent * event ) {

	if (event->key() == Qt::Key_Up && lineEdit_ChatInput->hasFocus()) { 
		if((keyUpDownChatCounter + 1) <= myChat->getChatLinesHistorySize()) { keyUpDownChatCounter++; }
// 		std::cout << "Up keyUpDownChatCounter: " << keyUpDownChatCounter << "\n";
		myChat->showChatHistoryIndex(keyUpDownChatCounter); 
	}
	else if(event->key() == Qt::Key_Down && lineEdit_ChatInput->hasFocus()) { 
		if((keyUpDownChatCounter - 1) >= 0) { keyUpDownChatCounter--; }
// 		std::cout << "Down keyUpDownChatCounter: " << keyUpDownChatCounter << "\n";
		myChat->showChatHistoryIndex(keyUpDownChatCounter); 
	}
	else { keyUpDownChatCounter = 0; }
}

bool startNetworkGameDialogImpl::eventFilter(QObject *obj, QEvent *event)
{
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

	if (obj == lineEdit_ChatInput && lineEdit_ChatInput->text() != "" && event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Tab) 
	{
		myChat->nickAutoCompletition();
		return true;
	} else {
		// pass the event on to the parent class
		return QDialog::eventFilter(obj, event);
	}
}

void startNetworkGameDialogImpl::receiveChatMsg(QString playerName, QString message) {
	myChat->receiveMessage(playerName, message);
}

void startNetworkGameDialogImpl::accept()
{
	myW->show();
	QDialog::accept();
}

void startNetworkGameDialogImpl::reject()
{
	myStartWindow->show();
	QDialog::reject();
}
