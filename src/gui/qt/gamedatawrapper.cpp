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

#include "gamedatawrapper.h"

GameDataWrapper::GameDataWrapper(const GameData &gameData)
: m_gameData(gameData)
{
}

GameDataWrapper::GameDataWrapper(const GameDataWrapper &other)
: m_gameData(other.GetGameData())
{
}

const GameData&
GameDataWrapper::GetGameData() const
{
	return m_gameData;
}

const GameDataWrapper &
GameDataWrapper::operator=(const GameDataWrapper &other)
{
	if (this != &other)
		m_gameData = other.GetGameData();
	return *this;
}

