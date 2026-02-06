#pragma once
#include <QtCore>
#include <QSoundEffect>
#include <QHash>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QTimer>
#include "configfile.h"

class QtAudioPlayer : public QObject
{
    Q_OBJECT
public:
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

private slots:
    void onAudioOutputsChanged();
    void onDefaultOutputChanged();
    void onDeviceChangeDebounceTimeout();

private:
    void applyDeviceToEffects();
    void scheduleDeviceCheck();

    ConfigFile *myConfig;
    QString myAppDataPath;
    bool audioEnabled;
    QHash<QString, QSharedPointer<QSoundEffect>> effects;
    
    // Device monitoring
    QMediaDevices* mediaDevices;
    QAudioDevice selectedDevice;      // User-selected device (empty = use default)
    QAudioDevice lastDefaultDevice;   // Track default device changes
    
    // Debounce for Bluetooth reconnects etc.
    QTimer* deviceChangeDebounceTimer;
    static constexpr int DEVICE_CHANGE_DEBOUNCE_MS = 500;
};