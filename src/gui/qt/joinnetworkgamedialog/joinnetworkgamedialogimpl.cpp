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
#include "joinnetworkgamedialogimpl.h"
#include "session.h"
#include "configfile.h"
#include "tinyxml.h"

using namespace std;

joinNetworkGameDialogImpl::joinNetworkGameDialogImpl(QWidget *parent)
      : QDialog(parent)
{

    setupUi(this);

	//Profile Name darf nicht mit einer Zahl beginnen --> XML konform
	QRegExp rx("[A-Z|a-z]+[A-Z|a-z|\\d]*");
 	QValidator *validator = new QRegExpValidator(rx, this);
 	lineEdit_profileName->setValidator(validator);

	lineEdit_ipAddress->setFocus();

	myServerProfilesFile = myConfig.readConfigString("DataDir")+"serverprofiles.xml";

	//Anlegen wenn noch nicht existiert!
	QFile serverProfilesfile(QString::fromStdString(myServerProfilesFile));

	if(!serverProfilesfile.exists()) {
		
		TiXmlDocument doc;  
		TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", ""); 
		doc.LinkEndChild( decl );  
		
		TiXmlElement * root = new TiXmlElement( "PokerTH" );  
		doc.LinkEndChild( root );  		
		
		TiXmlElement * profiles = new TiXmlElement( "ServerProfiles" );  
		root->LinkEndChild( profiles );  
	
		doc.SaveFile( myServerProfilesFile );
	}
	
	//Liste Füllen
	fillServerProfileList();




// 	QShortcut *connectKey = new QShortcut(QKeySequence(Qt::Key_Enter), this);
// 	connect( connectKey, SIGNAL(activated() ), pushButton_connect, SLOT( click() ) );

	connect( pushButton_connect, SIGNAL( clicked() ), this, SLOT( startClient() ) );
	connect( pushButton_save, SIGNAL( clicked() ), this, SLOT( saveServerProfile() ) );
	connect( pushButton_delete, SIGNAL( clicked() ), this, SLOT( deleteServerProfile() ) );

}

void joinNetworkGameDialogImpl::startClient() {

	// TODO: Check input values!
}

void joinNetworkGameDialogImpl::fillServerProfileList() {

	
	treeWidget->clear();

	TiXmlDocument doc(myServerProfilesFile); 
	if(!doc.LoadFile()) {	
		QMessageBox::warning(this, tr("Load Server-Profile-File Error"),
			tr("Could not load Server-Profiles-File:\n"+QString::fromStdString(myServerProfilesFile).toAscii()),
			QMessageBox::Close);		
	}
	TiXmlHandle docHandle( &doc );		

	TiXmlElement* profile = docHandle.FirstChild( "PokerTH" ).FirstChild( "ServerProfiles" ).FirstChild().ToElement();
	if ( profile ) {

                for( profile; profile; profile = profile->NextSiblingElement()) {

			
			
		   	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget,0);
			item->setData(0, 0, profile->Attribute("Name"));
			item->setData(1, 0, profile->Attribute("Address"));
			item->setData(2, 0, profile->Attribute("Port"));
			
			string isIpv6 = "no";
			int tempInt = 0;
			profile->QueryIntAttribute("IsIpv6", &tempInt );
			if( tempInt == 1 ) { isIpv6 = "yes"; }
			item->setData(3, 0, QString::fromStdString(isIpv6));

			treeWidget->addTopLevelItem(item);

		}
	} 
	else { cout << "No Profiles Found \n";  }

	treeWidget->resizeColumnToContents ( 0 );  
	treeWidget->resizeColumnToContents ( 1 );
	treeWidget->resizeColumnToContents ( 2 );
	treeWidget->resizeColumnToContents ( 3 );
}


void joinNetworkGameDialogImpl::saveServerProfile() {

// 	bool toIntTrue;

	TiXmlDocument doc(myServerProfilesFile); 
	if(!doc.LoadFile()) {	
		QMessageBox::warning(this, tr("Load Server-Profile-File Error"),
			tr("Could not load Server-Profiles-File:\n"+QString::fromStdString(myServerProfilesFile).toAscii()),
			QMessageBox::Close);		
	}
	TiXmlHandle docHandle( &doc );		

	TiXmlElement* profiles = docHandle.FirstChild( "PokerTH" ).FirstChild( "ServerProfiles" ).ToElement();
	if ( profiles ) {

		TiXmlElement * testProfile = docHandle.FirstChild( "PokerTH" ).FirstChild( "ServerProfiles" ).FirstChild( lineEdit_profileName->text().toStdString() ).ToElement();	
		
		if( testProfile ) {
			// Wenn der Name schon existiert --> Hinweis und nicht speichern
			QMessageBox::warning(this, tr("Save Server Profile Error"),
				tr("A Profile with this Name already exists.\nPlease select another Profile Name!"),
				QMessageBox::Close);
		}
		else {
			// Wenn der Name nicht existiert --> speichern
			TiXmlElement * profile = new TiXmlElement( lineEdit_profileName->text().toStdString() );
			profiles->LinkEndChild( profile );
			profile->SetAttribute("Name", lineEdit_profileName->text().toStdString());
			profile->SetAttribute("Address", lineEdit_ipAddress->text().toStdString());
			profile->SetAttribute("Password", lineEdit_password->text().toStdString());
			profile->SetAttribute("Port", spinBox_port->value());
			profile->SetAttribute("IsIpv6", checkBox_ipv6->isChecked());
 			
		}
        } 
	else { QMessageBox::warning(this, tr("Read Server-Profile List Error"),
			tr("Could not read Server-Profiles List"),
			QMessageBox::Close);	 }

	if(!doc.SaveFile()) {	QMessageBox::warning(this, tr("Save Server-Profile-File Error"),
			tr("Could not save Server-Profiles-File:\n"+QString::fromStdString(myServerProfilesFile).toAscii()),
			QMessageBox::Close);	 }

	fillServerProfileList();
}

void joinNetworkGameDialogImpl::deleteServerProfile() {

	// TODO: Check input values!
}

void joinNetworkGameDialogImpl::keyPressEvent ( QKeyEvent * event ) {

// 	std::cout << "key" << event->key();
	
	if (event->key() == 16777220) { pushButton_connect->click(); } //ENTER 
	
}
