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
#include "aboutpokerthimpl.h"
#include "configfile.h"
#include "game_defs.h"
#include <QtCore>

#ifdef ANDROID
#ifndef ANDROID_TEST
#include "QtGui/5.7.1/QtGui/qpa/qplatformnativeinterface.h"
#include <jni.h>
#endif
#endif

aboutPokerthImpl::aboutPokerthImpl(QWidget *parent, ConfigFile *c)
	: QDialog(parent), myConfig(c)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	setupUi(this);

	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());

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

#ifdef ANDROID
	int api = -2;
	this->setWindowState(Qt::WindowFullScreen);
#ifndef ANDROID_TEST
	JavaVM *currVM = (JavaVM *)QApplication::platformNativeInterface()->nativeResourceForIntegration("JavaVM");
	JNIEnv* env;
	if (currVM->AttachCurrentThread(&env, NULL)<0) {
		qCritical()<<"AttachCurrentThread failed";
	} else {
		jclass jclassApplicationClass = env->FindClass("android/os/Build$VERSION");
		if (jclassApplicationClass) {
			api = env->GetStaticIntField(jclassApplicationClass, env->GetStaticFieldID(jclassApplicationClass,"SDK_INT", "I"));
		}
		currVM->DetachCurrentThread();
	}
#endif
	label_pokerthVersion->setText(QString(tr("PokerTH %1 for Android (API%2)").arg(POKERTH_BETA_RELEASE_STRING).arg(api)));
#else
	label_pokerthVersion->setText(QString(tr("PokerTH %1").arg(POKERTH_BETA_RELEASE_STRING)));
#endif
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
	QString thisYear = QDate::currentDate().toString("yyyy");
	infoText.append("(c)2006-"+thisYear+", Felix Hammer, Florian Thauer, Lothar May");
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

	QFile file(QDir::toNativeSeparators(myAppDataPath+"misc/third_party_libs.txt"));
	QString string;
	if(file.exists()) {
		if (file.open( QIODevice::ReadOnly)) {
			QTextStream stream( &file );
			string = stream.readAll();
			textBrowser_thirdPartyLicenceText->setPlainText(string);
		}
	}
}
