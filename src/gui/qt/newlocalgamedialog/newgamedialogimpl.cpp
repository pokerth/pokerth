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
#include "newgamedialogimpl.h"
#include "changecompleteblindsdialogimpl.h"
#include "configfile.h"

newGameDialogImpl::newGameDialogImpl(QMainWindow *parent, ConfigFile *c)
	: QDialog(parent), myConfig(c)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	setupUi(this);

	myChangeCompleteBlindsDialog = new changeCompleteBlindsDialogImpl;

	connect( radioButton_changeBlindsSettings, SIGNAL( clicked(bool) ), this, SLOT( callChangeBlindsDialog(bool) ) );

}

void newGameDialogImpl::exec()
{

	spinBox_quantityPlayers->setValue(myConfig->readConfigInt("NumberOfPlayers"));
	spinBox_startCash->setValue(myConfig->readConfigInt("StartCash"));
	spinBox_gameSpeed->setValue(myConfig->readConfigInt("GameSpeed"));

	//fill changeCompleteBlindsDialog
	myChangeCompleteBlindsDialog->spinBox_firstSmallBlind->setValue(myConfig->readConfigInt("FirstSmallBlind"));
	myChangeCompleteBlindsDialog->radioButton_raiseBlindsAtHands->setChecked(myConfig->readConfigInt("RaiseBlindsAtHands"));
	myChangeCompleteBlindsDialog->radioButton_raiseBlindsAtMinutes->setChecked(myConfig->readConfigInt("RaiseBlindsAtMinutes"));
	myChangeCompleteBlindsDialog->spinBox_raiseSmallBlindEveryHands->setValue(myConfig->readConfigInt("RaiseSmallBlindEveryHands"));
	myChangeCompleteBlindsDialog->spinBox_raiseSmallBlindEveryMinutes->setValue(myConfig->readConfigInt("RaiseSmallBlindEveryMinutes"));
	myChangeCompleteBlindsDialog->radioButton_alwaysDoubleBlinds->setChecked(myConfig->readConfigInt("AlwaysDoubleBlinds"));
	myChangeCompleteBlindsDialog->radioButton_manualBlindsOrder->setChecked(myConfig->readConfigInt("ManualBlindsOrder"));

	myChangeCompleteBlindsDialog->listWidget_blinds->clear();
	myChangeCompleteBlindsDialog->spinBox_input->setMinimum(myChangeCompleteBlindsDialog->spinBox_firstSmallBlind->value()+1);

	std::list<int> myBlindsList = myConfig->readConfigIntList("ManualBlindsList");
	std::list<int>::iterator it1;

	for(it1= myBlindsList.begin(); it1 != myBlindsList.end(); ++it1) {
		myChangeCompleteBlindsDialog->listWidget_blinds->addItem(QString::number(*it1,10));
	}
	myChangeCompleteBlindsDialog->sortBlindsList();

	myChangeCompleteBlindsDialog->radioButton_afterThisAlwaysDoubleBlinds->setChecked(myConfig->readConfigInt("AfterMBAlwaysDoubleBlinds"));
	myChangeCompleteBlindsDialog->radioButton_afterThisAlwaysRaiseAbout->setChecked(myConfig->readConfigInt("AfterMBAlwaysRaiseAbout"));
	myChangeCompleteBlindsDialog->spinBox_afterThisAlwaysRaiseValue->setValue(myConfig->readConfigInt("AfterMBAlwaysRaiseValue"));
	myChangeCompleteBlindsDialog->radioButton_afterThisStayAtLastBlind->setChecked(myConfig->readConfigInt("AfterMBStayAtLastBlind"));

	QDialog::exec();

}

void newGameDialogImpl::callChangeBlindsDialog(bool show)
{

	if(show) {
		myChangeCompleteBlindsDialog->exec();
		if(myChangeCompleteBlindsDialog->result() == QDialog::Accepted ) {}
		else {
			radioButton_useSavedBlindsSettings->setChecked(TRUE);
		}

	}
}
