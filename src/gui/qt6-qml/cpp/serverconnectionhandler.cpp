#include "serverconnectionhandler.h"
#include "session.h"
#include "configfile.h"
#include "core/appimage_utils.h"
#include <net/socket_msg.h>
#include <QByteArray>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTimer>
#include <QDebug>

ServerConnectionHandler::ServerConnectionHandler(QObject *parent)
	: QObject(parent)
	, m_session(nullptr)
	, m_config(nullptr)
	, m_connectionProgress(0)
	, m_statusMessage("")
	, m_isConnecting(false)
	, m_pendingIsGuest(false)
	, m_savedUsername("")
	, m_savedPassword("")
	, m_rememberPassword(false)
	, m_retryCount(0)
{
}

ServerConnectionHandler::~ServerConnectionHandler()
{
}

void ServerConnectionHandler::setSession(boost::shared_ptr<Session> session)
{
	m_session = session;
}

void ServerConnectionHandler::setConfig(ConfigFile *config)
{
	m_config = config;

	// Load saved credentials when config is set
	if (m_config) {
		loadCredentials();
	}
}

void ServerConnectionHandler::connectToServer(const QString &username, const QString &password, bool isGuest, bool rememberPassword)
{
	if (!m_session) {
		qWarning() << "ServerConnectionHandler: Cannot connect - no session!";
		updateProgress(0, tr("Error: No session available"));
		emit connectionFailed(tr("No session available"));
		return;
	}

	// Guest login must not overwrite persisted user credentials.
	if (!isGuest) {
		saveCredentials(username, password, rememberPassword);
	}

	// Store pending credentials for retry/reuse
	m_pendingUsername = username;
	m_pendingPassword = password;
	m_pendingIsGuest = isGuest;
	m_retryCount = 0;

	// Manual login: a reconnect that may still be running is thereby
	// finished, and a rejoin offer belongs to the player again from here on -
	// accepted unasked it would otherwise force them into an old table.
	m_loggedIn = false;
	endAutoReconnect();
	emit autoRejoinArmed(false);

	m_isConnecting = true;
	emit isConnectingChanged(true);
	updateProgress(10, tr("Connecting to server..."));

	// Terminate any existing network client before starting a new one
	boost::shared_ptr<Session> sess = m_session;
	sess->terminateNetworkClient();

	// Start the internet client connection (like the old GUI does)
	sess->startInternetClient();
}

void ServerConnectionHandler::cancelConnection()
{
	if (!m_session) {
		return;
	}

	m_isConnecting = false;
	emit isConnectingChanged(false);
	updateProgress(0, tr("Connection canceled"));

	// TODO: Implement actual cancellation logic with Session
}

bool ServerConnectionHandler::openExternalUrl(const QUrl &url) const
{
	if (!url.isValid())
		return false;

#ifdef Q_OS_LINUX
	const QString targetString = url.toString();

	// External host tools must not inherit bundled Qt libraries.
	// Restore the original LD_LIBRARY_PATH (saved by the launcher) or strip it
	// entirely so system tools like xdg-open / kde-open work correctly.
	auto startDetachedHostTool = [](const QString &program, const QStringList &args) {
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		const QString origLdLibraryPath = QString::fromLocal8Bit(qgetenv("POKERTH_ORIG_LD_LIBRARY_PATH"));
		if (origLdLibraryPath.isEmpty())
			env.remove(QStringLiteral("LD_LIBRARY_PATH"));
		else
			env.insert(QStringLiteral("LD_LIBRARY_PATH"), origLdLibraryPath);
		env.remove(QStringLiteral("LD_PRELOAD"));
		QProcess process;
		process.setProcessEnvironment(env);
		process.setProgram(program);
		process.setArguments(args);
		return process.startDetached();
	};

	if (startDetachedHostTool(QStringLiteral("xdg-open"), {targetString}))
		return true;
	if (startDetachedHostTool(QStringLiteral("gio"), {QStringLiteral("open"), targetString}))
		return true;
	if (startDetachedHostTool(QStringLiteral("kde-open"), {targetString}))
		return true;
#endif

	return AppImageUtils::openUrlSafe(url);
}

