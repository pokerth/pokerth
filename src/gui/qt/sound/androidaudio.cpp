/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#include "androidaudio.h"
#include "androidsoundeffect.h"
#include "configfile.h"

#include <QDebug>

AndroidAudio::AndroidAudio(ConfigFile *c, QObject *parent) :
	QObject(parent), mEngineObject(NULL), mEngineEngine(NULL), mOutputMixObject(NULL), mSounds(), mSoundCount(0), mPlayerObject(NULL), myConfig(c), audioEnabled(false)
{
	initAudio();
}

AndroidAudio::~AndroidAudio()
{
	closeAudio();
}

void AndroidAudio::initAudio()
{
	if (!audioEnabled && myConfig->readConfigInt("PlaySoundEffects")) {
		createEngine();
		startSoundPlayer();
		audioEnabled = true;
	}
}

void AndroidAudio::closeAudio()
{

	if(audioEnabled) {
		destroyEngine();

		for (int32_t i = 0; i < mSoundCount; ++i) {
			qDeleteAll(mSounds);
		}
		audioEnabled = false;
	}
}

void AndroidAudio::reInit()
{
	initAudio();
}

// create the engine and output mix objects
void AndroidAudio::createEngine()
{
	SLresult result;

	// create engine
	result = slCreateEngine(&mEngineObject, 0, NULL, 0, NULL, NULL);
	Q_ASSERT(SL_RESULT_SUCCESS == result);

	// realize the engine
	result = (*mEngineObject)->Realize(mEngineObject, SL_BOOLEAN_FALSE);
	Q_ASSERT(SL_RESULT_SUCCESS == result);

	// get the engine interface, which is needed in order to create other objects
	result = (*mEngineObject)->GetInterface(mEngineObject, SL_IID_ENGINE, &mEngineEngine);
	Q_ASSERT(SL_RESULT_SUCCESS == result);

	// create output mix
	const SLInterfaceID ids[] = {};
	const SLboolean req[] = {};
	result = (*mEngineEngine)->CreateOutputMix(mEngineEngine, &mOutputMixObject, 0, ids, req);
	Q_ASSERT(SL_RESULT_SUCCESS == result);

	// realize the output mix
	result = (*mOutputMixObject)->Realize(mOutputMixObject, SL_BOOLEAN_FALSE);
	Q_ASSERT(SL_RESULT_SUCCESS == result);

	qDebug() << "Created Android Audio Engine";
}

void AndroidAudio::destroyEngine()
{
	if (mOutputMixObject != NULL) {
		(*mOutputMixObject)->Destroy(mOutputMixObject);
	}

	if (mEngineObject != NULL) {
		(*mEngineObject)->Destroy(mEngineObject);
	}

	if (mPlayerObject != NULL) {
		(*mPlayerObject)->Destroy(mPlayerObject);
	}

	for (int32_t i = 0; i < mSoundCount; ++i) {
		mSounds.values().at(i)->unload();
	}

	qDebug() << "Destroyed Android Audio Engine";
}

void AndroidAudio::registerSound(const QString& path, const QString& name)
{
//    qDebug() << "registerSound:" << path << name;
	AndroidSoundEffect *lSound = new AndroidSoundEffect(path, this);
//    qDebug() << "registerSound:created";
	mSounds[name] = lSound;

//    qDebug() << "registerSound:loading";
	lSound->load();
//    qDebug() << "registerSound:loaded";
}

void AndroidAudio::startSoundPlayer()
{
//    qDebug() << "Starting Sound Player";

	SLresult lRes;

	//Configure the sound player input/output
	SLDataLocator_AndroidSimpleBufferQueue lDataLocatorIn;
	lDataLocatorIn.locatorType = SL_DATALOCATOR_BUFFERQUEUE;
	lDataLocatorIn.numBuffers = 1;

	//Set the data format as mono-pcm-16bit-44100
	SLDataFormat_PCM lDataFormat;
	lDataFormat.formatType = SL_DATAFORMAT_PCM;
	lDataFormat.numChannels = 2;
	lDataFormat.samplesPerSec = SL_SAMPLINGRATE_44_1;
	lDataFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	lDataFormat.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
	lDataFormat.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	lDataFormat.endianness = SL_BYTEORDER_LITTLEENDIAN;

	SLDataSource lDataSource;
	lDataSource.pLocator = &lDataLocatorIn;
	lDataSource.pFormat = &lDataFormat;

	SLDataLocator_OutputMix lDataLocatorOut;
	lDataLocatorOut.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	lDataLocatorOut.outputMix = mOutputMixObject;

	SLDataSink lDataSink;
	lDataSink.pLocator = &lDataLocatorOut;
	lDataSink.pFormat = NULL;

	//Create the sound player
	const SLuint32 lSoundPlayerIIDCount = 2;
	const SLInterfaceID lSoundPlayerIIDs[] = { SL_IID_PLAY, SL_IID_BUFFERQUEUE };
	const SLboolean lSoundPlayerReqs[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

//    qDebug() << "Configured Sound Player";

	lRes = (*mEngineEngine)->CreateAudioPlayer(mEngineEngine, &mPlayerObject, &lDataSource, &lDataSink, lSoundPlayerIIDCount, lSoundPlayerIIDs, lSoundPlayerReqs);
	Q_ASSERT(SL_RESULT_SUCCESS == lRes);

//    qDebug() << "Created Sound Player";

	lRes = (*mPlayerObject)->Realize(mPlayerObject, SL_BOOLEAN_FALSE);
	Q_ASSERT(SL_RESULT_SUCCESS == lRes);

	qDebug() << "Realised Sound Player";
	lRes = (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_PLAY, &mPlayerPlay);
	Q_ASSERT(SL_RESULT_SUCCESS == lRes);

	lRes = (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_BUFFERQUEUE, &mPlayerQueue);
	Q_ASSERT(SL_RESULT_SUCCESS == lRes);

	lRes = (*mPlayerPlay)->SetPlayState(mPlayerPlay, SL_PLAYSTATE_PLAYING);
	Q_ASSERT(SL_RESULT_SUCCESS == lRes);

//    qDebug() << "Created Buffer Player";
}

void AndroidAudio::playSound(const std::string& name, int i)
{

	if(audioEnabled && myConfig->readConfigInt("PlaySoundEffects")) {

		this->registerSound(QString(":/android/android-data/sounds/default/"+QString::fromStdString(name)+".wav"), QString::fromStdString(name));
		this->reallyPlaySound(QString::fromStdString(name));
	}
}

void AndroidAudio::reallyPlaySound(const QString& name)
{
	SLresult lRes;
	SLuint32 lPlayerState;

	AndroidSoundEffect* sound = mSounds[name];

	if (!sound) {
		qDebug() << "No such sound:" << name;
		return;
	}
	//Get the current state of the player
	(*mPlayerObject)->GetState(mPlayerObject, &lPlayerState);

	//If the player is realised
	if (lPlayerState == SL_OBJECT_STATE_REALIZED) {
		//Get the buffer and length of the effect
		int16_t* lBuffer = (int16_t *)sound->mBuffer;
		off_t lLength = sound->mLength;

		//Remove any sound from the queue
		lRes = (*mPlayerQueue)->Clear(mPlayerQueue);
		Q_ASSERT(SL_RESULT_SUCCESS == lRes);

		//Play the new sound
		(*mPlayerQueue)->Enqueue(mPlayerQueue, lBuffer, lLength);
		Q_ASSERT(SL_RESULT_SUCCESS == lRes);
	}
}
