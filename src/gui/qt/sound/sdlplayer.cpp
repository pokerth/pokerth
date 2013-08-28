/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
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
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#include "sdlplayer.h"

#if (defined __APPLE__)
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

using namespace std;

SDLPlayer::SDLPlayer(ConfigFile *c)
	: sound(NULL), soundData(NULL), currentChannel(0), audioEnabled(false), myConfig(c)
{
	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());
	SDL_Init(SDL_INIT_AUDIO);
	initAudio();
}


SDLPlayer::~SDLPlayer()
{
	closeAudio();
	SDL_Quit();
}

void SDLPlayer::initAudio()
{
	if (!audioEnabled && myConfig->readConfigInt("PlaySoundEffects")) {
		int		audio_rate = 44100;
		Uint16	audio_format = AUDIO_S16; /* 16-bit stereo */
		int		audio_channels = 2;
		int		audio_buffers = 4096;
		sound = NULL;

		if( Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) == 0) {
			Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
			audioEnabled = 1;
		} else {
			qDebug() << "Mix_OpenAudio() was not successfull, no sound possible :(";
		}
	}
}

void SDLPlayer::playSound(string audioString, int playerID)
{
	if(audioEnabled && myConfig->readConfigInt("PlaySoundEffects")) {

		QFile myFile(myAppDataPath + "sounds/default/" + QString::fromStdString(audioString)+".wav");

		if(myFile.open(QIODevice::ReadOnly)) {

			//set 3d position for player
			int position = 0;
			int distance = 0;

			switch (playerID) {

			case 0: {
				position = 180;
				distance = 10;
			}
			break;
			case 1: {
				position = 281;
				distance = 50;
			}
			break;
			case 2: {
				position = 315;
				distance = 120;
			}
			break;
			case 3: {
				position = 338;
				distance = 160;
			}
			break;
			case 4: {
				position = 23;
				distance = 160;
			}
			break;
			case 5: {
				position = 45;
				distance = 120;
			}
			break;
			case 6: {
				position = 79;
				distance = 50;
			}
			break;
			default: {
				position = 0;
				distance = 0;
			}
			break;
			}

			QDataStream in(&myFile);
			soundData = new Uint8[(int)myFile.size()];
			in.readRawData( (char*)soundData, (int)myFile.size() );
			sound = Mix_QuickLoad_WAV(soundData);

			// set channel 0 to settings volume
			Mix_Volume(-1,myConfig->readConfigInt("SoundVolume")*10);

			// set 3d effect
			if(!Mix_SetPosition(0, position, distance)) {
				printf("Mix_SetPosition: %s\n", Mix_GetError());
				// no position effect, is it ok?
			}
			currentChannel = Mix_PlayChannel(-1, sound,0);
		}
	}
}

void SDLPlayer::closeAudio()
{
	if(audioEnabled) {
		Mix_HaltChannel(currentChannel);
		Mix_FreeChunk(sound);
		sound = NULL;
		delete[] soundData;
		soundData = NULL;
		Mix_CloseAudio();
		qDebug() << "Mix_CloseAudio()";
		audioEnabled = false;
	}
}

void SDLPlayer::reInit()
{
	closeAudio();
	initAudio();
}
