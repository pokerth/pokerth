//
// C++ Interface: MyChatInputLineEdit
//
// Description: 
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MYCHATINPUTLINEEDIT_H
#define MYCHATINPUTLINEEDIT_H

#include <QtGui>
#include <QtCore>

class MyChatInputLineEdit : public QLineEdit
{
Q_OBJECT
public:
    MyChatInputLineEdit(QGroupBox*);

    ~MyChatInputLineEdit();


public slots:
	
	void keyPressEvent( QKeyEvent * event );

private: 

// 	QTimer *chatBlinkTimer;
// 	QTabBar *myTabBar;

};

#endif
