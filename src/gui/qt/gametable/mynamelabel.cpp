//
// C++ Implementation: mycardspixmaplabel
//
// Description: 
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "mynamelabel.h"
#include "gametableimpl.h"
#include "session.h"
#include "gametablestylereader.h"

MyNameLabel::MyNameLabel(QGroupBox* parent)
    : QLabel(parent), myW(0)
{
}


MyNameLabel::~MyNameLabel()
{
}

void MyNameLabel::setText ( const QString &t, bool trans, bool guest, bool computerplayer) {

    QString text;
    QColor transColor;
    transColor.setNamedColor("#"+myW->getMyGameTableStyle()->getPlayerNickTextColor());
    QString red = QString::number(transColor.red());
    QString green = QString::number(transColor.green());
    QString blue = QString::number(transColor.blue());

    if(trans) {
        this->setStyleSheet("QLabel { "+ myW->getMyGameTableStyle()->getFont2String() +" font-size: "+myW->getMyGameTableStyle()->getPlayerNameLabelFontSize()+"px; font-weight: bold; color: rgba("+red+", "+green+", "+blue+", 80); }");
    }
    else {
        this->setStyleSheet("QLabel { "+ myW->getMyGameTableStyle()->getFont2String() +" font-size: "+myW->getMyGameTableStyle()->getPlayerNameLabelFontSize()+"px; font-weight: bold; color: #"+myW->getMyGameTableStyle()->getPlayerNickTextColor()+"; }");
    }

    if(myW->getSession()) {

        if(myW->getSession()->getGameType() == Session::GAME_TYPE_INTERNET && !guest && !computerplayer ) {
//          for internet game show players name with links to their profile included
            this->setTextFormat(Qt::RichText);
            QString linkString = QString("http://pokerth.net/redirect_user_profile.php?nick="+QUrl::toPercentEncoding(t));

            if(trans) {
                text = "<a style='color: rgba("+red+", "+green+", "+blue+", 80);' href='"+linkString+"'>"+t+"</a>";
            }
            else {
                text = "<a style='color: #"+myW->getMyGameTableStyle()->getPlayerNickTextColor()+";' href='"+linkString+"'>"+t+"</a>";
            }

        }
        else {
            this->setTextFormat(Qt::PlainText);
            text = t;
        }
    }
    else { text = t; }

    QLabel::setText(text);

}
