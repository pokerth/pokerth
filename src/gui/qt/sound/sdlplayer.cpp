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

#include <iostream>

using namespace std;

SDLPlayer::SDLPlayer():QObject()
{
	currentChannel = 0;
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
	sound = NULL;

	SDL_Init(SDL_INIT_AUDIO);

	if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) == 0)
		Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
}

void SDLPlayer::playSound(string audioString) {

	audioDone();

	QFile myFile(":sounds/resources/sounds/"+QString::fromStdString(audioString)+".wav");
        if(!myFile.open(QIODevice::ReadOnly)) cout << "could not load wav" << endl;
        
	QDataStream in(&myFile);
        Uint8 *myMem = new Uint8[(int)myFile.size()];
        in.readRawData( (char*)myMem, (int)myFile.size() );
	
        sound = Mix_QuickLoad_WAV( myMem ); 

	currentChannel = Mix_PlayChannel(0, sound,0);

	delete[] myMem; 

}

void SDLPlayer::audioDone() {

	Mix_HaltChannel(currentChannel);
	Mix_FreeChunk(sound);
	sound = NULL;
}

void SDLPlayer::closeAudio() {

	Mix_HaltChannel(currentChannel);
	Mix_FreeChunk(sound);
	sound = NULL;
	Mix_CloseAudio();
	SDL_Quit();
}
