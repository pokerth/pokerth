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
/* Network server helper to accept connections. */

#ifndef _SERVERACCEPTHELPER_H_
#define _SERVERACCEPTHELPER_H_

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>

// TCP keepalive configuration (cross-platform)
#ifdef _WIN32
#include <winsock2.h>
#include <mstcpip.h>   // SIO_KEEPALIVE_VALS
#else
#include <sys/socket.h>
#include <netinet/tcp.h>
#endif
#include <sstream>
#include <sys/socket.h>  // für setsockopt
#include <netinet/tcp.h>  // für TCP_KEEPIDLE, TCP_KEEPINTVL, TCP_KEEPCNT

#include <net/serveracceptinterface.h>
#include <net/serverlobbythread.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>
#include <core/loghelper.h>
#include <game_defs.h>
#include <gui/guiinterface.h>
#include <core/loghelper.h>

template <typename P>
class ServerAcceptHelper : public ServerAcceptInterface
{
public:
    typedef typename P::acceptor P_acceptor;
    typedef typename P::endpoint P_endpoint;
    typedef typename P::socket P_socket;

    ServerAcceptHelper(ServerCallback &serverCallback, boost::shared_ptr<boost::asio::io_context> ioService, bool tls)
        : m_ioService(ioService), m_serverCallback(serverCallback)
    {
        m_tls = tls;
        m_acceptor.reset(new P_acceptor(*m_ioService));
        if (m_tls) {
            try{
                m_sslContext.reset(new boost::asio::ssl::context(boost::asio::ssl::context::sslv23));
                m_sslContext->set_options(
                    boost::asio::ssl::context::default_workarounds
                    | boost::asio::ssl::context::no_sslv2
                    | boost::asio::ssl::context::no_sslv3
                );

                // @TODO: find better way to specify cert and key files
                m_sslContext->use_certificate_chain_file("/srv/pokerth_env/repos/pokerth-test/tls/server.crt");
                m_sslContext->use_private_key_file("/srv/pokerth_env/repos/pokerth-test/tls/server.key", boost::asio::ssl::context::pem);

                std::string ciphers = "ECDHE-RSA-AES128-GCM-SHA256:...";
                if (SSL_CTX_set_cipher_list(m_sslContext->native_handle(), ciphers.c_str()) != 1) {
                    LOG_ERROR("Error setting cipher list: " << ERR_error_string(ERR_get_error(), nullptr));
                }
                const char *c13 = "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256";
                if (SSL_CTX_set_ciphersuites(m_sslContext->native_handle(), c13) != 1) {
                    LOG_MSG("Could not set TLS1.3 ciphersuites (maybe older OpenSSL).");
                }
                // SSL_CTX_set_info_callback(m_sslContext->native_handle(), &SslServerInfoCallback);
                LOG_MSG("TLS context configured.");
             } catch (std::exception& e) {
                 LOG_ERROR(e.what());
             }
         }
     }

    virtual ~ServerAcceptHelper()
    {
    }

    // Set the parameters.
    virtual void Listen(unsigned serverPort, bool ipv6, const std::string &/*logDir*/,
                        boost::shared_ptr<ServerLobbyThread> lobbyThread)
    {
        m_lobbyThread = lobbyThread;

        try {
            InternalListen(serverPort, ipv6);
        } catch (const PokerTHException &e) {
            LOG_ERROR(e.what());
            GetCallback().SignalNetServerError(e.GetErrorId(), e.GetOsErrorCode());
        } catch (...) {
            // This is probably an asio exception. Assume that bind failed,
            // which is the most frequent case.
            LOG_ERROR("Cannot bind/listen on port.");
            GetCallback().SignalNetServerError(ERR_SOCK_BIND_FAILED, 0);
        }
    }

    virtual void Close()
    {
        boost::system::error_code ec;
        m_acceptor->close(ec);
        // Ignore any error, because we are terminating.
    }
protected:

