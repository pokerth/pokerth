#ifndef MYMENUBAR_H
#define MYMENUBAR_H

#include <QtGui>
#include <QtCore>

class MyMenuBar  : public QMenuBar
{
	Q_OBJECT
public:
	MyMenuBar(QMainWindow*);

	void paintEvent(QPaintEvent * event);
};

#endif
