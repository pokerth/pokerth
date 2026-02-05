#include "qtaudioplayer.h"
#include <QDebug>
#include <QFileInfo>

// All sound files to preload
static const char* SOUND_FILES[] = {
    "allin", "bet", "blinds_raises_level1", "blinds_raises_level2", 
    "blinds_raises_level3", "call", "check", "dealtwocards", "fold",
    "lobbychatnotify", "onlinegameready", "playerconnected", "raise", "yourturn"
};

QtAudioPlayer::QtAudioPlayer(ConfigFile *config)
    : myConfig(config), audioEnabled(false)
{
    myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());
    initAudio();
}

QtAudioPlayer::~QtAudioPlayer()
{
    closeAudio();
}

void QtAudioPlayer::initAudio()
{
    if (audioEnabled)
        return;
        
    if (!myConfig->readConfigInt("PlaySoundEffects"))
        return;

    qDebug() << "[Audio] Initializing Qt audio with path:" << myAppDataPath;
    
    // Volume slider is 1-10, QSoundEffect expects 0.0-1.0
    float vol = myConfig->readConfigInt("SoundVolume") / 10.0f;
    qDebug() << "[Audio] Volume:" << vol;
    
    // Preload ALL sound files at startup
    for (const char* soundName : SOUND_FILES) {
        QString key = QString::fromLatin1(soundName);
        QString filePath = myAppDataPath + "sounds/default/" + key + ".wav";
        
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            qWarning() << "[Audio] Sound file not found:" << filePath;
            continue;
        }
        
        auto effect = QSharedPointer<QSoundEffect>::create();
        effect->setSource(QUrl::fromLocalFile(filePath));
        effect->setLoopCount(1);
        effect->setVolume(vol);
        
        // Connect status signal for debugging
        connect(effect.data(), &QSoundEffect::statusChanged, this, [key, effect]() {
            if (effect->status() == QSoundEffect::Error) {
                qWarning() << "[Audio] Error loading sound:" << key;
            }
        });
        
        effects.insert(key, effect);
        qDebug() << "[Audio] Preloaded:" << key;
    }
    
    audioEnabled = true;
    qDebug() << "[Audio] Initialization complete," << effects.size() << "sounds loaded";
}

void QtAudioPlayer::playSound(std::string audioName, int /*playerID*/)
{
    if (!audioEnabled || !myConfig->readConfigInt("PlaySoundEffects"))
        return;

    const QString key = QString::fromStdString(audioName);
    
    if (!effects.contains(key)) {
        qWarning() << "[Audio] Unknown sound:" << key;
        return;
    }

    auto effect = effects.value(key);
    if (!effect) {
        qWarning() << "[Audio] Null effect for:" << key;
        return;
    }
    
    // Check status
    if (effect->status() == QSoundEffect::Error) {
        qWarning() << "[Audio] Cannot play (error state):" << key;
        return;
    }
    
    if (effect->isLoaded()) {
        // Sound is ready - play immediately
        // If already playing, stop and restart for responsive feedback
        if (effect->isPlaying()) {
            effect->stop();
        }
        effect->play();
    } else if (effect->status() == QSoundEffect::Loading) {
        // Still loading - queue playback when ready (only once)
        QMetaObject::Connection* conn = new QMetaObject::Connection();
        *conn = connect(effect.data(), &QSoundEffect::loadedChanged, this, [this, effect, key, conn]() {
            if (effect->isLoaded()) {
                qDebug() << "[Audio] Delayed play after load:" << key;
                effect->play();
            }
            disconnect(*conn);
            delete conn;
        });
    } else {
        qWarning() << "[Audio] Cannot play, status:" << effect->status() << "for:" << key;
    }
}

void QtAudioPlayer::closeAudio()
{
    qDebug() << "[Audio] Closing audio";
    for (auto& e : effects) {
        if (e) {
            e->stop();
            e->disconnect();
        }
    }
    effects.clear();
    audioEnabled = false;
}

void QtAudioPlayer::reInit()
{
    qDebug() << "[Audio] Reinitializing";
    closeAudio();
    initAudio();
}