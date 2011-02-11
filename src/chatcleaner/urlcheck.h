#ifndef URLCHECK_H
#define URLCHECK_H

#include <QtCore>

class UrlCheck: public QObject
{
	Q_OBJECT
public:
	UrlCheck();

	void setUrlStrings(QStringList us) {
		urlStrings = us;
	}
	void setUrlExceptionStrings(QStringList ues) {
		urlExceptionStrings = ues;
	}
	bool run(QString);

private:

	QStringList urlStrings;
	QStringList urlExceptionStrings;
};

#endif // URLCHECK_H
