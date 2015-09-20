#ifndef MYLISTVIEWITEMDATA_H
#define MYLISTVIEWITEMDATA_H

#include <QObject>
#include <QQmlListProperty>

class MyListViewItemData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString myId READ myId WRITE setMyId NOTIFY myIdChanged)
    Q_PROPERTY(QString myTitle READ myTitle WRITE setMyTitle NOTIFY myTitleChanged)
    Q_PROPERTY(QString myType READ myType WRITE setMyType NOTIFY myTypeChanged)
    Q_PROPERTY(QString myValue READ myValue WRITE setMyValue NOTIFY myValueChanged)

    //ComboBox special
    Q_PROPERTY(QStringList myValuesList READ myValuesList WRITE setMyValuesList NOTIFY myValuesListChanged)
    Q_PROPERTY(bool myValueIsIndex READ myValueIsIndex WRITE setMyValueIsIndex NOTIFY myValueIsIndexChanged)

    //SpinBox special
    Q_PROPERTY(QString myMaxValue READ myMaxValue WRITE setMyMaxValue NOTIFY myMaxValueChanged)
    Q_PROPERTY(QString myMinValue READ myMinValue WRITE setMyMinValue NOTIFY myMinValueChanged)
    Q_PROPERTY(QString myPrefix READ myPrefix WRITE setMyPrefix NOTIFY myPrefixChanged)

    //BlindsRaiseInterval-Selector special
    Q_PROPERTY(QString myRaiseOnHandsType READ myRaiseOnHandsType WRITE setMyRaiseOnHandsType NOTIFY myRaiseOnHandsTypeChanged)
    Q_PROPERTY(QString myRaiseOnHandsInterval READ myRaiseOnHandsInterval WRITE setMyRaiseOnHandsInterval NOTIFY myRaiseOnHandsIntervalChanged)
    Q_PROPERTY(QString myRaiseOnMinutesInterval READ myRaiseOnMinutesInterval WRITE setMyRaiseOnMinutesInterval NOTIFY myRaiseOnMinutesIntervalChanged)

    //BlindsRaiseMode-Selector special
    Q_PROPERTY(QString myAlwaysDoubleBlinds READ myAlwaysDoubleBlinds WRITE setMyAlwaysDoubleBlinds NOTIFY myAlwaysDoubleBlindsChanged)
    Q_PROPERTY(QStringList myManualBlindsList READ myManualBlindsList WRITE setMyManualBlindsList NOTIFY myManualBlindsListChanged)
    Q_PROPERTY(QString myAfterMBAlwaysDoubleBlinds READ myAfterMBAlwaysDoubleBlinds WRITE setMyAfterMBAlwaysDoubleBlinds NOTIFY myAfterMBAlwaysDoubleBlindsChanged)
    Q_PROPERTY(QString myAfterMBAlwaysRaiseAbout READ myAfterMBAlwaysRaiseAbout WRITE setMyAfterMBAlwaysRaiseAbout NOTIFY myAfterMBAlwaysRaiseAboutChanged)
    Q_PROPERTY(QString myAfterMBAlwaysRaiseValue READ myAfterMBAlwaysRaiseValue WRITE setMyAfterMBAlwaysRaiseValue NOTIFY myAfterMBAlwaysRaiseValueChanged)
    Q_PROPERTY(QString myAfterMBStayAtLastBlind READ myAfterMBStayAtLastBlind WRITE setMyAfterMBStayAtLastBlind NOTIFY myAfterMBStayAtLastBlindChanged)

    QString m_myId;

    QString m_myTitle;

    QString m_myType;

    QString m_myValue;

    bool m_myValueIsIndex;

    QString m_myMaxValue;

    QString m_myMinValue;

    QString m_myPrefix;

    QString m_myRaiseOnHandsType;

    QString m_myRaiseOnHandsInterval;

    QString m_myRaiseOnMinutesInterval;

    QString m_myAlwaysDoubleBlinds;

    QString m_myAfterMBAlwaysDoubleBlinds;

    QString m_myAfterMBAlwaysRaiseAbout;

    QString m_myAfterMBStayAtLastBlind;

    QString m_myAfterMBAlwaysRaiseValue;

    QStringList m_myManualBlindsList;

    QStringList m_myValuesList;

