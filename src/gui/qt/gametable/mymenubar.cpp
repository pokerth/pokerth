#include "mymenubar.h"

MyMenuBar::MyMenuBar(QMainWindow* parent)
 : QMenuBar(parent)
{
         //this->setAttribute(Qt::WA_TranslucentBackground);
}

void MyMenuBar::paintEvent(QPaintEvent *e) {

        QMenuBar::paintEvent(e);
}
