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
#include "joinnetworkgamedialogimpl.h"
#include "session.h"
#include <QScreen>
#include "mymessagebox.h"
#include "configfile.h"
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <net/socket_startup.h>
#include "core/appimage_utils.h"

#ifdef ANDROID
#include "mobileinputhelper.h"
#endif

using namespace std;

joinNetworkGameDialogImpl::joinNetworkGameDialogImpl(QWidget *parent, ConfigFile *c)
	: QDialog(parent), myConfig(c)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::Dialog);
#endif
	setupUi(this);
	AppImageUtils::patchExternalLinks(this);
#ifdef ANDROID
	MobileInputHelper::prepareAndroidDialog(this);
	MobileInputHelper::prepareMobileLineEdit(lineEdit_profileName);
	MobileInputHelper::prepareMobileLineEdit(lineEdit_ipAddress);
#endif
// 	QShortcut *connectKey = new QShortcut(QKeySequence(Qt::Key_Enter), this);
// 	connect( connectKey, SIGNAL(activated() ), pushButton_connect, SLOT( click() ) );

	if (myConfig->readConfigInt("CLA_NoWriteAccess")) {

		pushButton_save->setDisabled(true);
		pushButton_delete->setDisabled(true);
		treeWidget->setDisabled(true);
	}

	connect( lineEdit_ipAddress, SIGNAL( editingFinished()), this, SLOT( checkIp() ) );
	connect( lineEdit_ipAddress, SIGNAL( textChanged(QString)), this, SLOT( connectButtonTest() ) );

	connect( pushButton_connect, SIGNAL( clicked() ), this, SLOT( startClient() ) );
	connect( pushButton_save, SIGNAL( clicked() ), this, SLOT( saveServerProfile() ) );
	connect( pushButton_delete, SIGNAL( clicked() ), this, SLOT( deleteServerProfile() ) );

	connect( treeWidget, SIGNAL( itemClicked ( QTreeWidgetItem*, int) ), this, SLOT( itemFillForm (QTreeWidgetItem*, int) ) );

}

int joinNetworkGameDialogImpl::exec()
{

	bool toIntTrue;

	spinBox_port->setValue(QString::fromUtf8(myConfig->readConfigString("ServerPort").c_str()).toInt(&toIntTrue, 10));

	//Profile Name darf nicht mit einer Zahl beginnen --> XML konform
	QRegularExpression rx("[A-Z|a-z]+[A-Z|a-z|\\d]*");
	QValidator *validator = new QRegularExpressionValidator(rx, this);
	lineEdit_profileName->setValidator(validator);

	pushButton_delete->setDisabled(true);

	lineEdit_ipAddress->setFocus();

	if (myConfig->readConfigInt("CLA_NoWriteAccess") == 0 ) {
		//if discwrite-access
		myServerProfilesFile = myConfig->readConfigString("UserDataDir")+"serverprofiles.xml";

		//Anlegen wenn noch nicht existiert!
		QFile serverProfilesfile(QString::fromUtf8(myServerProfilesFile.c_str()));

		if(!serverProfilesfile.exists()) {
			QDomDocument xmlDoc;

			QDomProcessingInstruction xmlVers = xmlDoc.createProcessingInstruction("xml","version=\"1.0\" encoding='utf-8'");
			xmlDoc.appendChild(xmlVers);

			QDomElement root = xmlDoc.createElement( "PokerTH" );
			xmlDoc.appendChild( root );

			QDomElement profiles = xmlDoc.createElement( "ServerProfiles" );
			xmlDoc.appendChild( root );

			QFile file( QString::fromUtf8(myServerProfilesFile.c_str()) );
			if( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
			{
			}else{
				QTextStream stream( &file );
				stream << xmlDoc.toString();
			}
			file.close();
		}

		//Liste Füllen
		fillServerProfileList();
	}

	checkBox_ipv6->setEnabled(socket_has_ipv6());
	checkBox_sctp->setEnabled(socket_has_sctp());

	connectButtonTest();

	return QDialog::exec();

}

void joinNetworkGameDialogImpl::startClient()
{

	// TODO: Check input values!
}

void joinNetworkGameDialogImpl::fillServerProfileList()
{
	treeWidget->clear();

	QDomDocument xmlDoc;
	QFile file(QString::fromUtf8(myServerProfilesFile.c_str()));
	if (!file.open(QIODevice::ReadOnly) || !xmlDoc.setContent(&file)) {
		file.close();
		MyMessageBox::warning(this, tr("Load Server-Profile-File Error"),
							  tr("Could not load server-profiles-file:\n%1").arg(QString::fromUtf8(myServerProfilesFile.c_str())),
							  QMessageBox::Close);
	}else {

		QDomElement profile = xmlDoc.documentElement().firstChildElement( "ServerProfiles" ).firstChildElement();

		if ( !profile.isNull() ) {

			for(QDomElement n = profile; !n.isNull(); n = n.nextSiblingElement()){

				QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget,0);
				item->setData(0, 0, n.attribute("Name"));
				item->setData(1, 0, n.attribute("Address"));
				item->setData(2, 0, n.attribute("Port"));

				string isIpv6 = "no";
				int tempInt = 0;
				tempInt = n.attribute("IsIpv6").toInt();
				if( tempInt == 1 ) {
					isIpv6 = "yes";
				}
				item->setData(3, 0, QString::fromUtf8(isIpv6.c_str()));

				treeWidget->addTopLevelItem(item);
			}

		} else {
			cout << "No Profiles Found \n";
		}
	}

	treeWidget->resizeColumnToContents ( 0 );
	treeWidget->resizeColumnToContents ( 1 );
	treeWidget->resizeColumnToContents ( 2 );
	treeWidget->resizeColumnToContents ( 3 );
}

