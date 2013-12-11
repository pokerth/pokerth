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
#include "settingsdialogimpl.h"
#include "myavatarlistitem.h"
#include "mystylelistitem.h"
#include "gametablestylereader.h"
#include "carddeckstylereader.h"
#include "configfile.h"
#include <net/socket_startup.h>
#include <QSet>
enum StyleType { POKERTH_DISTRIBUTED_STYLE, ADDITIONAL_STYLE };

using namespace std;

settingsDialogImpl::settingsDialogImpl(QWidget *parent, ConfigFile *c, selectAvatarDialogImpl *s)
	: QDialog(parent), myConfig(c), mySelectAvatarDialogImpl(s)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	setupUi(this);

#ifdef ANDROID
	stackedWidget->removeWidget(page_styles);
	listWidget->takeItem(1);
	label_soundvol->hide();
	label_soundVolume->hide();
	horizontalSlider_soundVolume->hide();
	this->setWindowState(Qt::WindowFullScreen);
#endif

	myManualBlindsOrderDialog = new manualBlindsOrderDialogImpl;

	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());

	pushButton_openFlipsidePicture->setIcon(QIcon(QPixmap(myAppDataPath+"gfx/gui/misc/fileopen16.png")));
	pushButton_openLogDir->setIcon(QIcon(QPixmap(myAppDataPath+"gfx/gui/misc/fileopen16.png")));

	if (myConfig->readConfigInt("CLA_NoWriteAccess")) {
		groupBox_logOnOff->setDisabled(true);
	}

	comboBox_switchLanguage->addItem(QString(tr("Afrikaans")+" "+QString::fromUtf8("(Afrikaans)")),"af");
	comboBox_switchLanguage->addItem(QString(tr("Bulgarian")+" "+QString::fromUtf8("(Български)")),"bg");
	comboBox_switchLanguage->addItem(QString(tr("Catalan")+" "+QString::fromUtf8("(Català)")),"ca");
	comboBox_switchLanguage->addItem(QString(tr("Chinese")+" "+QString::fromUtf8("(简体中文)")),"zhcn");
	comboBox_switchLanguage->addItem(QString(tr("Czech")+" "+QString::fromUtf8("(čeština)")),"cz");
	comboBox_switchLanguage->addItem(QString(tr("Danish")+" "+QString::fromUtf8("(Dansk)")),"dk");
	comboBox_switchLanguage->addItem(QString(tr("Dutch")+" "+QString::fromUtf8("(Nederlands)")),"nl");
	comboBox_switchLanguage->addItem(QString(tr("English")+" "+QString::fromUtf8("(English)")),"en");
	comboBox_switchLanguage->addItem(QString(tr("Finnish")+" "+QString::fromUtf8("(suomi)")),"fi");
	comboBox_switchLanguage->addItem(QString(tr("French")+" "+QString::fromUtf8("(français)")),"fr");
	comboBox_switchLanguage->addItem(QString(tr("Galician")+" "+QString::fromUtf8("(Galego)")),"gl");
	comboBox_switchLanguage->addItem(QString(tr("German")+" "+QString::fromUtf8("(Deutsch)")),"de");
	comboBox_switchLanguage->addItem(QString(tr("Greek")+" "+QString::fromUtf8("(Ελληνικά)")),"gr");
	comboBox_switchLanguage->addItem(QString(tr("Hungarian")+" "+QString::fromUtf8("(Magyar)")),"hu");
	comboBox_switchLanguage->addItem(QString(tr("Italian")+" "+QString::fromUtf8("(italiano)")),"it");
	comboBox_switchLanguage->addItem(QString(tr("Japanese")+" "+QString::fromUtf8("(日本語)")),"jp");
	comboBox_switchLanguage->addItem(QString(tr("Lithuania")+" "+QString::fromUtf8("(Lietuviškai)")),"lt");
	comboBox_switchLanguage->addItem(QString(tr("Norwegian")+" "+QString::fromUtf8("(Norsk)")),"no");
	comboBox_switchLanguage->addItem(QString(tr("Polish")+" "+QString::fromUtf8("(polski)")),"pl");
	comboBox_switchLanguage->addItem(QString(tr("Portuguese-Brazilian")+" "+QString::fromUtf8("(português brasileiro)")),"ptbr");
	comboBox_switchLanguage->addItem(QString(tr("Portuguese-Portuguese")+" "+QString::fromUtf8("(português português)")),"ptpt");
	comboBox_switchLanguage->addItem(QString(tr("Russian")+" "+QString::fromUtf8("(Pyccĸий)")),"ru");
	comboBox_switchLanguage->addItem(QString(tr("Scottish Gaelic")+" "+QString::fromUtf8("(Gàidhlig)")),"gd");
	comboBox_switchLanguage->addItem(QString(tr("Slovak")+" "+QString::fromUtf8("(Slovenčina)")),"sk");
	comboBox_switchLanguage->addItem(QString(tr("Spanish")+" "+QString::fromUtf8("(Español)")),"es");
	comboBox_switchLanguage->addItem(QString(tr("Swedish")+" "+QString::fromUtf8("(svenska)")),"sv");
	comboBox_switchLanguage->addItem(QString(tr("Tamil")+" "+QString::fromUtf8("(தமிழ்)")),"ta");
	comboBox_switchLanguage->addItem(QString(tr("Turkish")+" "+QString::fromUtf8("(Tϋrkçe)")),"tr");
	comboBox_switchLanguage->addItem(QString(tr("Vietnamese")+" "+QString::fromUtf8("(Tiếng Việt)")),"vi");

#ifdef GUI_800x480
	connect( pushButton_ok, SIGNAL( clicked() ), this, SLOT( isAccepted() ) );
#else
	connect( buttonBox, SIGNAL( accepted() ), this, SLOT( isAccepted() ) );
#endif

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
	connect( pushButton_resetSettings, SIGNAL(clicked()), this, SLOT(resetSettings()));

	connect( checkBox_UseInternetGamePassword, SIGNAL( toggled(bool) ), this, SLOT( clearInternetGamePassword(bool)) );

	// 	connect( spinBox_netFirstSmallBlind, SIGNAL( valueChanged(int) ), this, SLOT ( checkProperNetFirstSmallBlind(int)));
	// 	connect( spinBox_firstSmallBlind, SIGNAL( valueChanged(int) ), this, SLOT ( checkProperFirstSmallBlind(int)));

	connect( radioButton_netManualBlindsOrder, SIGNAL( toggled(bool) ), this, SLOT( setFirstSmallBlindMargin() ));
	connect( radioButton_manualBlindsOrder, SIGNAL( toggled(bool) ), this, SLOT( setFirstSmallBlindMargin() ));

	connect( comboBox_switchLanguage, SIGNAL( currentIndexChanged (int) ), this, SLOT ( setLanguageChanged(int) ));

	connect(groupBox_automaticServerConfig, SIGNAL(toggled(bool)), this, SLOT(toggleGroupBoxAutomaticServerConfig(bool)));
	connect(groupBox_manualServerConfig, SIGNAL(toggled(bool)), this, SLOT(toggleGroupBoxManualServerConfig(bool)));

	connect( treeWidget_gameTableStyles, SIGNAL( currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*) ), this, SLOT ( showCurrentGameTableStylePreview() ));
	connect( pushButton_addGameTableStyle, SIGNAL( clicked() ), this, SLOT( addGameTableStyle()) );
	connect( pushButton_removeGameTableStyle, SIGNAL( clicked() ), this, SLOT( removeGameTableStyle()) );

	connect( treeWidget_cardDeckStyles, SIGNAL( currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*) ), this, SLOT ( showCurrentCardDeckStylePreview() ));
	connect( pushButton_addCardDeckStyle, SIGNAL( clicked() ), this, SLOT( addCardDeckStyle()) );
	connect( pushButton_removeCardDeckStyle, SIGNAL( clicked() ), this, SLOT( removeCardDeckStyle()) );
	connect( pushButton_internetGameRemoveIgnoredPlayer, SIGNAL( clicked()), this, SLOT( removePlayerFromIgnoredPlayersList()));

