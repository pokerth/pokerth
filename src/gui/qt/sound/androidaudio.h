#ifndef ANDROIDAUDIO_H
#define ANDROIDAUDIO_H

#include <QObject>
#include <QMap>

#include "androidsoundeffect.h"

// for native audio
#include <jni.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

class AndroidAudio : public QObject
{
    Q_OBJECT
public:
    explicit AndroidAudio(QObject *parent = 0);
    ~AndroidAudio();

signals:

public slots:

    void registerSound(const QString& path, const QString &name);
    void reallyPlaySound(const QString& name);
    void playSound(const std::string& name, int i);

private:
    void createEngine();
    void destroyEngine();
    void startSoundPlayer();

    // engine interfaces
    SLObjectItf mEngineObject;
    SLEngineItf mEngineEngine;

    // output mix interfaces
    SLObjectItf mOutputMixObject;

    // buffer queue player interfaces - Effects
    SLObjectItf mPlayerObject;
    SLPlayItf mPlayerPlay;
    SLBufferQueueItf mPlayerQueue;

    QMap<QString, AndroidSoundEffect*> mSounds;
    int32_t mSoundCount;

};

#endif // ANDROIDAUDIO_H
