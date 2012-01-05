#include "sdlplayer.h"
#include "configfile.h"
#include "session.h"
#include "game.h"
#include "soundevents.h"

SoundEvents::SoundEvents(ConfigFile *c): myConfig(c), lastSBValue(0), lastSBLevel(0), newGameNow(false)
{
	mySDLPlayer = new SDLPlayer(myConfig);
}

SoundEvents::~SoundEvents()
{
	mySDLPlayer->deleteLater();
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

		if(lastSBLevel == 1 || lastSBLevel == 2) {
			mySDLPlayer->playSound("blinds_raises_level1", 0);
		}
		if(lastSBLevel == 3 || lastSBLevel == 4) {
			mySDLPlayer->playSound("blinds_raises_level2", 0);
		}
		if(lastSBLevel >= 5) {
			mySDLPlayer->playSound("blinds_raises_level3", 0);
		}
	}
}

void SoundEvents::newGameStarts()
{
	newGameNow = true;
}