void ServerConnectionHandler::updateProgress(int progress, const QString &message)
{
	if (m_connectionProgress != progress) {
		m_connectionProgress = progress;
		emit connectionProgressChanged(progress);
	}

	if (m_statusMessage != message) {
		m_statusMessage = message;
		emit statusMessageChanged(message);
	}
}

void ServerConnectionHandler::handleLoginDialog()
{
	// This is called when the server requires login credentials
	// We have them from connectToServer, now send them to the session
	if (!m_session) {
		qWarning() << "ServerConnectionHandler: No session in handleLoginDialog!";
		return;
	}

	if (!m_pendingUsername.isEmpty()) {
		updateProgress(30, tr("Authenticating..."));

		// Send login credentials to the session (like the old GUI does)
		m_session->setLogin(
			m_pendingUsername.toStdString(),
			m_pendingPassword.toStdString(),
			m_pendingIsGuest
		);
	}
}

void ServerConnectionHandler::onNetClientConnect(int actionID)
{
	// Update progress based on connection phase
	// 1 = MSG_SOCK_INIT_DONE, 2 = MSG_SOCK_SERVER_LIST_DONE,
	// 3 = MSG_SOCK_RESOLVE_DONE, 4 = MSG_SOCK_CONNECT_DONE, 5 = MSG_SOCK_SESSION_DONE
	switch (actionID) {
	case 1: // MSG_SOCK_INIT_DONE
		updateProgress(20, tr("Initialized..."));
		break;
	case 2: // MSG_SOCK_SERVER_LIST_DONE
		updateProgress(40, tr("Server list received..."));
		break;
	case 3: // MSG_SOCK_RESOLVE_DONE
		updateProgress(50, tr("Resolving server..."));
		break;
	case 4: // MSG_SOCK_CONNECT_DONE
		updateProgress(70, tr("Connecting..."));
		break;
	case 5: // MSG_SOCK_SESSION_DONE - Final successful connection
		m_isConnecting = false;
		emit isConnectingChanged(false);
		updateProgress(100, tr("Connected successfully!"));
		// From here on an abort is a connection loss and not a
		// failed connection attempt - only that justifies the
		// automatic reconnect.
		m_loggedIn = true;
		m_retryCount = 0;
		// The rejoin flag deliberately stays armed: InitAck can deliver the
		// offer shortly after this point as well. It is disarmed
		// by the LobbyHandler after use or on a manual login.
		endAutoReconnect();
		emit connectionSucceeded();
		emit showLobby();
		break;
	default:
		break;
	}
}

void ServerConnectionHandler::onNetClientLoginShow()
{
	handleLoginDialog();
}

