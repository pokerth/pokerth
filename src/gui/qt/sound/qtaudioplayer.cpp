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
    : myConfig(config), audioEnabled(false), mediaDevices(nullptr), deviceChangeDebounceTimer(nullptr)
{
    myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());
    
    // Initialize device monitoring
    mediaDevices = new QMediaDevices(this);
    lastDefaultDevice = QMediaDevices::defaultAudioOutput();
    
    // Debounce timer for Bluetooth reconnects etc.
    deviceChangeDebounceTimer = new QTimer(this);
    deviceChangeDebounceTimer->setSingleShot(true);
    connect(deviceChangeDebounceTimer, &QTimer::timeout,
            this, &QtAudioPlayer::onDeviceChangeDebounceTimeout);
    
    // Connect device change signals
    connect(mediaDevices, &QMediaDevices::audioOutputsChanged,
            this, &QtAudioPlayer::onAudioOutputsChanged);
    
    // Note: defaultAudioOutputChanged is available in newer Qt versions
    // We also check in audioOutputsChanged as fallback
    
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
    
    // Determine which device to use
    QAudioDevice deviceToUse = selectedDevice.isNull() 
        ? QMediaDevices::defaultAudioOutput() 
        : selectedDevice;
    
    if (deviceToUse.isNull()) {
        qWarning() << "[Audio] No audio output device available!";
        return;
    }
    
    qDebug() << "[Audio] Using device:" << deviceToUse.description();
    
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
        effect->setAudioDevice(deviceToUse);
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

// --- Audio Device Management ---

QList<QAudioDevice> QtAudioPlayer::availableDevices() const
{
    return QMediaDevices::audioOutputs();
}

QAudioDevice QtAudioPlayer::currentDevice() const
{
    if (!selectedDevice.isNull()) {
        return selectedDevice;
    }
    return QMediaDevices::defaultAudioOutput();
}

void QtAudioPlayer::setAudioDevice(const QAudioDevice& device)
{
    if (selectedDevice == device) {
        return;
    }
    
    qDebug() << "[Audio] Setting audio device to:" 
             << (device.isNull() ? "System Default" : device.description());
    
    selectedDevice = device;
    
    // Apply to all existing effects without full reinit
    applyDeviceToEffects();
}

void QtAudioPlayer::applyDeviceToEffects()
{
    if (!audioEnabled) {
        return;
    }
    
    QAudioDevice deviceToUse = selectedDevice.isNull() 
        ? QMediaDevices::defaultAudioOutput() 
        : selectedDevice;
    
    if (deviceToUse.isNull()) {
        qWarning() << "[Audio] No audio device available for apply!";
        return;
    }
    
    qDebug() << "[Audio] Applying device:" << deviceToUse.description() 
             << "to" << effects.size() << "effects";
    
    for (auto& effect : effects) {
        if (effect) {
            effect->setAudioDevice(deviceToUse);
        }
    }
}

void QtAudioPlayer::onAudioOutputsChanged()
{
    qDebug() << "[Audio] Audio outputs changed - scheduling debounced check";
    
    // Restart debounce timer - this handles rapid connect/disconnect events
    // (e.g., Bluetooth momentarily losing connection)
    scheduleDeviceCheck();
}

void QtAudioPlayer::scheduleDeviceCheck()
{
    // Restart timer on each change event - only act after stable period
    deviceChangeDebounceTimer->start(DEVICE_CHANGE_DEBOUNCE_MS);
}

void QtAudioPlayer::onDeviceChangeDebounceTimeout()
{
    qDebug() << "[Audio] Debounce timeout - processing device change. Available devices:";
    for (const auto& dev : QMediaDevices::audioOutputs()) {
        qDebug() << "  -" << dev.description() << (dev.isDefault() ? "(default)" : "");
    }
    
    // Check if default device changed
    QAudioDevice newDefault = QMediaDevices::defaultAudioOutput();
    if (newDefault != lastDefaultDevice) {
        onDefaultOutputChanged();
        lastDefaultDevice = newDefault;
    }
    
    // If user selected a specific device that's no longer available, fall back to default
    if (!selectedDevice.isNull()) {
        bool deviceStillExists = false;
        for (const auto& dev : QMediaDevices::audioOutputs()) {
            if (dev == selectedDevice) {
                deviceStillExists = true;
                break;
            }
        }
        
        if (!deviceStillExists) {
            qDebug() << "[Audio] Selected device disconnected, falling back to default";
            selectedDevice = QAudioDevice(); // Clear selection, use default
            applyDeviceToEffects();
        }
    }
}

void QtAudioPlayer::onDefaultOutputChanged()
{
    QAudioDevice newDefault = QMediaDevices::defaultAudioOutput();
    qDebug() << "[Audio] Default audio output changed to:" 
             << (newDefault.isNull() ? "None" : newDefault.description());
    
    // Only auto-switch if user hasn't selected a specific device
    if (selectedDevice.isNull()) {
        qDebug() << "[Audio] Following default device change...";
        applyDeviceToEffects();
    }
}