#ifdef GUI_800x480
	//make the scrollbar touchable for mobile guis
	comboBox_switchLanguage->setStyleSheet("QObject {font: 30px} QScrollBar:vertical { border: 1px solid grey; background: white; width: 60px; margin: 0px -1px 0px 0px; } QScrollBar::handle:vertical { border-radius: 3px; border: 2px solid grey; background: LightGrey ; min-height: 60px; } QScrollBar::add-line:vertical { background: none; } QScrollBar::sub-line:vertical { background: none; } QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical { background: none; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
#endif

}

settingsDialogImpl::~settingsDialogImpl()
{
}

void settingsDialogImpl::prepareDialog()
{
	playerNickIsChanged = false;

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
	//disable player nicks page if the dialogs is called ingame to prevent changes during game
	this->page_3->setDisabled(calledIngame);

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
	spinBox_netDelayBetweenHands->setValue(myConfig->readConfigInt("NetDelayBetweenHands"));
	spinBox_netTimeOutPlayerAction->setValue(myConfig->readConfigInt("NetTimeOutPlayerAction"));
	spinBox_serverPort->setValue(myConfig->readConfigInt("ServerPort"));
	checkBox_useIpv6->setChecked(myConfig->readConfigInt("ServerUseIpv6"));
	checkBox_useSctp->setChecked(myConfig->readConfigInt("ServerUseSctp"));

	//Internet Game Settings

	checkBox_InternetServerUseSctp->hide(); //temporarely disabled until sctp support is back
	label_whatIsSctp->hide();

	lineEdit_InternetServerListAddress->setText(QString::fromUtf8(myConfig->readConfigString("InternetServerListAddress").c_str()));
	lineEdit_InternetServerAddress->setText(QString::fromUtf8(myConfig->readConfigString("InternetServerAddress").c_str()));
	lineEdit_InternetServerPassword->setText(QString::fromUtf8(myConfig->readConfigString("ServerPassword").c_str()));
	spinBox_InternetServerPort->setValue(myConfig->readConfigInt("InternetServerPort"));
	checkBox_InternetServerUseIpv6->setChecked(myConfig->readConfigInt("InternetServerUseIpv6"));
	checkBox_InternetServerUseSctp->setChecked(myConfig->readConfigInt("InternetServerUseSctp"));
	if(myConfig->readConfigInt("UseAvatarServer")) {
		lineEdit_avatarServerAddress->setText(QString::fromUtf8(myConfig->readConfigString("AvatarServerAddress").c_str()));
		checkBox_useAvatarServer->setCheckState(Qt::Checked);
		lineEdit_avatarServerAddress->setEnabled(true);
	} else {
		checkBox_useAvatarServer->setCheckState(Qt::Unchecked);
		lineEdit_avatarServerAddress->setEnabled(false);
	}
	checkBox_UseInternetGamePassword->setChecked(myConfig->readConfigInt("UseInternetGamePassword"));
	if(myConfig->readConfigInt("UseInternetGamePassword")) {
		lineEdit_InternetGamePassword->setText(QString::fromUtf8(myConfig->readConfigString("InternetGamePassword").c_str()));
	}
	checkBox_UseLobbyChat->setChecked(myConfig->readConfigInt("UseLobbyChat"));
	checkBox_InetGame_AutoLeaveTheTableAfterGameFinished->setChecked(myConfig->readConfigInt("NetAutoLeaveGameAfterFinish"));
	comboBox_internetGameType->setCurrentIndex(myConfig->readConfigInt("InternetGameType"));
	lineEdit_internetGameName->setText(QString::fromUtf8(myConfig->readConfigString("InternetGameName").c_str()));
	if(myConfig->readConfigInt("InternetServerConfigMode")) {
		groupBox_manualServerConfig->setChecked(true);
	} else {
		groupBox_automaticServerConfig->setChecked(true);
	}

	std::list<std::string> playerIgnoreList = myConfig->readConfigStringList("PlayerIgnoreList");
	std::list<std::string>::iterator it5;
	treeWidget_internetGameIgnoredPlayers->clear();
	for(it5= playerIgnoreList.begin(); it5 != playerIgnoreList.end(); ++it5) {
		QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget_internetGameIgnoredPlayers);
		item->setText(0, QString::fromUtf8(it5->c_str()));
	}
	checkBox_allowSpectators->setChecked(myConfig->readConfigInt("InternetGameAllowSpectators"));

	//Interface
	comboBox_switchLanguage->setCurrentIndex(comboBox_switchLanguage->findData(QString::fromUtf8(myConfig->readConfigString("Language").c_str()).section('_', 0, 0)));
#ifndef GUI_800x480
	checkBox_showLeftToolbox->setChecked(myConfig->readConfigInt("ShowLeftToolBox"));
	checkBox_showRightToolbox->setChecked(myConfig->readConfigInt("ShowRightToolBox"));
	checkBox_alternateFKeysUserActionMode->setChecked(myConfig->readConfigInt("AlternateFKeysUserActionMode"));
