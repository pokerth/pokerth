/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
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
#ifndef LOBBYCHAT_H
#define LOBBYCHAT_H

#include <string>
#include <QtCore>

class gameLobbyDialogImpl;
class Session;
class ConfigFile;

class LobbyChat : public QObject
{
Q_OBJECT

public:
	LobbyChat(gameLobbyDialogImpl*, ConfigFile*);

	~LobbyChat();

public slots:
	
	void sendMessage();
	void connected(QString server);
	void selfJoined(QString ownName, QString channel);
	void playerJoined(QString playerName);
	void playerChanged(QString oldNick, QString newNick);
	void playerKicked(QString nickName, QString byWhom, QString reason);
	void playerLeft(QString playerName);
	void displayMessage(QString playerName, QString msg);
	void checkInputLength(QString);
	void clearChat();

	void chatError(int errorCode);
	void chatServerError(int errorCode);

private:
	gameLobbyDialogImpl *myLobby;
	ConfigFile *myConfig;

	QString myNick;
	
// friend class GuiWrapper;
};

#endif