    void InternalListen(unsigned serverPort, bool ipv6)
    {
        if (serverPort < 1024)
            throw ServerException(__FILE__, __LINE__, ERR_SOCK_INVALID_PORT, 0);

        // TODO consider sctp
        // Prepare Listen.
        if (ipv6) {
            m_endpoint.reset(new P_endpoint(P::v6(), serverPort));
        } else {
            m_endpoint.reset(new P_endpoint(P::v4(), serverPort));
        }

        m_acceptor->open(m_endpoint->protocol());
        m_acceptor->set_option(typename P::acceptor::reuse_address(true));
        if (ipv6) { // In IPv6 mode: Be compatible with IPv4.
            m_acceptor->set_option(boost::asio::ip::v6_only(false));
        }
        m_acceptor->bind(*m_endpoint);
        // Use larger backlog (128) to handle burst connections from bots
        m_acceptor->listen(128);

        if(m_tls){
            boost::shared_ptr<P_socket> newSocket(new P_socket(*m_ioService));
            m_acceptor->async_accept(
                *newSocket,
                boost::bind(&ServerAcceptHelper::HandleAccept, this, newSocket,
                            boost::asio::placeholders::error)
            );
        }else{
            // Start first asynchronous Accept.
            boost::shared_ptr<P_socket> newSocket(new P_socket(*m_ioService));
            m_acceptor->async_accept(
                *newSocket,
                boost::bind(&ServerAcceptHelper::HandleAccept, this, newSocket,
                            boost::asio::placeholders::error)
            );
        }
    }

    void HandleAccept(boost::shared_ptr<P_socket> acceptedSocket,
                      const boost::system::error_code &error)
    {
        // IMMER neuen async_accept starten - auch bei Fehlern!
        // Das verhindert dass der Accept-Loop stoppt
        auto startNextAccept = [this]() {
            try {
                if (!m_acceptor || !m_acceptor->is_open()) {
                    LOG_ERROR("[TLS-SERVER] CRITICAL: Acceptor is closed, cannot start next accept");
                    return;
                }
                boost::shared_ptr<P_socket> nextSocket(new P_socket(*m_ioService));
                m_acceptor->async_accept(
                    *nextSocket,
                    boost::bind(&ServerAcceptHelper::HandleAccept, this, nextSocket,
                                boost::asio::placeholders::error)
                );
            } catch (const std::exception& e) {
                LOG_ERROR("[TLS-SERVER] CRITICAL: Failed to start next async_accept: " << e.what());
            } catch (...) {
                LOG_ERROR("[TLS-SERVER] CRITICAL: Unknown exception in startNextAccept");
            }
        };
        
        if (!error) {
            // Get peer endpoint for debug logging
            std::string peerAddr;
            try {
                auto endpoint = acceptedSocket->remote_endpoint();
                peerAddr = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
            } catch (...) {
                peerAddr = "unknown";
            }
            
            
            try {
                acceptedSocket->non_blocking(true);
                acceptedSocket->set_option(typename P::no_delay(true));
                acceptedSocket->set_option(boost::asio::socket_base::keep_alive(true));
                
                // WiFi-friendly TCP keepalive: tolerate brief WiFi
                // power-save sleep (common on Windows laptops, 10-30s).
                // First probe after 30s idle, then every 10s, fail after 6.
                // Total detection: 30 + 6*10 = 90s.
                {
                    int fd = acceptedSocket->native_handle();
#ifdef _WIN32
                    // Windows: use SIO_KEEPALIVE_VALS via WSAIoctl
                    struct tcp_keepalive keepaliveVals;
                    keepaliveVals.onoff = 1;
                    keepaliveVals.keepalivetime = 30000;      // 30s until first probe (ms)
                    keepaliveVals.keepaliveinterval = 10000;  // 10s between probes (ms)
                    DWORD bytesReturned = 0;
                    int wsaRet = WSAIoctl(static_cast<SOCKET>(fd), SIO_KEEPALIVE_VALS,
                             &keepaliveVals, sizeof(keepaliveVals),
                             NULL, 0, &bytesReturned, NULL, NULL);
                    if (wsaRet != 0) {
                        LOG_ERROR("WSAIoctl SIO_KEEPALIVE_VALS failed: " << WSAGetLastError());
                    }
                    // TCP_KEEPCNT: Limit keepalive probes to 6 (matching Linux).
                    // Available since Windows 10 1703; silently fails on older.
#ifndef TCP_KEEPCNT
#define TCP_KEEPCNT 16
#endif
                    {
                        int keepcnt = 6;
                        setsockopt(static_cast<SOCKET>(fd), IPPROTO_TCP, TCP_KEEPCNT,
                                   (const char*)&keepcnt, sizeof(keepcnt));
                    }
#else
                    // Linux / macOS: per-socket keepalive tuning
                    int keepidle  = 30;   // seconds until first keepalive probe
                    int keepintvl = 10;   // seconds between subsequent probes
                    int keepcnt   = 6;    // number of failed probes before disconnect
                    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,  &keepidle,  sizeof(keepidle));
                    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
                    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,   &keepcnt,   sizeof(keepcnt));
                    // TCP_USER_TIMEOUT: abort connection if data remains
                    // unacknowledged for 90s (matches keepalive detection).
                    // Without this, a dead client can keep the server
                    // waiting for minutes.
#ifdef TCP_USER_TIMEOUT
                    unsigned int user_timeout_ms = 90000;
                    setsockopt(fd, IPPROTO_TCP, TCP_USER_TIMEOUT, &user_timeout_ms, sizeof(user_timeout_ms));
#endif
#endif
                }

                // Increase socket buffers for game-start traffic bursts.
                {
                    boost::asio::socket_base::send_buffer_size    sndbuf(131072);
                    boost::asio::socket_base::receive_buffer_size rcvbuf(131072);
                    boost::system::error_code buf_ec;
                    acceptedSocket->set_option(sndbuf, buf_ec);
                    acceptedSocket->set_option(rcvbuf, buf_ec);
                }
            } catch (const std::exception& e) {
                LOG_ERROR("[TLS-SERVER] Failed to set socket options: " << e.what());
                startNextAccept();
                return;
            }

