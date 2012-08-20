/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/
#include <QtCore>

#ifndef SDLPLAYER_H
#define SDLPLAYER_H

#include "configfile.h"
#include <string>

#ifndef ANDROID
#ifdef __APPLE__
#include <SDL_mixer.h>
#else
#include <SDL/SDL_mixer.h>
#endif
#endif

// struct Mix_Chunk;

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class SDLPlayer : public QObject
{
	Q_OBJECT

public:
	SDLPlayer(ConfigFile*);

	~SDLPlayer();

	void initAudio();
	void playSound(std::string, int playerID);
	void closeAudio();

private:

#ifndef ANDROID
	Mix_Chunk *sound;
	unsigned char *soundData;
	int currentChannel;

	bool audioEnabled;

	ConfigFile *myConfig;
	QString myAppDataPath;
#endif
};

#endif
