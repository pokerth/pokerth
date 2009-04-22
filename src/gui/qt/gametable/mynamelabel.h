//
// C++ Interface: mycardspixmaplabel
//
// Description: 
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MYNAMELABEL_H
#define MYNAMELABEL_H

#include <iostream>

#include <QtGui>
#include <QtCore>

class MyNameLabel : public QLabel
{
Q_OBJECT
public:
    MyNameLabel(QGroupBox*);
    ~MyNameLabel();

public slots:

        void setText ( const QString &, const bool = FALSE);

        void paintEvent(QPaintEvent*);
private:

        QString myText;
        bool transparent;
};

#endif
