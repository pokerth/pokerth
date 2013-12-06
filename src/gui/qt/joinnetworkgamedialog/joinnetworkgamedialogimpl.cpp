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
#include "mymessagebox.h"
#include "configfile.h"
#include <tinyxml.h>
#include <net/socket_startup.h>

using namespace std;

joinNetworkGameDialogImpl::joinNetworkGameDialogImpl(QWidget *parent, ConfigFile *c)
	: QDialog(parent), myConfig(c)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	setupUi(this);
#ifdef ANDROID
	this->setWindowState(Qt::WindowFullScreen);
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
	QRegExp rx("[A-Z|a-z]+[A-Z|a-z|\\d]*");
	QValidator *validator = new QRegExpValidator(rx, this);
	lineEdit_profileName->setValidator(validator);

	pushButton_delete->setDisabled(true);

	lineEdit_ipAddress->setFocus();

	if (myConfig->readConfigInt("CLA_NoWriteAccess") == 0 ) {
		//if discwrite-access
		myServerProfilesFile = myConfig->readConfigString("UserDataDir")+"serverprofiles.xml";

		//Anlegen wenn noch nicht existiert!
		QFile serverProfilesfile(QString::fromUtf8(myServerProfilesFile.c_str()));

		if(!serverProfilesfile.exists()) {

			TiXmlDocument doc;
			TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", "");
			doc.LinkEndChild( decl );

			TiXmlElement * root = new TiXmlElement( "PokerTH" );
			doc.LinkEndChild( root );

			TiXmlElement * profiles = new TiXmlElement( "ServerProfiles" );
			root->LinkEndChild( profiles );

			doc.SaveFile(QString::fromUtf8(myServerProfilesFile.c_str()).toStdString());
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

	TiXmlDocument doc(QString::fromUtf8(myServerProfilesFile.c_str()).toStdString());
	if(!doc.LoadFile()) {
		MyMessageBox::warning(this, tr("Load Server-Profile-File Error"),
							  tr("Could not load server-profiles-file:\n%1").arg(QString::fromUtf8(myServerProfilesFile.c_str())),
							  QMessageBox::Close);
	}
	TiXmlHandle docHandle( &doc );

	TiXmlElement* profile = docHandle.FirstChild( "PokerTH" ).FirstChild( "ServerProfiles" ).FirstChild().ToElement();
	if ( profile ) {

		for( ; profile; profile = profile->NextSiblingElement()) {

			QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget,0);
			item->setData(0, 0, QString::fromUtf8(profile->Attribute("Name")));
			item->setData(1, 0, QString::fromUtf8(profile->Attribute("Address")));
			item->setData(2, 0, profile->Attribute("Port"));

			string isIpv6 = "no";
			int tempInt = 0;
			profile->QueryIntAttribute("IsIpv6", &tempInt );
			if( tempInt == 1 ) {
				isIpv6 = "yes";
			}
			item->setData(3, 0, QString::fromUtf8(isIpv6.c_str()));

			string isSctp = "no";
			int tempInt1 = 0;
			profile->QueryIntAttribute("IsSctp", &tempInt1 );
			if( tempInt1 == 1 ) {
				isSctp = "yes";
			}
			item->setData(4, 0, QString::fromUtf8(isSctp.c_str()));

			treeWidget->addTopLevelItem(item);
		}
	} else {
		cout << "No Profiles Found \n";
	}

	treeWidget->resizeColumnToContents ( 0 );
	treeWidget->resizeColumnToContents ( 1 );
	treeWidget->resizeColumnToContents ( 2 );
	treeWidget->resizeColumnToContents ( 3 );
}

void joinNetworkGameDialogImpl::itemFillForm (QTreeWidgetItem* item, int /*column*/)
{

	bool toIntTrue;

	TiXmlDocument doc(QString::fromUtf8(myServerProfilesFile.c_str()).toStdString());
	if(!doc.LoadFile()) {
		MyMessageBox::warning(this, tr("Load Server-Profile-File Error"),
							  tr("Could not load server-profiles-file:\n%1").arg(QString::fromUtf8(myServerProfilesFile.c_str())),
							  QMessageBox::Close);
	}
	TiXmlHandle docHandle( &doc );

	TiXmlElement* profile = docHandle.FirstChild( "PokerTH" ).FirstChild( "ServerProfiles" ).FirstChild( item->data(0,0).toString().toStdString() ).ToElement();
	if ( profile ) {

		lineEdit_profileName->setText(QString::fromUtf8(profile->Attribute("Name")));
		lineEdit_ipAddress->setText(QString::fromUtf8(profile->Attribute("Address")));
		spinBox_port->setValue(QString::fromUtf8(profile->Attribute("Port")).toInt(&toIntTrue, 10));
		checkBox_ipv6->setChecked(QString::fromUtf8(profile->Attribute("IsIpv6")).toInt(&toIntTrue, 10));
		checkBox_sctp->setChecked(QString::fromUtf8(profile->Attribute("IsSctp")).toInt(&toIntTrue, 10));

	}

	pushButton_delete->setEnabled(true);
}

