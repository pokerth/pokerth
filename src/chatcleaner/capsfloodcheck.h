#ifndef CAPSFLOODCHECK_H
#define CAPSFLOODCHECK_H

#include <QtCore>

class CapsFloodCheck: public QObject
{
	Q_OBJECT
public:
	CapsFloodCheck();

	void setCapsNumberToTrigger(int n) {
		capsNumberToTrigger = n;
	}
	bool run(QString);

private:

	int capsNumberToTrigger;
};

#endif // CAPSFLOODCHECK_H
