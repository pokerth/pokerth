#include "createlocalgameviewimpl.h"
#include "startviewimpl.h"
#include "configfile.h"
#include "mylistviewitemdata.h"
#include "gamedata.h"
#include <QtQml>

CreateLocalGameViewImpl::CreateLocalGameViewImpl(QObject *parent, QQmlApplicationEngine *e, boost::shared_ptr<ConfigFile> c, StartViewImpl *s )
    : QObject(parent) ,myQmlEngine(e), myConfig(c), myStartView(s)
{
    //Build the listView data structure
    MyListViewItemData *item = NULL;

    //ComboBox number of players
    item = new MyListViewItemData;
    item->setMyId("comboBox_NumberOfPlayers");
    item->setMyTitle(tr("Number of players"));
    item->setMyType("ComboBox");
    const int n = 9;
    for (int i = 10; i > (10-n); --i) item->appendToMyValuesList(QString::number(i));
    item->setMyValueIsIndex(false);
    myListData.append(item);

    //SpinBox start cash-
    item = new MyListViewItemData;
    item->setMyId("spinBox_StartCash");
    item->setMyTitle(tr("Start cash"));
    item->setMyType("SpinBox");
    item->setMyMaxValue("1000000");
    item->setMyMinValue("1000");
    item->setMyPrefix("$");
    myListData.append(item);

    //SpinBox start blind
    item = new MyListViewItemData;
    item->setMyId("spinBox_FirstSmallBlind");
    item->setMyTitle(tr("First small blind"));
    item->setMyType("SpinBox");
    item->setMyMaxValue("20000");
    item->setMyMinValue("5");
    item->setMyPrefix("$");
    myListData.append(item);

    //Blinds raise interval selector
    item = new MyListViewItemData;
    item->setMyId("selector_BlindsRaiseInterval");
    item->setMyTitle(tr("Blinds raise interval"));
    item->setMyType("BlindsRaiseInterval");
    myListData.append(item);

    //Blinds raise mode selector
    item = new MyListViewItemData;
    item->setMyId("selector_BlindsRaiseMode");
    item->setMyTitle(tr("Blinds raise mode"));
    item->setMyType("BlindsRaiseMode");
    myListData.append(item);

    //ComboBox game speed
    item = new MyListViewItemData;
    item->setMyId("comboBox_GameSpeed");
    item->setMyTitle(tr("Game speed"));
    item->setMyType("ComboBox");
    const int m = 11;
    for (int i = 11; i > (11-m); --i) item->appendToMyValuesList(QString::number(i));
    item->setMyValueIsIndex(false);
    myListData.append(item);

    myQmlEngine->rootContext()->setContextProperty("CreateLocalGameViewData", QVariant::fromValue(myListData));
}

CreateLocalGameViewImpl::~CreateLocalGameViewImpl()
{

}

MyListViewItemData* CreateLocalGameViewImpl::listItem(QString id)
{
    for (int i = 0; i < myListData.size(); ++i) {
        if (static_cast<MyListViewItemData*>(myListData.at(i))->myId() == id)
            return static_cast<MyListViewItemData*>(myListData.at(i));
    }
    qDebug("ERROR: listItem in createLocalGameViewImpl could not be found");
    return NULL;
}


