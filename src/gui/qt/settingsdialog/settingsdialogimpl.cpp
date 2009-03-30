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
#include "settingsdialogimpl.h"
#include "myavatarlistitem.h"
#include "gametablestylereader.h"
#include "carddeckstylereader.h"

#include "configfile.h"
#include <net/socket_startup.h>

#define POKERTH_DISTRIBUTED_GAME_TABLE_STYLES 1
#define POKERTH_DISTRIBUTED_CARD_DECK_STYLES 2

using namespace std;

settingsDialogImpl::settingsDialogImpl(QWidget *parent, ConfigFile *c, selectAvatarDialogImpl *s)
    : QDialog(parent), myConfig(c), mySelectAvatarDialogImpl(s)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif	
  	setupUi(this);

	myManualBlindsOrderDialog = new manualBlindsOrderDialogImpl; 	

	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());

	pushButton_openFlipsidePicture->setIcon(QIcon(QPixmap(myAppDataPath+"gfx/gui/misc/fileopen16.png")));
	pushButton_openLogDir->setIcon(QIcon(QPixmap(myAppDataPath+"gfx/gui/misc/fileopen16.png")));

	if (myConfig->readConfigInt("CLA_NoWriteAccess")) { groupBox_logOnOff->setDisabled(TRUE); }

	comboBox_switchLanguage->addItem(tr("Bulgarian"),"bg");
	comboBox_switchLanguage->addItem(tr("Chinese"),"zhcn");
	comboBox_switchLanguage->addItem(tr("Danish"),"dk");
	comboBox_switchLanguage->addItem(tr("Dutch"),"nl");
	comboBox_switchLanguage->addItem(tr("English"),"en");
	comboBox_switchLanguage->addItem(tr("French"),"fr");
	comboBox_switchLanguage->addItem(tr("Finnish"),"fi");
	comboBox_switchLanguage->addItem(tr("German"),"de");
	comboBox_switchLanguage->addItem(tr("Greek"),"gr");
	comboBox_switchLanguage->addItem(tr("Hungarian"),"hu");
	comboBox_switchLanguage->addItem(tr("Italian"),"it");
	comboBox_switchLanguage->addItem(tr("Norwegian"),"no");
	comboBox_switchLanguage->addItem(tr("Polish"),"pl");
	comboBox_switchLanguage->addItem(tr("Portuguese"),"ptbr");
	comboBox_switchLanguage->addItem(tr("Russian"),"ru");
	comboBox_switchLanguage->addItem(tr("Slovak"),"sk");
	comboBox_switchLanguage->addItem(tr("Spanish"),"es");
	comboBox_switchLanguage->addItem(tr("Swedish"),"sv");
	comboBox_switchLanguage->addItem(tr("Turkish"),"tr");

	connect( buttonBox, SIGNAL( accepted() ), this, SLOT( isAccepted() ) );
	connect( lineEdit_HumanPlayerName, SIGNAL( textChanged( const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent1Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent2Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent3Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent4Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent5Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent6Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent7Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent8Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( lineEdit_Opponent9Name, SIGNAL( textChanged(const QString &) ), this, SLOT( playerNickChanged() ) );
	connect( pushButton_openFlipsidePicture, SIGNAL( clicked() ), this, SLOT( setFlipsidePicFileName()) );
	connect( pushButton_openLogDir, SIGNAL( clicked() ), this, SLOT( setLogDir()) );
	connect( pushButton_HumanPlayerAvatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile0()) );
	connect( pushButton_Opponent1Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile1()) );
	connect( pushButton_Opponent2Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile2()) );
	connect( pushButton_Opponent3Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile3()) );
	connect( pushButton_Opponent4Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile4()) );
	connect( pushButton_Opponent5Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile5()) );
	connect( pushButton_Opponent6Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile6()) );
	connect( pushButton_Opponent7Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile7()) );
	connect( pushButton_Opponent8Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile8()) );
	connect( pushButton_Opponent9Avatar, SIGNAL( clicked() ), this, SLOT( setAvatarFile9()) );
	connect( pushButton_editManualBlindsOrder, SIGNAL( clicked() ), this, SLOT( callManualBlindsOrderDialog()) );
	connect( pushButton_netEditManualBlindsOrder, SIGNAL( clicked() ), this, SLOT( callNetManualBlindsOrderDialog()) );

	connect( checkBox_UseInternetGamePassword, SIGNAL( toggled(bool) ), this, SLOT( clearInternetGamePassword(bool)) ); 

// 	connect( spinBox_netFirstSmallBlind, SIGNAL( valueChanged(int) ), this, SLOT ( checkProperNetFirstSmallBlind(int)));
// 	connect( spinBox_firstSmallBlind, SIGNAL( valueChanged(int) ), this, SLOT ( checkProperFirstSmallBlind(int)));

	connect( radioButton_netManualBlindsOrder, SIGNAL( toggled(bool) ), this, SLOT( setFirstSmallBlindMargin() ));
	connect( radioButton_manualBlindsOrder, SIGNAL( toggled(bool) ), this, SLOT( setFirstSmallBlindMargin() ));

	connect( comboBox_switchLanguage, SIGNAL( currentIndexChanged (int) ), this, SLOT ( setLanguageChanged(int) ));

	connect(groupBox_automaticServerConfig, SIGNAL(toggled(bool)), this, SLOT(toggleGroupBoxAutomaticServerConfig(bool)));
	connect(groupBox_manualServerConfig, SIGNAL(toggled(bool)), this, SLOT(toggleGroupBoxManualServerConfig(bool)));

	connect( listWidget_gameTableStyles, SIGNAL( currentItemChanged(QListWidgetItem*, QListWidgetItem*) ), this, SLOT ( showCurrentGameTableStylePreview() ));
	connect( pushButton_activateGameTableStyle, SIGNAL( clicked() ), this, SLOT( setSelectedGameTableStyleActivated()) );
	connect( pushButton_addGameTableStyle, SIGNAL( clicked() ), this, SLOT( addGameTableStyle()) );
	connect( pushButton_removeGameTableStyle, SIGNAL( clicked() ), this, SLOT( removeGameTableStyle()) );

	connect( listWidget_cardDeckStyles, SIGNAL( currentItemChanged(QListWidgetItem*, QListWidgetItem*) ), this, SLOT ( showCurrentCardDeckStylePreview() ));
	connect( pushButton_activateCardDeckStyle, SIGNAL( clicked() ), this, SLOT( setSelectedCardDeckStyleActivated()) );
	connect( pushButton_addCardDeckStyle, SIGNAL( clicked() ), this, SLOT( addCardDeckStyle()) );
	connect( pushButton_removeCardDeckStyle, SIGNAL( clicked() ), this, SLOT( removeCardDeckStyle()) );
}

