#include "qtaudioplayer.h"
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>

// All sound files to preload
static const char* SOUND_FILES[] = {
    "allin", "bet", "blinds_raises_level1", "blinds_raises_level2", 
    "blinds_raises_level3", "call", "check", "dealtwocards", "fold",
    "lobbychatnotify", "onlinegameready", "playerconnected", "raise", "yourturn"
};

QtAudioPlayer::QtAudioPlayer(ConfigFile *config)
    : myConfig(config), audioEnabled(false), backend(AudioBackend::QSoundEffectBackend),
      mediaDevices(nullptr), deviceChangeDebounceTimer(nullptr)
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
    
    // Check for forced backend via environment variable
    QString forcedBackend = qEnvironmentVariable("POKERTH_AUDIO_BACKEND");
    if (!forcedBackend.isEmpty()) {
        qDebug() << "[Audio] POKERTH_AUDIO_BACKEND=" << forcedBackend;
    }
    
    // === Audio subsystem diagnostics ===
    {
        auto outputs = QMediaDevices::audioOutputs();
        qDebug() << "[Audio] Available output devices:" << outputs.size();
        for (const auto& dev : outputs) {
            qDebug() << "[Audio]   -" << dev.description() 
                     << "id:" << dev.id()
                     << (dev.isDefault() ? "(DEFAULT)" : "");
        }
    }
    
    // Determine which backend to use
    if (forcedBackend.toLower() == "paplay") {
        qDebug() << "[Audio] Forced paplay backend via environment variable";
        backend = AudioBackend::PaPlayBackend;
    } else if (forcedBackend.toLower() == "qsoundeffect") {
        qDebug() << "[Audio] Forced QSoundEffect backend via environment variable";
        backend = AudioBackend::QSoundEffectBackend;
    } else {
        // Auto-detect best backend
#ifdef Q_OS_LINUX
        // On Linux, QSoundEffect/QAudioSink can silently fail on PipeWire setups
        // even though probing reports success. 
        // Prefer paplay/pw-play which reliably creates PulseAudio streams.
        if (detectPaPlay()) {
            qDebug() << "[Audio] Auto-select: using paplay backend (most reliable on Linux)";
            backend = AudioBackend::PaPlayBackend;
        } else {
            qDebug() << "[Audio] Auto-select: paplay not available, using QSoundEffect";
            backend = AudioBackend::QSoundEffectBackend;
        }
#else
        backend = AudioBackend::QSoundEffectBackend;
#endif
    }
    
    // Initialize selected backend
    float vol = myConfig->readConfigInt("SoundVolume") / 10.0f;
    qDebug() << "[Audio] Volume:" << vol;
    
    if (backend == AudioBackend::PaPlayBackend) {
        if (detectPaPlay()) {
            initPaPlayBackend();
        } else {
            qWarning() << "[Audio] paplay not found - falling back to QSoundEffect";
            backend = AudioBackend::QSoundEffectBackend;
            QAudioDevice deviceToUse = selectedDevice.isNull() 
                ? QMediaDevices::defaultAudioOutput() 
                : selectedDevice;
            initQSoundEffectBackend(deviceToUse, vol);
        }
    } else {
        QAudioDevice deviceToUse = selectedDevice.isNull() 
            ? QMediaDevices::defaultAudioOutput() 
            : selectedDevice;
        initQSoundEffectBackend(deviceToUse, vol);
    }
    
    audioEnabled = true;
    qDebug() << "[Audio] Initialization complete, backend:" 
             << (backend == AudioBackend::PaPlayBackend ? "paplay" : "QSoundEffect");
}

void QtAudioPlayer::initQSoundEffectBackend(const QAudioDevice& device, float volume)
{
    qDebug() << "[Audio] Initializing QSoundEffect backend";
    
    for (const char* soundName : SOUND_FILES) {
        QString key = QString::fromLatin1(soundName);
        QString filePath = myAppDataPath + "sounds/default/" + key + ".wav";
        
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            qWarning() << "[Audio] Sound file not found:" << filePath;
            continue;
        }
        
        auto effect = QSharedPointer<QSoundEffect>::create();
        // Only set audio device explicitly if user chose a non-default device.
        if (!selectedDevice.isNull() && !device.isNull()) {
            effect->setAudioDevice(device);
        }
        effect->setSource(QUrl::fromLocalFile(filePath));
        effect->setLoopCount(1);
        effect->setVolume(volume);
        
        connect(effect.data(), &QSoundEffect::statusChanged, this, [key, effect]() {
            if (effect->status() == QSoundEffect::Error) {
                qWarning() << "[Audio] Error loading sound:" << key;
            }
        });
        
        effects.insert(key, effect);
    }
    qDebug() << "[Audio] QSoundEffect:" << effects.size() << "sounds loaded";
}

void QtAudioPlayer::initPaPlayBackend()
{
    qDebug() << "[Audio] Initializing paplay backend, binary:" << paplayBinary;
    
    for (const char* soundName : SOUND_FILES) {
        QString key = QString::fromLatin1(soundName);
        QString filePath = myAppDataPath + "sounds/default/" + key + ".wav";
        
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            qWarning() << "[Audio] Sound file not found:" << filePath;
            continue;
        }
        
        soundFilePaths.insert(key, fileInfo.absoluteFilePath());
    }
    qDebug() << "[Audio] paplay:" << soundFilePaths.size() << "sounds registered";
}

