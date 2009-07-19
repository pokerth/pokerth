#ifndef BADWORDCHECK_H
#define BADWORDCHECK_H

#include <QtCore>

class BadWordCheck: public QObject {
Q_OBJECT
public:
    BadWordCheck();
	
	void setBadWords(QStringList bw) { badWords = bw; }
	
	bool run(QString);
	
private:
	
	QStringList badWords;
};

#endif // BADWORDCHECK_H
