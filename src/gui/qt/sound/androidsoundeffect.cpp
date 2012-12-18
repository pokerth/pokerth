#include <QtGlobal> // need this to get Q_OS_ANDROID #define, which we need before we include anything else!

#if defined(Q_OS_ANDROID)

#include "androidsoundeffect.h"
#include <QFile>

AndroidSoundEffect::AndroidSoundEffect(const QString& path, QObject *parent) :
    QObject(parent), mBuffer(NULL), mLength(0), mPath(path)
{
}

AndroidSoundEffect::~AndroidSoundEffect()
{
    this->unload();
}

bool AndroidSoundEffect::load()
{
    QFile lSoundFile(mPath);

    qDebug() << "opening:" << mPath;

    if (!lSoundFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open: " << mPath;
        return false;
    }

    // parse WAV file
    qDebug() << "    reading header:";

    char id[4];
    lSoundFile.read(id, 4);
    if( strncmp(id, "RIFF", 4) != 0 ) {
        qDebug() << "not a WAV file - header not RIFF";
        lSoundFile.close();
        return false;
    }

    int size = 0;
    lSoundFile.read((char *)&size, 4);
    qDebug() << "    size:" << size;

    lSoundFile.read(id, 4);
    if( strncmp(id, "WAVE", 4) != 0 ) {
        qDebug() << "not a WAV file - header not WAVE";
        lSoundFile.close();
        return false;
    }

    lSoundFile.read(id, 4); // "fmt "

    int format_length = 0;
    lSoundFile.read((char *)&format_length, 4);
    qDebug() << "    format_length:" << format_length;

    short format_tag = 0;
    lSoundFile.read((char *)&format_tag, 2);

    short n_channels = 0;
    lSoundFile.read((char *)&n_channels, 2);
    qDebug() << "    n_channels:" << n_channels;

    int sample_rate = 0;
    lSoundFile.read((char *)&sample_rate, 4);
    qDebug() << "    sample_rate:" << sample_rate;

    int avg_bytes_sec = 0;
    lSoundFile.read((char *)&avg_bytes_sec, 4);
    qDebug() << "    avg_bytes_sec:" << avg_bytes_sec;

    short block_align = 0;
    lSoundFile.read((char *)&block_align, 2);

    short bits_per_sample = 0;
    lSoundFile.read((char *)&bits_per_sample, 2);
    qDebug() << "    bits_per_sample:" << bits_per_sample;

    lSoundFile.read(id, 4);
    if( strncmp(id, "data", 4) != 0 ) {
        qDebug() << "not a WAV file - didn't find data";
        lSoundFile.close();
        return false;
    }

    lSoundFile.read((char *)&mLength, 4);

    qDebug() << "    reading data:" << mLength;

    mBuffer = (char*)malloc((mLength));

    int dataRead = lSoundFile.read(mBuffer, mLength);
    if (dataRead != mLength) {
        qDebug() << "didn't read correct amount of data' :" << mPath;
        lSoundFile.close();
        delete [] mBuffer;
        mBuffer = NULL;
        return false;
    }

    qDebug() << "    closing:";
    lSoundFile.close();
    return true;
}


bool AndroidSoundEffect::unload()
{
    delete[] mBuffer;
    mBuffer = NULL;
    mLength = 0;
    return true;
}

#endif
