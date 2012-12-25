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
#include "mycashlabel.h"
#include "gametableimpl.h"
#include "gametablestylereader.h"

MyCashLabel::MyCashLabel(QGroupBox* parent)
	: QLabel(parent), myW(0)
{
}


MyCashLabel::~MyCashLabel()
{
}

void MyCashLabel::setText ( const QString &t, bool trans)
{
	QColor transColor;
	transColor.setNamedColor("#"+myW->getMyGameTableStyle()->getPlayerCashTextColor());
	QString red = QString::number(transColor.red());
	QString green = QString::number(transColor.green());
	QString blue = QString::number(transColor.blue());

	if(trans) {
		this->setStyleSheet("QLabel { "+ myW->getMyGameTableStyle()->getFont2String() +" font-size: "+myW->getMyGameTableStyle()->getPlayerCashLabelFontSize()+"px; font-weight: bold; color: rgba("+red+", "+green+", "+blue+", 80); }");
	} else {
		this->setStyleSheet("QLabel { "+ myW->getMyGameTableStyle()->getFont2String() +" font-size: "+myW->getMyGameTableStyle()->getPlayerCashLabelFontSize()+"px; font-weight: bold; color: #"+myW->getMyGameTableStyle()->getPlayerCashTextColor()+"; }");
	}

	QLabel::setText(t);

}
