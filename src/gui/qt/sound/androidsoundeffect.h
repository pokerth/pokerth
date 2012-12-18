#ifndef ANDROIDSOUNDEFFECT_H
#define ANDROIDSOUNDEFFECT_H

#include <QObject>
#include <QDebug>
#include <QString>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

class AndroidSoundEffect : public QObject
{
    Q_OBJECT
public:
    explicit AndroidSoundEffect(const QString& pPath, QObject *parent = 0);
    ~AndroidSoundEffect();

    const char* getPath();
    bool load();
    bool unload();

signals:

public slots:

private:
    char* mBuffer;
    off_t mLength;

    QString mPath;

    friend class AndroidAudio;
};

#endif // ANDROIDSOUNDEFFECT_H
