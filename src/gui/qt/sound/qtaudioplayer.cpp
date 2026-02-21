#include "qtaudioplayer.h"
#include "core/appimage_utils.h"
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>

// All sound files to preload
static const char* SOUND_FILES[] = {
    "allin", "bet", "blinds_raises_level1", "blinds_raises_level2", 
    "blinds_raises_level3", "call", "check", "dealtwocards", "fold",
    "lobbychatnotify", "onlinegameready", "playerconnected", "raise", "yourturn"
};

// --- WavMixer implementation ---

WavMixer::WavMixer(QObject* parent)
    : QIODevice(parent), volume(1.0f)
{
    open(QIODevice::ReadOnly);
}

bool WavMixer::loadWav(const QString& key, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QByteArray fileData = file.readAll();
    file.close();

    if (fileData.size() < 44)
        return false;
    if (fileData.mid(0, 4) != "RIFF" || fileData.mid(8, 4) != "WAVE")
        return false;

    // Parse chunks: validate fmt, extract data
    int pos = 12;
    bool fmtValid = false;

    while (pos + 8 <= fileData.size()) {
        QByteArray chunkId = fileData.mid(pos, 4);
        quint32 chunkSize = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(fileData.constData() + pos + 4));

        if (chunkId == "fmt " && chunkSize >= 16) {
            const uchar* fmt = reinterpret_cast<const uchar*>(fileData.constData() + pos + 8);
            quint16 audioFormat = qFromLittleEndian<quint16>(fmt);
            quint16 channels    = qFromLittleEndian<quint16>(fmt + 2);
            quint32 sampleRate  = qFromLittleEndian<quint32>(fmt + 4);
            quint16 bitsPerSample = qFromLittleEndian<quint16>(fmt + 14);

            if (audioFormat == 1 && channels == 2 && sampleRate == 44100 && bitsPerSample == 16) {
                fmtValid = true;
            } else {
                qWarning() << "[Audio] Unsupported WAV format in" << key
                           << "- need PCM 16-bit stereo 44100Hz";
                return false;
            }
        }

        if (chunkId == "data" && fmtValid) {
            qint64 avail = fileData.size() - pos - 8;
            if (static_cast<qint64>(chunkSize) > avail)
                chunkSize = static_cast<quint32>(avail);

            WavSample sample;
            sample.pcmData = fileData.mid(pos + 8, chunkSize);
            samples.insert(key, sample);
            return true;
        }

        pos += 8 + chunkSize;
        if (chunkSize & 1) pos++; // Pad to even boundary
    }
    return false;
}

void WavMixer::play(const QString& key)
{
    QMutexLocker lock(&mutex);
    auto it = samples.constFind(key);
    if (it == samples.constEnd())
        return;

    ActiveVoice voice;
    voice.pcmData = &it->pcmData;
    voice.position = 0;
    voices.append(voice);
}

void WavMixer::setVolume(float vol)
{
    QMutexLocker lock(&mutex);
    volume = qBound(0.0f, vol, 1.0f);
}

void WavMixer::stopAll()
{
    QMutexLocker lock(&mutex);
    voices.clear();
}

qint64 WavMixer::readData(char* data, qint64 maxSize)
{
    QMutexLocker lock(&mutex);

    // Align to frame boundary (4 bytes = 2 channels x 16-bit)
    maxSize &= ~3LL;
    if (maxSize <= 0)
        return 0;

    memset(data, 0, static_cast<size_t>(maxSize));

    if (voices.isEmpty())
        return maxSize; // Output silence

    const qint64 numSamples = maxSize / 2; // 16-bit samples
    qint16* out = reinterpret_cast<qint16*>(data);

    for (int v = voices.size() - 1; v >= 0; --v) {
        ActiveVoice& voice = voices[v];
        const qint16* src = reinterpret_cast<const qint16*>(
            voice.pcmData->constData() + voice.position);
        qint64 remaining = (voice.pcmData->size() - voice.position) / 2;
        qint64 toMix = qMin(numSamples, remaining);

        for (qint64 i = 0; i < toMix; ++i) {
            qint32 mixed = static_cast<qint32>(out[i])
                         + static_cast<qint32>(src[i] * volume);
            out[i] = static_cast<qint16>(qBound(-32768, mixed, 32767));
        }

        voice.position += toMix * 2; // Back to bytes
        if (voice.position >= voice.pcmData->size()) {
            voices.removeAt(v);
        }
    }

    return maxSize;
}