void CreateLocalGameViewImpl::readConfigValues()
{
    listItem("comboBox_NumberOfPlayers")->setMyValue(QString::number(myConfig->readConfigInt("NumberOfPlayers")));
    listItem("spinBox_StartCash")->setMyValue(QString::number(myConfig->readConfigInt("StartCash")));
    listItem("spinBox_FirstSmallBlind")->setMyValue(QString::number(myConfig->readConfigInt("FirstSmallBlind")));
    listItem("selector_BlindsRaiseInterval")->setMyRaiseOnHandsType(QString::number(myConfig->readConfigInt("RaiseBlindsAtHands")));
    listItem("selector_BlindsRaiseInterval")->setMyRaiseOnHandsInterval(QString::number(myConfig->readConfigInt("RaiseSmallBlindEveryHands")));
    listItem("selector_BlindsRaiseInterval")->setMyRaiseOnMinutesInterval(QString::number(myConfig->readConfigInt("RaiseSmallBlindEveryMinutes")));
    listItem("selector_BlindsRaiseMode")->setMyAlwaysDoubleBlinds(QString::number(myConfig->readConfigInt("AlwaysDoubleBlinds")));

    std::list<int> myList = myConfig->readConfigIntList("ManualBlindsList");
    std::list<int>::iterator it1;
    listItem("selector_BlindsRaiseMode")->clearMyManualBlindsList();
    for(it1= myList.begin(); it1 != myList.end(); ++it1) {
        listItem("selector_BlindsRaiseMode")->appendToMyManualBlindsList(QString::number(*it1));
    }

    listItem("selector_BlindsRaiseMode")->setMyAfterMBAlwaysDoubleBlinds(QString::number(myConfig->readConfigInt("AfterMBAlwaysDoubleBlinds")));
    listItem("selector_BlindsRaiseMode")->setMyAfterMBAlwaysRaiseAbout(QString::number(myConfig->readConfigInt("AfterMBAlwaysRaiseAbout")));
    listItem("selector_BlindsRaiseMode")->setMyAfterMBAlwaysRaiseValue(QString::number(myConfig->readConfigInt("AfterMBAlwaysRaiseValue")));
    listItem("selector_BlindsRaiseMode")->setMyAfterMBStayAtLastBlind(QString::number(myConfig->readConfigInt("AfterMBStayAtLastBlind")));
    listItem("comboBox_GameSpeed")->setMyValue(QString::number(myConfig->readConfigInt("GameSpeed")));
}

void CreateLocalGameViewImpl::startGame()
{
    qDebug() << "Start Game from CreateLocalGameViewImpl:";
    qDebug() << "Number of players: " << listItem("comboBox_NumberOfPlayers")->myValue();

    GameData gameData;
    gameData.maxNumberOfPlayers = listItem("comboBox_NumberOfPlayers")->myValue().toInt();
    gameData.startMoney = listItem("spinBox_StartCash")->myValue().toInt();
    gameData.firstSmallBlind = listItem("spinBox_FirstSmallBlind")->myValue().toInt();
    if(listItem("selector_BlindsRaiseInterval")->myRaiseOnHandsType() == "1") {
        gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
        gameData.raiseSmallBlindEveryHandsValue = listItem("selector_BlindsRaiseInterval")->myRaiseOnHandsInterval().toInt();
    } else {
        gameData.raiseIntervalMode = RAISE_ON_MINUTES;
        gameData.raiseSmallBlindEveryMinutesValue = listItem("selector_BlindsRaiseInterval")->myRaiseOnMinutesInterval().toInt();
    }

    if(listItem("selector_BlindsRaiseMode")->myAlwaysDoubleBlinds() == "1") {
        gameData.raiseMode = DOUBLE_BLINDS;
    } else {
        gameData.raiseMode = MANUAL_BLINDS_ORDER;
        std::list<int> tempBlindList;
        int i;
        bool ok = true;
        for(i = 0; i < listItem("selector_BlindsRaiseMode")->myManualBlindsList().count(); i++) {
            tempBlindList.push_back(listItem("selector_BlindsRaiseMode")->myManualBlindsList().at(i).toInt());
        }
        gameData.manualBlindsList = tempBlindList;

        if(listItem("selector_BlindsRaiseMode")->myAfterMBAlwaysDoubleBlinds() == "1") {
            gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS;
        } else {
            if(listItem("selector_BlindsRaiseMode")->myAfterMBAlwaysRaiseAbout() == "1") {
                gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
                gameData.afterMBAlwaysRaiseValue = listItem("selector_BlindsRaiseMode")->myAfterMBAlwaysRaiseValue().toInt();
            } else {
                gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND;
            }
        }
    }
    gameData.guiSpeed = listItem("comboBox_GameSpeed")->myValue().toInt();
    gameData.delayBetweenHandsSec = 7;
    gameData.playerActionTimeoutSec = 20;

    myStartView->startLocalGame(gameData);
}
