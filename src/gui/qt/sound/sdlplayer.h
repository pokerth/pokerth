//
// C++ Interface: sdlplayer
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <QtCore>

#ifndef SDLPLAYER_H
#define SDLPLAYER_H

#include <string>
#include "configfile.h"
#include "qthelper.h"

struct Mix_Chunk;

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class SDLPlayer : public QObject{
Q_OBJECT

public:
	SDLPlayer(ConfigFile*);

	~SDLPlayer();

	void initAudio();
	void playSound(std::string, int playerID);
	void audioDone();
	void closeAudio();

private:

	Mix_Chunk *sound;
	unsigned char *soundData;
	int currentChannel;

	bool audioEnabled;

	ConfigFile *myConfig;
	QtHelper *myQtHelper;

};

#endif