#endif
	checkBox_showFadeOutCardsAnimation->setChecked(myConfig->readConfigInt("ShowFadeOutCardsAnimation"));
	checkBox_showFlipCardsAnimation->setChecked(myConfig->readConfigInt("ShowFlipCardsAnimation"));
	checkBox_showBlindButtons->setChecked(myConfig->readConfigInt("ShowBlindButtons"));
	checkBox_showCountryFlagInAvatar->setChecked(myConfig->readConfigInt("ShowCountryFlagInAvatar"));
	checkBox_showPingStateInAvatar->setChecked(myConfig->readConfigInt("ShowPingStateInAvatar"));
	checkBox_antiPeekMode->setChecked(myConfig->readConfigInt("AntiPeekMode"));
	checkBox_enableBetInputFocusSwitch->setChecked(myConfig->readConfigInt("EnableBetInputFocusSwitch"));
	checkBox_cardsChanceMonitor->setChecked(myConfig->readConfigInt("ShowCardsChanceMonitor"));
	checkBox_dontTranslatePokerStrings->setChecked(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle"));
	checkBox_disableSplashscreen->setChecked(myConfig->readConfigInt("DisableSplashScreenOnStartup"));
	checkBox_enableAccidentallyCallBlocker->setChecked(myConfig->readConfigInt("AccidentallyCallBlocker"));
	checkBox_dontHideAvatarsOfIgnored->setChecked(myConfig->readConfigInt("DontHideAvatarsOfIgnored"));
	checkBox_disableChatEmoticons->setChecked(myConfig->readConfigInt("DisableChatEmoticons"));


	//S t y l e
	//TABLE

#ifdef GUI_800x480
	// 	define PokerTH default GameTableStyle for Maemo
	treeWidget_gameTableStyles->clear();
	QString filename;
#ifdef MAEMO
	filename = "defaulttablestyle_800x480.xml";
#elif ANDROID
	filename = "android_tablestyle_800x480.xml";
#endif
	GameTableStyleReader defaultTableStyle(myConfig, this);
	defaultTableStyle.readStyleFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default_800x480/"+filename);
	if(defaultTableStyle.getLoadedSuccessfull()) {
		QStringList tempStringList1;
		tempStringList1 << defaultTableStyle.getStyleDescription() << defaultTableStyle.getStyleMaintainerName();
		MyStyleListItem *defaultTableItem = new MyStyleListItem(tempStringList1, treeWidget_gameTableStyles);
		defaultTableItem->setData(0, 15, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default_800x480/"+filename);
		defaultTableItem->setData(0, 16, POKERTH_DISTRIBUTED_STYLE);
		defaultTableItem->setData(0, Qt::ToolTipRole, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default_800x480/"+filename);
		defaultTableItem->setData(2, Qt::ToolTipRole, defaultTableStyle.getMyStateToolTipInfo());
		if(defaultTableStyle.getState()) defaultTableItem->setIcon(2, QIcon(":/gfx/emblem-important.png"));
		else defaultTableItem->setIcon(2, QIcon(":/gfx/dialog_ok_apply.png"));
	}
#else
	// 	define PokerTH default GameTableStyle
	treeWidget_gameTableStyles->clear();

	GameTableStyleReader defaultTableStyle(myConfig, this);
	defaultTableStyle.readStyleFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default/defaulttablestyle.xml");
	if(defaultTableStyle.getLoadedSuccessfull()) {
		QStringList tempStringList1;
		tempStringList1 << defaultTableStyle.getStyleDescription() << defaultTableStyle.getStyleMaintainerName();
		MyStyleListItem *defaultTableItem = new MyStyleListItem(tempStringList1, treeWidget_gameTableStyles);
		defaultTableItem->setData(0, 15, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default/defaulttablestyle.xml");
		defaultTableItem->setData(0, 16, POKERTH_DISTRIBUTED_STYLE);
		defaultTableItem->setData(0, Qt::ToolTipRole, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/default/defaulttablestyle.xml");
		defaultTableItem->setData(2, Qt::ToolTipRole, defaultTableStyle.getMyStateToolTipInfo());
		if(defaultTableStyle.getState()) defaultTableItem->setIcon(2, QIcon(":/gfx/emblem-important.png"));
		else defaultTableItem->setIcon(2, QIcon(":/gfx/dialog_ok_apply.png"));
	}
	//add danuxi table
	GameTableStyleReader danuxi1TableStyle(myConfig, this);
	danuxi1TableStyle.readStyleFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/danuxi1/danuxi1tablestyle.xml");
	if(danuxi1TableStyle.getLoadedSuccessfull()) {
		QStringList tempStringList2;
		tempStringList2 << danuxi1TableStyle.getStyleDescription() << danuxi1TableStyle.getStyleMaintainerName();
		MyStyleListItem *danuxi1TableItem = new MyStyleListItem(tempStringList2, treeWidget_gameTableStyles);
		danuxi1TableItem->setData(0, 15, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/danuxi1/danuxi1tablestyle.xml");
		danuxi1TableItem->setData(0, 16, POKERTH_DISTRIBUTED_STYLE);
		danuxi1TableItem->setData(0, Qt::ToolTipRole, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/gui/table/danuxi1/danuxi1tablestyle.xml");
		danuxi1TableItem->setData(2, Qt::ToolTipRole, danuxi1TableStyle.getMyStateToolTipInfo());
		if(danuxi1TableStyle.getState()) danuxi1TableItem->setIcon(2, QIcon(":/gfx/emblem-important.png"));
		else danuxi1TableItem->setIcon(2, QIcon(":/gfx/dialog_ok_apply.png"));
	}
#endif

	//load secondary styles into list (if fallback no entry)
	myGameTableStylesList = myConfig->readConfigStringList("GameTableStylesList");
	list<std::string>::iterator it1;
	for(it1= myGameTableStylesList.begin(); it1 != myGameTableStylesList.end(); ++it1) {
		GameTableStyleReader nextStyle(myConfig, this);
		nextStyle.readStyleFile(QString::fromUtf8(it1->c_str()));
		if(!nextStyle.getFallBack() && nextStyle.getLoadedSuccessfull()) {
			QStringList tempStringList1;
			tempStringList1 << nextStyle.getStyleDescription() << nextStyle.getStyleMaintainerName();
			MyStyleListItem *nextItem = new MyStyleListItem(tempStringList1, treeWidget_gameTableStyles);
			nextItem->setData(0, 15,QString::fromUtf8(it1->c_str()));
			nextItem->setData(0, 16, ADDITIONAL_STYLE);
			nextItem->setData(0, Qt::ToolTipRole,QString::fromUtf8(it1->c_str()));
			nextItem->setData(2, Qt::ToolTipRole, nextStyle.getMyStateToolTipInfo());
			if(nextStyle.getState()) nextItem->setIcon(2, QIcon(":/gfx/emblem-important.png"));
			else nextItem->setIcon(2, QIcon(":/gfx/dialog_ok_apply.png"));
			treeWidget_gameTableStyles->addTopLevelItem(nextItem);
		}
	}
	treeWidget_gameTableStyles->sortItems(0, Qt::AscendingOrder);

	//set current Game Table Style from config file or fallback to first entry
	GameTableStyleReader currentGameTableStyle(myConfig, this);
	currentGameTableStyle.readStyleFile(QString::fromUtf8(myConfig->readConfigString("CurrentGameTableStyle").c_str()));
	if(currentGameTableStyle.getLoadedSuccessfull()) {
		int i;
		bool currentGameTableFound(false);
		for(i=0; i < treeWidget_gameTableStyles->topLevelItemCount(); i++) {
			QTreeWidgetItem *item = treeWidget_gameTableStyles->topLevelItem(i);
			if(item->data(0, 15) == currentGameTableStyle.getCurrentFileName()) {
				item->setIcon(0, QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
				treeWidget_gameTableStyles->setCurrentItem(item);
				currentGameTableFound=true;
			} else item->setIcon(0, QIcon());
		}
		if(!currentGameTableFound) {
			qDebug() << "Config ERROR: current game table style file not found in List. Try to mark default as selected.";
			QTreeWidgetItem *item = treeWidget_gameTableStyles->topLevelItem(0);
			if(item) {
				item->setIcon(0, QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
				treeWidget_gameTableStyles->setCurrentItem(item);
			}
		}
	} else {
		qDebug() << "Config ERROR: current game table style file could not be loaded. Try to mark default as selected.";
		QTreeWidgetItem *item = treeWidget_gameTableStyles->topLevelItem(0);
		if(item) {
			item->setIcon(0, QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
			treeWidget_gameTableStyles->setCurrentItem(item);
		}
	}

	//refresh Game Table Style Preview
	showCurrentGameTableStylePreview();

	//CARDS
	// 	define PokerTH 1.0 default carddeck
	treeWidget_cardDeckStyles->clear();
	CardDeckStyleReader defaultCardStyle10(myConfig, this);
	defaultCardStyle10.readStyleFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default_800x480/defaultdeckstyle_800x480.xml");
	if(defaultCardStyle10.getLoadedSuccessfull()) {
		QStringList tempStringList1;
		tempStringList1 << defaultCardStyle10.getStyleDescription() << defaultCardStyle10.getStyleMaintainerName();
		MyStyleListItem *defaultCardItem = new MyStyleListItem(tempStringList1, treeWidget_cardDeckStyles);
		defaultCardItem->setData(0, 15, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default_800x480/defaultdeckstyle_800x480.xml");
		defaultCardItem->setData(0, 16, POKERTH_DISTRIBUTED_STYLE);
		defaultCardItem->setData(0, Qt::ToolTipRole, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default_800x480/defaultdeckstyle_800x480.xml");
		defaultCardItem->setData(2, Qt::ToolTipRole, defaultCardStyle10.getMyStateToolTipInfo());
		if(defaultCardStyle10.getState()) defaultCardItem->setIcon(2, QIcon(":/gfx/emblem-important.png"));
		else defaultCardItem->setIcon(2, QIcon(":/gfx/dialog_ok_apply.png"));
	}
#ifndef GUI_800x480
	//define PokerTH old default CardDeck
	CardDeckStyleReader defaultCardStyle(myConfig, this);
	defaultCardStyle.readStyleFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default/defaultdeckstyle.xml");
	if(defaultCardStyle.getLoadedSuccessfull()) {
		QStringList tempStringList1;
		tempStringList1 << defaultCardStyle.getStyleDescription() << defaultCardStyle.getStyleMaintainerName();
		MyStyleListItem *defaultCardItem = new MyStyleListItem(tempStringList1, treeWidget_cardDeckStyles);
		defaultCardItem->setData(0, 15, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default/defaultdeckstyle.xml");
		defaultCardItem->setData(0, 16, POKERTH_DISTRIBUTED_STYLE);
		defaultCardItem->setData(0, Qt::ToolTipRole, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default/defaultdeckstyle.xml");
		defaultCardItem->setData(2, Qt::ToolTipRole, defaultCardStyle.getMyStateToolTipInfo());
		if(defaultCardStyle.getState()) defaultCardItem->setIcon(2, QIcon(":/gfx/emblem-important.png"));
		else defaultCardItem->setIcon(2, QIcon(":/gfx/dialog_ok_apply.png"));
	}
	//define PokerTH old default CardDeck4c
	CardDeckStyleReader default4cCardStyle(myConfig, this);
	default4cCardStyle.readStyleFile(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default4c/default4cdeckstyle.xml");
	if(default4cCardStyle.getLoadedSuccessfull()) {
		QStringList tempStringList1;
		tempStringList1 << default4cCardStyle.getStyleDescription() << default4cCardStyle.getStyleMaintainerName();
		MyStyleListItem *default4cCardItem = new MyStyleListItem(tempStringList1, treeWidget_cardDeckStyles);
		default4cCardItem->setData(0, 15, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default4c/default4cdeckstyle.xml");
		default4cCardItem->setData(0, 16, POKERTH_DISTRIBUTED_STYLE);
		default4cCardItem->setData(0, Qt::ToolTipRole, QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"gfx/cards/default4c/default4cdeckstyle.xml");
		default4cCardItem->setData(2, Qt::ToolTipRole, default4cCardStyle.getMyStateToolTipInfo());
		if(default4cCardStyle.getState()) default4cCardItem->setIcon(2, QIcon(":/gfx/emblem-important.png"));
		else default4cCardItem->setIcon(2, QIcon(":/gfx/dialog_ok_apply.png"));
	}
#endif
	//load secondary card styles into list (if fallback no entry)
	myCardDeckStylesList = myConfig->readConfigStringList("CardDeckStylesList");
	list<std::string>::iterator it2;
	for(it2= myCardDeckStylesList.begin(); it2 != myCardDeckStylesList.end(); ++it2) {
		CardDeckStyleReader nextStyle(myConfig, this);
		nextStyle.readStyleFile(QString::fromUtf8(it2->c_str()));
		if(!nextStyle.getFallBack() && nextStyle.getLoadedSuccessfull()) {
			QStringList tempStringList1;
			tempStringList1 << nextStyle.getStyleDescription() << nextStyle.getStyleMaintainerName();
			MyStyleListItem *nextItem = new MyStyleListItem(tempStringList1, treeWidget_cardDeckStyles);
			nextItem->setData(0, 15,QString::fromUtf8(it2->c_str()));
			nextItem->setData(0, 16, ADDITIONAL_STYLE);
			nextItem->setData(0, Qt::ToolTipRole,QString::fromUtf8(it2->c_str()));
			nextItem->setData(2, Qt::ToolTipRole, nextStyle.getMyStateToolTipInfo());
			if(nextStyle.getState()) nextItem->setIcon(2, QIcon(":/gfx/emblem-important.png"));
			else nextItem->setIcon(2, QIcon(":/gfx/dialog_ok_apply.png"));
			treeWidget_cardDeckStyles->addTopLevelItem(nextItem);
		}
	}
	treeWidget_cardDeckStyles->sortItems(0, Qt::AscendingOrder);

	//set current card deck style from config file
	CardDeckStyleReader currentCardDeckStyle(myConfig, this);
	currentCardDeckStyle.readStyleFile(QString::fromUtf8(myConfig->readConfigString("CurrentCardDeckStyle").c_str()));
	if(currentCardDeckStyle.getLoadedSuccessfull()) {
		int j;
		bool currentCardDeckFound(false);
		for(j=0; j < treeWidget_cardDeckStyles->topLevelItemCount(); j++) {
			QTreeWidgetItem *item = treeWidget_cardDeckStyles->topLevelItem(j);
			if(item->data(0, 15) == currentCardDeckStyle.getCurrentFileName()) {
				item->setIcon(0, QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
				treeWidget_cardDeckStyles->setCurrentItem(item);
				currentCardDeckFound=true;
			} else item->setIcon(0, QIcon());
		}
		if(!currentCardDeckFound) {
			qDebug() << "Config ERROR: current card deck style file not found in List. Try to mark default as selected.";
			QTreeWidgetItem *item = treeWidget_cardDeckStyles->topLevelItem(0);
			if(item) {
				item->setIcon(0, QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
				treeWidget_cardDeckStyles->setCurrentItem(item);
			}
		}
	} else {
		qDebug() << "Config ERROR: current card deck style file could not be loaded. Try to mark default as selected.";
		QTreeWidgetItem *item = treeWidget_cardDeckStyles->topLevelItem(0);
		if(item) {
			item->setIcon(0, QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
			treeWidget_cardDeckStyles->setCurrentItem(item);
		}
	}

	// 	refresh Card Deck Style Preview
	showCurrentCardDeckStylePreview();

	//adjust column sizes for styles treeWidgets
	treeWidget_gameTableStyles->resizeColumnToContents(0);
	treeWidget_gameTableStyles->resizeColumnToContents(1);
	treeWidget_gameTableStyles->resizeColumnToContents(2);
	treeWidget_cardDeckStyles->resizeColumnToContents(0);
	treeWidget_cardDeckStyles->resizeColumnToContents(1);
	treeWidget_cardDeckStyles->resizeColumnToContents(2);


	radioButton_flipsideTux->setChecked(myConfig->readConfigInt("FlipsideTux"));
	radioButton_flipsideOwn->setChecked(myConfig->readConfigInt("FlipsideOwn"));
	if(radioButton_flipsideOwn->isChecked()) {
		lineEdit_OwnFlipsideFilename->setEnabled(true);
		pushButton_openFlipsidePicture->setEnabled(true);
	}
	lineEdit_OwnFlipsideFilename->setText(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str()));

	//Sound
	groupBox_playSoundEffects->setChecked(myConfig->readConfigInt("PlaySoundEffects"));
	horizontalSlider_soundVolume->setValue(myConfig->readConfigInt("SoundVolume"));
	checkBox_playGameActions->setChecked(myConfig->readConfigInt("PlayGameActions"));
	checkBox_playLobbyChatNotification->setChecked(myConfig->readConfigInt("PlayLobbyChatNotification"));
	checkBox_playNetworkGameNotification->setChecked(myConfig->readConfigInt("PlayNetworkGameNotification"));
	checkBox_playBlindRaiseSound->setChecked(myConfig->readConfigInt("PlayBlindRaiseNotification"));

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
	languageIsChanged = false;

	//disable reset config button when dialog is called from ingame
	if(calledIngame) {
		pushButton_resetSettings->setDisabled(true);
		label_resetNote->show();
	} else {
		pushButton_resetSettings->setEnabled(true);
		label_resetNote->hide();
	}

}

void settingsDialogImpl::exec(bool in_game)
{
	calledIngame = in_game;
	prepareDialog();
	QDialog::exec();
}

void settingsDialogImpl::isAccepted()
{

	settingsCorrect = true;

	// 	Player Nicks
	//check if all player nicks are unique --> otherwise dont save
	QSet<QString> checkSetPlayerNicks;
	checkSetPlayerNicks.insert(lineEdit_HumanPlayerName->text().trimmed());
	checkSetPlayerNicks.insert(lineEdit_Opponent1Name->text().trimmed());
	checkSetPlayerNicks.insert(lineEdit_Opponent2Name->text().trimmed());
	checkSetPlayerNicks.insert(lineEdit_Opponent3Name->text().trimmed());
	checkSetPlayerNicks.insert(lineEdit_Opponent4Name->text().trimmed());
	checkSetPlayerNicks.insert(lineEdit_Opponent5Name->text().trimmed());
	checkSetPlayerNicks.insert(lineEdit_Opponent6Name->text().trimmed());
	checkSetPlayerNicks.insert(lineEdit_Opponent7Name->text().trimmed());
	checkSetPlayerNicks.insert(lineEdit_Opponent8Name->text().trimmed());
	checkSetPlayerNicks.insert(lineEdit_Opponent9Name->text().trimmed());

	if(checkSetPlayerNicks.count() != 10) {
		MyMessageBox::warning(this, tr("Settings Error"),
							  tr("The opponent names are not unique.\n"
								 "Please choose different names for each Opponent!"),
							  QMessageBox::Ok);
		settingsCorrect = false;
	} else {
		//save nicks and avatars
		myConfig->writeConfigString("MyName", lineEdit_HumanPlayerName->text().trimmed().toUtf8().constData());
		myConfig->writeConfigString("MyAvatar", pushButton_HumanPlayerAvatar->getMyLink().toUtf8().constData());

		myConfig->writeConfigString("Opponent1Name", lineEdit_Opponent1Name->text().trimmed().toUtf8().constData());
		myConfig->writeConfigString("Opponent1Avatar", pushButton_Opponent1Avatar->getMyLink().toUtf8().constData());

		myConfig->writeConfigString("Opponent2Name", lineEdit_Opponent2Name->text().trimmed().toUtf8().constData());
		myConfig->writeConfigString("Opponent2Avatar", pushButton_Opponent2Avatar->getMyLink().toUtf8().constData());

		myConfig->writeConfigString("Opponent3Name", lineEdit_Opponent3Name->text().trimmed().toUtf8().constData());
		myConfig->writeConfigString("Opponent3Avatar", pushButton_Opponent3Avatar->getMyLink().toUtf8().constData());

		myConfig->writeConfigString("Opponent4Name", lineEdit_Opponent4Name->text().trimmed().toUtf8().constData());
		myConfig->writeConfigString("Opponent4Avatar", pushButton_Opponent4Avatar->getMyLink().toUtf8().constData());

		myConfig->writeConfigString("Opponent5Name", lineEdit_Opponent5Name->text().trimmed().toUtf8().constData());
		myConfig->writeConfigString("Opponent5Avatar", pushButton_Opponent5Avatar->getMyLink().toUtf8().constData());

		myConfig->writeConfigString("Opponent6Name", lineEdit_Opponent6Name->text().trimmed().toUtf8().constData());
		myConfig->writeConfigString("Opponent6Avatar", pushButton_Opponent6Avatar->getMyLink().toUtf8().constData());

		myConfig->writeConfigString("Opponent7Name", lineEdit_Opponent7Name->text().trimmed().toUtf8().constData());
		myConfig->writeConfigString("Opponent7Avatar", pushButton_Opponent7Avatar->getMyLink().toUtf8().constData());

		myConfig->writeConfigString("Opponent8Name", lineEdit_Opponent8Name->text().trimmed().toUtf8().constData());
		myConfig->writeConfigString("Opponent8Avatar", pushButton_Opponent8Avatar->getMyLink().toUtf8().constData());

		myConfig->writeConfigString("Opponent9Name", lineEdit_Opponent9Name->text().trimmed().toUtf8().constData());
		myConfig->writeConfigString("Opponent9Avatar", pushButton_Opponent9Avatar->getMyLink().toUtf8().constData());
	}

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
	myConfig->writeConfigInt("NetDelayBetweenHands", spinBox_netDelayBetweenHands->value());
	myConfig->writeConfigInt("NetTimeOutPlayerAction", spinBox_netTimeOutPlayerAction->value());
	myConfig->writeConfigInt("ServerPort", spinBox_serverPort->value());
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
		} else {
			MyMessageBox::warning(this, tr("Settings Error"),
								  tr("The entered server list address is not a valid URL.\n"
									 "Please enter a valid server list address!"),
								  QMessageBox::Ok);
			settingsCorrect = false;
		}
	}
	if(groupBox_manualServerConfig->isChecked()) {

		myConfig->writeConfigInt("InternetServerConfigMode", 1);
		myConfig->writeConfigString("InternetServerAddress", lineEdit_InternetServerAddress->text().toUtf8().constData());
		myConfig->writeConfigString("ServerPassword", lineEdit_InternetServerPassword->text().toUtf8().constData());
		myConfig->writeConfigInt("InternetServerPort", spinBox_InternetServerPort->value());
	}
	if(checkBox_useAvatarServer->isChecked()) {
		myConfig->writeConfigInt("UseAvatarServer", 1);
		myConfig->writeConfigString("AvatarServerAddress", lineEdit_avatarServerAddress->text().toUtf8().constData());
	} else {
		myConfig->writeConfigInt("UseAvatarServer", 0);
	}

	myConfig->writeConfigInt("InternetServerUseIpv6", checkBox_InternetServerUseIpv6->isChecked());
	myConfig->writeConfigInt("InternetServerUseSctp", checkBox_InternetServerUseSctp->isChecked());
	myConfig->writeConfigInt("UseInternetGamePassword", checkBox_UseInternetGamePassword->isChecked());
	myConfig->writeConfigString("InternetGamePassword", lineEdit_InternetGamePassword->text().toUtf8().constData());
	myConfig->writeConfigInt("UseLobbyChat", checkBox_UseLobbyChat->isChecked());
	myConfig->writeConfigInt("NetAutoLeaveGameAfterFinish", checkBox_InetGame_AutoLeaveTheTableAfterGameFinished->isChecked());
	myConfig->writeConfigInt("InternetGameType", comboBox_internetGameType->currentIndex());
	myConfig->writeConfigString("InternetGameName", lineEdit_internetGameName->text().toUtf8().constData());

	int k;
	std::list<std::string> playerIgnoreList;
	for(k=0; k<treeWidget_internetGameIgnoredPlayers->topLevelItemCount(); k++) {
		playerIgnoreList.push_back(QString("%1").arg(treeWidget_internetGameIgnoredPlayers->topLevelItem(k)->text(0)).toUtf8().constData());
	}
	myConfig->writeConfigStringList("PlayerIgnoreList", playerIgnoreList);
	myConfig->writeConfigInt("InternetGameAllowSpectators", checkBox_allowSpectators->isChecked());

	// 	Interface
	myConfig->writeConfigString("Language", comboBox_switchLanguage->itemData(comboBox_switchLanguage->currentIndex()).toString().toUtf8().constData());
#ifndef GUI_800x480
	myConfig->writeConfigInt("ShowLeftToolBox", checkBox_showLeftToolbox->isChecked());
	myConfig->writeConfigInt("ShowRightToolBox", checkBox_showRightToolbox->isChecked());
	myConfig->writeConfigInt("AlternateFKeysUserActionMode", checkBox_alternateFKeysUserActionMode->isChecked());
#endif
	myConfig->writeConfigInt("ShowFadeOutCardsAnimation", checkBox_showFadeOutCardsAnimation->isChecked());
	myConfig->writeConfigInt("ShowFlipCardsAnimation", checkBox_showFlipCardsAnimation->isChecked());
	myConfig->writeConfigInt("ShowBlindButtons", checkBox_showBlindButtons->isChecked());
	myConfig->writeConfigInt("ShowCardsChanceMonitor", checkBox_cardsChanceMonitor->isChecked());
	myConfig->writeConfigInt("AntiPeekMode", checkBox_antiPeekMode->isChecked());
	myConfig->writeConfigInt("ShowCountryFlagInAvatar", checkBox_showCountryFlagInAvatar->isChecked());
	myConfig->writeConfigInt("ShowPingStateInAvatar", checkBox_showPingStateInAvatar->isChecked());
	myConfig->writeConfigInt("DontTranslateInternationalPokerStringsFromStyle", checkBox_dontTranslatePokerStrings->isChecked());
	myConfig->writeConfigInt("DisableSplashScreenOnStartup", checkBox_disableSplashscreen->isChecked());
	myConfig->writeConfigInt("EnableBetInputFocusSwitch", checkBox_enableBetInputFocusSwitch->isChecked());
	myConfig->writeConfigInt("AccidentallyCallBlocker", checkBox_enableAccidentallyCallBlocker->isChecked());
	myConfig->writeConfigInt("DontHideAvatarsOfIgnored", checkBox_dontHideAvatarsOfIgnored->isChecked());
	myConfig->writeConfigInt("DisableChatEmoticons", checkBox_disableChatEmoticons->isChecked());

	myConfig->writeConfigInt("FlipsideTux", radioButton_flipsideTux->isChecked());
	myConfig->writeConfigInt("FlipsideOwn", radioButton_flipsideOwn->isChecked());

	if(radioButton_flipsideOwn->isChecked()) {
		QFile ownFlipsideFilename(lineEdit_OwnFlipsideFilename->text());
		if(ownFlipsideFilename.exists() && lineEdit_OwnFlipsideFilename->text() != "") {
			myConfig->writeConfigString("FlipsideOwnFile", lineEdit_OwnFlipsideFilename->text().toUtf8().constData());
		} else {
			MyMessageBox::warning(this, tr("Settings Error"),
								  tr("The entered flipside picture doesn't exist.\n"
									 "Please enter an valid picture!"),
								  QMessageBox::Ok);
			settingsCorrect = false;
		}
	}

	//S T Y L E
	// 	TABLE
	//save game table styles list
	myGameTableStylesList.clear();
	int i;
	for(i=0; i < treeWidget_gameTableStyles->topLevelItemCount(); i++) {
		QTreeWidgetItem *item = treeWidget_gameTableStyles->topLevelItem(i);
		if(item->data(0, 16).toInt() == ADDITIONAL_STYLE) {
			myGameTableStylesList.push_back(item->data(0, 15).toString().toUtf8().constData());
		}
	}
	myConfig->writeConfigStringList("GameTableStylesList", myGameTableStylesList);
	//save current game table style
	for(i=0; i < treeWidget_gameTableStyles->topLevelItemCount(); i++) {
		QTreeWidgetItem *item = treeWidget_gameTableStyles->topLevelItem(i);
		if(!item->icon(0).isNull())
			myConfig->writeConfigString("CurrentGameTableStyle", item->data(0, 15).toString().toUtf8().constData());
	}
	// 	CARDS
	//save card deck styles list
	myCardDeckStylesList.clear();
	int j;
	for(j=0; j < treeWidget_cardDeckStyles->topLevelItemCount(); j++) {
		QTreeWidgetItem *item = treeWidget_cardDeckStyles->topLevelItem(j);
		if(item->data(0, 16).toInt() == ADDITIONAL_STYLE) {
			myCardDeckStylesList.push_back(item->data(0, 15).toString().toUtf8().constData());
		}
	}
	myConfig->writeConfigStringList("CardDeckStylesList", myCardDeckStylesList);
	//save current card deck style
	for(j=0; j < treeWidget_cardDeckStyles->topLevelItemCount(); j++) {
		QTreeWidgetItem *item = treeWidget_cardDeckStyles->topLevelItem(j);
		if(!item->icon(0).isNull())
			myConfig->writeConfigString("CurrentCardDeckStyle", item->data(0, 15).toString().toUtf8().constData());
	}

	//Sound
	myConfig->writeConfigInt("PlaySoundEffects", groupBox_playSoundEffects->isChecked());
	myConfig->writeConfigInt("SoundVolume", horizontalSlider_soundVolume->value());
	myConfig->writeConfigInt("PlayGameActions", checkBox_playGameActions->isChecked());
	myConfig->writeConfigInt("PlayLobbyChatNotification", checkBox_playLobbyChatNotification->isChecked());
	myConfig->writeConfigInt("PlayNetworkGameNotification", checkBox_playNetworkGameNotification->isChecked());
	myConfig->writeConfigInt("PlayBlindRaiseNotification", checkBox_playBlindRaiseSound->isChecked());

	//	Log
	myConfig->writeConfigInt("LogOnOff", groupBox_logOnOff->isChecked());
	if (myConfig->readConfigInt("LogOnOff")) {
		// if log On
		QDir logDir(lineEdit_logDir->text());
		if(logDir.exists() && lineEdit_logDir->text() != "") {
			myConfig->writeConfigString("LogDir", lineEdit_logDir->text().toUtf8().constData());
		} else {
			MyMessageBox::warning(this, tr("Settings Error"),
								  tr("The log file directory doesn't exist.\n"
									 "Please select an valid directory!"),
								  QMessageBox::Ok);
			settingsCorrect = false;
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
		MyMessageBox::information(this, tr("Language Changed"),
								  tr("You have changed application language to %1.\nPlease restart PokerTH to load new language!").arg(comboBox_switchLanguage->itemText(changedLanguageIndex)),
								  QMessageBox::Ok);
	}

	//Wenn alles richtig eingegeben wurde --> Dialog schließen
	if(settingsCorrect) {
		this->accept();
	}
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

void settingsDialogImpl::setAvatarFile0()
{

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_HumanPlayerAvatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_HumanPlayerAvatar->setIcon(QIcon(pushButton_HumanPlayerAvatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile1()
{

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent1Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent1Avatar->setIcon(QIcon(pushButton_Opponent1Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile2()
{

	callSelectAvatarDialog();

	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent2Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent2Avatar->setIcon(QIcon(pushButton_Opponent2Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile3()
{

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent3Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent3Avatar->setIcon(QIcon(pushButton_Opponent3Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile4()
{

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent4Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent4Avatar->setIcon(QIcon(pushButton_Opponent4Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile5()
{

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent5Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent5Avatar->setIcon(QIcon(pushButton_Opponent5Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile6()
{

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent6Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent6Avatar->setIcon(QIcon(pushButton_Opponent6Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile7()
{

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent7Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent7Avatar->setIcon(QIcon(pushButton_Opponent7Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile8()
{

	callSelectAvatarDialog();
	if(mySelectAvatarDialogImpl->getSettingsCorrect()) {
		pushButton_Opponent8Avatar->setMyLink(mySelectAvatarDialogImpl->getAvatarLink());
		pushButton_Opponent8Avatar->setIcon(QIcon(pushButton_Opponent8Avatar->getMyLink()));
	}
}

void settingsDialogImpl::setAvatarFile9()
{

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

	if (!dir.isEmpty()) {

		QDir logDir(dir);
		if(logDir.exists()) {
			lineEdit_logDir->setText(dir);
			MyMessageBox::information(this, tr("Settings Information"),
									  tr("You have changed the log file directory.\n"
										 "Please restart PokerTH to use the new directory for the log files!"),
									  QMessageBox::Ok);
		} else {
			MyMessageBox::warning(this, tr("Settings Error"),
								  tr("The log file directory doesn't exist.\n"
									 "Please select an valid directory!"),
								  QMessageBox::Ok);
		}
	}
}

void settingsDialogImpl::clearInternetGamePassword(bool clear)
{

	if(!clear) {
		lineEdit_InternetGamePassword->clear();
	}
}

void settingsDialogImpl::callManualBlindsOrderDialog()
{

	myManualBlindsOrderDialog->listWidget_blinds->clear();
	myManualBlindsOrderDialog->spinBox_input->setMinimum(spinBox_firstSmallBlind->value());

	list<int>::iterator it1;
	for(it1= myManualBlindsList.begin(); it1 != myManualBlindsList.end(); ++it1) {
		myManualBlindsOrderDialog->listWidget_blinds->addItem(QString::number(*it1,10));
	}
	myManualBlindsOrderDialog->sortBlindsList();

	myManualBlindsOrderDialog->radioButton_alwaysDoubleBlinds->setChecked(myAfterMBAlwaysDoubleBlinds);
	myManualBlindsOrderDialog->radioButton_alwaysRaiseAbout->setChecked(myAfterMBAlwaysRaiseAbout);
	myManualBlindsOrderDialog->spinBox_alwaysRaiseValue->setValue(myAfterMBAlwaysRaiseValue);
	myManualBlindsOrderDialog->radioButton_stayAtLastBlind->setChecked(myAfterMBStayAtLastBlind);

	myManualBlindsOrderDialog->exec();
	if(myManualBlindsOrderDialog->result() == QDialog::Accepted) {

		bool ok = true;
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

void settingsDialogImpl::callNetManualBlindsOrderDialog()
{

	myManualBlindsOrderDialog->listWidget_blinds->clear();
	myManualBlindsOrderDialog->spinBox_input->setMinimum(spinBox_netFirstSmallBlind->value());

	list<int>::iterator it1;
	for(it1= myNetManualBlindsList.begin(); it1 != myNetManualBlindsList.end(); ++it1) {
		myManualBlindsOrderDialog->listWidget_blinds->addItem(QString::number(*it1,10));
	}
	myManualBlindsOrderDialog->sortBlindsList();

	myManualBlindsOrderDialog->radioButton_alwaysDoubleBlinds->setChecked(myNetAfterMBAlwaysDoubleBlinds);
	myManualBlindsOrderDialog->radioButton_alwaysRaiseAbout->setChecked(myNetAfterMBAlwaysRaiseAbout);
	myManualBlindsOrderDialog->spinBox_alwaysRaiseValue->setValue(myNetAfterMBAlwaysRaiseValue);
	myManualBlindsOrderDialog->radioButton_stayAtLastBlind->setChecked(myNetAfterMBStayAtLastBlind);

	myManualBlindsOrderDialog->exec();
	if(myManualBlindsOrderDialog->result() == QDialog::Accepted) {

		bool ok = true;
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

void settingsDialogImpl::setFirstSmallBlindMargin()
{

	if(radioButton_manualBlindsOrder->isChecked() && !myManualBlindsList.empty()) {
		if(spinBox_firstSmallBlind->value() > myManualBlindsList.front()) {
			MyMessageBox::warning(this, tr("Blinds Error"),
								  tr("The first element in your manual-blinds-list \nis smaller than current first-small-blind!\nThis first-small-blind-value will be set to maximum allowed value."),
								  QMessageBox::Close);
		}
		spinBox_firstSmallBlind->setMaximum(myManualBlindsList.front());
	} else {
		spinBox_firstSmallBlind->setMaximum(9999);
	}
	if(radioButton_netManualBlindsOrder->isChecked() && !myNetManualBlindsList.empty()) {
		if(spinBox_netFirstSmallBlind->value() > myNetManualBlindsList.front()) {
			MyMessageBox::warning(this, tr("Blinds Error"),
								  tr("The first element in your manual-blinds-list \nis smaller than current first-small-blind!\nThis first-small-blind-value will be set to maximum allowed value."),
								  QMessageBox::Close);
		}
		spinBox_netFirstSmallBlind->setMaximum(myNetManualBlindsList.front());
	} else {
		spinBox_netFirstSmallBlind->setMaximum(9999);
	}


}

void settingsDialogImpl::setLanguageChanged(int index)
{

	languageIsChanged = true;
	changedLanguageIndex = index;

}

void settingsDialogImpl::toggleGroupBoxAutomaticServerConfig(bool toggleState)
{
	if(toggleState) groupBox_manualServerConfig->setChecked(false);
	else groupBox_manualServerConfig->setChecked(true);
}

void settingsDialogImpl::toggleGroupBoxManualServerConfig(bool toggleState)
{
	if(toggleState) groupBox_automaticServerConfig->setChecked(false);
	else groupBox_automaticServerConfig->setChecked(true);
}

void settingsDialogImpl::showCurrentGameTableStylePreview()
{
	QTreeWidgetItem* item = treeWidget_gameTableStyles->currentItem();
	if(item) {
		GameTableStyleReader style(myConfig, this);
		style.readStyleFile(item->data(0, 15).toString());
		QPixmap preview(style.getPreview());
		if(preview.height() > 160 || preview.width() > 120) label_gameTableStylePreview->setPixmap(preview.scaled(160,120,Qt::KeepAspectRatio,Qt::SmoothTransformation));
		else label_gameTableStylePreview->setPixmap(preview);

		QString MaintainerName = tr("Maintainer Name");
		QString MaintainerEMail = tr("Maintainer EMail");
		QString CreateDate = tr("Create Date");
		QString WindowBehaviour = tr("Windows Behaviour");
		QString scaleable = tr("scaleable");
		QString fixed = tr("fixed");
		QString MinimumSize = tr("Minimum Size");
		QString MaximumSize = tr("Maximum Size");
		QString FixedSize = tr("Fixed Size");
		QString State = tr("State");

		QString windowsSubString;
		if(style.getIfFixedWindowSize().toInt()) {
			windowsSubString = "<b>"+WindowBehaviour+":</b> "+fixed+"<br><b>"+FixedSize+":</b> "+style.getFixedWindowWidth()+"x"+style.getFixedWindowHeight();
		} else {
			windowsSubString = "<b>"+WindowBehaviour+":</b> "+scaleable+"<br><b>"+MinimumSize+":</b> "+style.getMinimumWindowWidth()+"x"+style.getMinimumWindowHeight()+"<br><b>"+MaximumSize+":</b> "+style.getMaximumWindowWidth()+"x"+style.getMaximumWindowHeight();
		}

		QString maintainerEMailString;
		if(style.getStyleMaintainerEMail() != "NULL" && style.getStyleMaintainerEMail() != "") {
			maintainerEMailString = "<b>"+MaintainerEMail+":</b> "+style.getStyleMaintainerEMail()+"<br>";
		}

		label_gameTableStyleInfo->setWordWrap(true);
		label_gameTableStyleInfo->setText("<b>"+MaintainerName+":</b> "+style.getStyleMaintainerName()+"<br>"+maintainerEMailString+"<b>"+CreateDate+":</b> "+style.getStyleCreateDate()+"<br>"+windowsSubString+"<br><b>"+State+": </b>"+style.getMyStateToolTipInfo());

		//active the current selected item directly
		setSelectedGameTableStyleActivated();

		//disable remove button for distributed styles
		if(item->data(0, 16).toInt() == POKERTH_DISTRIBUTED_STYLE) {
			pushButton_removeGameTableStyle->setDisabled(true);
		} else {
			pushButton_removeGameTableStyle->setDisabled(false);
		}

	}
}

void settingsDialogImpl::setSelectedGameTableStyleActivated()
{
	QTreeWidgetItem* selectedItem = treeWidget_gameTableStyles->currentItem();
	if(selectedItem) {
		int i;
		for(i=0; i < treeWidget_gameTableStyles->topLevelItemCount(); i++) {
			QTreeWidgetItem *item = treeWidget_gameTableStyles->topLevelItem(i);
			if(item == selectedItem) {
				item->setIcon(0, QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
			} else item->setIcon(0, QIcon());
		}
	}
}

void settingsDialogImpl::addGameTableStyle()
{
	QDir dir(QString::fromUtf8(myConfig->readConfigString("LastGameTableStyleDir").c_str()));
	QString dirString;

	if(dir.exists()) dirString = dir.absolutePath();
	else dirString = QDir::home().absolutePath();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Please select your game table style"),
					   dirString,
					   tr("PokerTH game table styles (*.xml)"));

	if (!fileName.isEmpty()) {

		int i;
		bool fileNameAlreadyFound(false);
		for(i=0; i < treeWidget_gameTableStyles->topLevelItemCount(); i++) {
			QTreeWidgetItem *item = treeWidget_gameTableStyles->topLevelItem(i);
			if(item->data(0, 15).toString() == fileName)
				fileNameAlreadyFound = true;
		}

		if(fileNameAlreadyFound) {
			MyMessageBox::warning(this, tr("Game Table Style Error"),
								  tr("Selected game table style file is already in the list. \nPlease select another one to add!"),
								  QMessageBox::Ok);
		} else {
			GameTableStyleReader newStyle(myConfig, this);
			newStyle.readStyleFile(fileName);
			if(!newStyle.getFallBack() && newStyle.getLoadedSuccessfull()) {
				QStringList tempStringList1;
				tempStringList1 << newStyle.getStyleDescription() << newStyle.getStyleMaintainerName();
				MyStyleListItem *newItem = new MyStyleListItem(tempStringList1, treeWidget_gameTableStyles);
				newItem->setData(0, 15,fileName);
				newItem->setData(0, 16, ADDITIONAL_STYLE);
				newItem->setData(0, Qt::ToolTipRole,fileName);
				newItem->setData(2, Qt::ToolTipRole, newStyle.getMyStateToolTipInfo());
				if(newStyle.getState() != GT_STYLE_OK) newItem->setIcon(2, QIcon(":/gfx/emblem-important.png"));
				else newItem->setIcon(2, QIcon(":/gfx/dialog-ok-apply.png"));
				treeWidget_gameTableStyles->addTopLevelItem(newItem);
				treeWidget_gameTableStyles->setCurrentItem(newItem);
				treeWidget_gameTableStyles->sortItems(0, Qt::AscendingOrder);

				qDebug() << "settings: " << newStyle.getStyleDescription() << newStyle.getState();
				if(newStyle.getState() != GT_STYLE_OK) newStyle.showErrorMessage();
			} else {
				MyMessageBox::warning(this, tr("Game Table Style File Error"),
									  tr("Could not load game table style file correctly. \nStyle will not be placed into list!"),
									  QMessageBox::Ok);
			}
		}
		//save current filepath
		myConfig->writeConfigString("LastGameTableStyleDir",QFileInfo(fileName).absolutePath().toUtf8().constData());

		//adjust column sizes for styles treeWidgets
		treeWidget_gameTableStyles->resizeColumnToContents(0);
		treeWidget_gameTableStyles->resizeColumnToContents(1);
		treeWidget_gameTableStyles->resizeColumnToContents(2);
	}
}

void settingsDialogImpl::removeGameTableStyle()
{
	QTreeWidgetItem* selectedItem = treeWidget_gameTableStyles->currentItem();
	//never delete PokerTH defaullt Styles
	if(selectedItem && selectedItem->data(0, 16).toInt() == ADDITIONAL_STYLE) {
		// if selected is activated --> swith activation to first default
		if(!selectedItem->icon(0).isNull()) {
			QTreeWidgetItem* firstItem = treeWidget_gameTableStyles->topLevelItem(0);
			firstItem->setIcon(0, QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
		}
		//remove from List
		treeWidget_gameTableStyles->takeTopLevelItem(treeWidget_gameTableStyles->currentIndex().row());

	}
}

void settingsDialogImpl::showCurrentCardDeckStylePreview()
{
	QTreeWidgetItem* item = treeWidget_cardDeckStyles->currentItem();
	if(item) {
		CardDeckStyleReader style(myConfig, this);
		style.readStyleFile(item->data(0, 15).toString());
		QPixmap preview(style.getPreview());
		if(preview.height() > 160 || preview.width() > 120) label_cardDeckStylePreview->setPixmap(preview.scaled(160,120,Qt::KeepAspectRatio,Qt::SmoothTransformation));
		else label_cardDeckStylePreview->setPixmap(preview);

		QString MaintainerName = tr("Maintainer Name");
		QString MaintainerEMail = tr("Maintainer EMail");
		QString CreateDate = tr("Create Date");

		QString maintainerEMailString;
		if(style.getStyleMaintainerEMail() != "NULL" && style.getStyleMaintainerEMail() != "") {
			maintainerEMailString = "<b>"+MaintainerEMail+":</b> "+style.getStyleMaintainerEMail()+"<br>";
		}

#ifdef GUI_800x480
		label_cardDeckStyleInfo->setWordWrap(true);
#endif
		label_cardDeckStyleInfo->setText("<b>"+MaintainerName+":</b> "+style.getStyleMaintainerName()+"<br>"+maintainerEMailString+"<b>"+CreateDate+":</b> "+style.getStyleCreateDate()+"");
		//active the current selected item directly
		setSelectedCardDeckStyleActivated();

		//disable remove button for distributed styles
		if(item->data(0, 16).toInt() == POKERTH_DISTRIBUTED_STYLE) {
			pushButton_removeCardDeckStyle->setDisabled(true);
		} else {
			pushButton_removeCardDeckStyle->setDisabled(false);
		}
	}
}

void settingsDialogImpl::setSelectedCardDeckStyleActivated()
{
	QTreeWidgetItem* selectedItem = treeWidget_cardDeckStyles->currentItem();
	if(selectedItem) {
		int i;
		for(i=0; i < treeWidget_cardDeckStyles->topLevelItemCount(); i++) {
			QTreeWidgetItem *item = treeWidget_cardDeckStyles->topLevelItem(i);
			if(item == selectedItem) {
				item->setIcon(0, QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
			} else item->setIcon(0, QIcon());
		}
	}
}

void settingsDialogImpl::addCardDeckStyle()
{
	QDir dir(QString::fromUtf8(myConfig->readConfigString("LastCardDeckStyleDir").c_str()));
	QString dirString;

	if(dir.exists()) dirString = dir.absolutePath();
	else dirString = QDir::home().absolutePath();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Please select your card deck style"),
					   dirString,
					   tr("PokerTH card deck styles (*.xml)"));

	if (!fileName.isEmpty()) {

		int i;
		bool fileNameAlreadyFound(false);
		for(i=0; i < treeWidget_cardDeckStyles->topLevelItemCount(); i++) {
			QTreeWidgetItem *item = treeWidget_cardDeckStyles->topLevelItem(i);
			if(item->data(0, 15).toString() == fileName)
				fileNameAlreadyFound = true;
		}

		if(fileNameAlreadyFound) {
			MyMessageBox::warning(this, tr("Card Deck Style Error"),
								  tr("Selected card deck style file is already in the list. \nPlease select another one to add!"),
								  QMessageBox::Ok);
		} else {
			CardDeckStyleReader newStyle(myConfig, this);
			newStyle.readStyleFile(fileName);
			if(!newStyle.getFallBack() && newStyle.getLoadedSuccessfull()) {
				QStringList tempStringList1;
				tempStringList1 << newStyle.getStyleDescription() << newStyle.getStyleMaintainerName();
				MyStyleListItem *newItem = new MyStyleListItem(tempStringList1, treeWidget_cardDeckStyles);
				newItem->setData(0, 15,fileName);
				newItem->setData(0, 16, ADDITIONAL_STYLE);
				newItem->setData(0, Qt::ToolTipRole,fileName);
				newItem->setData(2, Qt::ToolTipRole, newStyle.getMyStateToolTipInfo());
				if(newStyle.getState() != CD_STYLE_OK) newItem->setIcon(2, QIcon(":/gfx/emblem-important.png"));
				else newItem->setIcon(2, QIcon(":/gfx/dialog-ok-apply.png"));

				qDebug() << "settings: " << newStyle.getStyleDescription() << newStyle.getState();
				if(newStyle.getState() != CD_STYLE_OK) newStyle.showErrorMessage();

				treeWidget_cardDeckStyles->addTopLevelItem(newItem);
				treeWidget_cardDeckStyles->setCurrentItem(newItem);
				treeWidget_cardDeckStyles->sortItems(0, Qt::AscendingOrder);

			} else {
				MyMessageBox::warning(this, tr("Card Deck Style File Error"),
									  tr("Could not load card deck style file correctly. \nStyle will not be placed into list!"),
									  QMessageBox::Ok);
			}
		}
		//save current filepath
		myConfig->writeConfigString("LastCardDeckStyleDir",QFileInfo(fileName).absolutePath().toUtf8().constData());

		treeWidget_cardDeckStyles->resizeColumnToContents(0);
		treeWidget_cardDeckStyles->resizeColumnToContents(1);
		treeWidget_cardDeckStyles->resizeColumnToContents(2);
	}
}

void settingsDialogImpl::removeCardDeckStyle()
{
	QTreeWidgetItem* selectedItem = treeWidget_cardDeckStyles->currentItem();
	//never delete PokerTH defaullt Styles
	if(selectedItem && selectedItem->data(0, 16).toInt() == ADDITIONAL_STYLE) {
		// if selected is activated --> swith activation to first default
		if(!selectedItem->icon(0).isNull()) {
			QTreeWidgetItem* firstItem = treeWidget_cardDeckStyles->topLevelItem(0);
			firstItem->setIcon(0, QIcon(QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str())+"/gfx/gui/misc/rating.png"));
		}
		//remove from List
		treeWidget_cardDeckStyles->takeTopLevelItem(treeWidget_cardDeckStyles->currentIndex().row());
	}
}

void settingsDialogImpl::removePlayerFromIgnoredPlayersList()
{
	if(treeWidget_internetGameIgnoredPlayers->selectedItems().count()) {
		treeWidget_internetGameIgnoredPlayers->takeTopLevelItem(treeWidget_internetGameIgnoredPlayers->currentIndex().row());
	}
}

void settingsDialogImpl::resetSettings()
{
	int ret = MyMessageBox::warning(this, tr("PokerTH - Settings"),
									tr("Attention: this will delete all your personal settings and close PokerTH!\n"
									   "Do you really want to reset factory settings?"),
									QMessageBox::Yes | QMessageBox::No);
	if(ret == QMessageBox::Yes) {
		myConfig->deleteConfigFile();
		exit(0);
	}

}

