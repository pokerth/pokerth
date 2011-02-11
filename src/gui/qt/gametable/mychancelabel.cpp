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
#include "mychancelabel.h"
#include "gametableimpl.h"
#include "gametablestylereader.h"

using namespace std;

MyChanceLabel::MyChanceLabel(QWidget* parent)
	: QLabel(parent), myW(0), myStyle(0)
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

void MyChanceLabel::refreshChance(vector< vector<int> > chance)
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
	font.setPixelSize(10);
	painter.setFont(font);

	//Draw Texts
	QColor possible("#"+myStyle->getChanceLabelPossibleColor());
	QColor impossible("#"+myStyle->getChanceLabelImpossibleColor());

	if(RFChance[1] == 0) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,0), QPointF(85,13)),Qt::AlignRight,"Royal Flush");
	painter.drawText(QRectF(QPointF(196,0), QPointF(236,13)),Qt::AlignRight,QString("%1%").arg(RFChance[0]));
	if(SFChance[1] == 0) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,13), QPointF(85,26)),Qt::AlignRight,"Straight Flush");
	painter.drawText(QRectF(QPointF(196,13), QPointF(236,26)),Qt::AlignRight,QString("%1%").arg(SFChance[0]));
	if(FOAKChance[1] == 0) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,26), QPointF(85,39)),Qt::AlignRight,"Four of a Kind");
	painter.drawText(QRectF(QPointF(196,26), QPointF(236,39)),Qt::AlignRight,QString("%1%").arg(FOAKChance[0]));
	if(FHChance[1] == 0) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,39), QPointF(85,52)),Qt::AlignRight,"Full House");
	painter.drawText(QRectF(QPointF(196,39), QPointF(236,52)),Qt::AlignRight,QString("%1%").arg(FHChance[0]));
	if(FLChance[1] == 0) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,52), QPointF(85,65)),Qt::AlignRight,"Flush");
	painter.drawText(QRectF(QPointF(196,52), QPointF(236,65)),Qt::AlignRight,QString("%1%").arg(FLChance[0]));
	if(STRChance[1] == 0) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,65), QPointF(85,78)),Qt::AlignRight,"Straight");
	painter.drawText(QRectF(QPointF(196,65), QPointF(236,78)),Qt::AlignRight,QString("%1%").arg(STRChance[0]));
	if(TOAKChance[1] == 0) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,78), QPointF(85,91)),Qt::AlignRight,"Three of a Kind");
	painter.drawText(QRectF(QPointF(196,78), QPointF(236,91)),Qt::AlignRight,QString("%1%").arg(TOAKChance[0]));
	if(TPChance[1] == 0) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,91), QPointF(85,104)),Qt::AlignRight,"Two Pairs");
	painter.drawText(QRectF(QPointF(196,91), QPointF(236,104)),Qt::AlignRight,QString("%1%").arg(TPChance[0]));
	if(OPChance[1] == 0) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,104), QPointF(85,117)),Qt::AlignRight,"One Pair");
	painter.drawText(QRectF(QPointF(196,104), QPointF(236,117)),Qt::AlignRight,QString("%1%").arg(OPChance[0]));
	if(HCChance[1] == 0) {
		painter.setPen(impossible);
	} else {
		painter.setPen(possible);
	}
	painter.drawText(QRectF(QPointF(2,117), QPointF(85,130)),Qt::AlignRight,"Highest Card");
	painter.drawText(QRectF(QPointF(196,117), QPointF(236,130)),Qt::AlignRight,QString("%1%").arg(HCChance[0]));

	//Draw gfx
	painter.setPen(QColor(0,0,0));

	QLinearGradient linearGrad(QPointF(89,3), QPointF(190,10));
	linearGrad.setColorAt(0, Qt::blue);
	linearGrad.setColorAt(0.5, Qt::yellow);
	linearGrad.setColorAt(1, Qt::red);
	painter.setBrush(linearGrad);

	if(RFChance[1] != 0) painter.drawRect(89,3,(108*RFChance[0])/100,7);
	if(SFChance[1] != 0) painter.drawRect(89,16,(108*SFChance[0])/100,7);
	if(FOAKChance[1] != 0) painter.drawRect(89,29,(108*FOAKChance[0])/100,7);
	if(FHChance[1] != 0) painter.drawRect(89,42,(108*FHChance[0])/100,7);
	if(FLChance[1] != 0) painter.drawRect(89,55,(108*FLChance[0])/100,7);
	if(STRChance[1] != 0) painter.drawRect(89,68,(108*STRChance[0])/100,7);
	if(TOAKChance[1] != 0) painter.drawRect(89,81,(108*TOAKChance[0])/100,7);
	if(TPChance[1] != 0) painter.drawRect(89,94,(108*TPChance[0])/100,7);
	if(OPChance[1] != 0) painter.drawRect(89,107,(108*OPChance[0])/100,7);
	if(HCChance[1] != 0) painter.drawRect(89,120,(108*HCChance[0])/100,7);
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

	update();
}