public:
    inline MyListViewItemData(QObject *parent = 0) : QObject(parent) {}
    inline ~MyListViewItemData() {}

    QString myId() const
    {
        return m_myId;
    }

    QString myTitle() const
    {
        return m_myTitle;
    }

    QString myType() const
    {
        return m_myType;
    }

    QString myValue() const
    {
        return m_myValue;
    }

    bool myValueIsIndex() const
    {
        return m_myValueIsIndex;
    }

    QString myMaxValue() const
    {
        return m_myMaxValue;
    }

    QString myMinValue() const
    {
        return m_myMinValue;
    }

    QString myPrefix() const
    {
        return m_myPrefix;
    }

    QString myRaiseOnHandsType() const
    {
        return m_myRaiseOnHandsType;
    }

    QString myRaiseOnHandsInterval() const
    {
        return m_myRaiseOnHandsInterval;
    }

    QString myRaiseOnMinutesInterval() const
    {
        return m_myRaiseOnMinutesInterval;
    }

    QString myAlwaysDoubleBlinds() const
    {
        return m_myAlwaysDoubleBlinds;
    }

    QString myAfterMBAlwaysDoubleBlinds() const
    {
        return m_myAfterMBAlwaysDoubleBlinds;
    }

    QString myAfterMBAlwaysRaiseAbout() const
    {
        return m_myAfterMBAlwaysRaiseAbout;
    }

    QString myAfterMBStayAtLastBlind() const
    {
        return m_myAfterMBStayAtLastBlind;
    }

    QString myAfterMBAlwaysRaiseValue() const
    {
        return m_myAfterMBAlwaysRaiseValue;
    }

    QStringList myManualBlindsList() const
    {
        return m_myManualBlindsList;
    }

    void appendToMyManualBlindsList(QString str)
    {
        m_myManualBlindsList.append(str);
    }

    void clearMyManualBlindsList()
    {
        m_myManualBlindsList.clear();
    }

    QStringList myValuesList() const
    {
        return m_myValuesList;
    }

    void appendToMyValuesList(QString str)
    {
        m_myValuesList.append(str);
    }

    void clearMyValuesList()
    {
        m_myValuesList.clear();
    }
signals:

    void myIdChanged(QString myId);

    void myTitleChanged(QString myTitle);

    void myTypeChanged(QString myType);

    void myValueChanged(QString myValue);

    void myValueIsIndexChanged(bool myValueIsIndex);

    void myMaxValueChanged(QString myMaxValue);

    void myMinValueChanged(QString myMinValue);

    void myPrefixChanged(QString myPrefix);

    void myRaiseOnHandsTypeChanged(QString myRaiseOnHandsType);

    void myRaiseOnHandsIntervalChanged(QString myRaiseOnHandsInterval);

    void myRaiseOnMinutesIntervalChanged(QString myRaiseOnMinutesInterval);

    void myAlwaysDoubleBlindsChanged(QString myAlwaysDoubleBlinds);

    void myAfterMBAlwaysDoubleBlindsChanged(QString myAfterMBAlwaysDoubleBlinds);

    void myAfterMBAlwaysRaiseAboutChanged(QString myAfterMBAlwaysRaiseAbout);

    void myAfterMBAlwaysRaiseValueChanged(QString myAfterMBAlwaysRaiseValue);

    void myAfterMBStayAtLastBlindChanged(QString myAfterMBStayAtLastBlind);

    void myManualBlindsListChanged(QStringList myManualBlindsList);

    void myValuesListChanged(QStringList myValuesList);

