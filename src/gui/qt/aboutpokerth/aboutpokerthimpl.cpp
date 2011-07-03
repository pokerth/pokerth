/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/
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

	QFile gplFile(QDir::toNativeSeparators(myAppDataPath+"misc/agpl.html"));
	QString gplString;
	if(gplFile.exists()) {
		if (gplFile.open( QIODevice::ReadOnly)) {
			QTextStream stream( &gplFile );
			gplString = stream.readAll();
			textBrowser_licence->setHtml(gplString);
		}
	}


	label_logo->setPixmap(QPixmap(":/gfx/logoChip3D.png"));

	label_pokerthVersion->setStyleSheet("QLabel { font-size: 16px; font-weight: bold;}");
	label_pokerthVersion->setText(QString(tr("PokerTH %1").arg(POKERTH_BETA_RELEASE_STRING)));
	this->setWindowTitle(QString(tr("About PokerTH %1").arg(POKERTH_BETA_RELEASE_STRING)));

}
