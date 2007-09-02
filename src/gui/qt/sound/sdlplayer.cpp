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

SDLPlayer::SDLPlayer(ConfigFile *c)
: soundData(NULL), currentChannel(0) , audioEnabled(0), myConfig(c)
{
	SDL_Init(SDL_INIT_AUDIO);
	initAudio();

	myQtHelper = new QtHelper;
}


SDLPlayer::~SDLPlayer()
{
	closeAudio();
	SDL_Quit();
}

void SDLPlayer::initAudio() {

	if (!audioEnabled && myConfig->readConfigInt("PlaySoundEffects"))
	{
		int		audio_rate = 44100;
		Uint16	audio_format = AUDIO_S16; /* 16-bit stereo */
		int		audio_channels = 2;
		int		audio_buffers = 4096;
		sound = NULL;

		if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) == 0) {
			Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
			audioEnabled = 1;
		}
	}
}

void SDLPlayer::playSound(string audioString, int playerID) {

	if(audioEnabled && myConfig->readConfigInt("PlaySoundEffects")) {
		
		QFile myFile(myQtHelper->getDataPath() + "sounds/default/" + QString::fromStdString(audioString)+".wav");
	
		if(myFile.open(QIODevice::ReadOnly)) {
	
			//set 3d position for player
			int position = 0;
			int distance = 0;
			
			switch (playerID) {

			case 0: { position = 180; distance = 10; }
			break;
			case 1: { position = 281; distance = 50; }
			break;
			case 2: { position = 315; distance = 120; }
			break;
			case 3: { position = 338; distance = 160; }
			break;
			case 4: { position = 23; distance = 160; }
			break;
			case 5: { position = 45; distance = 120; }
			break;
			case 6: { position = 79; distance = 50; }
			break;
			default: { position = 0; distance = 0; }
			break;
			}

			audioDone();
	
			QDataStream in(&myFile);
			soundData = new Uint8[(int)myFile.size()];
			in.readRawData( (char*)soundData, (int)myFile.size() );
			
			sound = Mix_QuickLoad_WAV(soundData); 
		
			 
  			// set channel 0 to settings volume
			Mix_Volume(0,myConfig->readConfigInt("SoundVolume")*10);

			// set 3d effect
			if(!Mix_SetPosition(0, position, distance)) {
    				printf("Mix_SetPosition: %s\n", Mix_GetError());
    				// no position effect, is it ok?
			}
			currentChannel = Mix_PlayChannel(-1, sound,0);
		}
	// 	else cout << "could not load " << audioString << ".wav" << endl;
	
		//test
	//	audioDone();       
	//	sound = Mix_LoadWAV( QString(QString::fromStdString(audioString)+QString(".wav")).toStdString().c_str() );  
	//	currentChannel = Mix_PlayChannel(-1, sound,0);

	}
}

void SDLPlayer::audioDone() {

	if(audioEnabled) {
		Mix_HaltChannel(currentChannel);
		Mix_FreeChunk(sound);
		sound = NULL;
		delete[] soundData;
		soundData = NULL;
	}
}

void SDLPlayer::closeAudio() {
	
	if(audioEnabled) {
		audioDone();
		Mix_CloseAudio();
		audioEnabled = false;
	}
}
