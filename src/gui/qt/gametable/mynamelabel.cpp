/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#include "mynamelabel.h"
#include "gametableimpl.h"
#include "session.h"
#include "game.h"
#include "playerinterface.h"
#include "gametablestylereader.h"

MyNameLabel::MyNameLabel(QGroupBox* parent)
	: QLabel(parent), myW(0)
{
}


MyNameLabel::~MyNameLabel()
{
}

void MyNameLabel::setText ( const QString &t, bool trans, bool guest, bool computerplayer)
{

	QString text;
	QColor transColor;
	transColor.setNamedColor("#"+myW->getMyGameTableStyle()->getPlayerNickTextColor());
	QString red = QString::number(transColor.red());
	QString green = QString::number(transColor.green());
	QString blue = QString::number(transColor.blue());

	if(trans) {
		this->setStyleSheet("QLabel { "+ myW->getMyGameTableStyle()->getFont2String() +" font-size: "+myW->getMyGameTableStyle()->getPlayerNameLabelFontSize()+"px; font-weight: bold; color: rgba("+red+", "+green+", "+blue+", 80); }");
	} else {
		this->setStyleSheet("QLabel { "+ myW->getMyGameTableStyle()->getFont2String() +" font-size: "+myW->getMyGameTableStyle()->getPlayerNameLabelFontSize()+"px; font-weight: bold; color: #"+myW->getMyGameTableStyle()->getPlayerNickTextColor()+"; }");
	}

	if(myW->getSession()) {

		if(myW->getSession()->getGameType() == Session::GAME_TYPE_INTERNET && !guest && !computerplayer ) {
//          for internet game show players name with links to their profile included
			this->setTextFormat(Qt::RichText);
			QString linkString;

			GameInfo info(myW->getSession()->getClientGameInfo(myW->getSession()->getClientCurrentGameId()));
			if(info.data.gameType == GAME_TYPE_RANKING) {

				QString nickList;
				//build nick string list
				if(myW->getSession()->getCurrentGame()) {

					boost::shared_ptr<Game> currentGame = myW->getSession()->getCurrentGame();
					PlayerListConstIterator it_c;
					PlayerList seatsList = currentGame->getSeatsList();
					int playerCounter = 0;
					for (it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
						if((*it_c)->getMyActiveStatus()) {
							++playerCounter;
							nickList += QString("&nick%1=").arg(playerCounter);
							nickList += QUrl::toPercentEncoding(QString::fromUtf8((*it_c)->getMyName().c_str()));
						}
					}
				}

				linkString = QString("http://pokerth.net/redirect_user_profile.php?tableview=1"+nickList+"&table="+QUrl::toPercentEncoding(QString::fromUtf8(info.name.c_str())));
			} else {
				linkString = QString("http://pokerth.net/redirect_user_profile.php?nick="+QUrl::toPercentEncoding(t));
			}

			// truncate nicknames longer than 13 characters
			QString t2 = t;
			if(t2.size() > 13) {
				int chop = t.size() - 13 + 3;
				t2.chop(chop);
				t2.append("...");
			}

			QString tTruncated;

			if(trans) {
				text = "<a style='color: rgba("+red+", "+green+", "+blue+", 80);' href='"+linkString+"'>"+t2+"</a>";
			} else {
				text = "<a style='color: #"+myW->getMyGameTableStyle()->getPlayerNickTextColor()+";' href='"+linkString+"'>"+t2+"</a>";
			}

		} else {
			this->setTextFormat(Qt::PlainText);
			text = t;
		}
	} else {
		text = t;
	}

	QLabel::setText(text);

}
