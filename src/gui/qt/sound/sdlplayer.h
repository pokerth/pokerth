//
// C++ Interface: sdlplayer
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <QtCore>

#ifndef SDLPLAYER_H
#define SDLPLAYER_H

#ifdef _WIN32 
	#include <SDL.h>
	#include <SDL_mixer.h>
#else
	#ifdef __APPLE__
		#include <SDL.h>
		#include <SDL_mixer.h>
	#else 
		#include <SDL/SDL.h>
		#include <SDL/SDL_mixer.h>
	#endif
#endif

#include <string>
#include "configfile.h"
#include "qthelper.h"

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class SDLPlayer : public QObject{
Q_OBJECT

public:
	SDLPlayer(ConfigFile*);

	~SDLPlayer();

	void initAudio();
	void playSound(std::string, int playerID);
	void audioDone();
	void closeAudio();

private:

	int audio_rate;
	Uint16 audio_format;
	int audio_channels;
	int audio_buffers;
	Mix_Chunk *sound;
	Uint8 *soundData;
	int currentChannel;

	bool audioEnabled;

	ConfigFile *myConfig;
	QtHelper *myQtHelper;

};

#endif
