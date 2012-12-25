/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
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

//    qDebug() << "opening:" << mPath;

	if (!lSoundFile.open(QIODevice::ReadOnly)) {
		qDebug() << "Could not open: " << mPath;
		return false;
	}

	// parse WAV file
//    qDebug() << "    reading header:";

	char id[4];
	lSoundFile.read(id, 4);
	if( strncmp(id, "RIFF", 4) != 0 ) {
		qDebug() << "not a WAV file - header not RIFF";
		lSoundFile.close();
		return false;
	}

	int size = 0;
	lSoundFile.read((char *)&size, 4);
//    qDebug() << "    size:" << size;

	lSoundFile.read(id, 4);
	if( strncmp(id, "WAVE", 4) != 0 ) {
		qDebug() << "not a WAV file - header not WAVE";
		lSoundFile.close();
		return false;
	}

	lSoundFile.read(id, 4); // "fmt "

	int format_length = 0;
	lSoundFile.read((char *)&format_length, 4);
//    qDebug() << "    format_length:" << format_length;

	short format_tag = 0;
	lSoundFile.read((char *)&format_tag, 2);

	short n_channels = 0;
	lSoundFile.read((char *)&n_channels, 2);
//    qDebug() << "    n_channels:" << n_channels;

	int sample_rate = 0;
	lSoundFile.read((char *)&sample_rate, 4);
//    qDebug() << "    sample_rate:" << sample_rate;

	int avg_bytes_sec = 0;
	lSoundFile.read((char *)&avg_bytes_sec, 4);
//    qDebug() << "    avg_bytes_sec:" << avg_bytes_sec;

	short block_align = 0;
	lSoundFile.read((char *)&block_align, 2);

	short bits_per_sample = 0;
	lSoundFile.read((char *)&bits_per_sample, 2);
//    qDebug() << "    bits_per_sample:" << bits_per_sample;

	lSoundFile.read(id, 4);
	if( strncmp(id, "data", 4) != 0 ) {
		qDebug() << "not a WAV file - didn't find data";
		lSoundFile.close();
		return false;
	}

	lSoundFile.read((char *)&mLength, 4);

//    qDebug() << "    reading data:" << mLength;

	mBuffer = (char*)malloc((mLength));

	int dataRead = lSoundFile.read(mBuffer, mLength);
	if (dataRead != mLength) {
		qDebug() << "didn't read correct amount of data' :" << mPath;
		lSoundFile.close();
		delete [] mBuffer;
		mBuffer = NULL;
		return false;
	}

//    qDebug() << "    closing:";
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
