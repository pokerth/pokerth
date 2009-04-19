#include "mymenubar.h"

MyMenuBar::MyMenuBar(QMainWindow* parent)
 : QMenuBar(parent)
{
}

void MyMenuBar::paintEvent(QPaintEvent * event) {

        QMenuBar::paintEvent(event);
}