void joinNetworkGameDialogImpl::itemFillForm (QTreeWidgetItem* item, int /*column*/)
{

	bool toIntTrue;

	QDomDocument xmlDoc;
	QFile file(QString::fromUtf8(myServerProfilesFile.c_str()));
	if (!file.open(QIODevice::ReadOnly) || !xmlDoc.setContent(&file)) {
		file.close();
		MyMessageBox::warning(this, tr("Load Server-Profile-File Error"),
							  tr("Could not load server-profiles-file:\n%1").arg(QString::fromUtf8(myServerProfilesFile.c_str())),
							  QMessageBox::Close);
	}else {

		QDomElement profile = xmlDoc.documentElement().firstChildElement( "ServerProfiles" ).firstChildElement( QString::fromStdString(item->data(0,0).toString().toStdString()));

		if ( !profile.isNull()) {
			lineEdit_profileName->setText(profile.attribute("Name"));
			lineEdit_ipAddress->setText(profile.attribute("Address"));
			spinBox_port->setValue(profile.attribute("Port").toInt(&toIntTrue, 10));
			checkBox_ipv6->setChecked(profile.attribute("IsIpv6").toInt(&toIntTrue, 10));
			checkBox_sctp->setChecked(profile.attribute("IsSctp").toInt(&toIntTrue, 10));
		}

	}

	pushButton_delete->setEnabled(true);
}