public slots:
    void setMyId(QString myId)
    {
        if (m_myId == myId)
            return;

        m_myId = myId;
        emit myIdChanged(myId);
    }
    void setMyTitle(QString myTitle)
    {
        if (m_myTitle == myTitle)
            return;

        m_myTitle = myTitle;
        emit myTitleChanged(myTitle);
    }
    void setMyType(QString myType)
    {
        if (m_myType == myType)
            return;

        m_myType = myType;
        emit myTypeChanged(myType);
    }
    void setMyValue(QString myValue)
    {
        if (m_myValue == myValue)
            return;

        m_myValue = myValue;
        emit myValueChanged(myValue);
    }
    void setMyValueIsIndex(bool myValueIsIndex)
    {
        if (m_myValueIsIndex == myValueIsIndex)
            return;

        m_myValueIsIndex = myValueIsIndex;
        emit myValueIsIndexChanged(myValueIsIndex);
    }
    void setMyMaxValue(QString myMaxValue)
    {
        if (m_myMaxValue == myMaxValue)
            return;

        m_myMaxValue = myMaxValue;
        emit myMaxValueChanged(myMaxValue);
    }
    void setMyMinValue(QString myMinValue)
    {
        if (m_myMinValue == myMinValue)
            return;

        m_myMinValue = myMinValue;
        emit myMinValueChanged(myMinValue);
    }
    void setMyPrefix(QString myPrefix)
    {
        if (m_myPrefix == myPrefix)
            return;

        m_myPrefix = myPrefix;
        emit myPrefixChanged(myPrefix);
    }
    void setMyRaiseOnHandsType(QString myRaiseOnHandsType)
    {
        if (m_myRaiseOnHandsType == myRaiseOnHandsType)
            return;

        m_myRaiseOnHandsType = myRaiseOnHandsType;
        emit myRaiseOnHandsTypeChanged(myRaiseOnHandsType);
    }
    void setMyRaiseOnHandsInterval(QString myRaiseOnHandsInterval)
    {
        if (m_myRaiseOnHandsInterval == myRaiseOnHandsInterval)
            return;

        m_myRaiseOnHandsInterval = myRaiseOnHandsInterval;
        emit myRaiseOnHandsIntervalChanged(myRaiseOnHandsInterval);
    }
    void setMyRaiseOnMinutesInterval(QString myRaiseOnMinutesInterval)
    {
        if (m_myRaiseOnMinutesInterval == myRaiseOnMinutesInterval)
            return;

        m_myRaiseOnMinutesInterval = myRaiseOnMinutesInterval;
        emit myRaiseOnMinutesIntervalChanged(myRaiseOnMinutesInterval);
    }
    void setMyAlwaysDoubleBlinds(QString myAlwaysDoubleBlinds)
    {
        if (m_myAlwaysDoubleBlinds == myAlwaysDoubleBlinds)
            return;

        m_myAlwaysDoubleBlinds = myAlwaysDoubleBlinds;
        emit myAlwaysDoubleBlindsChanged(myAlwaysDoubleBlinds);
    }
    void setMyAfterMBAlwaysDoubleBlinds(QString myAfterMBAlwaysDoubleBlinds)
    {
        if (m_myAfterMBAlwaysDoubleBlinds == myAfterMBAlwaysDoubleBlinds)
            return;

        m_myAfterMBAlwaysDoubleBlinds = myAfterMBAlwaysDoubleBlinds;
        emit myAfterMBAlwaysDoubleBlindsChanged(myAfterMBAlwaysDoubleBlinds);
    }
    void setMyAfterMBAlwaysRaiseAbout(QString myAfterMBAlwaysRaiseAbout)
    {
        if (m_myAfterMBAlwaysRaiseAbout == myAfterMBAlwaysRaiseAbout)
            return;

        m_myAfterMBAlwaysRaiseAbout = myAfterMBAlwaysRaiseAbout;
        emit myAfterMBAlwaysRaiseAboutChanged(myAfterMBAlwaysRaiseAbout);
    }
    void setMyAfterMBAlwaysRaiseValue(QString myAfterMBAlwaysRaiseValue)
    {
        if (m_myAfterMBAlwaysRaiseValue == myAfterMBAlwaysRaiseValue)
            return;

        m_myAfterMBAlwaysRaiseValue = myAfterMBAlwaysRaiseValue;
        emit myAfterMBAlwaysRaiseValueChanged(myAfterMBAlwaysRaiseValue);
    }    void setMyAfterMBStayAtLastBlind(QString myAfterMBStayAtLastBlind)
    {
        if (m_myAfterMBStayAtLastBlind == myAfterMBStayAtLastBlind)
            return;

        m_myAfterMBStayAtLastBlind = myAfterMBStayAtLastBlind;
        emit myAfterMBStayAtLastBlindChanged(myAfterMBStayAtLastBlind);
    }

    void setMyManualBlindsList(QStringList myManualBlindsList)
    {
        if (m_myManualBlindsList == myManualBlindsList)
            return;

        m_myManualBlindsList = myManualBlindsList;
        emit myManualBlindsListChanged(myManualBlindsList);
    }
    void setMyValuesList(QStringList myValuesList)
    {
        if (m_myValuesList == myValuesList)
            return;

        m_myValuesList = myValuesList;
        emit myValuesListChanged(myValuesList);
    }
};

#endif // MYLISTVIEWITEMDATA_H