void joinNetworkGameDialogImpl::saveServerProfile()
{

// 	bool toIntTrue;

	TiXmlDocument doc(QString::fromUtf8(myServerProfilesFile.c_str()).toStdString());
	if(!doc.LoadFile()) {
		MyMessageBox::warning(this, tr("Load Server-Profile-File Error"),
							  tr("Could not load server-profiles-file:\n%1").arg(QString::fromUtf8(myServerProfilesFile.c_str())),
							  QMessageBox::Close);
	}
	TiXmlHandle docHandle( &doc );

	TiXmlElement* profiles = docHandle.FirstChild( "PokerTH" ).FirstChild( "ServerProfiles" ).ToElement();
	if ( profiles ) {

		TiXmlElement * testProfile = docHandle.FirstChild( "PokerTH" ).FirstChild( "ServerProfiles" ).FirstChild( lineEdit_profileName->text().toStdString() ).ToElement();

		if( testProfile ) {
			// Wenn der Name schon existiert --> Überschreiben?
			MyMessageBox msgBox(QMessageBox::Warning, tr("Save Server Profile Error"),
								QString(tr("A profile with the name: %1 already exists.\nWould you like to overwrite ?")).arg(lineEdit_profileName->text()), QMessageBox::Yes | QMessageBox::No, this);
			switch (msgBox.exec()) {

			case QMessageBox::Yes: {
				// yes was clicked
				// remove the old
				testProfile->Parent()->RemoveChild(testProfile);
				// write the new
				TiXmlElement * profile1 = new TiXmlElement( lineEdit_profileName->text().toUtf8().constData() );
				profiles->LinkEndChild( profile1 );
				profile1->SetAttribute("Name", lineEdit_profileName->text().toUtf8().constData());
				profile1->SetAttribute("Address", lineEdit_ipAddress->text().toUtf8().constData());
				profile1->SetAttribute("Port", spinBox_port->value());
				profile1->SetAttribute("IsIpv6", checkBox_ipv6->isChecked());
				profile1->SetAttribute("IsSctp", checkBox_sctp->isChecked());
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
			TiXmlElement * profile2 = new TiXmlElement( lineEdit_profileName->text().toStdString() );
			profiles->LinkEndChild( profile2 );
			profile2->SetAttribute("Name", lineEdit_profileName->text().toUtf8().constData());
			profile2->SetAttribute("Address", lineEdit_ipAddress->text().toUtf8().constData());
			profile2->SetAttribute("Port", spinBox_port->value());
			profile2->SetAttribute("IsIpv6", checkBox_ipv6->isChecked());
			profile2->SetAttribute("IsSctp", checkBox_sctp->isChecked());

		}
	} else {
		MyMessageBox::warning(this, tr("Read Server-Profile List Error"),
							  tr("Could not read server-profiles list"),
							  QMessageBox::Close);
	}

	if(!doc.SaveFile()) {
		MyMessageBox::warning(this, tr("Save Server-Profile-File Error"),
							  tr("Could not save server-profiles-file:\n%1").arg(QString::fromUtf8(myServerProfilesFile.c_str())),
							  QMessageBox::Close);
	}

	fillServerProfileList();
}

void joinNetworkGameDialogImpl::deleteServerProfile()
{

	TiXmlDocument doc(QString::fromUtf8(myServerProfilesFile.c_str()).toStdString());
	if(!doc.LoadFile()) {
		MyMessageBox::warning(this, tr("Load Server-Profile-File Error"),
							  tr("Could not load server-profiles-file:\n%1").arg(QString::fromUtf8(myServerProfilesFile.c_str())),
							  QMessageBox::Close);
	} else {
		TiXmlHandle docHandle( &doc );

		TiXmlElement* profile = docHandle.FirstChild( "PokerTH" ).FirstChild( "ServerProfiles" ).FirstChild( treeWidget->currentItem()->data(0,0).toString().toUtf8().constData() ).ToElement();

		if ( profile ) {
			profile->Parent()->RemoveChild(profile);
		}

		if(!doc.SaveFile()) {
			MyMessageBox::warning(this, tr("Save Server-Profile-File Error"),
								  tr("Could not save server-profiles-file:\n%1").arg(QString::fromUtf8(myServerProfilesFile.c_str())),
								  QMessageBox::Close);
		}

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
