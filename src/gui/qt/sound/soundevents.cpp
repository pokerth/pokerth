#include "sdlplayer.h"
#include "configfile.h"
#include "soundevents.h"

SoundEvents::SoundEvents(ConfigFile *c): myConfig(c)
{
	mySDLPlayer = new SDLPlayer(myConfig);
}
