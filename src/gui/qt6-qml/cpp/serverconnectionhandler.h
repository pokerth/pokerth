/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/

#ifndef SERVERCONNECTIONHANDLER_H
#define SERVERCONNECTIONHANDLER_H

#include <QObject>
#include <QString>
#include <boost/shared_ptr.hpp>

// Forward declarations
class Session;
class ConfigFile;

class ServerConnectionHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int connectionProgress READ connectionProgress NOTIFY connectionProgressChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool isConnecting READ isConnecting NOTIFY isConnectingChanged)
    Q_PROPERTY(QString savedUsername READ savedUsername NOTIFY savedUsernameChanged)
    Q_PROPERTY(QString savedPassword READ savedPassword NOTIFY savedPasswordChanged)
    Q_PROPERTY(bool rememberPassword READ rememberPassword NOTIFY rememberPasswordChanged)

public:
    explicit ServerConnectionHandler(QObject *parent = nullptr);
    virtual ~ServerConnectionHandler();

    void setSession(boost::shared_ptr<Session> session);
    void setConfig(ConfigFile *config);

    int connectionProgress() const { return m_connectionProgress; }
    QString statusMessage() const { return m_statusMessage; }
    bool isConnecting() const { return m_isConnecting; }
    QString savedUsername() const { return m_savedUsername; }
    QString savedPassword() const { return m_savedPassword; }
    bool rememberPassword() const { return m_rememberPassword; }

public slots:
    // Called from QML to start connection
    void connectToServer(const QString &username, const QString &password, bool isGuest, bool rememberPassword = false);
    void cancelConnection();
    void loadCredentials();
    void saveCredentials(const QString &username, const QString &password, bool rememberPassword);

signals:
    void connectionProgressChanged(int progress);
    void statusMessageChanged(const QString &message);
    void isConnectingChanged(bool connecting);
    void connectionSucceeded();
    void connectionFailed(const QString &errorMessage);
    void showLobby();
    void savedUsernameChanged();
    void savedPasswordChanged();
    void rememberPasswordChanged();

public slots:
    void onNetClientConnect(int actionID);
    void onNetClientLoginShow();
    void onNetClientError(int errorID, int osErrorID);

private:
    void updateProgress(int progress, const QString &message);
    void handleLoginDialog();

    boost::shared_ptr<Session> m_session;
    ConfigFile *m_config;
    
    int m_connectionProgress;
    QString m_statusMessage;
    bool m_isConnecting;
    
    QString m_pendingUsername;
    QString m_pendingPassword;
    bool m_pendingIsGuest;
    
    QString m_savedUsername;
    QString m_savedPassword;
    bool m_rememberPassword;
    
    int m_retryCount;
};

#endif // SERVERCONNECTIONHANDLER_H
