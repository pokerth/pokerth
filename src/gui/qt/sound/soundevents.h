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

protected:
	// Prevent copy construction.
	// This is only a declaration.
	SoundEvents(const SoundEvents &);

private:
	SDLPlayer *mySDLPlayer;
	ConfigFile *myConfig;

	int lastSBValue;
	unsigned int lastSBLevel;
	bool newGameNow;

};

#endif // SOUNDEVENTS_H