QString ServerConnectionHandler::networkErrorMessage(int errorID)
{
	// Texts as in startWindowImpl::networkError(int) in the widgets client; the
	// codes come from socket_msg.h (no longer as bare numbers that
	// could silently shift against the header values).
	switch (errorID) {

	// ── Socket-/Verbindungsebene ─────────────────────────────────────────
	case ERR_SOCK_SERVERADDR_NOT_SET:
		return tr("Server address was not set.");
	case ERR_SOCK_INVALID_PORT:
		return tr("An invalid port was set (ports 0-1023 are not allowed).");
	case ERR_SOCK_CREATION_FAILED:
		return tr("Could not create a socket for TCP communication.");
	case ERR_SOCK_SET_ADDR_FAILED:
		return tr("Could not set the IP address.");
	case ERR_SOCK_SET_PORT_FAILED:
		return tr("Could not set the port for this type of address.");
	case ERR_SOCK_RESOLVE_FAILED:
		return tr("The server name could not be resolved.");
	case ERR_SOCK_BIND_FAILED:
		return tr("Bind failed - please choose a different port.");
	case ERR_SOCK_LISTEN_FAILED:
		return tr("Internal network error: \"listen\" failed.");
	case ERR_SOCK_ACCEPT_FAILED:
		return tr("Server execution was terminated.");
	case ERR_SOCK_CONNECT_FAILED:
	case ERR_SOCK_CONNECT_IPV6_FAILED:
		return tr("Could not connect to the server.\nThe server might still be processing a previous connection attempt.\nPlease wait a moment and try again.");
	case ERR_SOCK_CONNECT_TIMEOUT:
	case ERR_SOCK_CONNECT_IPV6_TIMEOUT:
		return tr("Connection timed out.\nThe server might be busy or still processing a previous connection.\nPlease wait a moment and try again.");
	case ERR_SOCK_SELECT_FAILED:
		return tr("Internal network error: \"select\" failed.");
	case ERR_SOCK_SEND_FAILED:
		return tr("Internal network error: \"send\" failed.");
	case ERR_SOCK_RECV_FAILED:   // On closing, Windows sometimes reports recv errors.
	case ERR_SOCK_CONN_RESET:
		// The connection was lost during the session (e.g. WLAN gone). It is
		// shown after the login by the global connectionLostPopup in pokerth.qml.
		// angezeigt.
		return tr("The connection to the server was lost.");
	case ERR_SOCK_CONN_EXISTS:
		return tr("Internal network error: Duplicate TCP connection.");
	case ERR_SOCK_INVALID_PACKET:
		return tr("An invalid network packet was received.\nPlease make sure that all players use the same version of PokerTH.");
	case ERR_SOCK_INVALID_STATE:
		return tr("Internal state error.\nPlease make sure that all players use the same version of PokerTH.");
	case ERR_SOCK_INVALID_TYPE:
		return tr("Internal network error: invalid socket type.");
	case ERR_SOCK_INVALID_SERVERLIST_URL:
	case ERR_SOCK_TRANSFER_INVALID_URL:
		return tr("Invalid server list URL.\nPlease correct the address in the settings.");
	case ERR_SOCK_INVALID_SERVERLIST_XML:
		return tr("The PokerTH internet server list contains invalid data.\nIf you use a custom server list, please make sure its format is correct.");
	case ERR_SOCK_UNZIP_FAILED:
		return tr("Could not unzip the PokerTH internet server list.");
	case ERR_SOCK_TRANSFER_INIT_FAILED:
	case ERR_SOCK_TRANSFER_SELECT_FAILED:
	case ERR_SOCK_TRANSFER_FAILED:
		return tr("Could not download the PokerTH internet server list.\nPlease make sure you are directly connected to the internet.");
	case ERR_SOCK_TRANSFER_OPEN_FAILED:
		return tr("Could not open the target file when downloading the server list.");

	// ── Login / Spielebene ───────────────────────────────────────────────
	case ERR_NET_VERSION_NOT_SUPPORTED:
		return tr("The PokerTH server does not support this version of the game.\nPlease go to https://www.pokerth.net and download the latest version.");
	case ERR_NET_SERVER_MAINTENANCE:
		return tr("The server is down for maintenance. Please try again later.");
	case ERR_NET_SERVER_FULL:
		return tr("Sorry, this server is already full.");
	case ERR_NET_INVALID_PASSWORD:
		return tr("Invalid login.\nPlease check your username and password.");
	case ERR_NET_INVALID_PASSWORD_STR:
		return tr("The password is too long. Please choose another one.");
	case ERR_NET_PLAYER_NAME_IN_USE:
		// The widgets client opens the change-nickname dialog here; here the
		// login page stays, on which the name can be changed directly.
		return tr("This player name is already in use. Please choose a different name.");
	case ERR_NET_INVALID_PLAYER_NAME:
		return tr("The player name is invalid. Please choose a different name.");
	case ERR_NET_INVALID_PLAYER_CARDS:
		return tr("Internal error: invalid player cards.");
	case ERR_NET_INVALID_PLAYER_RESULTS:
		return tr("Internal error: invalid player results.");
	case ERR_NET_INVALID_GAME_NAME:
		return tr("The game name is either too short or too long. Please choose another one.");
	case ERR_NET_INVALID_GAME_ROUND:
		return tr("Internal error: invalid game round.");
	case ERR_NET_INVALID_SESSION:
		return tr("Internal error: invalid session.");
	case ERR_NET_UNKNOWN_GAME:
		return tr("The game could not be found.");
	case ERR_NET_INVALID_CHAT_TEXT:
		return tr("The chat text is invalid.");
	case ERR_NET_UNKNOWN_PLAYER_ID:
		return tr("The server referred to an unknown player. Aborting.");
	case ERR_NET_NO_CURRENT_PLAYER:
		return tr("Internal error: The current player could not be found.");
	case ERR_NET_PLAYER_NOT_ACTIVE:
		return tr("Internal error: The current player is not active.");
	case ERR_NET_PLAYER_KICKED:
		return tr("You were kicked from the server.");
	case ERR_NET_PLAYER_BANNED:
		return tr("You were temporarily banned from the server.");
	case ERR_NET_PLAYER_BLOCKED:
		return tr("Your account is blocked indefinitely.");
	case ERR_NET_SESSION_TIMED_OUT:
		return tr("Your server connection timed out due to inactivity. You are very welcome to reconnect!");
	case ERR_NET_INVALID_PLAYER_COUNT:
		return tr("The client player count is invalid.");
	case ERR_NET_TOO_MANY_MANUAL_BLINDS:
		return tr("Too many manual blinds were set. Please reconfigure the manual blinds.");
	case ERR_NET_INVALID_AVATAR_FILE:
	case ERR_NET_WRONG_AVATAR_SIZE:
		return tr("An invalid avatar file was configured. Please choose a different avatar.");
	case ERR_NET_AVATAR_TOO_LARGE:
		return tr("The selected avatar file is too large. Please choose a different avatar.");
	case ERR_NET_BUF_INVALID_SIZE:
		return tr("Internal error: invalid buffer size.");
	case ERR_NET_INVALID_REQUEST_ID:
		return tr("An internal avatar error occured. Please report this to an admin in the lobby chat.");
	case ERR_NET_START_TIMEOUT:
		return tr("Could not start game: Synchronization failed.");
	case ERR_NET_GAME_TERMINATION_FAILED:
		return tr("The game could not be terminated.");
	case ERR_NET_INTERNAL_GAME_ERROR:
		return tr("An internal game error occured.");
	case ERR_NET_DEALER_NOT_FOUND:
		return tr("Internal error: The dealer could not be found.");
	case ERR_NET_INIT_BLOCKED:
		return tr("You cannot login at this time. Please try again in a few seconds.");
	case ERR_NET_GSASL_INIT_FAILED:
		return tr("Internal error: The authentication could not be initialized.");
	case ERR_NET_GSASL_NO_SCRAM:
		return tr("The server does not support the required authentication method.");
	case ERR_NET_DB_CONNECT_FAILED:
		return tr("The server could not reach its database. Please try again later.");

	// ERR_SOCK_INTERNAL and everything unknown: name the code as well, so that a
	// report from the field can still be assigned (no log on mobile devices).
	default:
		return tr("An internal error occured. (Error code %1)").arg(errorID);
	}
}

