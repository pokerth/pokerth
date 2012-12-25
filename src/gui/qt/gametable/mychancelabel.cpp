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
#include "mychancelabel.h"
#include "gametableimpl.h"
#include "gametablestylereader.h"

using namespace std;

MyChanceLabel::MyChanceLabel(QWidget* parent)
	: QLabel(parent), myW(0), myStyle(0), myFoldState(false)
{
	RFChance[0] = 0;
	SFChance[0] = 0;
	FOAKChance[0] = 0;
	FHChance[0] = 0;
	FLChance[0] = 0;
	STRChance[0] = 0;
	TOAKChance[0] = 0;
	TPChance[0] = 0;
	OPChance[0] = 0;
	HCChance[0] = 0;

	RFChance[1] = 0;
	SFChance[1] = 0;
	FOAKChance[1] = 0;
	FHChance[1] = 0;
	FLChance[1] = 0;
	STRChance[1] = 0;
	TOAKChance[1] = 0;
	TPChance[1] = 0;
	OPChance[1] = 0;
	HCChance[1] = 0;
}

MyChanceLabel::~MyChanceLabel()
{
}

void MyChanceLabel::refreshChance(vector< vector<int> > chance, bool fold)
{
	RFChance[0] = chance[0][9];
	SFChance[0] = chance[0][8];
	FOAKChance[0] = chance[0][7];
	FHChance[0] = chance[0][6];
	FLChance[0] = chance[0][5];
	STRChance[0] = chance[0][4];
	TOAKChance[0] = chance[0][3];
	TPChance[0] = chance[0][2];
	OPChance[0] = chance[0][1];
	HCChance[0] = chance[0][0];

	RFChance[1] = chance[1][9];
	SFChance[1] = chance[1][8];
	FOAKChance[1] = chance[1][7];
	FHChance[1] = chance[1][6];
	FLChance[1] = chance[1][5];
	STRChance[1] = chance[1][4];
	TOAKChance[1] = chance[1][3];
	TPChance[1] = chance[1][2];
	OPChance[1] = chance[1][1];
	HCChance[1] = chance[1][0];

	myFoldState = fold;

	update();
}