void joinNetworkGameDialogImpl::saveServerProfile()
{

// 	bool toIntTrue;

	QDomDocument xmlDoc;
	QFile file(QString::fromUtf8(myServerProfilesFile.c_str()));
	if (!file.open(QIODevice::ReadOnly) || !xmlDoc.setContent(&file)) {
		file.close();
		MyMessageBox::warning(this, tr("Load Server-Profile-File Error"),
							  tr("Could not load server-profiles-file:\n%1").arg(QString::fromUtf8(myServerProfilesFile.c_str())),
							  QMessageBox::Close);
	}else {

		QDomElement profiles = xmlDoc.documentElement().firstChildElement( "ServerProfiles" );

		if ( !profiles.isNull()) {

			QDomElement testProfile = xmlDoc.documentElement().firstChildElement( "ServerProfiles" ).firstChildElement( QString::fromStdString(lineEdit_profileName->text().toStdString()) );

			if( !testProfile.isNull() ) {
				// Wenn der Name schon existiert --> Überschreiben?
				MyMessageBox msgBox(QMessageBox::Warning, tr("Save Server Profile Error"),
									QString(tr("A profile with the name: %1 already exists.\nWould you like to overwrite ?")).arg(lineEdit_profileName->text()), QMessageBox::Yes | QMessageBox::No, this);
				switch (msgBox.exec()) {

				case QMessageBox::Yes: {
					// yes was clicked
					// remove the old
					testProfile.parentNode().removeChild(testProfile);
					// write the new
					QDomElement profile1 = xmlDoc.createElement( QString::fromStdString(lineEdit_profileName->text().toUtf8().constData()) );
					profiles.appendChild( profile1 );
					profile1.attribute("Name", lineEdit_profileName->text().toUtf8().constData());
					profile1.attribute("Address", lineEdit_ipAddress->text().toUtf8().constData());
					profile1.attribute("Port", QString::number(spinBox_port->value()));
					profile1.attribute("IsIpv6", QString::number(checkBox_ipv6->isChecked()));
					profile1.attribute("IsSctp", QString::number(checkBox_sctp->isChecked()));
				}
				break;
				case QMessageBox::No:
					// no was clicked
					break;
				default:
					// should never be reached
					break;
				}

			} else {
				// Wenn der Name nicht existiert --> speichern
				QDomElement profile2 = xmlDoc.createElement( QString::fromStdString(lineEdit_profileName->text().toStdString()) );
				profiles.appendChild( profile2 );
				profile2.attribute("Name", lineEdit_profileName->text().toUtf8().constData());
				profile2.attribute("Address", lineEdit_ipAddress->text().toUtf8().constData());
				profile2.attribute("Port", QString::number(spinBox_port->value()));
				profile2.attribute("IsIpv6", QString::number(checkBox_ipv6->isChecked()));
				profile2.attribute("IsSctp", QString::number(checkBox_sctp->isChecked()));

			}
		} else {
			MyMessageBox::warning(this, tr("Read Server-Profile List Error"),
								tr("Could not read server-profiles list"),
								QMessageBox::Close);
		}
	}

	QFile file2( QString::fromUtf8(myServerProfilesFile.c_str()) );
	if( !file2.open( QIODevice::WriteOnly | QIODevice::Text ) )
	{
		MyMessageBox::warning(this, tr("Save Server-Profile-File Error"),
							  tr("Could not save server-profiles-file:\n%1").arg(QString::fromUtf8(myServerProfilesFile.c_str())),
							  QMessageBox::Close);
	}else{
		QTextStream stream( &file2 );
		stream << xmlDoc.toString();
	}
	file2.close();

	fillServerProfileList();
}

void joinNetworkGameDialogImpl::deleteServerProfile()
{
	QDomDocument xmlDoc;
	QFile file(QString::fromUtf8(myServerProfilesFile.c_str()));
	if (!file.open(QIODevice::ReadOnly) || !xmlDoc.setContent(&file)) {
		MyMessageBox::warning(this, tr("Load Server-Profile-File Error"),
									  tr("Could not load server-profiles-file:\n%1").arg(QString::fromUtf8(myServerProfilesFile.c_str())),
									  QMessageBox::Close);
	}else{

		QDomElement profile = xmlDoc.documentElement().firstChildElement( "ServerProfiles" ).firstChildElement( QString::fromStdString(treeWidget->currentItem()->data(0,0).toString().toUtf8().constData()) );

		if ( !profile.isNull() ) {
			profile.parentNode().removeChild(profile);
		}

		QFile file2( QString::fromUtf8(myServerProfilesFile.c_str()) );
		if( !file2.open( QIODevice::WriteOnly | QIODevice::Text ) )
		{
			MyMessageBox::warning(this, tr("Save Server-Profile-File Error"),
								  tr("Could not save server-profiles-file:\n%1").arg(QString::fromUtf8(myServerProfilesFile.c_str())),
								  QMessageBox::Close);
		}else{
			QTextStream stream( &file2 );
			stream << xmlDoc.toString();
		}
		file2.close();

		//Liste Füllen
		fillServerProfileList();
	}

	pushButton_delete->setDisabled(true);
}

void joinNetworkGameDialogImpl::keyPressEvent ( QKeyEvent * event )
{
#ifndef ANDROID
	if (event->key() == 16777220) {
		pushButton_connect->click();    //ENTER
	}
#else
	QDialog::keyPressEvent(event);
#endif

}

void joinNetworkGameDialogImpl::checkIp()
{

	//remove whitespaces
	QString tmp = lineEdit_ipAddress->text();
	lineEdit_ipAddress->setText(tmp.remove(" "));
}

void joinNetworkGameDialogImpl::connectButtonTest()
{

	if(lineEdit_ipAddress->text().isEmpty()) {
		pushButton_connect->setDisabled(true);
	} else {
		pushButton_connect->setDisabled(false);
	}
}
