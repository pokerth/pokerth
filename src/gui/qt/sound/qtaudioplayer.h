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
#include "configfile.h"

class QtAudioPlayer : public QObject
{
    Q_OBJECT
public:
    enum class AudioBackend {
        QSoundEffectBackend,  // Qt6 native (fast, but broken on some PipeWire setups)
        PaPlayBackend         // PulseAudio paplay command (robust fallback)
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
    
    // Device monitoring
    QMediaDevices* mediaDevices;
    QAudioDevice selectedDevice;      // User-selected device (empty = use default)
    QAudioDevice lastDefaultDevice;   // Track default device changes
    
    // Debounce for Bluetooth reconnects etc.
    QTimer* deviceChangeDebounceTimer;
    static constexpr int DEVICE_CHANGE_DEBOUNCE_MS = 500;
};