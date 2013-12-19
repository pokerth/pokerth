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
#include "soundevents.h"

#ifdef ANDROID
#ifdef ANDROID_API8
#include "androidapi8dummy.h"
#else
#include "androidaudio.h"
#endif
#else
#include "sdlplayer.h"
#endif


#include "configfile.h"
#include "session.h"
#include "game.h"

#include <QtCore>
#include <QtGui>

SoundEvents::SoundEvents(ConfigFile *c): myConfig(c), lastSBValue(0), lastSBLevel(0), newGameNow(false)
{

#ifdef ANDROID
#ifdef ANDROID_API8
	myPlayer = new AndroidApi8Dummy(myConfig);
#else
	myPlayer = new AndroidAudio(myConfig);
#endif
#else
	myPlayer = new SDLPlayer(myConfig);
#endif

}

SoundEvents::~SoundEvents()
{
	myPlayer->deleteLater();
}

void SoundEvents::reInitSoundEngine()
{
	myPlayer->reInit();
}

void SoundEvents::blindsWereSet(int sB)
{
	if(newGameNow) {
		lastSBLevel = 0;
		lastSBValue = sB;
		newGameNow = false;
	}

	if(sB > lastSBValue) {

		lastSBValue = sB;
		++lastSBLevel;

		if(myConfig->readConfigInt("PlayBlindRaiseNotification")) {
			if(lastSBLevel == 1 || lastSBLevel == 2) {
				myPlayer->playSound("blinds_raises_level1", 0);
			}
			if(lastSBLevel == 3 || lastSBLevel == 4) {
				myPlayer->playSound("blinds_raises_level2", 0);
			}
			if(lastSBLevel >= 5) {
				myPlayer->playSound("blinds_raises_level3", 0);
			}
		}
	}
}

void SoundEvents::newGameStarts()
{
	newGameNow = true;
}

//HACK until playSound is done by events
void SoundEvents::playSound(std::string audioString, int playerID)
{
	myPlayer->playSound(audioString, playerID);
}
