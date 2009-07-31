#ifndef LETTERREPEATINGCHECK_H
#define LETTERREPEATINGCHECK_H

#include <QtCore>

class LetterRepeatingCheck: public QObject {
Q_OBJECT
public:
    LetterRepeatingCheck();
	
	void setLetterNumberToTrigger(int n) { letterNumberToTrigger = n; }
	bool run(QString);
	
private:

	int letterNumberToTrigger;
};

#endif // CAPSFLOODCHECK_H
