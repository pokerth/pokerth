/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#include "startnetworkgamedialogimpl.h"
#include "startwindowimpl.h"
#include "session.h"
#include "configfile.h"
#include "chattools.h"
#include "soundevents.h"
#include <net/socket_msg.h>

startNetworkGameDialogImpl::startNetworkGameDialogImpl(startWindowImpl *parent, ConfigFile *config)
	: QDialog(parent), myW(NULL), myStartWindow(parent), keyUpDownChatCounter(0), myPlayerId(0), isAdmin(false), myConfig(config), myChat(NULL)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	setupUi(this);
#ifdef ANDROID
	this->setWindowState(Qt::WindowFullScreen);
#endif
	myChat = new ChatTools(lineEdit_ChatInput, myConfig, LAN_LOBBY_CHAT, textBrowser_ChatDisplay);

	lineEdit_ChatInput->installEventFilter(this);

#ifndef GUI_800x480
	connect( pushButton_cancel, SIGNAL( clicked() ), this, SLOT( cancel() ) );
#endif
	connect( pushButton_startGame, SIGNAL( clicked() ), this, SLOT( startGame() ) );
	connect( pushButton_Kick, SIGNAL( clicked() ), this, SLOT( kickPlayer() ) );
	connect( treeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem*) ), this, SLOT( playerSelected(QTreeWidgetItem*, QTreeWidgetItem*) ) );
	connect( lineEdit_ChatInput, SIGNAL( returnPressed () ), myChat, SLOT( sendMessage() ) );
	connect( lineEdit_ChatInput, SIGNAL( textChanged (QString) ), myChat, SLOT( checkInputLength(QString) ) );
	connect( lineEdit_ChatInput, SIGNAL( textEdited (QString) ), myChat, SLOT( setChatTextEdited() ) );

	clearDialog();
}

int startNetworkGameDialogImpl::exec()
{
	return QDialog::exec();
}

void startNetworkGameDialogImpl::startGame()
{
	assert(mySession);
	mySession->sendStartEvent(checkBox_fillUpWithComputerOpponents->isChecked());
}

void startNetworkGameDialogImpl::cancel()
{

}

void startNetworkGameDialogImpl::refresh(int actionID)
{

	if (actionID == MSG_NET_GAME_CLIENT_START) {
		QTimer::singleShot(500, this, SLOT(accept()));
	}
}

void startNetworkGameDialogImpl::joinedNetworkGame(unsigned playerId, QString playerName, bool admin)
{

	myPlayerId = playerId;
	isAdmin = admin;
	addConnectedPlayer(playerId, playerName, admin);
}

void startNetworkGameDialogImpl::addConnectedPlayer(unsigned playerId, QString playerName, bool)
{

	// TODO mark admin
	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget, 0);
	item->setData(0, Qt::UserRole, playerId);
	item->setData(0, Qt::DisplayRole, playerName);

	GameInfo info = mySession->getClientGameInfo(1);

	if(this->isVisible() && myConfig->readConfigInt("PlayNetworkGameNotification")) {
		if(treeWidget->topLevelItemCount() < info.data.maxNumberOfPlayers) {
			myW->getMySoundEventHandler()->playSound("playerconnected", 0);
		} else {
			myW->getMySoundEventHandler()->playSound("onlinegameready", 0);
		}
	}

	checkPlayerQuantity();
}

void startNetworkGameDialogImpl::updatePlayer(unsigned playerId, QString newPlayerName)
{
	QTreeWidgetItemIterator it(treeWidget);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == playerId) {
			(*it)->setData(0, Qt::DisplayRole, newPlayerName);
			break;
		}
		++it;
	}
}

void startNetworkGameDialogImpl::removePlayer(unsigned playerId, QString)
{

	QTreeWidgetItemIterator it(treeWidget);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == playerId) {
			treeWidget->takeTopLevelItem(treeWidget->indexOfTopLevelItem(*it));
			break;
		}
		++it;
	}

	checkPlayerQuantity();
}

void startNetworkGameDialogImpl::newGameAdmin(unsigned playerId, QString)
{
	if (myPlayerId == playerId) {
		isAdmin = true;
		checkPlayerQuantity();
	}
}

void startNetworkGameDialogImpl::gameCreated(unsigned /*gameId*/)
{
	GameInfo info = mySession->getClientGameInfo(1);
	label_maxPlayerNumber->setText(QString::number(info.data.maxNumberOfPlayers));
}

void startNetworkGameDialogImpl::playerSelected(QTreeWidgetItem* item, QTreeWidgetItem*)
{

	if (item)
		pushButton_Kick->setEnabled(isAdmin);
}

void startNetworkGameDialogImpl::kickPlayer()
{


	QTreeWidgetItem *item = treeWidget->currentItem();
	if (item) {
		QString playerName = item->text(0);
		if(playerName == QString::fromUtf8(myConfig->readConfigString("MyName").c_str())) {
			{
				MyMessageBox::warning(this, tr("Server Error"),
									  tr("You should not kick yourself from this game!"),
									  QMessageBox::Close);
			}
		} else {
			assert(mySession);
			mySession->kickPlayer(item->data(0, Qt::UserRole).toUInt());
		}
	}
	pushButton_Kick->setEnabled(false);
}

void startNetworkGameDialogImpl::checkPlayerQuantity()
{

	if(isAdmin) {
		pushButton_Kick->show();
		pushButton_startGame->show();
		checkBox_fillUpWithComputerOpponents->show();

		if (treeWidget->topLevelItemCount() >= 2) {
			pushButton_startGame->setEnabled(true);
		} else {
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

void startNetworkGameDialogImpl::keyPressEvent ( QKeyEvent * event )
{

	if (event->key() == Qt::Key_Up && lineEdit_ChatInput->hasFocus()) {
		if((keyUpDownChatCounter + 1) <= myChat->getChatLinesHistorySize()) {
			keyUpDownChatCounter++;
		}
// 		std::cout << "Up keyUpDownChatCounter: " << keyUpDownChatCounter << "\n";
		myChat->showChatHistoryIndex(keyUpDownChatCounter);
	} else if(event->key() == Qt::Key_Down && lineEdit_ChatInput->hasFocus()) {
		if((keyUpDownChatCounter - 1) >= 0) {
			keyUpDownChatCounter--;
		}
// 		std::cout << "Down keyUpDownChatCounter: " << keyUpDownChatCounter << "\n";
		myChat->showChatHistoryIndex(keyUpDownChatCounter);
	} else {
		keyUpDownChatCounter = 0;
	}
}

bool startNetworkGameDialogImpl::eventFilter(QObject *obj, QEvent *event)
{
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

	if (obj == lineEdit_ChatInput && lineEdit_ChatInput->text() != "" && event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Tab) {
		myChat->nickAutoCompletition();
		return true;
	} else {
		// pass the event on to the parent class
		return QDialog::eventFilter(obj, event);
	}
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
