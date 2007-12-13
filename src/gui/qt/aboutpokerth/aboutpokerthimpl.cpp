/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
 *   f.thauer@web.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "aboutpokerthimpl.h"
#include "configfile.h"
#include "game_defs.h"
#include <QtCore>


aboutPokerthImpl::aboutPokerthImpl(QWidget *parent, ConfigFile *c)
    : QDialog(parent), myConfig(c)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif	
	setupUi(this);

	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());
	
	QPalette myPalette = textBrowser_licence->palette();
	QColor myColor = myPalette.color(QPalette::Window);
	myPalette.setColor(QPalette::Base, myColor);
	textBrowser_licence->setPalette(myPalette);
	textBrowser_2->setPalette(myPalette);
	textBrowser_3->setPalette(myPalette);
	textBrowser_4->setPalette(myPalette);

// 	textBrowser_licence->setSource(QUrl(QDir::toNativeSeparators(myAppDataPath+"misc/gpl2.html")));
	QFile gplFile(QDir::toNativeSeparators(myAppDataPath+"misc/gpl2.html"));
	QString gplString;
	if(gplFile.exists()) {
		if (gplFile.open( QIODevice::ReadWrite)) {
			QTextStream stream( &gplFile );
			gplString = stream.readAll();
			textBrowser_licence->setHtml(gplString);
		}
	}
	
	
	label_logo->setPixmap(QPixmap(myAppDataPath+"gfx/gui/misc/logoChip3D.png"));

	label_pokerthVersion->setStyleSheet("QLabel { font-size: 16px; font-weight: bold;}");
	label_pokerthVersion->setText(QString(tr("PokerTH %1").arg(POKERTH_BETA_RELEASE_STRING)));
	this->setWindowTitle(QString(tr("About PokerTH %1").arg(POKERTH_BETA_RELEASE_STRING)));

	textBrowser_3->append("<p style=\"margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\"font-weight:600;\">thiger, dunkanx, BerndA</span> </p>\n<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- for different patches</p>");
	
}
