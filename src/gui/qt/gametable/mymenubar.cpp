#include "mymenubar.h"

MyMenuBar::MyMenuBar(QMainWindow* parent)
 : QMenuBar(parent)
{
        this->setAttribute(Qt::WA_TranslucentBackground);
		this->setStyleSheet("QMenuBar { border-width: 0px; border-style: none;} ");		
}

void MyMenuBar::paintEvent(QPaintEvent *e) {

        QMenuBar::paintEvent(e);
}
