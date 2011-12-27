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
	ConfigFile *myConfig;
	gameTableImpl *myW;

	unsigned int lastSBValue;
	unsigned int lastSBLevel;
	unsigned int lastGameId;

};

#endif // SOUNDEVENTS_H