void MyChanceLabel::paintEvent(QPaintEvent * /*event*/)
{

	QPainter painter(this);


#ifdef _WIN32
	QString font1String = "font-family: \"Arial\";";
#else
#ifdef __APPLE__
	QString font1String = "font-family: \"Lucida Grande\";";
#else
	QString font1String = "font-family: \"Nimbus Sans L\";";
#endif
#endif
	QFont font;
	font.setFamily(font1String);
#ifdef GUI_800x480
	font.setPixelSize(18);
#else
	font.setPixelSize(10);
#endif
	painter.setFont(font);

	//Draw Texts
	QColor possible("#"+myStyle->getChanceLabelPossibleColor());
	QColor impossible("#"+myStyle->getChanceLabelImpossibleColor());

#ifdef GUI_800x480
	int start_y = 3;
	int height = 27;
	int text_width = 140;
	int percent_width = 50;
	int percent_start = 320;
	int bar_height = 13;
	int bar_width = 172;
#else
	int start_y = 0;
	int height = 13;
	int text_width = 85;
	int percent_width = 40;
	int percent_start = 196;
	int bar_height = 7;
	int bar_width = 108;
#endif

	if(RFChance[1] == 0 || myFoldState) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,start_y), QPointF(text_width,start_y+height)),Qt::AlignRight,"Royal Flush");
	painter.drawText(QRectF(QPointF(percent_start,start_y), QPointF(percent_start+percent_width,start_y+height)),Qt::AlignRight,QString("%1%").arg(RFChance[0]));
	if(SFChance[1] == 0 || myFoldState) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,start_y+height), QPointF(text_width,start_y+2*height)),Qt::AlignRight,"Straight Flush");
	painter.drawText(QRectF(QPointF(percent_start,start_y+height), QPointF(percent_start+percent_width,start_y+2*height)),Qt::AlignRight,QString("%1%").arg(SFChance[0]));
	if(FOAKChance[1] == 0 || myFoldState) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,start_y+2*height), QPointF(text_width,start_y+3*height)),Qt::AlignRight,"Four of a Kind");
	painter.drawText(QRectF(QPointF(percent_start,start_y+2*height), QPointF(percent_start+percent_width,start_y+3*height)),Qt::AlignRight,QString("%1%").arg(FOAKChance[0]));
	if(FHChance[1] == 0 || myFoldState) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,start_y+3*height), QPointF(text_width,start_y+4*height)),Qt::AlignRight,"Full House");
	painter.drawText(QRectF(QPointF(percent_start,start_y+3*height), QPointF(percent_start+percent_width,start_y+4*height)),Qt::AlignRight,QString("%1%").arg(FHChance[0]));
	if(FLChance[1] == 0 || myFoldState) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,start_y+4*height), QPointF(text_width,start_y+5*height)),Qt::AlignRight,"Flush");
	painter.drawText(QRectF(QPointF(percent_start,start_y+4*height), QPointF(percent_start+percent_width,start_y+5*height)),Qt::AlignRight,QString("%1%").arg(FLChance[0]));
	if(STRChance[1] == 0 || myFoldState) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,start_y+5*height), QPointF(text_width,start_y+6*height)),Qt::AlignRight,"Straight");
	painter.drawText(QRectF(QPointF(percent_start,start_y+5*height), QPointF(percent_start+percent_width,start_y+6*height)),Qt::AlignRight,QString("%1%").arg(STRChance[0]));
	if(TOAKChance[1] == 0 || myFoldState) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,start_y+6*height), QPointF(text_width,start_y+7*height)),Qt::AlignRight,"Three of a Kind");
	painter.drawText(QRectF(QPointF(percent_start,start_y+6*height), QPointF(percent_start+percent_width,start_y+7*height)),Qt::AlignRight,QString("%1%").arg(TOAKChance[0]));
	if(TPChance[1] == 0 || myFoldState) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,start_y+7*height), QPointF(text_width,start_y+8*height)),Qt::AlignRight,"Two Pair");
	painter.drawText(QRectF(QPointF(percent_start,start_y+7*height), QPointF(percent_start+percent_width,start_y+8*height)),Qt::AlignRight,QString("%1%").arg(TPChance[0]));
	if(OPChance[1] == 0 || myFoldState) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,start_y+8*height), QPointF(text_width,start_y+9*height)),Qt::AlignRight,"One Pair");
	painter.drawText(QRectF(QPointF(percent_start,start_y+8*height), QPointF(percent_start+percent_width,start_y+9*height)),Qt::AlignRight,QString("%1%").arg(OPChance[0]));
	if(HCChance[1] == 0 || myFoldState) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,start_y+9*height), QPointF(text_width,start_y+10*height)),Qt::AlignRight,"Highest Card");
	painter.drawText(QRectF(QPointF(percent_start,start_y+9*height), QPointF(percent_start+percent_width,start_y+10*height)),Qt::AlignRight,QString("%1%").arg(HCChance[0]));

	//Draw gfx
	painter.setPen(QColor(0,0,0));

	QLinearGradient linearGrad(QPointF(text_width+4,3), QPointF(text_width+4+bar_width,10));
	linearGrad.setColorAt(0, Qt::blue);
	linearGrad.setColorAt(0.5, Qt::yellow);
	linearGrad.setColorAt(1, Qt::red);
	painter.setBrush(linearGrad);

	if(myFoldState) {
		painter.setOpacity(0.4);
	} else {
		painter.setOpacity(1.0);
	}

	if(RFChance[1] != 0) painter.drawRect(text_width+4,start_y+0*height+3,(bar_width*RFChance[0])/100,bar_height);
	if(SFChance[1] != 0) painter.drawRect(text_width+4,start_y+1*height+3,(bar_width*SFChance[0])/100,bar_height);
	if(FOAKChance[1] != 0) painter.drawRect(text_width+4,start_y+2*height+3,(bar_width*FOAKChance[0])/100,bar_height);
	if(FHChance[1] != 0) painter.drawRect(text_width+4,start_y+3*height+3,(bar_width*FHChance[0])/100,bar_height);
	if(FLChance[1] != 0) painter.drawRect(text_width+4,start_y+4*height+3,(bar_width*FLChance[0])/100,bar_height);
	if(STRChance[1] != 0) painter.drawRect(text_width+4,start_y+5*height+3,(bar_width*STRChance[0])/100,bar_height);
	if(TOAKChance[1] != 0) painter.drawRect(text_width+4,start_y+6*height+3,(bar_width*TOAKChance[0])/100,bar_height);
	if(TPChance[1] != 0) painter.drawRect(text_width+4,start_y+7*height+3,(bar_width*TPChance[0])/100,bar_height);
	if(OPChance[1] != 0) painter.drawRect(text_width+4,start_y+8*height+3,(bar_width*OPChance[0])/100,bar_height);
	if(HCChance[1] != 0) painter.drawRect(text_width+4,start_y+9*height+3,(bar_width*HCChance[0])/100,bar_height);
}


void MyChanceLabel::resetChance()
{
	RFChance[0] = 0;
	SFChance[0] = 0;
	FOAKChance[0] = 0;
	FHChance[0] = 0;
	FLChance[0] = 0;
	STRChance[0] = 0;
	TOAKChance[0] = 0;
	TPChance[0] = 0;
	OPChance[0] = 0;
	HCChance[0] = 0;

	RFChance[1] = 0;
	SFChance[1] = 0;
	FOAKChance[1] = 0;
	FHChance[1] = 0;
	FLChance[1] = 0;
	STRChance[1] = 0;
	TOAKChance[1] = 0;
	TPChance[1] = 0;
	OPChance[1] = 0;
	HCChance[1] = 0;

	myFoldState = false;
	update();
}
