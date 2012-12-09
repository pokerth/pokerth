#ifndef BADWORDCHECK_H
#define BADWORDCHECK_H

#include <QtCore>

class BadWordCheck: public QObject
{
	Q_OBJECT
public:
	BadWordCheck();

	void setBadWords(QStringList bw) {
		badWords = bw;
	}

    void setBadWordsException(QStringList bwe) {
        badWordsException = bwe;
    }

	bool run(QString);

private:

	QStringList badWords;
    QStringList badWordsException;
};

#endif // BADWORDCHECK_H
