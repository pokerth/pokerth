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
#ifdef GUI_800x480
#ifdef ANDROID
    myPalette.setColor(QPalette::Base, QColor(255,255,255,255));
    myPalette.setColor(QPalette::Text, QColor(0,0,0,255));
#else
    myPalette.setColor(QPalette::Base, QColor(0,0,0,255));
    myPalette.setColor(QPalette::Text, QColor(255,255,255,255));
#endif
#else
	QColor myColor = myPalette.color(QPalette::Window);
	myPalette.setColor(QPalette::Base, myColor);
#endif
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

#ifdef GUI_800x480
        label_pokerthVersion->setStyleSheet("QLabel { font-size: 30px; font-weight: bold;}");
#else
	label_pokerthVersion->setStyleSheet("QLabel { font-size: 16px; font-weight: bold;}");
#endif
	label_pokerthVersion->setText(QString(tr("PokerTH %1").arg(POKERTH_BETA_RELEASE_STRING)));
	this->setWindowTitle(QString(tr("About PokerTH %1").arg(POKERTH_BETA_RELEASE_STRING)));

        //add text to lables and textbrowsers
	QString thxToInfos;
	thxToInfos.append(tr("- Wikimedia Commons: for different popular avatar picture resources")+"<br>");
	thxToInfos.append(tr("- Benedikt, Erhard, Felix, Florian, Linus, Lothar, Steffi, Caro: for people avatar pictures")+"<br>");
	thxToInfos.append(tr("- ZeiZei: for misc avatar pictures")+"<br>");
	thxToInfos.append(tr("- kde-look.org: for different gpl licensed sounds")+"<br>");
	thxToInfos.append(tr("- doc_dos: for self recorded chip sounds")+"<br>");
	thxToInfos.append(tr("- thiger, dunkanx, BerndA, coldz, drull: for different patches")+"<br>");
	thxToInfos.append(tr("- kraut: for internet-game-server hosting and administration")+"<br>");
        thxToInfos.append(tr("- danuxi: for startwindow background gfx and danuxi1 table background")+"<br>");
	thxToInfos.append(tr("- heyn: for moderating forum and organise bugtracker and feature requests")+"<br>");
	thxToInfos.append(tr("- texas_outlaw: for new table sounds")+"<br>");
	textBrowser_3->setHtml(thxToInfos);

        QString infoText;
        infoText.append(tr("- Poker engine for the popular Texas Hold'em Poker")+"\n");
        infoText.append(tr("- Singleplayer games with up to 9 computer-opponents")+"\n");
        infoText.append(tr("- Multiplayer network games")+"\n");
        infoText.append(tr("- Internet online games")+"\n");
        infoText.append(tr("- Changeable gui with online style gallery")+"\n");
        infoText.append(tr("- Online ranking website with result tables")+"\n");
        infoText.append("\n");
        infoText.append("(c)2006-2012, Felix Hammer, Florian Thauer, Lothar May");
        label_infotext->setText(infoText);

        QString projectText;
        projectText.append("<b>"+tr("Project page:")+"</b><br>");
        projectText.append("&nbsp;&nbsp;&nbsp;&nbsp;<a href='http://www.pokerth.net'>http://www.pokerth.net</a><br>");
        projectText.append("<b>IRC:</b><br>");
        projectText.append("&nbsp;&nbsp;&nbsp;&nbsp;#pokerth (irc.freenode.net)<br>");
        projectText.append("<b>"+tr("Authors:")+"</b><br>");
        projectText.append("&nbsp;&nbsp;&nbsp;&nbsp;Felix Hammer (<a href=mailto:doitux@pokerth.net>doitux@pokerth.net</a>)<br>");
        projectText.append("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- "+tr("initial idea, basic architecture, gui implementation, gui graphics editing, linux package")+"<br>");
        projectText.append("&nbsp;&nbsp;&nbsp;&nbsp;Florian Thauer (<a href=mailto:floty@pokerth.net>floty@pokerth.net</a>)<br>");
        projectText.append("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- "+tr("initial idea, basic architecture, engine development")+"<br>");
        projectText.append("&nbsp;&nbsp;&nbsp;&nbsp;Lothar May (<a href=mailto:lotodore@pokerth.net>lotodore@pokerth.net</a>)<br>");
        projectText.append("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- "+tr("basic architecture, network development, windows package, MacOS package")+"<br>");
        projectText.append("&nbsp;&nbsp;&nbsp;&nbsp;Oskar Lindqvist (<a href=mailto:tranberry@pokerth.net>tranberry@pokerth.net</a>)<br>");
        projectText.append("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- "+tr("initial gui graphics design")+"<br>");
        textBrowser_2->setHtml(projectText);
}
