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
#include <QUrl>
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
	Q_PROPERTY(QUrl registerUrl READ registerUrl CONSTANT)
	// True between a connection loss during operation and the end of the
	// automatic reconnect (successful or given up).
	Q_PROPERTY(bool reconnecting READ reconnecting NOTIFY reconnectingChanged)

public:
	explicit ServerConnectionHandler(QObject *parent = nullptr);
	virtual ~ServerConnectionHandler();

	void setSession(boost::shared_ptr<Session> session);
	void setConfig(ConfigFile *config);

	int connectionProgress() const
	{
		return m_connectionProgress;
	}
	QString statusMessage() const
	{
		return m_statusMessage;
	}
	bool isConnecting() const
	{
		return m_isConnecting;
	}
	QString savedUsername() const
	{
		return m_savedUsername;
	}
	QString savedPassword() const
	{
		return m_savedPassword;
	}
	bool rememberPassword() const
	{
		return m_rememberPassword;
	}
	bool reconnecting() const
	{
		return m_reconnecting;
	}
	QUrl registerUrl() const
	{
		return QUrl(QStringLiteral("https://www.pokerth.net/ucp.php?mode=register"));
	}
	Q_INVOKABLE bool openExternalUrl(const QUrl &url) const;
	// Cancels a running reconnect and ends the session for good. From QML
	// when deliberately leaving and via the cancel button of the reconnect
	// notice.
	Q_INVOKABLE void abortAutoReconnect();

	// Plain text for an error code from socket_msg.h (ERR_SOCK_*/ERR_NET_*).
	// Counterpart to startWindowImpl::networkError(int) in the widgets client;
	// there every code has a message of its own, here only a handful of codes
	// were available as text (the rest arrived as a bare number).
	static QString networkErrorMessage(int errorID);

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
	void reconnectingChanged();
	// Every single reconnect attempt, for the progress display.
	void reconnectAttempt(int attempt, int maxAttempts);
	// Arming the automatic rejoin in the LobbyHandler: the offer of the
	// server (InitAck) already arrives during the login, i.e. BEFORE the end
	// of the reconnect - the flag has to be set before that.
	void autoRejoinArmed(bool armed);

public slots:
	void onNetClientConnect(int actionID);
	void onNetClientLoginShow();
	void onNetClientError(int errorID, int osErrorID);
	// Errors of the embedded server (hosting your own network game). Without
	// this path an occupied port when hosting went unnoticed, for example.
	void onNetServerError(int errorID, int osErrorID);

private:
	void updateProgress(int progress, const QString &message);
	void handleLoginDialog();
	// Only transport errors justify a silent reconnect. On kick, ban, block,
	// session timeout or rejected credentials the client would otherwise keep
	// running against the rejection - and would trigger the login rate limit
	// of the server (token bucket per IP) in the process.
	static bool isRecoverableTransportError(int errorID);
	// Starts the next attempt or gives up; returns true if an attempt is
	// running and the error is therefore NOT reported to QML.
	bool scheduleAutoReconnect(int errorID);
	void endAutoReconnect();

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

	// Automatic reconnect after a connection loss during operation
	// (Android: app in the background; desktop: WLAN sleep).
	bool m_loggedIn = false;      // login finished -> an abort is a loss,
	// not a failed connection attempt
	bool m_reconnecting = false;
	int  m_reconnectAttempt = 0;
};

#endif // SERVERCONNECTIONHANDLER_H
