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
#ifndef GUIWRAPPER_H
#define GUIWRAPPER_H

#include <guiinterface.h>
#include <string>
#include <iostream>

class mainWindowImpl;
class Log;



class GuiWrapper : public GuiInterface
{
public:
    GuiWrapper(mainWindowImpl*);

    ~GuiWrapper();

	void setGame(Game*);
	void setHand(LocalHand*);
	void setSession(Session*);	

	int getMaxQuantityPlayers() const;

	void refreshSet() const;
	void refreshChangePlayer() const;
	void refreshPot() const;
	void refreshGroupbox() const;
	void refreshAll() const;
	void refreshPlayerName() const;
	
	void dealHoleCards() const;
	void dealFlopCards() const;
	void dealTurnCard() const;
	void dealRiverCard() const;

	void highlightRoundLabel(std::string round) const;

	void nextPlayerAnimation() const;

	void preflopAnimation1() const;
	void preflopAnimation2() const;
	
	void flopAnimation1() const;
	void flopAnimation2() const;

	void turnAnimation1() const;
	void turnAnimation2() const;

	void riverAnimation1() const;
	void riverAnimation2() const;

	void postRiverAnimation1() const;
	void postRiverRunAnimation1() const;

	void nextRoundCleanGui() const;

	void meInAction() const;

	void showPlayerActionLogMsg(std::string playerName, int action, int setValue) const;

private: 

	mainWindowImpl *myW;
	Log *myLog;
	

};

#endif