void ServerConnectionHandler::onNetServerError(int errorID, int osErrorID)
{
	Q_UNUSED(osErrorID);

	qWarning() << "ServerConnectionHandler: Server error:" << errorID;

	m_isConnecting = false;
	emit isConnectingChanged(false);
	const QString errorMsg = networkErrorMessage(errorID);
	updateProgress(0, errorMsg);
	emit connectionFailed(errorMsg);
}

void ServerConnectionHandler::onNetClientError(int errorID, int osErrorID)
{
	Q_UNUSED(osErrorID);

	qWarning() << "ServerConnectionHandler: Network error:" << errorID << "retry count:" << m_retryCount;

	// ERR_SOCK_CONNECT_FAILED is often a TLS handshake issue that succeeds on retry.
	// Only while establishing the connection: after the login the automatic
	// reconnect below takes over, otherwise both mechanisms would run against each other.
	if (errorID == ERR_SOCK_CONNECT_FAILED && !m_loggedIn && m_retryCount < 1 && !m_pendingUsername.isEmpty()) {
		m_retryCount++;
		const int scheduledRetryCount = m_retryCount;
		updateProgress(15, tr("Connection failed, retrying..."));

		// Wait a moment before retrying
		QTimer::singleShot(2000, this, [this, scheduledRetryCount]() {
			// Ignore stale retry timers after a successful connection or a new connect attempt.
			if (!m_session || !m_isConnecting || m_retryCount != scheduledRetryCount) {
				return;
			}

			updateProgress(20, tr("Retrying connection..."));

			// Terminate previous attempt and retry
			m_session->terminateNetworkClient();
			m_session->startInternetClient();
		});
		return;
	}

	// Connection loss during operation: reconnect silently instead of
	// throwing the player back onto the login page. The server holds the seat at the
	// table for 5 minutes (SERVER_OFFLINE_RECONNECT_TIMEOUT_SEC in servergame.cpp),
	// the credentials are still in m_pending* - a silent re-login is
	// therefore possible even without "remember password".
	if (scheduleAutoReconnect(errorID))
		return;

	// Given up or hopeless from the start (kick/ban/timeout): the
	// session is over for good, a later rejoin offer must not be
	// accepted automatically any more.
	if (m_reconnecting)
		qWarning() << "[RECONNECT] giving up after" << m_reconnectAttempt << "attempts";
	endAutoReconnect();
	m_loggedIn = false;
	emit autoRejoinArmed(false);

	const QString errorMsg = networkErrorMessage(errorID);

	m_isConnecting = false;
	emit isConnectingChanged(false);
	updateProgress(0, errorMsg);
	emit connectionFailed(errorMsg);
}

