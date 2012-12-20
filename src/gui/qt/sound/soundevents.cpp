#include "soundevents.h"

#ifdef ANDROID
    #include "androidaudio.h"
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
    myPlayer = new AndroidAudio(myConfig);
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
