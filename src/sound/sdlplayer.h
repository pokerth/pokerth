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
#ifndef SDLPLAYER_H
#define SDLPLAYER_H

#include <iostream>
#include <string>

#include "SDL/SDL.h"
#include "SDL/SDL_mixer.h"

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class SDLPlayer{
public:
    SDLPlayer();

    ~SDLPlayer();

	void initAudio();
	void playSound(std::string);
	void audioDone();
	void closeAudio();

private:

	int audio_rate;
  	Uint16 audio_format;
 	int audio_channels;
  	int audio_buffers;
	Mix_Music *music;

};

#endif
