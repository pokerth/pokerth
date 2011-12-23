#ifndef SOUNDEVENTS_H
#define SOUNDEVENTS_H

class SDLPlayer;
class ConfigFile;
class SoundEvents
{
public:
	SoundEvents(ConfigFile*);
private:
	SDLPlayer *mySDLPlayer;
	ConfigFile *myConfig;


};

#endif // SOUNDEVENTS_H
