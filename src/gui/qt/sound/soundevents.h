#ifndef SOUNDEVENTS_H
#define SOUNDEVENTS_H

class SDLPlayer;
class ConfigFile;
class gameTableImpl;
class SoundEvents
{
public:
	SoundEvents(ConfigFile*, gameTableImpl*);
	~SoundEvents();

	void blindsWereSet(int sB);


private:
	SDLPlayer *mySDLPlayer;
	gameTableImpl *myW;
	ConfigFile *myConfig;

	unsigned int lastSBValue;
	unsigned int lastSBLevel;
	unsigned int lastGameId;

};

#endif // SOUNDEVENTS_H