            // Sofort neuen async_accept starten BEVOR wir den Handshake machen
            // Das erlaubt parallele Handshakes!
            startNextAccept();

            if (m_tls) {
                try {
                    typedef boost::asio::ssl::stream<P_socket> ssl_stream_t;
                    boost::shared_ptr<ssl_stream_t> sslStream(new ssl_stream_t(*m_ioService, *m_sslContext));
                    sslStream->next_layer() = std::move(*acceptedSocket);

                    
                    // Shared state to prevent timeout from closing an established connection
                    auto handshakeCompleted = std::make_shared<std::atomic<bool>>(false);
                    auto peerAddrShared = std::make_shared<std::string>(peerAddr);
                    
                    // Create a timeout timer for the handshake (12 seconds - increased for slow clients)
                    auto handshakeTimer = std::make_shared<boost::asio::steady_timer>(*m_ioService);
                    handshakeTimer->expires_after(std::chrono::seconds(12));
                    
                    // Wichtig: Alle Lambdas müssen thread-safe sein
                    handshakeTimer->async_wait(
                        [handshakeCompleted, sslStream, handshakeTimer, peerAddrShared](const boost::system::error_code& ec) {
                            try {
                                if (!ec && !handshakeCompleted->load(std::memory_order_acquire)) {
                                    // Timeout: close the socket to abort the handshake (only if not completed)
                                    boost::system::error_code closeEc;
                                    sslStream->lowest_layer().close(closeEc);
                                } else if (ec && ec != boost::asio::error::operation_aborted) {
                                }
                            } catch (const std::exception& e) {
                                LOG_ERROR("[TLS-SERVER] Exception in handshake timer: " << e.what());
                            } catch (...) {
                                LOG_ERROR("[TLS-SERVER] Unknown exception in handshake timer");
                            }
                        });

                    sslStream->async_handshake(
                        boost::asio::ssl::stream_base::server,
                        [this, sslStream, handshakeTimer, handshakeCompleted, peerAddrShared](const boost::system::error_code& error) {
                            try {
                                // Mark handshake as completed before any other action (memory_order_release für synchronization)
                                bool wasCompleted = handshakeCompleted->exchange(true, std::memory_order_acq_rel);
                                
                                
                                // Only handle if this is the first completion (not a timeout-triggered close)
                                if (!wasCompleted) {
                                    handshakeTimer->cancel();
                                    this->HandleHandshakeOnly(sslStream, error);
                                } else {
                                    // Handshake callback fired after timeout - socket already closed
                                }
                            } catch (const std::exception& e) {
                                LOG_ERROR("[TLS-SERVER] Exception in handshake callback: " << e.what());
                                try {
                                    handshakeTimer->cancel();
                                    boost::system::error_code ec;
                                    sslStream->lowest_layer().close(ec);
                                } catch (...) {}
                            } catch (...) {
                                LOG_ERROR("[TLS-SERVER] Unknown exception in handshake callback");
                            }
                        }
                    );
                } catch (const std::exception& e) {
                    LOG_ERROR("[TLS-SERVER] Exception starting TLS handshake: " << e.what());
                    // Socket cleanup
                    try {
                        boost::system::error_code ec;
                        acceptedSocket->close(ec);
                    } catch (...) {}
                } catch (...) {
                    LOG_ERROR("[TLS-SERVER] Unknown exception starting TLS handshake");
                }
            } else {
                try {
                    boost::shared_ptr<SessionData> sessionData(new SessionData(acceptedSocket, m_lobbyThread->GetNextSessionId(), m_lobbyThread->GetSessionDataCallback(), *m_ioService));
                    GetLobbyThread().AddConnection(sessionData);
                } catch (const std::exception& e) {
                    LOG_ERROR("[TLS-SERVER] Exception creating non-TLS session: " << e.what());
                    try {
                        boost::system::error_code ec;
                        acceptedSocket->close(ec);
                    } catch (...) {}
                }
            }
        } else {
            LOG_ERROR("[TLS-SERVER] Accept failed: " << error.message() << " - starting new accept");
            // WICHTIG: Auch bei Accept-Fehlern neuen async_accept starten!
            try {
                if (!m_acceptor || !m_acceptor->is_open()) {
                    LOG_ERROR("[TLS-SERVER] CRITICAL: Acceptor is closed, cannot restart after error");
                    return;
                }
                boost::shared_ptr<P_socket> nextSocket(new P_socket(*m_ioService));
                m_acceptor->async_accept(
                    *nextSocket,
                    boost::bind(&ServerAcceptHelper::HandleAccept, this, nextSocket,
                                boost::asio::placeholders::error)
                );
            } catch (const std::exception& e) {
                LOG_ERROR("[TLS-SERVER] CRITICAL: Failed to restart accept after error: " << e.what());
                GetCallback().SignalNetServerError(ERR_SOCK_ACCEPT_FAILED, 0);
            }
        }
    }

    // HandleHandshakeOnly - does NOT start new async_accept (already started in HandleAccept)
    void HandleHandshakeOnly(boost::shared_ptr<boost::asio::ssl::stream<P_socket>> sslStream,
                             const boost::system::error_code &error)
    {
        std::string peerAddr = "unknown";
        try {
            auto endpoint = sslStream->lowest_layer().remote_endpoint();
            peerAddr = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
        } catch (...) {}
        
        if (!error) {
            
            try {
                boost::shared_ptr<SessionData> sessionData(new SessionData(sslStream, m_lobbyThread->GetNextSessionId(), m_lobbyThread->GetSessionDataCallback(), *m_ioService, 0));
                GetLobbyThread().AddConnection(sessionData);
            } catch (const std::exception& e) {
                LOG_ERROR("[TLS-SERVER] EXCEPTION while creating/adding session: " << e.what());
            } catch (...) {
                LOG_ERROR("[TLS-SERVER] UNKNOWN EXCEPTION while creating/adding session");
            }
        } else {
            
            // Try to get more detailed SSL error information
            SSL* ssl = sslStream->native_handle();
            if (ssl) {
                unsigned long ssl_err = ERR_get_error();
                while (ssl_err != 0) {
                    char err_buf[256];
                    ERR_error_string_n(ssl_err, err_buf, sizeof(err_buf));
                    ssl_err = ERR_get_error();
                }
            }
            
            // Close the SSL stream and socket immediately to free resources
            boost::system::error_code ec;
            sslStream->lowest_layer().close(ec);
        }
        // NOTE: No new async_accept here - it's already running from HandleAccept
    }

    void HandleHandshake(boost::shared_ptr<boost::asio::ssl::stream<P_socket>> sslStream,
                         const boost::system::error_code &error)
    {
        std::string peerAddr = "unknown";
        try {
            auto endpoint = sslStream->lowest_layer().remote_endpoint();
            peerAddr = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
        } catch (...) {}
        
        if (!error) {
            
            try {
                boost::shared_ptr<SessionData> sessionData(new SessionData(sslStream, m_lobbyThread->GetNextSessionId(), m_lobbyThread->GetSessionDataCallback(), *m_ioService, 0));
                GetLobbyThread().AddConnection(sessionData);
            } catch (const std::exception& e) {
                LOG_ERROR("[TLS-SERVER] EXCEPTION while creating/adding session: " << e.what());
            } catch (...) {
                LOG_ERROR("[TLS-SERVER] UNKNOWN EXCEPTION while creating/adding session");
            }
        } else {
            
            // Try to get more detailed SSL error information
            SSL* ssl = sslStream->native_handle();
            if (ssl) {
                unsigned long ssl_err = ERR_get_error();
                while (ssl_err != 0) {
                    char err_buf[256];
                    ERR_error_string_n(ssl_err, err_buf, sizeof(err_buf));
                    ssl_err = ERR_get_error();
                }
            }
            
            // Close the SSL stream and socket immediately to free resources
            boost::system::error_code ec;
            sslStream->lowest_layer().close(ec);
        }

        
        // Check if acceptor is still open
        if (!m_acceptor->is_open()) {
            LOG_ERROR("[TLS-SERVER] CRITICAL: Acceptor is CLOSED - cannot start async_accept!");
            return;
        }
        
        boost::shared_ptr<P_socket> newSocket(new P_socket(*m_ioService));
        
        m_acceptor->async_accept(
            *newSocket,
            boost::bind(&ServerAcceptHelper::HandleAccept, this, newSocket,
                        boost::asio::placeholders::error)
        );
        
    }

    static inline void SslServerInfoCallback(const SSL *ssl, int where, int ret)
    {
        const char *state = SSL_state_string_long((SSL*)ssl);
        std::ostringstream ss;
        ss << "SSL handshake info: state=" << (state ? state : "unknown")
           << " where=" << where << " ret=" << ret;
#if defined(SSL_ST_INIT)
        if (where & SSL_ST_INIT) ss << " [ST_INIT]";
#endif
#if defined(SSL_ST_BEFORE)
        if (where & SSL_ST_BEFORE) ss << " [ST_BEFORE]";
#elif defined(TLS_ST_BEFORE)
        if (where & TLS_ST_BEFORE) ss << " [TLS_ST_BEFORE]";
#endif
#if defined(SSL_CB_LOOP)
        if (where & SSL_CB_LOOP) ss << " [LOOP]";
#endif
#if defined(SSL_CB_EXIT)
        if (where & SSL_CB_EXIT) ss << " [EXIT]";
#endif
#if defined(SSL_CB_READ)
        if (where & SSL_CB_READ) ss << " [READ]";
#endif
#if defined(SSL_CB_WRITE)
        if (where & SSL_CB_WRITE) ss << " [WRITE]";
#endif
#if defined(SSL_CB_ALERT)
        if (where & SSL_CB_ALERT) {
            ss << " [ALERT]";
            ss << " type=" << (ret >> 8) << " desc=" << (ret & 0xff);
#   ifdef SSL_alert_type_string_long
            ss << " (" << SSL_alert_type_string_long(ret) << ":" << SSL_alert_desc_string_long(ret) << ")";
#   endif
        }
#endif
        if (ssl) {
            const char *ver = SSL_get_version((SSL*)ssl);
            const char *cipher = SSL_get_cipher((SSL*)ssl);
            ss << " ver=" << (ver ? ver : "n/a") << " cipher=" << (cipher ? cipher : "n/a");
            if (const SSL_CIPHER *c = SSL_get_current_cipher((SSL*)ssl)) {
                ss << " cipher_name=" << SSL_CIPHER_get_name(c);
            }
        }
        LOG_MSG(ss.str());
    }

    ServerCallback &GetCallback()
    {
        return m_serverCallback;
    }

    ServerLobbyThread &GetLobbyThread()
    {
        return *m_lobbyThread;
    }

private:
    boost::shared_ptr<boost::asio::io_context> m_ioService;
    boost::shared_ptr<P_acceptor> m_acceptor;
    boost::shared_ptr<P_endpoint> m_endpoint;
    boost::shared_ptr<boost::asio::ssl::context> m_sslContext;
    ServerCallback &m_serverCallback;
    bool m_tls;

    boost::shared_ptr<ServerLobbyThread> m_lobbyThread;
};

#endif