bool QtAudioPlayer::detectPaPlay()
{
    // Check for paplay (PulseAudio) or pw-play (PipeWire native)
    paplayBinary = QStandardPaths::findExecutable("paplay");
    if (!paplayBinary.isEmpty()) {
        qDebug() << "[Audio] Found paplay:" << paplayBinary;
        return true;
    }
    
    paplayBinary = QStandardPaths::findExecutable("pw-play");
    if (!paplayBinary.isEmpty()) {
        qDebug() << "[Audio] Found pw-play:" << paplayBinary;
        return true;
    }
    
    qWarning() << "[Audio] Neither paplay nor pw-play found in PATH";
    return false;
}

void QtAudioPlayer::playSound(std::string audioName, int /*playerID*/)
{
    if (!audioEnabled || !myConfig->readConfigInt("PlaySoundEffects"))
        return;

    const QString key = QString::fromStdString(audioName);
    
    if (backend == AudioBackend::PaPlayBackend) {
        playSoundPaPlay(key);
    } else {
        playSoundQSoundEffect(key);
    }
}

void QtAudioPlayer::playSoundQSoundEffect(const QString& key)
{
    if (!effects.contains(key)) {
        qWarning() << "[Audio] Unknown sound:" << key;
        return;
    }

    auto effect = effects.value(key);
    if (!effect) return;
    
    if (effect->status() == QSoundEffect::Error) {
        qWarning() << "[Audio] Cannot play (error state):" << key;
        return;
    }
    
    if (effect->isLoaded()) {
        if (effect->isPlaying()) {
            effect->stop();
        }
        effect->play();
    } else if (effect->status() == QSoundEffect::Loading) {
        QMetaObject::Connection* conn = new QMetaObject::Connection();
        *conn = connect(effect.data(), &QSoundEffect::loadedChanged, this, [this, effect, key, conn]() {
            if (effect->isLoaded()) {
                effect->play();
            }
            disconnect(*conn);
            delete conn;
        });
    }
}

void QtAudioPlayer::playSoundPaPlay(const QString& key)
{
    if (!soundFilePaths.contains(key)) {
        qWarning() << "[Audio] Unknown sound:" << key;
        return;
    }
    
    const QString& filePath = soundFilePaths.value(key);
    
    // Volume: paplay uses --volume with PA volume (0-65536), 100% = 65536
    float vol = myConfig->readConfigInt("SoundVolume") / 10.0f;
    QString volumeStr = QString::number(qRound(vol * 65536.0f));
    
    QStringList args;
    if (paplayBinary.endsWith("paplay")) {
        args << "--volume" << volumeStr << filePath;
    } else {
        // pw-play uses --volume as 0.0-1.0 float
        args << "--volume" << QString::number(vol, 'f', 2) << filePath;
    }

    qDebug() << "[Audio] paplay:" << key << "vol:" << vol;
    
    bool ok = QProcess::startDetached(paplayBinary, args);
    if (!ok) {
        qWarning() << "[Audio] *** Failed to start" << paplayBinary << args;
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
    soundFilePaths.clear();
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

bool QtAudioPlayer::probeAudioOutput(const QAudioDevice& device)
{
    qDebug() << "[Audio] Probing audio output on:" << device.description();
    
    // Create a format matching our WAV files: 16-bit signed LE, stereo, 44100Hz
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    
    if (!device.isFormatSupported(format)) {
        qWarning() << "[Audio] Probe: device does NOT support 44100/16bit/stereo!";
        qDebug() << "[Audio] Probe: preferred format:" 
                 << device.preferredFormat().sampleRate()
                 << device.preferredFormat().channelCount()
                 << device.preferredFormat().sampleFormat();
        // Try with the device's preferred format
        format = device.preferredFormat();
    } else {
        qDebug() << "[Audio] Probe: format 44100/16bit/stereo is supported";
    }
    
    // Try creating a QAudioSink
    QAudioSink sink(device, format);
    
    // Create a small buffer of silence (100ms)
    int bytesPerSample = format.bytesPerSample() * format.channelCount();
    int bufferSize = format.sampleRate() / 10 * bytesPerSample; // 100ms
    QByteArray silenceData(bufferSize, '\0');
    QBuffer buffer(&silenceData);
    buffer.open(QIODevice::ReadOnly);
    
    // Try to start the sink
    sink.start(&buffer);
    
    auto state = sink.state();
    auto error = sink.error();
    
    qDebug() << "[Audio] Probe: QAudioSink state:" << state << "error:" << error;
    
    sink.stop();
    buffer.close();
    
    if (error != QAudio::NoError) {
        qWarning() << "[Audio] Probe: FAILED with error:" << error;
        return false;
    }
    
    if (state == QAudio::ActiveState || state == QAudio::IdleState) {
        qDebug() << "[Audio] Probe: SUCCESS - audio output is functional";
        return true;
    }
    
    qWarning() << "[Audio] Probe: unexpected state:" << state;
    return false;
}