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
#ifndef CREATEINTERNETGAMEDIALOGIMPL_H
#define CREATEINTERNETGAMEDIALOGIMPL_H

#ifdef GUI_800x480
#include "ui_createinternetgamedialog_800x480.h"
#else
#include "ui_createinternetgamedialog.h"
#endif

#include <QtGui>
#include <QtCore>

class Session;
class ConfigFile;
class changeCompleteBlindsDialogImpl;
class CommunitySuggest;
class QComboBox;
class QLabel;

class createInternetGameDialogImpl: public QDialog, public Ui::createInternetGameDialog
{
	Q_OBJECT
public:
	// Offizielle Community-Turnier-Vorlagen (BBC Steps / Monthly Cup / WEC) –
	// deckungsgleich mit den communityPresets des QML-Clients.
	// Die Vorlagentabelle selbst liegt in CommunitySuggest (dieselbe Tabelle
	// dient dort als Fingerprint zur Typ-Erkennung fremder Tische) – hier wird
	// sie nur noch gelesen.

	createInternetGameDialogImpl(QWidget *parent = 0, ConfigFile *c = 0, CommunitySuggest *suggest = 0);

	void exec(bool guestMode, QString playerName);

	// Suggest-Typ der aktuell gewählten Community-Vorlage ("" wenn keine bzw.
	// wenn kein Invite-Spiel/Community-Inhalt). Vom Warteraum nach dem Anlegen
	// gelesen, um den Suggest-Button des eigenen Spiels zu steuern.
	QString selectedSuggestType() const;
	changeCompleteBlindsDialogImpl* getChangeCompleteBlindsDialog()
	{
		return myChangeCompleteBlindsDialog;
	}
	bool eventFilter(QObject *obj, QEvent *event) override;
	void accept() override;

public slots:

	void createGame();
	void cancel();
	void fillFormular(bool guestMode, QString playerName);
	void keyPressEvent ( QKeyEvent * event ) override;
	void clearGamePassword(bool);
	void gameTypeChanged();
	// Wendet die gewählte Community-Vorlage auf das Formular an (bzw. stellt bei
	// „Eigene Einstellungen" die Werte aus gameTypeChanged() wieder her).
	void applyCommunityTemplate();

	void callChangeBlindsDialog(bool);
private:
	// Sichtbarkeit der Vorlagen-Auswahl: nur bei Invite-Spiel + Community-Inhalt.
	void updateCommunityTemplateVisibility();

	ConfigFile *myConfig;
	CommunitySuggest *mySuggest;
	changeCompleteBlindsDialogImpl *myChangeCompleteBlindsDialog;
	bool currentGuestMode;
	QString currentPlayerName;
	QLabel *startBlind;
	QLabel *raiseMode;

	QLabel *label_communityTemplate;
	QComboBox *comboBox_communityTemplate;

};

#endif
