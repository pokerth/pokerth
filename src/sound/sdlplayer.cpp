//
// C++ Implementation: sdlplayer
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "sdlplayer.h"

using namespace std;

SDLPlayer::SDLPlayer()
{
	initAudio();
	
}


SDLPlayer::~SDLPlayer()
{
}

void SDLPlayer::initAudio() {

	audio_rate = 44100;
  	audio_format = AUDIO_S16; /* 16-bit stereo */
  	audio_channels = 2;
  	audio_buffers = 4096;
	music = NULL;

	SDL_Init(SDL_INIT_AUDIO);

	if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
		printf("Unable to open audio!\n");
		exit(1);
	}
	
	Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
}

void SDLPlayer::playSound(std::string audioString) {

	cout << "play now" << endl;
	/* Actually loads up the music */
	string oggString = ".ogg";
// 	music = Mix_LoadMUS( audioString+oggString );
	
	/* This begins playing the music - the first argument is a
	pointer to Mix_Music structure, and the second is how many
	times you want it to loop (use -1 for infinite, and 0 to
	have it just play once) */
	Mix_PlayMusic(music, 0);

	/* We want to know when our music has stopped playing so we
           can free it up and set 'music' back to NULL.  SDL_Mixer
           provides us with a callback routine we can use to do
           exactly that */
//         Mix_HookMusicFinished(mainWindowImpl::musicDone());

}

void SDLPlayer::audioDone() {

	Mix_HaltMusic();
  	Mix_FreeMusic(music);
  	music = NULL;
}

void SDLPlayer::closeAudio() {

	Mix_HaltMusic();
	Mix_FreeMusic(music);
  	music = NULL;
	Mix_CloseAudio();
  	SDL_Quit();
}