qint64 WavMixer::writeData(const char*, qint64)
{
    return -1;
}

bool WavMixer::isSequential() const
{
    return true;
}

qint64 WavMixer::bytesAvailable() const
{
    // Infinite stream: always report data available so QAudioSink keeps pulling
    return 1024 * 1024;
}

// --- QtAudioPlayer ---

QtAudioPlayer::QtAudioPlayer(ConfigFile *config)
    : myConfig(config), audioEnabled(false), backend(AudioBackend::QSoundEffectBackend),
      mediaDevices(nullptr), deviceChangeDebounceTimer(nullptr),
      mixer(nullptr), mixerSink(nullptr)
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

    // qDebug() << "[Audio] Initializing Qt audio with path:" << myAppDataPath;
    
    // Check for forced backend via environment variable
    QString forcedBackend = qEnvironmentVariable("POKERTH_AUDIO_BACKEND");
    if (!forcedBackend.isEmpty()) {
        // qDebug() << "[Audio] POKERTH_AUDIO_BACKEND=" << forcedBackend;
    }
    
    // === Audio subsystem diagnostics ===
    {
        auto outputs = QMediaDevices::audioOutputs();
        // qDebug() << "[Audio] Available output devices:" << outputs.size();
        for (const auto& dev : outputs) {
            // qDebug() << "[Audio]   -" << dev.description() 
            //          << "id:" << dev.id()
            //          << (dev.isDefault() ? "(DEFAULT)" : "");
        }
    }
    
    // Determine which backend to use
    if (forcedBackend.toLower() == "paplay") {
        backend = AudioBackend::PaPlayBackend;
    } else if (forcedBackend.toLower() == "qsoundeffect") {
        backend = AudioBackend::QSoundEffectBackend;
    } else if (forcedBackend.toLower() == "mixer") {
        backend = AudioBackend::SoftwareMixerBackend;
    } else {
        // Auto-detect best backend
#ifdef Q_OS_LINUX
        // On Linux prefer paplay for PipeWire/PulseAudio compatibility
        if (detectPaPlay()) {
            backend = AudioBackend::PaPlayBackend;
        } else {
            backend = AudioBackend::SoftwareMixerBackend;
        }
#else
        // Windows/macOS: software mixer for low-latency, glitch-free playback
        backend = AudioBackend::SoftwareMixerBackend;
#endif
    }
    
    // Initialize selected backend
    float vol = myConfig->readConfigInt("SoundVolume") / 10.0f;
    
    QAudioDevice deviceToUse = selectedDevice.isNull() 
        ? QMediaDevices::defaultAudioOutput() 
        : selectedDevice;
    
    if (backend == AudioBackend::SoftwareMixerBackend) {
        initSoftwareMixerBackend(deviceToUse, vol);
    } else if (backend == AudioBackend::PaPlayBackend) {
        if (detectPaPlay()) {
            initPaPlayBackend();
        } else {
            qWarning() << "[Audio] paplay not found - falling back to software mixer";
            backend = AudioBackend::SoftwareMixerBackend;
            initSoftwareMixerBackend(deviceToUse, vol);
        }
    } else {
        initQSoundEffectBackend(deviceToUse, vol);
    }
    
    audioEnabled = true;
}

void QtAudioPlayer::initQSoundEffectBackend(const QAudioDevice& device, float volume)
{
    // qDebug() << "[Audio] Initializing QSoundEffect backend";
    
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
    // qDebug() << "[Audio] QSoundEffect:" << effects.size() << "sounds loaded";
}

void QtAudioPlayer::initPaPlayBackend()
{
    // qDebug() << "[Audio] Initializing paplay backend, binary:" << paplayBinary;
    
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
    // qDebug() << "[Audio] paplay:" << soundFilePaths.size() << "sounds registered";
}