bool ServerConnectionHandler::isRecoverableTransportError(int errorID)
{
	switch (errorID) {
	// Pure transport errors - the peer has not rejected us,
	// the line was gone. That is exactly what happens when Android has frozen
	// the process or the WLAN has gone to sleep.
	case ERR_SOCK_CONNECT_FAILED:
	case ERR_SOCK_CONNECT_TIMEOUT:
	case ERR_SOCK_CONNECT_IPV6_FAILED:
	case ERR_SOCK_CONNECT_IPV6_TIMEOUT:
	case ERR_SOCK_SELECT_FAILED:
	case ERR_SOCK_RECV_FAILED:
	case ERR_SOCK_SEND_FAILED:
	case ERR_SOCK_CONN_RESET:
		return true;
	default:
		// Everything else is a rejection by the server (118 kicked,
		// 119 banned, 120 blocked, 121 session timed out, 104/105 wrong
		// password, 106 name taken, 102 maintenance, 103 full) or a
		// protocol error. Retrying does not help there and would only trigger the
		// login rate limit of the server (token bucket per IP).
		return false;
	}
}

bool ServerConnectionHandler::scheduleAutoReconnect(int errorID)
{
	// A few attempts with a growing interval: the seat at the table is reserved
	// for 5 minutes, so there is no hurry - and a tight cycle would run into
	// the login rate limit (burst 5 per IP).
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
	// Deliberately mobile only: there the operating system forces the abort (app frozen
	// in the background), the player did not cause it and cannot
	// prevent it either. On the desktop the previous behaviour stays -
	// a message and the login page, the user decides for themselves.
	// The only switching point: without it neither the rejoin is armed
	// nor the QML notice triggered, the rest of the path stays inactive.
	Q_UNUSED(errorID);
	return false;
#else
	static const int kDelaysMs[] = { 2000, 5000, 15000 };
	static const int kMaxAttempts = static_cast<int>(sizeof(kDelaysMs) / sizeof(kDelaysMs[0]));

	if (!m_loggedIn || !m_session)
		return false;
	// Only for the internet session established via connectToServer: only there
	// are credentials for the silent re-login available, and only there is
	// startInternetClient() the right way back. A LAN join or
	// a self-hosted game runs via the NetworkGameHandler and would
	// otherwise wrongly be reconnected against the internet server.
	if (m_pendingUsername.isEmpty())
		return false;
	if (!isRecoverableTransportError(errorID))
		return false;
	if (m_reconnectAttempt >= kMaxAttempts)
		return false;

	const int delayMs = kDelaysMs[m_reconnectAttempt];
	m_reconnectAttempt++;

	if (!m_reconnecting) {
		m_reconnecting = true;
		emit reconnectingChanged();
		// Arm it before the login: the rejoin offer comes with the
		// InitAck and thus earlier than the end of the reconnect.
		emit autoRejoinArmed(true);
	}
	emit reconnectAttempt(m_reconnectAttempt, kMaxAttempts);
	qInfo() << "[RECONNECT] error" << errorID << "- attempt" << m_reconnectAttempt
			<< "of" << kMaxAttempts << "in" << delayMs << "ms";

	const int scheduledAttempt = m_reconnectAttempt;
	QTimer::singleShot(delayMs, this, [this, scheduledAttempt]() {
		// Discard stale timers after an abort, a successful connection or a
		// further attempt started in the meantime.
		if (!m_session || !m_reconnecting || m_reconnectAttempt != scheduledAttempt)
			return;
		m_isConnecting = true;
		emit isConnectingChanged(true);
		m_session->terminateNetworkClient();
		m_session->startInternetClient();
	});
	return true;
#endif
}

