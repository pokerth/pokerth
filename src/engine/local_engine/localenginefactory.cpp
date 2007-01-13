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
#include "localenginefactory.h"

#include "localhand.h"
#include "handinterface.h"

#include "localboard.h"
#include "boardinterface.h"

#include "localplayer.h"
#include "playerinterface.h"


LocalEngineFactory::LocalEngineFactory()
 : EngineFactory()
{
}


LocalEngineFactory::~LocalEngineFactory()
{
}


HandInterface* LocalEngineFactory::createHand() {

	HandInterface *hi = new LocalHand();	
	return hi;	
}


BoardInterface* LocalEngineFactory::createBoard() {

	BoardInterface *bi = new LocalBoard();	
	return bi;	
}


PlayerInterface* LocalEngineFactory::createPlayer() {

	PlayerInterface *pi = new LocalPlayer();	
	return pi;	

}
