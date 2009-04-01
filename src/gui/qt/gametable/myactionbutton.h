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
#ifndef MYACTIONBUTTON_H
#define MYACTIONBUTTON_H

#include <QtGui>
#include <QtCore>

class GameTableStyleReader;

class MyActionButton : public QPushButton
{
public:
    MyActionButton(QGroupBox*);

    ~MyActionButton();

	void paintEvent(QPaintEvent * event); 
	void setFKeyText ( const QString& theValue ){fKeyText = theValue;}

	void setMyStyle ( GameTableStyleReader* theValue ) { myStyle = theValue; }

private:

	QString fKeyText;
	GameTableStyleReader *myStyle;
};

#endif