void settingsDialogImpl::exec() {

	playerNickIsChanged = FALSE;
	
	//Player Nicks
	lineEdit_HumanPlayerName->setText(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()));
	pushButton_HumanPlayerAvatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("MyAvatar").c_str()));
	lineEdit_Opponent1Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent1Name").c_str()));
	pushButton_Opponent1Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent1Avatar").c_str()));
	lineEdit_Opponent2Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent2Name").c_str()));
	pushButton_Opponent2Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent2Avatar").c_str()));
	lineEdit_Opponent3Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent3Name").c_str()));
	pushButton_Opponent3Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent3Avatar").c_str()));
	lineEdit_Opponent4Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent4Name").c_str()));
	pushButton_Opponent4Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent4Avatar").c_str()));
	lineEdit_Opponent5Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent5Name").c_str()));
	pushButton_Opponent5Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent5Avatar").c_str()));
	lineEdit_Opponent6Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent6Name").c_str()));
	pushButton_Opponent6Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent6Avatar").c_str()));
	lineEdit_Opponent7Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent7Name").c_str()));
	pushButton_Opponent7Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent7Avatar").c_str()));
	lineEdit_Opponent8Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent8Name").c_str()));
	pushButton_Opponent8Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent8Avatar").c_str()));
	lineEdit_Opponent9Name->setText(QString::fromUtf8(myConfig->readConfigString("Opponent9Name").c_str()));
	pushButton_Opponent9Avatar->setMyLink(QString::fromUtf8(myConfig->readConfigString("Opponent9Avatar").c_str()));

	//refresh AvatarIcons
	pushButton_HumanPlayerAvatar->setIcon(QIcon(pushButton_HumanPlayerAvatar->getMyLink()));
	pushButton_Opponent1Avatar->setIcon(QIcon(pushButton_Opponent1Avatar->getMyLink()));
	pushButton_Opponent2Avatar->setIcon(QIcon(pushButton_Opponent2Avatar->getMyLink()));
	pushButton_Opponent3Avatar->setIcon(QIcon(pushButton_Opponent3Avatar->getMyLink()));
	pushButton_Opponent4Avatar->setIcon(QIcon(pushButton_Opponent4Avatar->getMyLink()));
	pushButton_Opponent5Avatar->setIcon(QIcon(pushButton_Opponent5Avatar->getMyLink()));
	pushButton_Opponent6Avatar->setIcon(QIcon(pushButton_Opponent6Avatar->getMyLink()));
	pushButton_Opponent7Avatar->setIcon(QIcon(pushButton_Opponent7Avatar->getMyLink()));
	pushButton_Opponent8Avatar->setIcon(QIcon(pushButton_Opponent8Avatar->getMyLink()));
	pushButton_Opponent9Avatar->setIcon(QIcon(pushButton_Opponent9Avatar->getMyLink()));

	//Local Game Settings
	spinBox_quantityPlayers->setValue(myConfig->readConfigInt("NumberOfPlayers"));
	spinBox_startCash->setValue(myConfig->readConfigInt("StartCash"));
	spinBox_firstSmallBlind->setValue(myConfig->readConfigInt("FirstSmallBlind"));
	radioButton_raiseBlindsAtHands->setChecked(myConfig->readConfigInt("RaiseBlindsAtHands"));
	radioButton_raiseBlindsAtMinutes->setChecked(myConfig->readConfigInt("RaiseBlindsAtMinutes"));
	spinBox_raiseSmallBlindEveryHands->setValue(myConfig->readConfigInt("RaiseSmallBlindEveryHands"));
	spinBox_raiseSmallBlindEveryMinutes->setValue(myConfig->readConfigInt("RaiseSmallBlindEveryMinutes"));
	radioButton_alwaysDoubleBlinds->setChecked(myConfig->readConfigInt("AlwaysDoubleBlinds"));
	radioButton_manualBlindsOrder->setChecked(myConfig->readConfigInt("ManualBlindsOrder"));
	spinBox_gameSpeed->setValue(myConfig->readConfigInt("GameSpeed"));
	checkBox_pauseBetweenHands->setChecked(myConfig->readConfigInt("PauseBetweenHands"));
	checkBox_showGameSettingsDialogOnNewGame->setChecked(myConfig->readConfigInt("ShowGameSettingsDialogOnNewGame"));
	
	//Network Game Settings
	spinBox_netQuantityPlayers->setValue(myConfig->readConfigInt("NetNumberOfPlayers"));
	spinBox_netStartCash->setValue(myConfig->readConfigInt("NetStartCash"));
	spinBox_netFirstSmallBlind->setValue(myConfig->readConfigInt("NetFirstSmallBlind"));
	radioButton_netRaiseBlindsAtHands->setChecked(myConfig->readConfigInt("NetRaiseBlindsAtHands"));
	radioButton_netRaiseBlindsAtMinutes->setChecked(myConfig->readConfigInt("NetRaiseBlindsAtMinutes"));
	spinBox_netRaiseSmallBlindEveryHands->setValue(myConfig->readConfigInt("NetRaiseSmallBlindEveryHands"));
	spinBox_netRaiseSmallBlindEveryMinutes->setValue(myConfig->readConfigInt("NetRaiseSmallBlindEveryMinutes"));
	radioButton_netAlwaysDoubleBlinds->setChecked(myConfig->readConfigInt("NetAlwaysDoubleBlinds"));
	radioButton_netManualBlindsOrder->setChecked(myConfig->readConfigInt("NetManualBlindsOrder"));
	spinBox_netGameSpeed->setValue(myConfig->readConfigInt("NetGameSpeed"));
	spinBox_netTimeOutPlayerAction->setValue(myConfig->readConfigInt("NetTimeOutPlayerAction"));
	spinBox_serverPort->setValue(myConfig->readConfigInt("ServerPort"));
	lineEdit_serverPassword->setText(QString::fromUtf8(myConfig->readConfigString("ServerPassword").c_str()));
	checkBox_useIpv6->setChecked(myConfig->readConfigInt("ServerUseIpv6"));
	checkBox_useSctp->setChecked(myConfig->readConfigInt("ServerUseSctp"));

	//Internet Game Settings 
	if(myConfig->readConfigInt("InternetServerConfigMode")) { groupBox_manualServerConfig->setChecked(TRUE); }
	else { groupBox_automaticServerConfig->setChecked(TRUE); }
	lineEdit_InternetServerListAddress->setText(QString::fromUtf8(myConfig->readConfigString("InternetServerListAddress").c_str()));
	lineEdit_InternetServerAddress->setText(QString::fromUtf8(myConfig->readConfigString("InternetServerAddress").c_str()));
	spinBox_InternetServerPort->setValue(myConfig->readConfigInt("InternetServerPort"));
	lineEdit_InternetServerPassword->setText(QString::fromUtf8(myConfig->readConfigString("InternetServerPassword").c_str()));
	checkBox_InternetServerUseIpv6->setChecked(myConfig->readConfigInt("InternetServerUseIpv6"));
	checkBox_InternetServerUseSctp->setChecked(myConfig->readConfigInt("InternetServerUseSctp"));
	if(myConfig->readConfigInt("UseAvatarServer")) {
		lineEdit_avatarServerAddress->setText(QString::fromUtf8(myConfig->readConfigString("AvatarServerAddress").c_str()));
		checkBox_useAvatarServer->setCheckState(Qt::Checked);
		lineEdit_avatarServerAddress->setEnabled(TRUE);
	}
	else { 
		checkBox_useAvatarServer->setCheckState(Qt::Unchecked); 
		lineEdit_avatarServerAddress->setEnabled(FALSE);
	}	
	checkBox_UseInternetGamePassword->setChecked(myConfig->readConfigInt("UseInternetGamePassword"));
	if(myConfig->readConfigInt("UseInternetGamePassword")) {
		lineEdit_InternetGamePassword->setText(QString::fromUtf8(myConfig->readConfigString("InternetGamePassword").c_str()));
	}
	groupBox_lobbyChat->setChecked(myConfig->readConfigInt("UseIRCLobbyChat"));
	lineEdit_IRCServerAddress->setText(QString::fromUtf8(myConfig->readConfigString("IRCServerAddress").c_str()));
	spinBox_IRCServerPort->setValue(myConfig->readConfigInt("IRCServerPort"));
	lineEdit_IRCChannel->setText(QString::fromUtf8(myConfig->readConfigString("IRCChannel").c_str()));
	lineEdit_IRCChannelPassword->setText(QString::fromUtf8(myConfig->readConfigString("IRCChannelPassword").c_str()));
	checkBox_IRCServerUseIpv6->setChecked(myConfig->readConfigInt("IRCServerUseIpv6"));

	//Interface
	comboBox_switchLanguage->setCurrentIndex(comboBox_switchLanguage->findData(QString::fromUtf8(myConfig->readConfigString("Language").c_str()).section('_', 0, 0)));
	checkBox_showLeftToolbox->setChecked(myConfig->readConfigInt("ShowLeftToolBox"));
	checkBox_showRightToolbox->setChecked(myConfig->readConfigInt("ShowRightToolBox"));
	checkBox_showStatusbarMessages->setChecked(myConfig->readConfigInt("ShowStatusbarMessages"));
	checkBox_showIntro->setChecked(myConfig->readConfigInt("ShowIntro"));
	checkBox_showFadeOutCardsAnimation->setChecked(myConfig->readConfigInt("ShowFadeOutCardsAnimation"));
	checkBox_showFlipCardsAnimation->setChecked(myConfig->readConfigInt("ShowFlipCardsAnimation"));
	checkBox_showBlindButtons->setChecked(myConfig->readConfigInt("ShowBlindButtons"));
	checkBox_antiPeekMode->setChecked(myConfig->readConfigInt("AntiPeekMode"));
	checkBox_alternateFKeysUserActionMode->setChecked(myConfig->readConfigInt("AlternateFKeysUserActionMode"));
	checkBox_enableBetInputFocusSwitch->setChecked(myConfig->readConfigInt("EnableBetInputFocusSwitch"));
	checkBox_cardsChanceMonitor->setChecked(myConfig->readConfigInt("ShowCardsChanceMonitor"));

	radioButton_flipsideTux->setChecked(myConfig->readConfigInt("FlipsideTux"));
	radioButton_flipsideOwn->setChecked(myConfig->readConfigInt("FlipsideOwn"));
	if(radioButton_flipsideOwn->isChecked()) { 
		lineEdit_OwnFlipsideFilename->setEnabled(TRUE);
		pushButton_openFlipsidePicture->setEnabled(TRUE);
	}
	lineEdit_OwnFlipsideFilename->setText(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str()));

	//S t y l e
	//TABLE
