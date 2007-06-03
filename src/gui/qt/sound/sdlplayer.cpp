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

SDLPlayer::SDLPlayer()
: currentChannel(0), soundData(NULL)
{
	initAudio();
}


SDLPlayer::~SDLPlayer()
{
	closeAudio();
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

	QFile myFile(":sounds/resources/sounds/"+QString::fromStdString(audioString)+".wav");

	if(myFile.open(QIODevice::ReadOnly)) {

		audioDone();

		QDataStream in(&myFile);
		soundData = new Uint8[(int)myFile.size()];
		in.readRawData( (char*)soundData, (int)myFile.size() );
		
		sound = Mix_QuickLoad_WAV(soundData); 
	
		currentChannel = Mix_PlayChannel(-1, sound,0);
	}
// 	else cout << "could not load " << audioString << ".wav" << endl;

	//test
//	audioDone();       
//	sound = Mix_LoadWAV( QString(QString::fromStdString(audioString)+QString(".wav")).toStdString().c_str() );  
//	currentChannel = Mix_PlayChannel(-1, sound,0);

}

void SDLPlayer::audioDone() {

	Mix_HaltChannel(currentChannel);
	Mix_FreeChunk(sound);
	sound = NULL;
	delete[] soundData;
	soundData = NULL;
}

void SDLPlayer::closeAudio() {

	audioDone();
	Mix_CloseAudio();
	SDL_Quit();
}
