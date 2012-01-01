#ifndef SOUNDEVENTS_H
#define SOUNDEVENTS_H

class SDLPlayer;
class ConfigFile;
class SoundEvents
{
public:
	SoundEvents(ConfigFile*);
	~SoundEvents();

	void blindsWereSet(int sB);
	void newGameStarts();

private:
	SDLPlayer *mySDLPlayer;
	ConfigFile *myConfig;

	int lastSBValue;
	unsigned int lastSBLevel;
	bool newGameNow;

};

#endif // SOUNDEVENTS_H