// 	define PokerTH default GameTableStyle
	listWidget_gameTableStyles->clear();

	GameTableStyleReader defaultTableStyle(myConfig);
	defaultTableStyle.readStyleFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default/defaulttablestyle.xml");
	QListWidgetItem *defaultTableItem = new QListWidgetItem(defaultTableStyle.getStyleDescription(),listWidget_gameTableStyles); 
	defaultTableItem->setData(15, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default/defaulttablestyle.xml");
	
	//load secondary styles into list (if fallback no entry)
	myGameTableStylesList = myConfig->readConfigStringList("GameTableStylesList");
	list<std::string>::iterator it1;
	for(it1= myGameTableStylesList.begin(); it1 != myGameTableStylesList.end(); it1++) {
		GameTableStyleReader nextStyle(myConfig);
		nextStyle.readStyleFile(QString::fromUtf8(it1->c_str()));
		if(!nextStyle.getFallBack()) {
			QListWidgetItem *nextItem = new QListWidgetItem(nextStyle.getStyleDescription()); 
			nextItem->setData(15,QString::fromUtf8(it1->c_str()));
			listWidget_gameTableStyles->addItem(nextItem);
		}
	}	

	//set current Game Table Style from config file
	GameTableStyleReader currentGameTableStyle(myConfig);
	currentGameTableStyle.readStyleFile(QString::fromUtf8(myConfig->readConfigString("CurrentGameTableStyle").c_str()));
	int i;
	bool currentGameTableFound(FALSE);
	for(i=0; i < listWidget_gameTableStyles->count(); i++) {
		QListWidgetItem *item = listWidget_gameTableStyles->item(i);
		if(item->data(15) == currentGameTableStyle.getCurrentFileName()) {
			item->setIcon(QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
			listWidget_gameTableStyles->setCurrentItem(item);
			currentGameTableFound=TRUE;
		}
		else item->setIcon(QIcon());
	}
	if(!currentGameTableFound) {
		qDebug() << "Config ERROR: current game table style file not found in List. Mark default as selected.";
		QListWidgetItem *item = listWidget_gameTableStyles->item(0);
		item->setIcon(QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
		listWidget_gameTableStyles->setCurrentItem(item);
	}

	//refresh Game Table Style Preview
	showCurrentGameTableStylePreview();
	
	//CARDS
	//define PokerTH default CardDeck
	listWidget_cardDeckStyles->clear();
	CardDeckStyleReader defaultCardStyle(myConfig);
	defaultCardStyle.readStyleFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default/defaultdeckstyle.xml");
	QListWidgetItem *defaultCardItem = new QListWidgetItem(defaultCardStyle.getStyleDescription(),listWidget_cardDeckStyles); 
	defaultCardItem->setData(15, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default/defaultdeckstyle.xml");
	//define PokerTH default CardDeck4c
	CardDeckStyleReader default4cCardStyle(myConfig);
	default4cCardStyle.readStyleFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default4c/default4cdeckstyle.xml");
	QListWidgetItem *default4cCardItem = new QListWidgetItem(default4cCardStyle.getStyleDescription(),listWidget_cardDeckStyles); 
	default4cCardItem->setData(15, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default4c/default4cdeckstyle.xml");

	//load secondary card styles into list (if fallback no entry)
	myCardDeckStylesList = myConfig->readConfigStringList("CardDeckStylesList");
	list<std::string>::iterator it2;
	for(it2= myCardDeckStylesList.begin(); it2 != myCardDeckStylesList.end(); it2++) {
		CardDeckStyleReader nextStyle(myConfig);
		nextStyle.readStyleFile(QString::fromUtf8(it2->c_str()));
		if(!nextStyle.getFallBack()) {
			QListWidgetItem *nextItem = new QListWidgetItem(nextStyle.getStyleDescription()); 
			nextItem->setData(15,QString::fromUtf8(it2->c_str()));
			listWidget_cardDeckStyles->addItem(nextItem);
		}
	}	

	//set current card deck style from config file
	CardDeckStyleReader currentCardDeckStyle(myConfig);
	currentCardDeckStyle.readStyleFile(QString::fromUtf8(myConfig->readConfigString("CurrentCardDeckStyle").c_str()));
	int j;
	bool currentCardDeckFound(FALSE);
	for(j=0; j < listWidget_cardDeckStyles->count(); j++) {
		QListWidgetItem *item = listWidget_cardDeckStyles->item(j);
		if(item->data(15) == currentCardDeckStyle.getCurrentFileName()) {
			item->setIcon(QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
			listWidget_cardDeckStyles->setCurrentItem(item);
			currentCardDeckFound=TRUE;
		}
		else item->setIcon(QIcon());
	}
	if(!currentCardDeckFound) {
		qDebug() << "Config ERROR: current card deck style file not found in List. Mark default as selected.";
		QListWidgetItem *item = listWidget_cardDeckStyles->item(0);
		item->setIcon(QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
		listWidget_cardDeckStyles->setCurrentItem(item);
	}

// 	refresh Card Deck Style Preview
	showCurrentCardDeckStylePreview();

	//Sound
	groupBox_playSoundEffects->setChecked(myConfig->readConfigInt("PlaySoundEffects"));
	horizontalSlider_soundVolume->setValue(myConfig->readConfigInt("SoundVolume"));
	checkBox_playGameActions->setChecked(myConfig->readConfigInt("PlayGameActions"));
	checkBox_playLobbyChatNotification->setChecked(myConfig->readConfigInt("PlayLobbyChatNotification"));	
	checkBox_playNetworkGameNotification->setChecked(myConfig->readConfigInt("PlayNetworkGameNotification"));	

	//Log 
	groupBox_logOnOff->setChecked(myConfig->readConfigInt("LogOnOff"));
	lineEdit_logDir->setText(QString::fromUtf8(myConfig->readConfigString("LogDir").c_str()));
	spinBox_logStoreDuration->setValue(myConfig->readConfigInt("LogStoreDuration"));
	comboBox_logInterval->setCurrentIndex(myConfig->readConfigInt("LogInterval"));

	bool tmpHasIpv6 = socket_has_ipv6();
	bool tmpHasSctp = socket_has_sctp();
	checkBox_useIpv6->setEnabled(tmpHasIpv6);
	checkBox_useSctp->setEnabled(tmpHasSctp);
	checkBox_InternetServerUseIpv6->setEnabled(tmpHasIpv6);
	checkBox_InternetServerUseSctp->setEnabled(tmpHasSctp);
	checkBox_IRCServerUseIpv6->setEnabled(tmpHasIpv6);

	//Manual Blinds Order Dialog (local)
	myManualBlindsList = myConfig->readConfigIntList("ManualBlindsList");
	myAfterMBAlwaysDoubleBlinds = myConfig->readConfigInt("AfterMBAlwaysDoubleBlinds");
	myAfterMBAlwaysRaiseAbout = myConfig->readConfigInt("AfterMBAlwaysRaiseAbout");
	myAfterMBAlwaysRaiseValue = myConfig->readConfigInt("AfterMBAlwaysRaiseValue");
	myAfterMBStayAtLastBlind = myConfig->readConfigInt("AfterMBStayAtLastBlind");
	
	//Manual Blinds Order Dialog (network)
	myNetManualBlindsList = myConfig->readConfigIntList("NetManualBlindsList");
	myNetAfterMBAlwaysDoubleBlinds = myConfig->readConfigInt("NetAfterMBAlwaysDoubleBlinds");
	myNetAfterMBAlwaysRaiseAbout = myConfig->readConfigInt("NetAfterMBAlwaysRaiseAbout");
	myNetAfterMBAlwaysRaiseValue = myConfig->readConfigInt("NetAfterMBAlwaysRaiseValue");
	myNetAfterMBStayAtLastBlind = myConfig->readConfigInt("NetAfterMBStayAtLastBlind");

	setFirstSmallBlindMargin();	

	//set this AFTER switch combobox like config-settings. This IS a currentIndexChanged() ;-)
	languageIsChanged = FALSE;

	QDialog::exec();

}

void settingsDialogImpl::isAccepted() {

	settingsCorrect = TRUE;

// 	Player Nicks
	myConfig->writeConfigString("MyName", lineEdit_HumanPlayerName->text().trimmed().toUtf8().constData());
	myConfig->writeConfigString("MyAvatar", pushButton_HumanPlayerAvatar->getMyLink().toUtf8().constData());  
	
	myConfig->writeConfigString("Opponent1Name", lineEdit_Opponent1Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent1Avatar", pushButton_Opponent1Avatar->getMyLink().toUtf8().constData());  

	myConfig->writeConfigString("Opponent2Name", lineEdit_Opponent2Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent2Avatar", pushButton_Opponent2Avatar->getMyLink().toUtf8().constData());  
	
	myConfig->writeConfigString("Opponent3Name", lineEdit_Opponent3Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent3Avatar", pushButton_Opponent3Avatar->getMyLink().toUtf8().constData());  
	
	myConfig->writeConfigString("Opponent4Name", lineEdit_Opponent4Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent4Avatar", pushButton_Opponent4Avatar->getMyLink().toUtf8().constData());  
	
	myConfig->writeConfigString("Opponent5Name", lineEdit_Opponent5Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent5Avatar", pushButton_Opponent5Avatar->getMyLink().toUtf8().constData());  

	myConfig->writeConfigString("Opponent6Name", lineEdit_Opponent6Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent6Avatar", pushButton_Opponent6Avatar->getMyLink().toUtf8().constData());
	
	myConfig->writeConfigString("Opponent7Name", lineEdit_Opponent7Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent7Avatar", pushButton_Opponent7Avatar->getMyLink().toUtf8().constData());  

	myConfig->writeConfigString("Opponent8Name", lineEdit_Opponent8Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent8Avatar", pushButton_Opponent8Avatar->getMyLink().toUtf8().constData());  

	myConfig->writeConfigString("Opponent9Name", lineEdit_Opponent9Name->text().toUtf8().constData());
	myConfig->writeConfigString("Opponent9Avatar", pushButton_Opponent9Avatar->getMyLink().toUtf8().constData());    

// 	Local Game Settings
	myConfig->writeConfigInt("NumberOfPlayers", spinBox_quantityPlayers->value());
	myConfig->writeConfigInt("StartCash", spinBox_startCash->value());
	myConfig->writeConfigInt("FirstSmallBlind", spinBox_firstSmallBlind->value());
	myConfig->writeConfigInt("RaiseBlindsAtHands", radioButton_raiseBlindsAtHands->isChecked());
	myConfig->writeConfigInt("RaiseBlindsAtMinutes", radioButton_raiseBlindsAtMinutes->isChecked());
	myConfig->writeConfigInt("RaiseSmallBlindEveryHands", spinBox_raiseSmallBlindEveryHands->value());
	myConfig->writeConfigInt("RaiseSmallBlindEveryMinutes", spinBox_raiseSmallBlindEveryMinutes->value());
	myConfig->writeConfigInt("AlwaysDoubleBlinds", radioButton_alwaysDoubleBlinds->isChecked());
	myConfig->writeConfigInt("ManualBlindsOrder", radioButton_manualBlindsOrder->isChecked());
	myConfig->writeConfigInt("GameSpeed", spinBox_gameSpeed->value());
	myConfig->writeConfigInt("PauseBetweenHands", checkBox_pauseBetweenHands->isChecked());
	myConfig->writeConfigInt("ShowGameSettingsDialogOnNewGame", checkBox_showGameSettingsDialogOnNewGame->isChecked());

	//Network Game Settings
	myConfig->writeConfigInt("NetNumberOfPlayers", spinBox_netQuantityPlayers->value());
	myConfig->writeConfigInt("NetStartCash", spinBox_netStartCash->value());
	myConfig->writeConfigInt("NetFirstSmallBlind", spinBox_netFirstSmallBlind->value());
	myConfig->writeConfigInt("NetRaiseBlindsAtHands", radioButton_netRaiseBlindsAtHands->isChecked());
	myConfig->writeConfigInt("NetRaiseBlindsAtMinutes", radioButton_netRaiseBlindsAtMinutes->isChecked());
	myConfig->writeConfigInt("NetRaiseSmallBlindEveryHands", spinBox_netRaiseSmallBlindEveryHands->value());
	myConfig->writeConfigInt("NetRaiseSmallBlindEveryMinutes", spinBox_netRaiseSmallBlindEveryMinutes->value());
	myConfig->writeConfigInt("NetAlwaysDoubleBlinds", radioButton_netAlwaysDoubleBlinds->isChecked());
	myConfig->writeConfigInt("NetManualBlindsOrder", radioButton_netManualBlindsOrder->isChecked());
	myConfig->writeConfigInt("NetGameSpeed", spinBox_netGameSpeed->value());
	myConfig->writeConfigInt("NetTimeOutPlayerAction", spinBox_netTimeOutPlayerAction->value());
	myConfig->writeConfigInt("ServerPort", spinBox_serverPort->value());
	myConfig->writeConfigString("ServerPassword", lineEdit_serverPassword->text().toUtf8().constData());
	myConfig->writeConfigInt("ServerUseIpv6", checkBox_useIpv6->isChecked());
	myConfig->writeConfigInt("ServerUseSctp", checkBox_useSctp->isChecked());

	//Internet Game Settings
	if(groupBox_automaticServerConfig->isChecked()) { 

		myConfig->writeConfigInt("InternetServerConfigMode", 0); 

// 		QUrl serverList;
// 		serverList.setUrl(lineEdit_InternetServerListAddress->text(),QUrl::StrictMode);
// 		QRegExp rx("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?");
// 		if(lineEdit_InternetServerListAddress->text().contains(rx)) {		
		if(lineEdit_InternetServerListAddress->text().contains("/") && lineEdit_InternetServerListAddress->text().contains(".") ) {

			myConfig->writeConfigString("InternetServerListAddress", lineEdit_InternetServerListAddress->text().toUtf8().constData());
		}
		else {
			QMessageBox::warning(this, tr("Settings Error"),
				tr("The entered server list address is not a valid URL.\n"
				"Please enter a valid server list address!"),
				QMessageBox::Ok);
				settingsCorrect = FALSE; 
		}
	}
	if(groupBox_manualServerConfig->isChecked()) { 
		
		myConfig->writeConfigInt("InternetServerConfigMode", 1); 
		myConfig->writeConfigString("InternetServerAddress", lineEdit_InternetServerAddress->text().toUtf8().constData());
		myConfig->writeConfigInt("InternetServerPort", spinBox_InternetServerPort->value());
		myConfig->writeConfigString("InternetServerPassword", lineEdit_InternetServerPassword->text().toUtf8().constData());
	}
	if(checkBox_useAvatarServer->isChecked()) {
		myConfig->writeConfigInt("UseAvatarServer", 1);
		myConfig->writeConfigString("AvatarServerAddress", lineEdit_avatarServerAddress->text().toUtf8().constData());
	}
	else {
		myConfig->writeConfigInt("UseAvatarServer", 0);
	}

	myConfig->writeConfigInt("InternetServerUseIpv6", checkBox_InternetServerUseIpv6->isChecked());
	myConfig->writeConfigInt("InternetServerUseSctp", checkBox_InternetServerUseSctp->isChecked());
	myConfig->writeConfigInt("UseInternetGamePassword", checkBox_UseInternetGamePassword->isChecked());
	myConfig->writeConfigString("InternetGamePassword", lineEdit_InternetGamePassword->text().toUtf8().constData());
	myConfig->writeConfigInt("UseIRCLobbyChat", groupBox_lobbyChat->isChecked());
	myConfig->writeConfigString("IRCServerAddress", lineEdit_IRCServerAddress->text().toUtf8().constData());
	myConfig->writeConfigInt("IRCServerPort", spinBox_IRCServerPort->value());
	myConfig->writeConfigString("IRCChannel", lineEdit_IRCChannel->text().toUtf8().constData());
	myConfig->writeConfigString("IRCChannelPassword", lineEdit_IRCChannelPassword->text().toUtf8().constData());
	myConfig->writeConfigInt("IRCServerUseIpv6", checkBox_IRCServerUseIpv6->isChecked());
	
// 	Interface
	myConfig->writeConfigString("Language", comboBox_switchLanguage->itemData(comboBox_switchLanguage->currentIndex()).toString().toUtf8().constData());
	myConfig->writeConfigInt("ShowLeftToolBox", checkBox_showLeftToolbox->isChecked());
	myConfig->writeConfigInt("ShowRightToolBox", checkBox_showRightToolbox->isChecked());
	myConfig->writeConfigInt("ShowStatusbarMessages", checkBox_showStatusbarMessages->isChecked());	
	myConfig->writeConfigInt("ShowIntro", checkBox_showIntro->isChecked());
	myConfig->writeConfigInt("ShowFadeOutCardsAnimation", checkBox_showFadeOutCardsAnimation->isChecked());
	myConfig->writeConfigInt("ShowFlipCardsAnimation", checkBox_showFlipCardsAnimation->isChecked());
	myConfig->writeConfigInt("ShowBlindButtons", checkBox_showBlindButtons->isChecked());
	myConfig->writeConfigInt("ShowCardsChanceMonitor", checkBox_cardsChanceMonitor->isChecked());
	myConfig->writeConfigInt("AntiPeekMode", checkBox_antiPeekMode->isChecked());
	myConfig->writeConfigInt("AlternateFKeysUserActionMode", checkBox_alternateFKeysUserActionMode->isChecked());
	myConfig->writeConfigInt("EnableBetInputFocusSwitch", checkBox_enableBetInputFocusSwitch->isChecked());
	myConfig->writeConfigInt("FlipsideTux", radioButton_flipsideTux->isChecked());
	myConfig->writeConfigInt("FlipsideOwn", radioButton_flipsideOwn->isChecked());

	if(radioButton_flipsideOwn->isChecked()) {
		if(QFile::QFile(lineEdit_OwnFlipsideFilename->text()).exists() && lineEdit_OwnFlipsideFilename->text() != "") {myConfig->writeConfigString("FlipsideOwnFile", lineEdit_OwnFlipsideFilename->text().toUtf8().constData()); }
		else {	QMessageBox::warning(this, tr("Settings Error"),
			tr("The entered flipside picture doesn't exist.\n"
			"Please enter an valid picture!"),
			QMessageBox::Ok);
			settingsCorrect = FALSE; 
		}
	}

	//S T Y L E
// 	TABLE
	//save game table styles list
	myGameTableStylesList.clear();
	int i;
	for(i=POKERTH_DISTRIBUTED_GAME_TABLE_STYLES; i < listWidget_gameTableStyles->count(); i++) {
		QListWidgetItem *item = listWidget_gameTableStyles->item(i);
		myGameTableStylesList.push_back(item->data(15).toString().toUtf8().constData());
	}
	myConfig->writeConfigStringList("GameTableStylesList", myGameTableStylesList);	
	//save current game table style
	for(i=0; i < listWidget_gameTableStyles->count(); i++) {
		QListWidgetItem *item = listWidget_gameTableStyles->item(i);
		if(!item->icon().isNull()) 
			myConfig->writeConfigString("CurrentGameTableStyle", item->data(15).toString().toUtf8().constData());	
	}
// 	CARDS
	//save card deck styles list
	myCardDeckStylesList.clear();
	int j;
	for(j=POKERTH_DISTRIBUTED_CARD_DECK_STYLES; j < listWidget_cardDeckStyles->count(); j++) {
		QListWidgetItem *item = listWidget_cardDeckStyles->item(j);
		myCardDeckStylesList.push_back(item->data(15).toString().toUtf8().constData());
	}
	myConfig->writeConfigStringList("CardDeckStylesList", myCardDeckStylesList);	
	//save current card deck style
	for(j=0; j < listWidget_cardDeckStyles->count(); j++) {
		QListWidgetItem *item = listWidget_cardDeckStyles->item(j);
		if(!item->icon().isNull()) 
			myConfig->writeConfigString("CurrentCardDeckStyle", item->data(15).toString().toUtf8().constData());	
	}	

	//Sound
	myConfig->writeConfigInt("PlaySoundEffects", groupBox_playSoundEffects->isChecked());
	myConfig->writeConfigInt("SoundVolume", horizontalSlider_soundVolume->value());
	myConfig->writeConfigInt("PlayGameActions", checkBox_playGameActions->isChecked());	
	myConfig->writeConfigInt("PlayLobbyChatNotification", checkBox_playLobbyChatNotification->isChecked());	
	myConfig->writeConfigInt("PlayNetworkGameNotification", checkBox_playNetworkGameNotification->isChecked());	

//	Log
	myConfig->writeConfigInt("LogOnOff", groupBox_logOnOff->isChecked());
	if (myConfig->readConfigInt("LogOnOff")) {
	// if log On
		if(QDir::QDir(lineEdit_logDir->text()).exists() && lineEdit_logDir->text() != "") { myConfig->writeConfigString("LogDir", lineEdit_logDir->text().toUtf8().constData());	}
		else { 
			QMessageBox::warning(this, tr("Settings Error"),
			tr("The log file directory doesn't exist.\n"
			"Please select an valid directory!"),
			QMessageBox::Ok);
			settingsCorrect = FALSE; 
		}
	
		myConfig->writeConfigInt("LogStoreDuration", spinBox_logStoreDuration->value());
		myConfig->writeConfigInt("LogInterval", comboBox_logInterval->currentIndex());
	}

	//Manual Blinds Order Dialog (local)
	myConfig->writeConfigIntList("ManualBlindsList",myManualBlindsList);
	myConfig->writeConfigInt("AfterMBAlwaysDoubleBlinds",myAfterMBAlwaysDoubleBlinds);
	myConfig->writeConfigInt("AfterMBAlwaysRaiseAbout",myAfterMBAlwaysRaiseAbout);
	myConfig->writeConfigInt("AfterMBAlwaysRaiseValue",myAfterMBAlwaysRaiseValue);
	myConfig->writeConfigInt("AfterMBStayAtLastBlind",myAfterMBStayAtLastBlind);

	//Manual Blinds Order Dialog (network)
	myConfig->writeConfigIntList("NetManualBlindsList",myNetManualBlindsList);
	myConfig->writeConfigInt("NetAfterMBAlwaysDoubleBlinds",myNetAfterMBAlwaysDoubleBlinds);
	myConfig->writeConfigInt("NetAfterMBAlwaysRaiseAbout",myNetAfterMBAlwaysRaiseAbout);
	myConfig->writeConfigInt("NetAfterMBAlwaysRaiseValue",myNetAfterMBAlwaysRaiseValue);
	myConfig->writeConfigInt("NetAfterMBStayAtLastBlind",myNetAfterMBStayAtLastBlind);


	//write buffer to disc 
	myConfig->writeBuffer();

	if(languageIsChanged) {
		QMessageBox::information(this, tr("Language Changed"),
			tr("You have changed application language to %1.\nPlease restart PokerTH to load new language!").arg(comboBox_switchLanguage->itemText(changedLanguageIndex)),
			QMessageBox::Ok);
	}

	//Wenn alles richtig eingegeben wurde --> Dialog schließen
	if(settingsCorrect) { this->accept(); }
}

void settingsDialogImpl::setFlipsidePicFileName()
 {
	QDir flipSideDir(QDir::homePath());
	QFile flipSideFile(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str()));
	if(flipSideFile.exists()) {
		flipSideDir = QDir(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str()));
	}

 	QString fileName = QFileDialog::getOpenFileName(this, tr("Select your flipside picture"),
                                               flipSideDir.absolutePath(),
                                                tr("Images (*.png *.jpg *.gif)"));

     if (!fileName.isEmpty())
         lineEdit_OwnFlipsideFilename->setText(fileName);
 }

void settingsDialogImpl::setAvatarFile0() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_HumanPlayerAvatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_HumanPlayerAvatar->setIcon(QIcon(pushButton_HumanPlayerAvatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile1() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent1Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent1Avatar->setIcon(QIcon(pushButton_Opponent1Avatar->getMyLink()));
	}     	
}

void settingsDialogImpl::setAvatarFile2() {

	callSelectAvatarDialog();
	
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent2Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent2Avatar->setIcon(QIcon(pushButton_Opponent2Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile3() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent3Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent3Avatar->setIcon(QIcon(pushButton_Opponent3Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile4() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent4Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent4Avatar->setIcon(QIcon(pushButton_Opponent4Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile5() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent5Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent5Avatar->setIcon(QIcon(pushButton_Opponent5Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile6() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent6Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent6Avatar->setIcon(QIcon(pushButton_Opponent6Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile7() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent7Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent7Avatar->setIcon(QIcon(pushButton_Opponent7Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile8() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent8Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent8Avatar->setIcon(QIcon(pushButton_Opponent8Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile9() {

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent9Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent9Avatar->setIcon(QIcon(pushButton_Opponent9Avatar->getMyLink()));
	}
}
void settingsDialogImpl::setLogDir()
 {
	 QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 QDir::homePath(),
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
		
     	if (!dir.isEmpty()) lineEdit_logDir->setText(dir);
 }

void settingsDialogImpl::clearInternetGamePassword(bool clear) {

	if(!clear) { lineEdit_InternetGamePassword->clear(); }
}

void settingsDialogImpl::callManualBlindsOrderDialog() { 
	
	myManualBlindsOrderDialog->listWidget_blinds->clear();
	myManualBlindsOrderDialog->spinBox_input->setMinimum(spinBox_firstSmallBlind->value());

	list<int>::iterator it1;
	for(it1= myManualBlindsList.begin(); it1 != myManualBlindsList.end(); it1++) {
		myManualBlindsOrderDialog->listWidget_blinds->addItem(QString::number(*it1,10));
	}
	myManualBlindsOrderDialog->sortBlindsList();

	myManualBlindsOrderDialog->radioButton_alwaysDoubleBlinds->setChecked(myAfterMBAlwaysDoubleBlinds);
	myManualBlindsOrderDialog->radioButton_alwaysRaiseAbout->setChecked(myAfterMBAlwaysRaiseAbout);
	myManualBlindsOrderDialog->spinBox_alwaysRaiseValue->setValue(myAfterMBAlwaysRaiseValue);
	myManualBlindsOrderDialog->radioButton_stayAtLastBlind->setChecked(myAfterMBStayAtLastBlind);

	myManualBlindsOrderDialog->exec(); 
	if(myManualBlindsOrderDialog->result() == QDialog::Accepted) {
				
		bool ok = TRUE;
		int i;
		myManualBlindsList.clear();
		for(i=0; i<myManualBlindsOrderDialog->listWidget_blinds->count(); i++) {
			myManualBlindsList.push_back(myManualBlindsOrderDialog->listWidget_blinds->item(i)->text().toInt(&ok,10));		
		}

		myAfterMBAlwaysDoubleBlinds = myManualBlindsOrderDialog->radioButton_alwaysDoubleBlinds->isChecked();
		myAfterMBAlwaysRaiseAbout = myManualBlindsOrderDialog->radioButton_alwaysRaiseAbout->isChecked();
		myAfterMBAlwaysRaiseValue = myManualBlindsOrderDialog->spinBox_alwaysRaiseValue->value();
		myAfterMBStayAtLastBlind = myManualBlindsOrderDialog->radioButton_stayAtLastBlind->isChecked();

		setFirstSmallBlindMargin();
	}
}

void settingsDialogImpl::callNetManualBlindsOrderDialog() { 

	myManualBlindsOrderDialog->listWidget_blinds->clear();
	myManualBlindsOrderDialog->spinBox_input->setMinimum(spinBox_netFirstSmallBlind->value());

	list<int>::iterator it1;
	for(it1= myNetManualBlindsList.begin(); it1 != myNetManualBlindsList.end(); it1++) {
		myManualBlindsOrderDialog->listWidget_blinds->addItem(QString::number(*it1,10));
	}
	myManualBlindsOrderDialog->sortBlindsList();
	
	myManualBlindsOrderDialog->radioButton_alwaysDoubleBlinds->setChecked(myNetAfterMBAlwaysDoubleBlinds);
	myManualBlindsOrderDialog->radioButton_alwaysRaiseAbout->setChecked(myNetAfterMBAlwaysRaiseAbout);
	myManualBlindsOrderDialog->spinBox_alwaysRaiseValue->setValue(myNetAfterMBAlwaysRaiseValue);
	myManualBlindsOrderDialog->radioButton_stayAtLastBlind->setChecked(myNetAfterMBStayAtLastBlind);

	myManualBlindsOrderDialog->exec(); 
	if(myManualBlindsOrderDialog->result() == QDialog::Accepted) {

		bool ok = TRUE;
		int i;
		myNetManualBlindsList.clear();
		for(i=0; i<myManualBlindsOrderDialog->listWidget_blinds->count(); i++) {
			myNetManualBlindsList.push_back(myManualBlindsOrderDialog->listWidget_blinds->item(i)->text().toInt(&ok,10));		
		}
		
		myNetAfterMBAlwaysDoubleBlinds = myManualBlindsOrderDialog->radioButton_alwaysDoubleBlinds->isChecked();
		myNetAfterMBAlwaysRaiseAbout = 	myManualBlindsOrderDialog->radioButton_alwaysRaiseAbout->isChecked();
		myNetAfterMBAlwaysRaiseValue = myManualBlindsOrderDialog->spinBox_alwaysRaiseValue->value();
		myNetAfterMBStayAtLastBlind = myManualBlindsOrderDialog->radioButton_stayAtLastBlind->isChecked();

		setFirstSmallBlindMargin();
	}
}


// void settingsDialogImpl::checkProperNetFirstSmallBlind(int currentSB) {
// 
// }
// 
// 
// void settingsDialogImpl::checkProperFirstSmallBlind(int currentSB) {
// 
// }

void settingsDialogImpl::setFirstSmallBlindMargin() {

	if(radioButton_manualBlindsOrder->isChecked() && !myManualBlindsList.empty()) {
		if(spinBox_firstSmallBlind->value() > myManualBlindsList.front()) {
			QMessageBox::warning(this, tr("Blinds Error"),
				tr("The first element in your manual-blinds-list \nis smaller than current first-small-blind!\nThis first-small-blind-value will be set to maximum allowed value."),
				QMessageBox::Close);
		}
		spinBox_firstSmallBlind->setMaximum(myManualBlindsList.front());
	}
	else {
		spinBox_firstSmallBlind->setMaximum(9999);
	}
	if(radioButton_netManualBlindsOrder->isChecked() && !myNetManualBlindsList.empty()) {
		if(spinBox_netFirstSmallBlind->value() > myNetManualBlindsList.front()) {
			QMessageBox::warning(this, tr("Blinds Error"),
				tr("The first element in your manual-blinds-list \nis smaller than current first-small-blind!\nThis first-small-blind-value will be set to maximum allowed value."),
				QMessageBox::Close);
		}
		spinBox_netFirstSmallBlind->setMaximum(myNetManualBlindsList.front());
	}	
	else {
		spinBox_netFirstSmallBlind->setMaximum(9999);
	}
	

}

void settingsDialogImpl::setLanguageChanged(int index) {

	languageIsChanged = TRUE;
	changedLanguageIndex = index;
	
}

void settingsDialogImpl::toggleGroupBoxAutomaticServerConfig(bool toggleState) { 
	if(toggleState) groupBox_manualServerConfig->setChecked(FALSE); 
	else groupBox_manualServerConfig->setChecked(TRUE); 
}

void settingsDialogImpl::toggleGroupBoxManualServerConfig(bool toggleState) { 
	if(toggleState) groupBox_automaticServerConfig->setChecked(FALSE); 
	else groupBox_automaticServerConfig->setChecked(TRUE); 
}

void settingsDialogImpl::showCurrentGameTableStylePreview()
{
	QListWidgetItem* item = listWidget_gameTableStyles->currentItem();
	if(item) {
		GameTableStyleReader style(myConfig);
		style.readStyleFile(item->data(15).toString());
		QPixmap preview(style.getPreview());
		if(preview.height() > 160 || preview.width() > 120) label_gameTableStylePreview->setPixmap(preview.scaled(160,120,Qt::KeepAspectRatio,Qt::SmoothTransformation));
		else label_gameTableStylePreview->setPixmap(preview); 
	}
}

void settingsDialogImpl::setSelectedGameTableStyleActivated()
{
	QListWidgetItem* selectedItem = listWidget_gameTableStyles->currentItem();
	if(selectedItem) {
		int i;
		for(i=0; i < listWidget_gameTableStyles->count(); i++) {
			QListWidgetItem *item = listWidget_gameTableStyles->item(i);
			if(item == selectedItem) {
				item->setIcon(QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
			}
			else item->setIcon(QIcon());
		}
	}
}

void settingsDialogImpl::addGameTableStyle()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Please select your game table style"),
                                               QDir::home().absolutePath(),
                                                tr("PokerTH game table styles (*.xml)"));

     	if (!fileName.isEmpty()) {

		int i;
		bool fileNameAlreadyFound(FALSE);
		for(i=0; i < listWidget_gameTableStyles->count(); i++) {
			QListWidgetItem *item = listWidget_gameTableStyles->item(i);
			if(item->data(15).toString() == fileName) 
				fileNameAlreadyFound = TRUE;
		}		

		if(fileNameAlreadyFound) {
			QMessageBox::warning(this, tr("Game Table Style Error"),
					tr("Selected game table style file is already in the list. \nPlease select another one to add!"),
					QMessageBox::Ok);	
		}
		else {
			GameTableStyleReader newStyle(myConfig);
			newStyle.readStyleFile(fileName);
			if(!newStyle.getFallBack()) {
				QListWidgetItem *newItem = new QListWidgetItem(newStyle.getStyleDescription()); 
				newItem->setData(15,fileName);
				listWidget_gameTableStyles->addItem(newItem);
			}
			else {
				QMessageBox::warning(this, tr("Game Table Style File Error"),
					tr("Could not read game table style file. \nStyle will not be placed into list!"),
					QMessageBox::Ok);	
			}	
		}
	}	
}

void settingsDialogImpl::removeGameTableStyle()
{
	//never delete PokerTH defaullt Styles
	if(listWidget_gameTableStyles->currentRow() >= POKERTH_DISTRIBUTED_GAME_TABLE_STYLES) {

		QListWidgetItem* selectedItem = listWidget_gameTableStyles->currentItem();
		if(selectedItem) {
			// if selected is activated --> swith activation to first default
			if(!selectedItem->icon().isNull()) { 
				QListWidgetItem* firstItem = listWidget_gameTableStyles->item(0);
				firstItem->setIcon(QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
			}
			//remove from List
			listWidget_gameTableStyles->takeItem(listWidget_gameTableStyles->currentRow());
		}
	}
}

void settingsDialogImpl::showCurrentCardDeckStylePreview()
{
	QListWidgetItem* item = listWidget_cardDeckStyles->currentItem();
	if(item) {
		CardDeckStyleReader style(myConfig);
		style.readStyleFile(item->data(15).toString());
		QPixmap preview(style.getPreview());
		if(preview.height() > 160 || preview.width() > 120) label_cardDeckStylePreview->setPixmap(preview.scaled(160,120,Qt::KeepAspectRatio,Qt::SmoothTransformation));
		else label_cardDeckStylePreview->setPixmap(preview); 
	}
}

void settingsDialogImpl::setSelectedCardDeckStyleActivated()
{
	QListWidgetItem* selectedItem = listWidget_cardDeckStyles->currentItem();
	if(selectedItem) {
		int i;
		for(i=0; i < listWidget_cardDeckStyles->count(); i++) {
			QListWidgetItem *item = listWidget_cardDeckStyles->item(i);
			if(item == selectedItem) {
				item->setIcon(QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
			}
			else item->setIcon(QIcon());
		}
	}
}

void settingsDialogImpl::addCardDeckStyle()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Please select your card deck style"),
                                               QDir::home().absolutePath(),
                                                tr("PokerTH card deck styles (*.xml)"));

     	if (!fileName.isEmpty()) {

		int i;
		bool fileNameAlreadyFound(FALSE);
		for(i=0; i < listWidget_cardDeckStyles->count(); i++) {
			QListWidgetItem *item = listWidget_cardDeckStyles->item(i);
			if(item->data(15).toString() == fileName)
				fileNameAlreadyFound = TRUE;
		}		

		if(fileNameAlreadyFound) {
			QMessageBox::warning(this, tr("Card Deck Style Error"),
					tr("Selected card deck style file is already in the list. \nPlease select another one to add!"),
					QMessageBox::Ok);	
		}
		else {
			CardDeckStyleReader newStyle(myConfig);
			newStyle.readStyleFile(fileName);
			if(!newStyle.getFallBack()) {
				QListWidgetItem *newItem = new QListWidgetItem(newStyle.getStyleDescription()); 
				newItem->setData(15,fileName);
				listWidget_cardDeckStyles->addItem(newItem);
			}
			else {
				QMessageBox::warning(this, tr("Card Deck Style File Error"),
					tr("Could not read card deck style file. \nStyle will not be placed into list!"),
					QMessageBox::Ok);	
			}	
		}
	}	
}

void settingsDialogImpl::removeCardDeckStyle()
{
	//never delete PokerTH defaullt Styles
	if(listWidget_cardDeckStyles->currentRow() >= POKERTH_DISTRIBUTED_CARD_DECK_STYLES) {

		QListWidgetItem* selectedItem = listWidget_cardDeckStyles->currentItem();
		if(selectedItem) {
			// if selected is activated --> swith activation to first default
			if(!selectedItem->icon().isNull()) { 
				QListWidgetItem* firstItem = listWidget_cardDeckStyles->item(0);
				firstItem->setIcon(QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
			}
			//remove from List
			listWidget_cardDeckStyles->takeItem(listWidget_cardDeckStyles->currentRow());
		}
	}
}