void ServerConnectionHandler::endAutoReconnect()
{
	m_reconnectAttempt = 0;
	if (m_reconnecting) {
		m_reconnecting = false;
		emit reconnectingChanged();
	}
}

void ServerConnectionHandler::abortAutoReconnect()
{
	if (m_reconnecting)
		qInfo() << "[RECONNECT] aborted by user";
	m_loggedIn = false;
	emit autoRejoinArmed(false);
	endAutoReconnect();
}

void ServerConnectionHandler::loadCredentials()
{
	if (!m_config) {
		return;
	}

	// Load username (always saved)
	std::string username = m_config->readConfigString("MyName");
	m_savedUsername = QString::fromUtf8(username.c_str());
	emit savedUsernameChanged();

	// Check if password should be loaded
	int savePassword = m_config->readConfigInt("InternetSavePassword");
	m_rememberPassword = (savePassword == 1);
	emit rememberPasswordChanged();

	// Load password if remember me was checked
	if (m_rememberPassword) {
		std::string encodedPassword = m_config->readConfigString("InternetLoginPassword");
		QByteArray decoded = QByteArray::fromBase64(encodedPassword.c_str());
		m_savedPassword = QString::fromUtf8(decoded);
		emit savedPasswordChanged();
	} else {
		m_savedPassword = "";
		emit savedPasswordChanged();
	}
}

void ServerConnectionHandler::saveCredentials(const QString &username, const QString &password, bool rememberPassword)
{
	if (!m_config) {
		qWarning() << "ServerConnectionHandler::saveCredentials - No config available";
		return;
	}

	// Always save username
	m_config->writeConfigString("MyName", username.toUtf8().constData());
	m_savedUsername = username;
	emit savedUsernameChanged();

	// Save remember password flag
	m_config->writeConfigInt("InternetSavePassword", rememberPassword ? 1 : 0);
	m_rememberPassword = rememberPassword;
	emit rememberPasswordChanged();

	// Save password Base64-encoded only if remember me is checked
	if (rememberPassword) {
		QByteArray encodedPassword = password.toUtf8().toBase64();
		m_config->writeConfigString("InternetLoginPassword", encodedPassword.constData());
		m_savedPassword = password;
		emit savedPasswordChanged();
	} else {
		// Clear saved password
		m_config->writeConfigString("InternetLoginPassword", "");
		m_savedPassword = "";
		emit savedPasswordChanged();
	}

	// Persist to disk; writeConfigString() only updates the in-memory buffer.
	m_config->writeBuffer();
}
