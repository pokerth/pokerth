#include "serverconnectionhandler.h"
#include "session.h"
#include "configfile.h"
#include "core/appimage_utils.h"
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

void ServerConnectionHandler::onNetClientError(int errorID, int osErrorID)
{
    Q_UNUSED(osErrorID);
    
    qWarning() << "ServerConnectionHandler: Network error:" << errorID << "retry count:" << m_retryCount;
    
    // Error 11 is often a TLS handshake issue that succeeds on retry
    if (errorID == 11 && m_retryCount < 1 && !m_pendingUsername.isEmpty()) {
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
    
    QString errorMsg;
    switch (errorID) {
        case 1:
            errorMsg = tr("Could not connect to server");
            break;
        case 2:
            errorMsg = tr("Authentication failed");
            break;
        case 3:
            errorMsg = tr("Server error");
            break;
        case 11:
            errorMsg = tr("TLS connection error");
            break;
        case 16:
            // ERR_SOCK_CONN_RESET: Verbindung während der Sitzung abgebrochen
            // (z.B. WLAN weg). Wird nach dem Login vom globalen
            // connectionLostPopup in pokerth.qml angezeigt.
            errorMsg = tr("The connection to the server was lost");
            break;
        case 101:
            errorMsg = tr("Protocol version not supported by server");
            break;
        case 102:
            errorMsg = tr("Server is under maintenance");
            break;
        case 103:
            errorMsg = tr("Server is full");
            break;
        case 104:
        case 105:
            errorMsg = tr("Invalid password");
            break;
        case 106:
            errorMsg = tr("Username already in use");
            break;
        case 107:
            errorMsg = tr("Invalid username");
            break;
        case 118:
            errorMsg = tr("You have been kicked from the server");
            break;
        case 119:
            errorMsg = tr("You are banned from this server");
            break;
        case 121:
            errorMsg = tr("Session timed out");
            break;
        case 133:
            errorMsg = tr("Connection blocked (too many attempts)");
            break;
        default:
            errorMsg = tr("Network error (%1)").arg(errorID);
            break;
    }
    
    m_isConnecting = false;
    emit isConnectingChanged(false);
    updateProgress(0, errorMsg);
    emit connectionFailed(errorMsg);
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
