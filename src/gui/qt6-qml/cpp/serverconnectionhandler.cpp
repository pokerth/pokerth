#include "serverconnectionhandler.h"
#include "session.h"
#include "configfile.h"
#include <QByteArray>
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
    
    // Save credentials
    saveCredentials(username, password, rememberPassword);
    
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
        updateProgress(15, tr("Connection failed, retrying..."));
        
        // Wait a moment before retrying
        QTimer::singleShot(2000, this, [this]() {
            if (!m_session) return;
            
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
        case 133:
            errorMsg = tr("Connection blocked (too many attempts)");
            break;
        default:
            errorMsg = tr("Unknown network error");
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
}
