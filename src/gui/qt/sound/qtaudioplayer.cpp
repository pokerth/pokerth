#include "qtaudioplayer.h"
#include "core/appimage_utils.h"
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <cmath>

#ifdef Q_OS_WIN
#pragma comment(lib, "winmm.lib")
#endif

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

bool WavMixer::hasActiveVoices()
{
    QMutexLocker lock(&mutex);
    return !voices.isEmpty();
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

    // Attenuate each voice when multiple sounds overlap to prevent
    // hard-clipping distortion that causes the "stuttering" effect
    // reported on Windows 11.  With 1 voice: full volume.
    // With 2+: scale each contribution by 1/sqrt(N) (equal-power mix).
    const int voiceCount = voices.size();
    const float attenuation = (voiceCount > 1)
        ? (1.0f / std::sqrt(static_cast<float>(voiceCount)))
        : 1.0f;
    const float effectiveVolume = volume * attenuation;

    for (int v = voices.size() - 1; v >= 0; --v) {
        ActiveVoice& voice = voices[v];
        const qint16* src = reinterpret_cast<const qint16*>(
            voice.pcmData->constData() + voice.position);
        qint64 remaining = (voice.pcmData->size() - voice.position) / 2;
        qint64 toMix = qMin(numSamples, remaining);

        for (qint64 i = 0; i < toMix; ++i) {
            qint32 mixed = static_cast<qint32>(out[i])
                         + static_cast<qint32>(src[i] * effectiveVolume);
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

    
    // Check for forced backend via environment variable
    QString forcedBackend = qEnvironmentVariable("POKERTH_AUDIO_BACKEND");
    if (!forcedBackend.isEmpty()) {
    }
    
    // === Audio subsystem diagnostics ===
    {
        auto outputs = QMediaDevices::audioOutputs();
        for (const auto& dev : outputs) {
        }
    }
    
    // Determine which backend to use
    if (forcedBackend.toLower() == "paplay") {
        backend = AudioBackend::PaPlayBackend;
    } else if (forcedBackend.toLower() == "qsoundeffect") {
        backend = AudioBackend::QSoundEffectBackend;
    } else if (forcedBackend.toLower() == "mixer") {
        backend = AudioBackend::SoftwareMixerBackend;
    } else if (forcedBackend.toLower() == "winmm") {
#ifdef Q_OS_WIN
        backend = AudioBackend::WinMMBackend;
#else
        qWarning() << "[Audio] WinMM backend only available on Windows, falling back";
#endif
    } else {
        // Auto-detect best backend
#ifdef Q_OS_LINUX
        // AppImage: QAudioSink may not work because the bundled glibc/libs
        // conflict with the host audio stack.  Use paplay via
        // startDetachedSafe() which restores the original LD_LIBRARY_PATH.
        if (AppImageUtils::isAppImage() && detectPaPlay()) {
            backend = AudioBackend::PaPlayBackend;
        } else {
            // Native builds: use the software mixer — a single persistent
            // QAudioSink stream that mixes all sounds in-process.  This
            // avoids spawning a new paplay/pw-play process per sound effect,
            // which floods PulseAudio/PipeWire with concurrent streams and
            // causes stuttering in other audio consumers (e.g. video players).
            backend = AudioBackend::SoftwareMixerBackend;
        }
#elif defined(Q_OS_WIN)
        // Windows: WinMM streaming mixer — single persistent waveOut handle
        // with double-buffering on a dedicated audio thread.  Avoids both
        // QAudioSink/WASAPI session issues (clipping, IdleState bugs) and
        // per-sound waveOutOpen churn that degraded audio over long sessions.
        backend = AudioBackend::WinMMBackend;
#else
        // macOS: software mixer for low-latency playback
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
        // If the QAudioSink failed to start (broken PulseAudio/PipeWire/
        // CoreAudio setup), fall back to an alternative backend.
        if (!mixerSink) {
            qWarning() << "[Audio] SoftwareMixer sink failed — trying fallback";
#ifdef Q_OS_LINUX
            if (detectPaPlay()) {
                backend = AudioBackend::PaPlayBackend;
                initPaPlayBackend();
            } else {
                qWarning() << "[Audio] No fallback available (paplay not found)";
                backend = AudioBackend::QSoundEffectBackend;
                initQSoundEffectBackend(deviceToUse, vol);
            }
#else
            // macOS: fall back to QSoundEffect
            backend = AudioBackend::QSoundEffectBackend;
            initQSoundEffectBackend(deviceToUse, vol);
#endif
        }
    } else if (backend == AudioBackend::PaPlayBackend) {
        if (detectPaPlay()) {
            initPaPlayBackend();
        } else {
            qWarning() << "[Audio] paplay not found - falling back to software mixer";
            backend = AudioBackend::SoftwareMixerBackend;
            initSoftwareMixerBackend(deviceToUse, vol);
        }
#ifdef Q_OS_WIN
    } else if (backend == AudioBackend::WinMMBackend) {
        initWinMMBackend(vol);
#endif
    } else {
        initQSoundEffectBackend(deviceToUse, vol);
    }
    
    audioEnabled = true;
}

void QtAudioPlayer::initQSoundEffectBackend(const QAudioDevice& device, float volume)
{
    
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
}

void QtAudioPlayer::initPaPlayBackend()
{
    
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
}

bool QtAudioPlayer::detectPaPlay()
{
    // Check for paplay (PulseAudio) or pw-play (PipeWire native)
    paplayBinary = QStandardPaths::findExecutable("paplay");
    if (!paplayBinary.isEmpty()) {
        return true;
    }
    
    paplayBinary = QStandardPaths::findExecutable("pw-play");
    if (!paplayBinary.isEmpty()) {
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
#ifdef Q_OS_WIN
    } else if (backend == AudioBackend::WinMMBackend) {
        playSoundWinMM(key);
#endif
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
    // Small buffers cause underruns that trigger IdleState transitions,
    // cutting off sounds mid-playback (e.g. blinds_raises WAVs).
    // Use 600ms on Windows to prevent stuttering/clipping, 200ms elsewhere.
#ifdef Q_OS_WIN
    mixerSink->setBufferSize(44100 * 4 * 3 / 5); // ~600ms for WASAPI
#else
    mixerSink->setBufferSize(44100 * 4 / 5);      // ~200ms for PulseAudio/CoreAudio
#endif

    connectMixerSinkSignals();

    mixerSink->start(mixer);

    if (mixerSink->error() != QAudio::NoError) {
        qWarning() << "[Audio] Failed to start mixer sink:" << mixerSink->error();
        delete mixerSink;
        mixerSink = nullptr;
    }

    // NOTE: The mixer sink streams continuously (silence when no sounds
    // are active).  We intentionally do NOT auto-suspend the sink after
    // idle periods because repeated QAudioSink::stop()/start() cycles
    // are unreliable on PulseAudio/PipeWire (Linux) and CoreAudio
    // (macOS) — after ~20 cycles the sink silently stops producing
    // output.  The CPU cost of streaming silence is negligible (one
    // memset per audio callback).
}

void QtAudioPlayer::playSoundSoftwareMixer(const QString& key)
{
    if (!mixer) return;

    mixer->play(key);

    // Safety: if the sink was stopped due to a device change or error,
    // restart it.  Under normal operation the sink runs continuously.
    if (mixerSink && mixerSink->state() != QAudio::ActiveState) {
        if (!mixer->isOpen()) {
            mixer->open(QIODevice::ReadOnly);
        }
        mixerSink->start(mixer);
    }
}

void QtAudioPlayer::connectMixerSinkSignals()
{
    if (!mixerSink)
        return;

    // Handle QAudioSink state transitions:
    //
    // IdleState     — the internal buffer drained.  WavMixer always
    //                 provides data (silence when idle), so this is a
    //                 transient underrun.  Ignore it — the sink keeps
    //                 pulling and recovers by itself.
    //
    // StoppedState  — check the error code.  NoError means an
    //                 intentional stop (closeAudio / applyDeviceToEffects).
    //                 Real errors (FatalError, device lost) trigger a
    //                 sink recreation via applyDeviceToEffects().
    //
    // SuspendedState — system suspended audio (e.g. screen lock on
    //                  macOS); try to resume immediately.
    connect(mixerSink, &QAudioSink::stateChanged, this, [this](QAudio::State newState) {
        if (!mixerSink || !mixer)
            return;
        if (newState == QAudio::IdleState) {
            // Transient underrun — do nothing.  The sink keeps pulling.
        } else if (newState == QAudio::StoppedState) {
            if (m_stoppingMixerIntentionally)
                return;   // Intentional stop (closeAudio / applyDeviceToEffects)
            auto err = mixerSink->error();
            if (err == QAudio::NoError)
                return;   // Clean stop — nothing to recover from
            qWarning() << "[Audio] Mixer sink stopped (error:" << err
                       << ") — scheduling debounced recreation";
            // Use the debounce timer instead of immediate retry.
            // PipeWire/PulseAudio may still be reconfiguring after a
            // device change; retrying instantly causes pw_stream_connect
            // to fail ("No such device") and can spin into an infinite
            // retry loop that degrades UI performance.
            scheduleDeviceCheck();
        } else if (newState == QAudio::SuspendedState) {
            qWarning() << "[Audio] Mixer sink suspended — attempting resume";
            mixerSink->resume();
        }
    });
}

// --- Win32 waveOut pool backend ---
// Pool of pre-opened waveOut handles for concurrent low-latency playback.
// Each slot can play one sound at a time; sounds on different slots overlap
// naturally.  Pre-loaded PCM data is volume-scaled per-play and submitted
// via waveOutWrite.  Zero CPU when idle, no threads, no streaming loop.
// Handles are opened once at init and closed at shutdown — no churn.

#ifdef Q_OS_WIN

// Parse a WAV file and return raw PCM data (must be 44100/16bit/stereo)
static bool parseWavForWinMM(const QString& filePath, QByteArray& pcmOut)
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

    int pos = 12;
    bool fmtValid = false;

    while (pos + 8 <= fileData.size()) {
        QByteArray chunkId = fileData.mid(pos, 4);
        quint32 chunkSize = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar*>(fileData.constData() + pos + 4));

        if (chunkId == "fmt " && chunkSize >= 16) {
            const uchar* fmt = reinterpret_cast<const uchar*>(fileData.constData() + pos + 8);
            quint16 audioFormat   = qFromLittleEndian<quint16>(fmt);
            quint16 channels      = qFromLittleEndian<quint16>(fmt + 2);
            quint32 sampleRate    = qFromLittleEndian<quint32>(fmt + 4);
            quint16 bitsPerSample = qFromLittleEndian<quint16>(fmt + 14);

            if (audioFormat == 1 && channels == 2 && sampleRate == 44100 && bitsPerSample == 16) {
                fmtValid = true;
            } else {
                qWarning() << "[Audio] Unsupported WAV format in" << filePath
                           << "- need PCM 16-bit stereo 44100Hz";
                return false;
            }
        }

        if (chunkId == "data" && fmtValid) {
            qint64 avail = fileData.size() - pos - 8;
            if (static_cast<qint64>(chunkSize) > avail)
                chunkSize = static_cast<quint32>(avail);
            pcmOut = fileData.mid(pos + 8, chunkSize);
            return true;
        }

        pos += 8 + chunkSize;
        if (chunkSize & 1) pos++;
    }
    return false;
}

void QtAudioPlayer::initWinMMBackend(float vol)
{
    winmmVolume = qBound(0.0f, vol, 1.0f);
    winmmPoolOpen = false;

    // Pre-load all WAV files into memory
    for (const char* soundName : SOUND_FILES) {
        QString key  = QString::fromLatin1(soundName);
        QString path = myAppDataPath + "sounds/default/" + key + ".wav";
        if (!QFileInfo::exists(path)) {
            qWarning() << "[Audio] Sound file not found:" << path;
            continue;
        }
        QByteArray pcm;
        if (parseWavForWinMM(path, pcm)) {
            winmmPcmData.insert(key, pcm);
        } else {
            qWarning() << "[Audio] Failed to parse WAV:" << path;
        }
    }

    // Open pool of waveOut handles (all same format: 44100/16bit/stereo)
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = 2;
    wfx.nSamplesPerSec  = 44100;
    wfx.wBitsPerSample  = 16;
    wfx.nBlockAlign     = 4;
    wfx.nAvgBytesPerSec = 44100 * 4;
    wfx.cbSize          = 0;

    for (int i = 0; i < WINMM_POOL_SIZE; i++) {
        winmmSlots[i].handle = nullptr;
        winmmSlots[i].prepared = false;
        MMRESULT res = waveOutOpen(&winmmSlots[i].handle, WAVE_MAPPER, &wfx,
                                   0, 0, CALLBACK_NULL);
        if (res != MMSYSERR_NOERROR) {
            qWarning() << "[Audio] waveOutOpen failed for slot" << i << ":" << res;
            winmmSlots[i].handle = nullptr;
        }
    }
    winmmPoolOpen = true;
}

void QtAudioPlayer::playSoundWinMM(const QString& key)
{
    if (!winmmPoolOpen) return;

    auto it = winmmPcmData.constFind(key);
    if (it == winmmPcmData.constEnd()) {
        qWarning() << "[Audio] Unknown sound:" << key;
        return;
    }

    // Re-read volume from config in case user changed it
    float vol = myConfig->readConfigInt("SoundVolume") / 10.0f;
    winmmVolume = qBound(0.0f, vol, 1.0f);

    // Find a free slot: prefer one that's finished playing,
    // then one that was never used
    int slot = -1;
    for (int i = 0; i < WINMM_POOL_SIZE; i++) {
        if (!winmmSlots[i].handle) continue;
        if (!winmmSlots[i].prepared) {
            // Never used or already reclaimed — grab it
            slot = i;
            break;
        }
        if (winmmSlots[i].header.dwFlags & WHDR_DONE) {
            // Finished playing — reclaim and reuse
            waveOutUnprepareHeader(winmmSlots[i].handle,
                                   &winmmSlots[i].header, sizeof(WAVEHDR));
            winmmSlots[i].prepared = false;
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        // All slots busy — force-reclaim slot 0 (oldest)
        for (int i = 0; i < WINMM_POOL_SIZE; i++) {
            if (!winmmSlots[i].handle) continue;
            waveOutReset(winmmSlots[i].handle);
            if (winmmSlots[i].prepared) {
                waveOutUnprepareHeader(winmmSlots[i].handle,
                                       &winmmSlots[i].header, sizeof(WAVEHDR));
                winmmSlots[i].prepared = false;
            }
            slot = i;
            break;
        }
    }

    if (slot < 0) return;  // No handles available at all

    // Copy PCM data and apply volume scaling
    winmmSlots[slot].buffer = *it;  // deep copy
    if (winmmVolume < 0.999f) {
        qint16* samples = reinterpret_cast<qint16*>(winmmSlots[slot].buffer.data());
        int numSamples = winmmSlots[slot].buffer.size() / 2;
        for (int s = 0; s < numSamples; s++) {
            samples[s] = static_cast<qint16>(qBound(-32768,
                static_cast<qint32>(samples[s] * winmmVolume), 32767));
        }
    }

    // Prepare header and submit for playback (non-blocking)
    memset(&winmmSlots[slot].header, 0, sizeof(WAVEHDR));
    winmmSlots[slot].header.lpData         = winmmSlots[slot].buffer.data();
    winmmSlots[slot].header.dwBufferLength = static_cast<DWORD>(winmmSlots[slot].buffer.size());

    waveOutPrepareHeader(winmmSlots[slot].handle,
                         &winmmSlots[slot].header, sizeof(WAVEHDR));
    winmmSlots[slot].prepared = true;
    waveOutWrite(winmmSlots[slot].handle,
                 &winmmSlots[slot].header, sizeof(WAVEHDR));
}

#endif // Q_OS_WIN

void QtAudioPlayer::closeAudio()
{
#ifdef Q_OS_WIN
    if (backend == AudioBackend::WinMMBackend && winmmPoolOpen) {
        // Stop all playback and close all handles
        for (int i = 0; i < WINMM_POOL_SIZE; i++) {
            if (winmmSlots[i].handle) {
                waveOutReset(winmmSlots[i].handle);
                if (winmmSlots[i].prepared) {
                    waveOutUnprepareHeader(winmmSlots[i].handle,
                                           &winmmSlots[i].header, sizeof(WAVEHDR));
                    winmmSlots[i].prepared = false;
                }
                waveOutClose(winmmSlots[i].handle);
                winmmSlots[i].handle = nullptr;
            }
        }
        winmmPcmData.clear();
        winmmPoolOpen = false;
    }
#endif
    if (mixerSink) {
        // Disconnect stateChanged BEFORE stopping so the handler cannot
        // queue a spurious applyDeviceToEffects() call that would fire
        // after initAudio() has already created a fresh sink.
        mixerSink->disconnect(this);
        m_stoppingMixerIntentionally = true;
        mixerSink->stop();
        m_stoppingMixerIntentionally = false;
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
    // Fast-path: if only the volume changed we can update the existing
    // mixer/effects in-place instead of tearing down the whole audio
    // subsystem (which on some platforms causes audible glitches and,
    // before the fix, triggered an infinite sink-recreation loop).
    if (audioEnabled) {
        float vol = myConfig->readConfigInt("SoundVolume") / 10.0f;

        if (backend == AudioBackend::SoftwareMixerBackend && mixer) {
            mixer->setVolume(vol);
            return;
        }

        if (backend == AudioBackend::QSoundEffectBackend && !effects.isEmpty()) {
            for (auto& e : effects) {
                if (e) e->setVolume(vol);
            }
            return;
        }

#ifdef Q_OS_WIN
        if (backend == AudioBackend::WinMMBackend) {
            winmmVolume = qBound(0.0f, vol, 1.0f);
            return;
        }
#endif
        // PaPlayBackend reads volume from config at play-time — nothing to do.
        if (backend == AudioBackend::PaPlayBackend) {
            return;
        }
    }

    // Full reinit for backend changes or first-time init.
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
    
#ifdef Q_OS_WIN
    if (backend == AudioBackend::WinMMBackend) {
        // WinMM uses WAVE_MAPPER which follows the system default device.
        // No action needed — the OS handles device routing.
        return;
    }
#endif
    if (backend == AudioBackend::SoftwareMixerBackend) {
        // Recreate the audio sink with the new device
        if (mixerSink) {
            mixerSink->disconnect(this);
            m_stoppingMixerIntentionally = true;
            mixerSink->stop();
            m_stoppingMixerIntentionally = false;
            delete mixerSink;
        }
        QAudioFormat format;
        format.setSampleRate(44100);
        format.setChannelCount(2);
        format.setSampleFormat(QAudioFormat::Int16);
        mixerSink = new QAudioSink(deviceToUse, format, this);
#ifdef Q_OS_WIN
        mixerSink->setBufferSize(44100 * 4 * 3 / 5); // ~600ms for WASAPI
#else
        mixerSink->setBufferSize(44100 * 4 / 5);      // ~200ms
#endif
        connectMixerSinkSignals();
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
            selectedDevice = QAudioDevice(); // Clear selection, use default
            applyDeviceToEffects();
        }
    }

    // Safety net: if the mixer sink is in a stopped/error state after the
    // debounce period (e.g. device went away, PipeWire error, or the
    // default-change handler above didn't cover this case), recreate it
    // with the current default device.  This runs at most once per
    // debounce interval (500 ms), so it cannot spin.
    if (backend == AudioBackend::SoftwareMixerBackend && mixer && mixerSink
            && mixerSink->state() == QAudio::StoppedState
            && mixerSink->error() != QAudio::NoError) {
        qWarning() << "[Audio] Mixer sink still in error state after debounce"
                   << "— recreating with current default device";
        applyDeviceToEffects();
    }
}

void QtAudioPlayer::onDefaultOutputChanged()
{
    QAudioDevice newDefault = QMediaDevices::defaultAudioOutput();
    
    // Only auto-switch if user hasn't selected a specific device
    if (selectedDevice.isNull()) {
        applyDeviceToEffects();
    }
}

bool QtAudioPlayer::probeAudioOutput(const QAudioDevice& device)
{
    
    // Create a format matching our WAV files: 16-bit signed LE, stereo, 44100Hz
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    
    if (!device.isFormatSupported(format)) {
        qWarning() << "[Audio] Probe: device does NOT support 44100/16bit/stereo!";
        // Try with the device's preferred format
        format = device.preferredFormat();
    } else {
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
    
    
    sink.stop();
    buffer.close();
    
    if (error != QAudio::NoError) {
        qWarning() << "[Audio] Probe: FAILED with error:" << error;
        return false;
    }
    
    if (state == QAudio::ActiveState || state == QAudio::IdleState) {
        return true;
    }
    
    qWarning() << "[Audio] Probe: unexpected state:" << state;
    return false;
}