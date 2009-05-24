#include "mymenubar.h"

MyMenuBar::MyMenuBar(QMainWindow* parent)
 : QMenuBar(parent)
{
#if (QT_VERSION >= 0x040501)
	this->setAttribute(Qt::WA_TranslucentBackground);
	this->setStyleSheet("QMenuBar { border-width: 0px; border-style: none;} ");		
#endif		
}

void MyMenuBar::paintEvent(QPaintEvent *e) {

        QMenuBar::paintEvent(e);
}