bool QtAudioPlayer::detectPaPlay()
{
    // Check for paplay (PulseAudio) or pw-play (PipeWire native)
    paplayBinary = QStandardPaths::findExecutable("paplay");
    if (!paplayBinary.isEmpty()) {
        // qDebug() << "[Audio] Found paplay:" << paplayBinary;
        return true;
    }
    
    paplayBinary = QStandardPaths::findExecutable("pw-play");
    if (!paplayBinary.isEmpty()) {
        // qDebug() << "[Audio] Found pw-play:" << paplayBinary;
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
    
    if (backend == AudioBackend::SoftwareMixerBackend) {
        playSoundSoftwareMixer(key);
    } else if (backend == AudioBackend::PaPlayBackend) {
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

    // qDebug() << "[Audio] paplay:" << key << "vol:" << vol;
    
    bool ok = AppImageUtils::startDetachedSafe(paplayBinary, args);
    if (!ok) {
        qWarning() << "[Audio] *** Failed to start" << paplayBinary << args;
    }
}

void QtAudioPlayer::initSoftwareMixerBackend(const QAudioDevice& device, float vol)
{
    mixer = new WavMixer(this);
    mixer->setVolume(vol);

    for (const char* soundName : SOUND_FILES) {
        QString key = QString::fromLatin1(soundName);
        QString filePath = myAppDataPath + "sounds/default/" + key + ".wav";

        if (!QFileInfo::exists(filePath)) {
            qWarning() << "[Audio] Sound file not found:" << filePath;
            continue;
        }
        if (!mixer->loadWav(key, filePath)) {
            qWarning() << "[Audio] Failed to parse WAV:" << filePath;
        }
    }

    // Single persistent audio output - eliminates per-sound WASAPI session latency
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice sinkDevice = device.isNull() ? QMediaDevices::defaultAudioOutput() : device;
    mixerSink = new QAudioSink(sinkDevice, format, this);
    // WASAPI on Windows needs a larger buffer than PulseAudio/CoreAudio.
    // 100ms causes underruns that make WASAPI transition to IdleState,
    // cutting off sounds mid-playback (e.g. blinds_raises WAVs).
#ifdef Q_OS_WIN
    mixerSink->setBufferSize(44100 * 4 * 2 / 5); // ~400ms for WASAPI
#else
    mixerSink->setBufferSize(44100 * 4 / 5);      // ~200ms for PulseAudio/CoreAudio
#endif

    // CRITICAL (Windows): When WASAPI encounters a brief underrun it
    // transitions the sink to IdleState and stops pulling data.  Without
    // this handler the sound is cut off and never resumes.  Restarting
    // the sink from the IdleState handler recovers playback seamlessly.
    //
    // Additionally, when Windows resumes from sleep/hibernate, WASAPI
    // invalidates the audio session entirely.  The sink transitions to
    // StoppedState (often with FatalError) or SuspendedState.  We must
    // detect these and fully recreate the QAudioSink to restore audio.
    connect(mixerSink, &QAudioSink::stateChanged, this, [this](QAudio::State newState) {
        if (!mixerSink || !mixer)
            return;
        if (newState == QAudio::IdleState) {
            // Sink ran out of data or WASAPI flagged an underrun.
            // Restart immediately so the next play() is audible.
            mixerSink->stop();
            mixerSink->start(mixer);
        } else if (newState == QAudio::StoppedState) {
            // Fatal error or device lost (e.g. after sleep/hibernate).
            // The existing sink is unusable — recreate it.
            qWarning() << "[Audio] Mixer sink stopped (error:"
                       << mixerSink->error() << ") — recreating sink";
            QMetaObject::invokeMethod(this, [this]() {
                if (!mixer) return;
                applyDeviceToEffects();   // recreates the sink
            }, Qt::QueuedConnection);
        } else if (newState == QAudio::SuspendedState) {
            // System suspended audio (e.g. entering sleep).  Try to
            // resume; if that fails the StoppedState handler above
            // will take over.
            qWarning() << "[Audio] Mixer sink suspended — attempting resume";
            mixerSink->resume();
        }
    });

    mixerSink->start(mixer);

    if (mixerSink->error() != QAudio::NoError) {
        qWarning() << "[Audio] Failed to start mixer sink:" << mixerSink->error();
        delete mixerSink;
        mixerSink = nullptr;
    }
}

void QtAudioPlayer::playSoundSoftwareMixer(const QString& key)
{
    if (mixer) {
        mixer->play(key);
    }
}

void QtAudioPlayer::closeAudio()
{
    if (mixerSink) {
        mixerSink->stop();
        delete mixerSink;
        mixerSink = nullptr;
    }
    if (mixer) {
        mixer->stopAll();
        mixer->close();
        delete mixer;
        mixer = nullptr;
    }
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
    // qDebug() << "[Audio] Reinitializing";
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
    
    // qDebug() << "[Audio] Setting audio device to:" 
    //          << (device.isNull() ? "System Default" : device.description());
    
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
    
    if (backend == AudioBackend::SoftwareMixerBackend) {
        // Recreate the audio sink with the new device
        if (mixerSink) {
            mixerSink->stop();
            delete mixerSink;
        }
        QAudioFormat format;
        format.setSampleRate(44100);
        format.setChannelCount(2);
        format.setSampleFormat(QAudioFormat::Int16);
        mixerSink = new QAudioSink(deviceToUse, format, this);
#ifdef Q_OS_WIN
        mixerSink->setBufferSize(44100 * 4 * 2 / 5); // ~400ms for WASAPI
#else
        mixerSink->setBufferSize(44100 * 4 / 5);      // ~200ms
#endif
        // Recovery handler: IdleState (underrun), StoppedState (device
        // lost after sleep/hibernate), SuspendedState (system suspend).
        connect(mixerSink, &QAudioSink::stateChanged, this, [this](QAudio::State newState) {
            if (!mixerSink || !mixer)
                return;
            if (newState == QAudio::IdleState) {
                mixerSink->stop();
                mixerSink->start(mixer);
            } else if (newState == QAudio::StoppedState) {
                qWarning() << "[Audio] Mixer sink stopped (error:"
                           << mixerSink->error() << ") — recreating sink";
                QMetaObject::invokeMethod(this, [this]() {
                    if (!mixer) return;
                    applyDeviceToEffects();
                }, Qt::QueuedConnection);
            } else if (newState == QAudio::SuspendedState) {
                qWarning() << "[Audio] Mixer sink suspended — attempting resume";
                mixerSink->resume();
            }
        });
        if (mixer) {
            mixerSink->start(mixer);
        }
        return;
    }
    
    for (auto& effect : effects) {
        if (effect) {
            effect->setAudioDevice(deviceToUse);
        }
    }
}

void QtAudioPlayer::onAudioOutputsChanged()
{
    // qDebug() << "[Audio] Audio outputs changed - scheduling debounced check";
    
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
    // qDebug() << "[Audio] Debounce timeout - processing device change. Available devices:";
    for (const auto& dev : QMediaDevices::audioOutputs()) {
        // qDebug() << "  -" << dev.description() << (dev.isDefault() ? "(default)" : "");
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
            // qDebug() << "[Audio] Selected device disconnected, falling back to default";
            selectedDevice = QAudioDevice(); // Clear selection, use default
            applyDeviceToEffects();
        }
    }
}

void QtAudioPlayer::onDefaultOutputChanged()
{
    QAudioDevice newDefault = QMediaDevices::defaultAudioOutput();
    // qDebug() << "[Audio] Default audio output changed to:" 
    //          << (newDefault.isNull() ? "None" : newDefault.description());
    
    // Only auto-switch if user hasn't selected a specific device
    if (selectedDevice.isNull()) {
        // qDebug() << "[Audio] Following default device change...";
        applyDeviceToEffects();
    }
}

bool QtAudioPlayer::probeAudioOutput(const QAudioDevice& device)
{
    // qDebug() << "[Audio] Probing audio output on:" << device.description();
    
    // Create a format matching our WAV files: 16-bit signed LE, stereo, 44100Hz
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    
    if (!device.isFormatSupported(format)) {
        qWarning() << "[Audio] Probe: device does NOT support 44100/16bit/stereo!";
        // qDebug() << "[Audio] Probe: preferred format:" 
        //          << device.preferredFormat().sampleRate()
        //          << device.preferredFormat().channelCount()
        //          << device.preferredFormat().sampleFormat();
        // Try with the device's preferred format
        format = device.preferredFormat();
    } else {
        // qDebug() << "[Audio] Probe: format 44100/16bit/stereo is supported";
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
    
    // qDebug() << "[Audio] Probe: QAudioSink state:" << state << "error:" << error;
    
    sink.stop();
    buffer.close();
    
    if (error != QAudio::NoError) {
        qWarning() << "[Audio] Probe: FAILED with error:" << error;
        return false;
    }
    
    if (state == QAudio::ActiveState || state == QAudio::IdleState) {
        // qDebug() << "[Audio] Probe: SUCCESS - audio output is functional";
        return true;
    }
    
    qWarning() << "[Audio] Probe: unexpected state:" << state;
    return false;
}