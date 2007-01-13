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

class mainWindowImpl;
class Log;



class GuiWrapper : public GuiInterface
{
public:
    GuiWrapper(mainWindowImpl*);

    ~GuiWrapper();

	

	virtual void showPlayerActionLogMsg(std::string playerName, int &action, int &setValue) const;
	virtual void refreshSet() const;
	virtual void refreshChangePlayer() const;

	virtual void dealFlopCards() const;
	virtual void dealTurnCard() const;
	virtual void dealRiverCard() const;

	virtual void nextPlayerAnimation() const;

	virtual void preflopAnimation1() const;
	virtual void preflopAnimation2() const;
	
	virtual void flopAnimation1() const;
	virtual void flopAnimation2() const;

	virtual void turnAnimation1() const;
	virtual void turnAnimation2() const;

	virtual void riverAnimation1() const;
	virtual void riverAnimation2() const;

	virtual void postRiverAnimation1() const;
// 	virtual void postRiverAnimation2() const;

private: 

	mainWindowImpl *myW;
	Log *myLog;
	

};

#endif
