#pragma once
#include <QtCore>
#include <QSoundEffect>
#include <QHash>
#include <QAudioDevice>
#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QTimer>
#include <QBuffer>
#include <QProcess>
#include <atomic>
#include "configfile.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmsystem.h>
#endif

// Software mixer: pre-loads WAV PCM data into memory,
// mixes active voices into a single continuous audio stream for QAudioSink.
// Eliminates per-sound WASAPI session startup latency on Windows.
struct WavSample {
    QByteArray pcmData;
};

struct ActiveVoice {
    const QByteArray* pcmData;
    qint64 position;
};

class WavMixer : public QIODevice
{
public:
    explicit WavMixer(QObject* parent = nullptr);
    bool loadWav(const QString& key, const QString& filePath);
    void play(const QString& key);
    void setVolume(float vol);
    void stopAll();
    bool hasActiveVoices();

    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) override;
    bool isSequential() const override;
    qint64 bytesAvailable() const override;

private:
    QHash<QString, WavSample> samples;
    QVector<ActiveVoice> voices;
    QMutex mutex;
    float volume;
};

class QtAudioPlayer : public QObject
{
    Q_OBJECT
public:
    enum class AudioBackend {
        QSoundEffectBackend,    // Qt6 native
        PaPlayBackend,          // PulseAudio paplay command (Linux)
        SoftwareMixerBackend,   // Pre-loaded WAVs + single QAudioSink (low-latency)
        WinMMBackend            // Win32 waveOut API (Windows, threaded, concurrent)
    };

    QtAudioPlayer(ConfigFile* config);
    ~QtAudioPlayer();

    void initAudio();
    void playSound(std::string audioName, int playerID);
    void closeAudio();
    void reInit();

    // Audio device management
    QList<QAudioDevice> availableDevices() const;
    QAudioDevice currentDevice() const;
    void setAudioDevice(const QAudioDevice& device);
    
    AudioBackend activeBackend() const { return backend; }



private slots:
    void onAudioOutputsChanged();
    void onDefaultOutputChanged();
    void onDeviceChangeDebounceTimeout();

private:
    void initQSoundEffectBackend(const QAudioDevice& device, float volume);
    void initPaPlayBackend();
    void playSoundQSoundEffect(const QString& key);
    void playSoundPaPlay(const QString& key);
    void initSoftwareMixerBackend(const QAudioDevice& device, float volume);
    void playSoundSoftwareMixer(const QString& key);
    void connectMixerSinkSignals();
    void initWinMMBackend(float volume);
    void playSoundWinMM(const QString& key);
    bool detectPaPlay();
    void applyDeviceToEffects();
    void scheduleDeviceCheck();
    bool probeAudioOutput(const QAudioDevice& device);

    ConfigFile *myConfig;
    QString myAppDataPath;
    bool audioEnabled;
    AudioBackend backend;
    
    // QSoundEffect backend
    QHash<QString, QSharedPointer<QSoundEffect>> effects;
    
    // paplay backend
    QHash<QString, QString> soundFilePaths;  // key -> file path
    QString paplayBinary;                     // path to paplay
    
#ifdef Q_OS_WIN
    // WinMM pool-based backend: N pre-opened waveOut handles for
    // concurrent low-latency playback.  Each slot plays one sound;
    // sounds on different slots overlap naturally.  Pre-loaded PCM
    // data is volume-scaled and submitted via waveOutWrite — zero
    // CPU when idle, no threads, no streaming loop.
    static const int WINMM_POOL_SIZE = 6;
    struct WinMMSlot {
        HWAVEOUT handle = nullptr;
        WAVEHDR header = {};
        QByteArray buffer;      // volume-scaled PCM copy
        bool prepared = false;  // header is prepared
    };
    WinMMSlot winmmSlots[WINMM_POOL_SIZE];
    bool winmmPoolOpen = false;
    QHash<QString, QByteArray> winmmPcmData;  // pre-loaded raw PCM
    float winmmVolume = 1.0f;
#endif
    
    // Software mixer backend
    WavMixer* mixer;
    QAudioSink* mixerSink;
    bool m_stoppingMixerIntentionally = false;
    
    // Device monitoring
    QMediaDevices* mediaDevices;
    QAudioDevice selectedDevice;      // User-selected device (empty = use default)
    QAudioDevice lastDefaultDevice;   // Track default device changes
    
    // Debounce for Bluetooth reconnects etc.
    QTimer* deviceChangeDebounceTimer;
    static constexpr int DEVICE_CHANGE_DEBOUNCE_MS = 500;
};