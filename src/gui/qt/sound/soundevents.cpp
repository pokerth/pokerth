#include "sdlplayer.h"
#include "configfile.h"
#include "gametableimpl.h"
#include "session.h"
#include "game.h"
#include "soundevents.h"

SoundEvents::SoundEvents(ConfigFile *c, gameTableImpl *w): myConfig(c), myW(w), lastSBValue(0), lastSBLevel(0), lastGameId(0)
{
	mySDLPlayer = new SDLPlayer(myConfig);
}

SoundEvents::~SoundEvents()
{
	mySDLPlayer->deleteLater();
}

void SoundEvents::blindsWereSet(int sB)
{
	int currentGameId = myW->getSession()->getCurrentGame()->getMyGameID();
	if(currentGameId != lastGameId)
	{
		lastSBLevel = 0;
		lastSBValue = sB;
		lastGameId = currentGameId;
	}

	if(sB > lastSBValue) {

		lastSBValue = sB;
		++lastSBLevel;

		if(lastSBLevel == 1 || lastSBLevel == 2)
		{
			mySDLPlayer->playSound("blinds_raises_level1", 0);
		}
		if(lastSBLevel == 3 || lastSBLevel == 4)
		{
			mySDLPlayer->playSound("blinds_raises_level2", 0);
		}
		if(lastSBLevel >= 5)
		{
			mySDLPlayer->playSound("blinds_raises_level3", 0);
		}
	}
}
