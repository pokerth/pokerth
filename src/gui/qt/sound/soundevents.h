#ifndef SOUNDEVENTS_H
#define SOUNDEVENTS_H

#include <string>

#ifdef ANDROID
#ifdef ANDROID_API8
class AndroidApi8Dummy;
#else
class AndroidAudio;
#endif
#else
class SDLPlayer;
#endif


class ConfigFile;
class SoundEvents
{
public:
	SoundEvents(ConfigFile*);
	~SoundEvents();

	void blindsWereSet(int sB);
	void newGameStarts();
	void playSound(std::string audioString, int playerID);
	void reInitSoundEngine();

protected:
	// Prevent copy construction.
	// This is only a declaration.
	SoundEvents(const SoundEvents &);

private:

#ifdef ANDROID
#ifdef ANDROID_API8
	AndroidApi8Dummy *myPlayer;
#else
	AndroidAudio *myPlayer;
#endif
#else
	SDLPlayer *myPlayer;
#endif
	ConfigFile *myConfig;
	int lastSBValue;
	unsigned int lastSBLevel;
	bool newGameNow;

};

#endif // SOUNDEVENTS_H
