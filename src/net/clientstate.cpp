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

#include <net/clientstate.h>
#include <net/clientthread.h>
#include <net/clientcontext.h>
#include <net/senderhelper.h>
#include <net/netpacket.h>
#include <net/clientexception.h>
#include <net/socket_helper.h>
#include <net/socket_msg.h>
#include <net/downloadhelper.h>
#include <core/loghelper.h>
#include <core/avatarmanager.h>
#include <core/crypthelper.h>
#include <qttoolsinterface.h>

#include <game.h>
#include <playerinterface.h>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QDebug>
#include <QFile>
#include <boost/bind/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

// TCP keepalive configuration (cross-platform)
#ifdef _WIN32
#include <winsock2.h>
#include <mstcpip.h>   // SIO_KEEPALIVE_VALS
// TCP_KEEPCNT available since Windows 10 version 1703 (SDK 15063).
// Define it here for older SDKs; setsockopt will simply fail on
// older Windows versions, which is harmless.
#ifndef TCP_KEEPCNT
#define TCP_KEEPCNT 16
#endif
#else
#include <sys/socket.h>
#include <netinet/tcp.h>
#endif

using namespace std;
using namespace boost::filesystem;

#ifdef BOOST_ASIO_HAS_STD_CHRONO
using namespace std::chrono;
#else
using namespace boost::chrono;
#endif

#define CLIENT_WAIT_TIMEOUT_MSEC	50
#define CLIENT_CONNECT_TIMEOUT_SEC	12


ClientState::~ClientState()
{
}

//-----------------------------------------------------------------------------

ClientStateInit &
ClientStateInit::Instance()
{
	static ClientStateInit state;
	return state;
}

ClientStateInit::ClientStateInit()
{
}

ClientStateInit::~ClientStateInit()
{
}

void
ClientStateInit::Enter(boost::shared_ptr<ClientThread> client)
{
	ClientContext &context = client->GetContext();

	if (context.GetServerAddr().empty())
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_SERVERADDR_NOT_SET, 0);

	if (context.GetServerPort() < 1024)
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_PORT, 0);

	client->CreateContextSession();
	client->GetCallback().SignalNetClientConnect(MSG_SOCK_INIT_DONE);

	if (context.GetUseServerList())
		client->SetState(ClientStateStartServerListDownload::Instance());
	else
		client->SetState(ClientStateStartResolve::Instance());
}

void
ClientStateInit::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

ClientStateStartResolve &
ClientStateStartResolve::Instance()
{
	static ClientStateStartResolve state;
	return state;
}

ClientStateStartResolve::ClientStateStartResolve()
{
}

ClientStateStartResolve::~ClientStateStartResolve()
{
}

void
ClientStateStartResolve::Enter(boost::shared_ptr<ClientThread> client)
{
	ClientContext &context = client->GetContext();
	ostringstream portStr;
	portStr << context.GetServerPort();

	context.GetResolver()->async_resolve(
		context.GetServerAddr(),
		portStr.str(),
		boost::bind(&ClientStateStartResolve::HandleResolve,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::iterator,
					client));
}

void
ClientStateStartResolve::Exit(boost::shared_ptr<ClientThread> client)
{
	client->GetContext().GetResolver()->cancel();
}

void
ClientStateStartResolve::HandleResolve(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::results_type endpoint_iterator,
									   boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this) {
		client->GetCallback().SignalNetClientConnect(MSG_SOCK_RESOLVE_DONE);
		// Use the first resolver result.
		ClientStateStartConnect::Instance().SetRemoteEndpoint(endpoint_iterator);
		client->SetState(ClientStateStartConnect::Instance());
	} else {
		if (ec != boost::asio::error::operation_aborted)
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_RESOLVE_FAILED, 0);
	}
}

//-----------------------------------------------------------------------------

ClientStateStartServerListDownload &
ClientStateStartServerListDownload::Instance()
{
	static ClientStateStartServerListDownload state;
	return state;
}

ClientStateStartServerListDownload::ClientStateStartServerListDownload()
{
}

ClientStateStartServerListDownload::~ClientStateStartServerListDownload()
{
}

void
ClientStateStartServerListDownload::Enter(boost::shared_ptr<ClientThread> client)
{
	path tmpServerListPath(client->GetCacheServerListFileName());
	if (tmpServerListPath.empty())
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_URL, 0);

	if (exists(tmpServerListPath)) {
		// Download the current server list once a day.
		// If the previous file is older than one day, delete it.
		// Also delete the file if it is empty.
		if (file_size(tmpServerListPath) == 0 || (last_write_time(tmpServerListPath) + 86400 < time(NULL))) {
			try {
				remove(tmpServerListPath);
			} catch (const boost::filesystem::filesystem_error &) {
				// File may be inaccessible (e.g. old snap revision directory).
				// Ignore and proceed with a fresh download.
			}
		}
	}

	if (exists(tmpServerListPath)) {
		// Use the existing server list.
		client->SetState(ClientStateReadingServerList::Instance());
	} else {
		// Download the server list.
		boost::shared_ptr<DownloadHelper> downloader(new DownloadHelper);
		downloader->Init(client->GetContext().GetServerListUrl(), tmpServerListPath.string());
		ClientStateDownloadingServerList::Instance().SetDownloadHelper(downloader);
		client->SetState(ClientStateDownloadingServerList::Instance());
	}
}

void
ClientStateStartServerListDownload::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

ClientStateDownloadingServerList &
ClientStateDownloadingServerList::Instance()
{
	static ClientStateDownloadingServerList state;
	return state;
}

ClientStateDownloadingServerList::ClientStateDownloadingServerList()
{
}

ClientStateDownloadingServerList::~ClientStateDownloadingServerList()
{
}

void
ClientStateDownloadingServerList::Enter(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().expires_after(milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
	client->GetStateTimer().async_wait(
		boost::bind(
			&ClientStateDownloadingServerList::TimerLoop, this, boost::asio::placeholders::error, client));
}

void
ClientStateDownloadingServerList::Exit(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().cancel();
}

void
ClientStateDownloadingServerList::SetDownloadHelper(boost::shared_ptr<DownloadHelper> helper)
{
	m_downloadHelper = helper;
}

void
ClientStateDownloadingServerList::TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this) {
		if (m_downloadHelper->Process()) {
			m_downloadHelper.reset();
			client->SetState(ClientStateReadingServerList::Instance());
		} else {
			client->GetStateTimer().expires_after(milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
			client->GetStateTimer().async_wait(
				boost::bind(
					&ClientStateDownloadingServerList::TimerLoop, this, boost::asio::placeholders::error, client));
		}
	}
}

//-----------------------------------------------------------------------------

ClientStateReadingServerList &
ClientStateReadingServerList::Instance()
{
	static ClientStateReadingServerList state;
	return state;
}

ClientStateReadingServerList::ClientStateReadingServerList()
{
}

ClientStateReadingServerList::~ClientStateReadingServerList()
{
}

void
ClientStateReadingServerList::Enter(boost::shared_ptr<ClientThread> client)
{
	ClientContext &context = client->GetContext();
	path zippedServerListPath(context.GetCacheDir());
	zippedServerListPath /= context.GetServerListUrl().substr(context.GetServerListUrl().find_last_of('/') + 1);
	path xmlServerListPath;
	if (zippedServerListPath.extension().string() == ".z") {
		xmlServerListPath = zippedServerListPath;
		xmlServerListPath.replace_extension("");

		// Unzip the file using zlib.
		try {
			std::ifstream inFile(zippedServerListPath.string().c_str(), ios_base::in | ios_base::binary);
			std::ofstream outFile(xmlServerListPath.string().c_str(), ios_base::out | ios_base::trunc);
			boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
			in.push(boost::iostreams::zlib_decompressor());
			in.push(inFile);
			boost::iostreams::copy(in, outFile);
		} catch (...) {
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_UNZIP_FAILED, 0);
		}
	} else
		xmlServerListPath = zippedServerListPath;

	// Parse the server address.
	QDomDocument xmlDoc;
	QString qPath = QString::fromLocal8Bit(xmlServerListPath.string().c_str());
	QFile file(qPath);
	if (file.open(QIODevice::ReadOnly) && xmlDoc.setContent(&file)) {
		file.close();

		client->ClearServerInfoMap();
		int serverCount = 0;
		unsigned lastServerInfoId = 0;
		QDomElement nextServer = xmlDoc.documentElement().firstChildElement("Server");

		while (!nextServer.isNull()) {
			ServerInfo serverInfo;
			{
				int tmpId = nextServer.attribute("id").toInt();
				// nextServer->QueryIntAttribute("id", &tmpId);
				serverInfo.id = (unsigned)tmpId;
			}
			QDomElement nameNode = nextServer.firstChildElement("Name");
			QDomElement sponsorNode = nextServer.firstChildElement("Sponsor");
			QDomElement countryNode = nextServer.firstChildElement("Country");
			QDomElement addr4Node = nextServer.firstChildElement("IPv4Address");
			QDomElement addr6Node = nextServer.firstChildElement("IPv6Address");
			QDomElement sctpNode = nextServer.firstChildElement("SCTP");
			QDomElement tlsNode = nextServer.firstChildElement("TLS");
			QDomElement portNode = nextServer.firstChildElement("ProtobufPort");

			// IPv6 support for avatar servers depends on this address and on libcurl.
			QDomElement avatarNode = nextServer.firstChildElement("AvatarServerAddress");

			if (nameNode.isNull() || addr4Node.isNull() || addr6Node.isNull() || portNode.isNull())
				throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_XML, 0);

			serverInfo.name = nameNode.attribute("value").toStdString();
			serverInfo.ipv4addr = addr4Node.attribute("value").toStdString();
			serverInfo.ipv6addr = addr6Node.attribute("value").toStdString();
			serverInfo.port = portNode.attribute("value").toInt();

			// Optional parameters:
			if (!sponsorNode.isNull())
				serverInfo.sponsor = sponsorNode.attribute("value").toStdString();
			if (!countryNode.isNull())
				serverInfo.country = countryNode.attribute("value").toStdString();
			if (!sctpNode.isNull()) {
				int tmpSctp;
				tmpSctp = sctpNode.attribute("value").toInt();
				serverInfo.supportsSctp = tmpSctp == 1 ? true : false;
			}
			if (!tlsNode.isNull()) {
				QString tlsValue = tlsNode.attribute("value").toLower();
				serverInfo.useTLS = (tlsValue == "on" || tlsValue == "true" || tlsValue == "1" || tlsValue == "yes");
			}
			if (!avatarNode.isNull())
				serverInfo.avatarServerAddr = avatarNode.attribute("value").toStdString();

			client->AddServerInfo(serverInfo.id, serverInfo);
			nextServer = nextServer.nextSiblingElement();
			lastServerInfoId = serverInfo.id;
			serverCount++;
		}

		if (serverCount == 1) {
			client->UseServer(lastServerInfoId);
			client->CreateContextSession();  // Recreate session with TLS setting from serverlist
			client->GetCallback().SignalNetClientConnect(MSG_SOCK_SERVER_LIST_DONE);
			client->SetState(ClientStateStartResolve::Instance());
		} else if (serverCount > 1) {
			client->GetCallback().SignalNetClientServerListShow();
			client->SetState(ClientStateWaitChooseServer::Instance());
		} else
			throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_XML, 0);
	} else
		throw ClientException(__FILE__, __LINE__, ERR_SOCK_INVALID_SERVERLIST_XML, 0);
}

void
ClientStateReadingServerList::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
	// Nothing to do.
}

//-----------------------------------------------------------------------------

ClientStateWaitChooseServer &
ClientStateWaitChooseServer::Instance()
{
	static ClientStateWaitChooseServer state;
	return state;
}

ClientStateWaitChooseServer::ClientStateWaitChooseServer()
{
}

ClientStateWaitChooseServer::~ClientStateWaitChooseServer()
{
}

void
ClientStateWaitChooseServer::Enter(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().expires_after(milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
	client->GetStateTimer().async_wait(
		boost::bind(
			&ClientStateWaitChooseServer::TimerLoop, this, boost::asio::placeholders::error, client));
}

void
ClientStateWaitChooseServer::Exit(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().cancel();
}

void
ClientStateWaitChooseServer::TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this) {
		unsigned serverId;
		if (client->GetSelectedServer(serverId)) {
			client->UseServer(serverId);
			client->CreateContextSession();  // Recreate session with TLS setting from serverlist
			client->GetCallback().SignalNetClientConnect(MSG_SOCK_SERVER_LIST_DONE);
			client->SetState(ClientStateStartResolve::Instance());
		} else {
			client->GetStateTimer().expires_after(milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
			client->GetStateTimer().async_wait(
				boost::bind(
					&ClientStateWaitChooseServer::TimerLoop, this, boost::asio::placeholders::error, client));
		}
	}
}

//-----------------------------------------------------------------------------

ClientStateStartConnect &
ClientStateStartConnect::Instance()
{
	static ClientStateStartConnect state;
	return state;
}

ClientStateStartConnect::ClientStateStartConnect()
	: m_handshakeRetryCount(0)
{
}

ClientStateStartConnect::~ClientStateStartConnect()
{
}

void
ClientStateStartConnect::Enter(boost::shared_ptr<ClientThread> client)
{
    m_handshakeRetryCount = 0; // Reset retry counter
    
    // Initialize handshake timer if using SSL
    if (client->GetContext().GetSessionData()->IsSsl() && !m_handshakeTimer) {
        m_handshakeTimer.reset(new boost::asio::steady_timer(client->GetStateTimer().get_executor()));
    }
    
    client->GetStateTimer().expires_after(seconds(CLIENT_CONNECT_TIMEOUT_SEC));
    client->GetStateTimer().async_wait(
        boost::bind(
            &ClientStateStartConnect::TimerTimeout, this, boost::asio::placeholders::error, client));

    boost::asio::ip::tcp::endpoint endpoint = m_remoteEndpointIterator->endpoint();

    if (client->GetContext().GetSessionData()->IsSsl()) {
        client->GetContext().GetSessionData()->GetSslStream()->lowest_layer().async_connect(
            endpoint,
            boost::bind(&ClientStateStartConnect::HandleConnect,
                        this,
                        boost::asio::placeholders::error,
                        ++m_remoteEndpointIterator,
                        client));
    } else {
        client->GetContext().GetSessionData()->GetAsioSocket()->async_connect(
            endpoint,
            boost::bind(&ClientStateStartConnect::HandleConnect,
                        this,
                        boost::asio::placeholders::error,
                        ++m_remoteEndpointIterator,
                        client));
    }
}

void
ClientStateStartConnect::Exit(boost::shared_ptr<ClientThread> client)
{
	
	// Cancel and reset all timers
	client->GetStateTimer().cancel();
	if (m_retryTimer) {
		m_retryTimer->cancel();
		m_retryTimer.reset();  // Komplett zurücksetzen
	}
	if (m_handshakeTimer) {
		m_handshakeTimer->cancel();
		m_handshakeTimer.reset();  // Komplett zurücksetzen
	}
	// Retry-Counter auch zurücksetzen für den nächsten Verbindungsversuch
	m_handshakeRetryCount = 0;
}

void
ClientStateStartConnect::SetRemoteEndpoint(boost::asio::ip::tcp::resolver::results_type endpointIterator)
{
	m_remoteEndpointIterator = endpointIterator.begin();
	m_remoteEndpoint = endpointIterator;
}

void
ClientStateStartConnect::HandleConnect(const boost::system::error_code& ec, boost::asio::ip::basic_resolver_iterator<boost::asio::ip::tcp> endpoint_iterator,
                                       boost::shared_ptr<ClientThread> client)
{
    if (&client->GetState() == this) {
        if (!ec) {
            // Set TCP_NODELAY to disable Nagle's algorithm for reduced latency
            boost::asio::ip::tcp::no_delay option(true);
            boost::system::error_code nodelay_ec;
            if (client->GetContext().GetSessionData()->IsSsl()) {
                client->GetContext().GetSessionData()->GetSslStream()->lowest_layer().set_option(option, nodelay_ec);
            } else {
                client->GetContext().GetSessionData()->GetAsioSocket()->set_option(option, nodelay_ec);
            }
            if (nodelay_ec) {
            } else {
            }

            // Enable TCP keepalive to detect dead connections and prevent
            // NAT/firewall idle-connection drops.  This is critical on Windows
            // where WiFi power management and home routers frequently cause
            // silent connection deaths.
            {
                boost::asio::socket_base::keep_alive ka_option(true);
                boost::system::error_code ka_ec;
                if (client->GetContext().GetSessionData()->IsSsl()) {
                    client->GetContext().GetSessionData()->GetSslStream()->lowest_layer().set_option(ka_option, ka_ec);
                } else {
                    client->GetContext().GetSessionData()->GetAsioSocket()->set_option(ka_option, ka_ec);
                }

                if (!ka_ec) {
                    // WiFi-friendly keepalive: tolerate brief WiFi power-save
                    // sleep (common on Windows laptops, typically 10-30s).
                    // First probe after 30s idle, then every 10s, fail after 6.
                    // Total detection time: 30 + 6*10 = 90s.
                    // (On Windows, probe count is OS-controlled ~10, so ~130s.)
                    int fd = -1;
                    if (client->GetContext().GetSessionData()->IsSsl()) {
                        fd = static_cast<int>(client->GetContext().GetSessionData()->GetSslStream()->lowest_layer().native_handle());
                    } else {
                        fd = static_cast<int>(client->GetContext().GetSessionData()->GetAsioSocket()->native_handle());
                    }
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
                        qDebug() << "WSAIoctl SIO_KEEPALIVE_VALS failed: " << WSAGetLastError();
                    }
                    // TCP_KEEPCNT: Limit the number of keepalive probes.
                    // Available since Windows 10 1703.  On older Windows
                    // the call silently fails, keeping the OS default (~10).
                    // With 6 probes at 10s intervals the total detection
                    // time matches Linux/macOS: 30 + 6*10 = 90s.
                    {
                        int keepcnt = 6;
                        setsockopt(static_cast<SOCKET>(fd), IPPROTO_TCP, TCP_KEEPCNT,
                                   (const char*)&keepcnt, sizeof(keepcnt));
                    }
#else
                    // Linux / macOS: per-socket keepalive tuning
#if defined(TCP_KEEPALIVE) || defined(TCP_KEEPIDLE)
                    int keepidle  = 30;   // seconds until first keepalive probe
                    int keepintvl = 10;   // seconds between subsequent probes
                    int keepcnt   = 6;    // failed probes before disconnect
#ifdef TCP_KEEPALIVE
                    setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &keepidle,  sizeof(keepidle));
#else
                    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,  &keepidle,  sizeof(keepidle));
#endif
                    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
                    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,   &keepcnt,   sizeof(keepcnt));
#endif
                    // TCP_USER_TIMEOUT: abort connection if sent data remains
                    // unacknowledged for 90s (matches keepalive detection time).
#ifdef TCP_USER_TIMEOUT
                    unsigned int user_timeout_ms = 90000;
                    setsockopt(fd, IPPROTO_TCP, TCP_USER_TIMEOUT, &user_timeout_ms, sizeof(user_timeout_ms));
#endif
#endif
                }

                // Increase socket buffers to absorb game-start traffic bursts
                // on high-latency WiFi connections (default is often 8-16KB).
                {
                    boost::asio::socket_base::send_buffer_size    sndbuf(131072);  // 128 KB
                    boost::asio::socket_base::receive_buffer_size rcvbuf(131072);  // 128 KB
                    boost::system::error_code buf_ec;
                    if (client->GetContext().GetSessionData()->IsSsl()) {
                        client->GetContext().GetSessionData()->GetSslStream()->lowest_layer().set_option(sndbuf, buf_ec);
                        client->GetContext().GetSessionData()->GetSslStream()->lowest_layer().set_option(rcvbuf, buf_ec);
                    } else {
                        client->GetContext().GetSessionData()->GetAsioSocket()->set_option(sndbuf, buf_ec);
                        client->GetContext().GetSessionData()->GetAsioSocket()->set_option(rcvbuf, buf_ec);
                    }
                }
            }

            if (client->GetContext().GetSessionData()->IsSsl()) {
                // Start handshake with a timeout
                
                // Create handshake timeout timer
                if (!m_handshakeTimer) {
                    m_handshakeTimer.reset(new boost::asio::steady_timer(client->GetStateTimer().get_executor()));
                } else {
                }
                m_handshakeTimer->expires_after(boost::asio::chrono::seconds(4));
                m_handshakeTimer->async_wait(
                    boost::bind(&ClientStateStartConnect::HandshakeTimeout,
                                this,
                                boost::asio::placeholders::error,
                                client));
                
                client->GetContext().GetSessionData()->GetSslStream()->async_handshake(
                    boost::asio::ssl::stream_base::client,
                    boost::bind(&ClientStateStartConnect::HandleSslHandshake,
                                this,
                                boost::asio::placeholders::error,
                                client));
            } else {
                client->GetCallback().SignalNetClientConnect(MSG_SOCK_CONNECT_DONE);
                client->SetState(ClientStateStartSession::Instance());
            }
        } else if (endpoint_iterator != m_remoteEndpoint.end()) {
            // Try next resolve entry.
            ClientContext &context = client->GetContext();
            boost::system::error_code closeEc;
            boost::asio::ip::tcp::endpoint endpoint = endpoint_iterator->endpoint();

            if (context.GetSessionData()->IsSsl()) {
                context.GetSessionData()->GetSslStream()->lowest_layer().close(closeEc);
                context.GetSessionData()->GetSslStream()->lowest_layer().async_connect(
                    endpoint,
                    boost::bind(&ClientStateStartConnect::HandleConnect,
                                this,
                                boost::asio::placeholders::error,
                                ++m_remoteEndpointIterator,
                                client));
            } else {
                context.GetSessionData()->GetAsioSocket()->close(closeEc);
                context.GetSessionData()->GetAsioSocket()->async_connect(
                    endpoint,
                    boost::bind(&ClientStateStartConnect::HandleConnect,
                                this,
                                boost::asio::placeholders::error,
                                ++m_remoteEndpointIterator,
                                client));
            }
        } else {
            if (ec != boost::asio::error::operation_aborted) {
                if (client->GetContext().GetAddrFamily() == AF_INET6) {
                    throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_IPV6_FAILED, ec.value());
                } else {
                    throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_FAILED, ec.value());
                }
            }
        }
    }
}

void
ClientStateStartConnect::HandleSslHandshake(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
    
    if (&client->GetState() == this) {
        // Cancel the handshake timeout timer
        if (m_handshakeTimer) {
            m_handshakeTimer->cancel();
        }
        
        if (!ec) {
            m_handshakeRetryCount = 0; // Reset counter on success
            if (m_retryTimer) {
                m_retryTimer->cancel(); // Cancel any pending retry timer
            }
            client->GetCallback().SignalNetClientConnect(MSG_SOCK_CONNECT_DONE);
            client->SetState(ClientStateStartSession::Instance());
        } else {
            if (ec != boost::asio::error::operation_aborted) {
                
                // Try to get more detailed SSL error information
                SSL* ssl = client->GetContext().GetSessionData()->GetSslStream()->native_handle();
                if (ssl) {
                    unsigned long ssl_err = ERR_get_error();
                    if (ssl_err != 0) {
                        char err_buf[256];
                        ERR_error_string_n(ssl_err, err_buf, sizeof(err_buf));
                    }
                }
                
                // Retry handshake up to 1 time
                if (m_handshakeRetryCount < 1) {
                    m_handshakeRetryCount++;
                    // Delay: 2s
                    int delayMs = 2000;
                    
                    // Reset timeout timer to give all retries enough time
                    client->GetStateTimer().cancel();
                    client->GetStateTimer().expires_after(seconds(CLIENT_CONNECT_TIMEOUT_SEC));
                    client->GetStateTimer().async_wait(
                        boost::bind(&ClientStateStartConnect::TimerTimeout,
                                    this,
                                    boost::asio::placeholders::error,
                                    client));
                    
                    RetryHandshake(client);
                } else {
                    // Close the session to clean up async operations
                    try {
                        client->GetContext().GetSessionData()->Close();
                    } catch (...) {
                        // Ignore errors during close
                    }
                    throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_FAILED, ec.value());
                }
            } else {
                // Close the session to clean up async operations
                try {
                    client->GetContext().GetSessionData()->Close();
                } catch (...) {
                    // Ignore errors during close
                }
            }
        }
    } else {
    }
}

void
ClientStateStartConnect::RetryHandshake(boost::shared_ptr<ClientThread> client)
{
    // Use exponential backoff: 500ms, 1s, 2s, 4s, 8s
    int delayMs = 250 * (1 << m_handshakeRetryCount);
    
    
    // Cancel any existing retry timer
    if (m_retryTimer) {
        m_retryTimer->cancel();
    }
    
    // Create or reuse the retry timer - use the same executor as GetStateTimer
    if (!m_retryTimer) {
        m_retryTimer.reset(new boost::asio::steady_timer(client->GetStateTimer().get_executor()));
    }
    
    // Schedule the retry with delay
    m_retryTimer->expires_after(boost::asio::chrono::milliseconds(delayMs));
    m_retryTimer->async_wait(
        boost::bind(&ClientStateStartConnect::RetryHandshakeTimer,
                    this,
                    boost::asio::placeholders::error,
                    client));
}

void
ClientStateStartConnect::RetryHandshakeTimer(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
    if (!ec && &client->GetState() == this) {
        
        ClientContext &context = client->GetContext();
        
        // Close the old session completely
        try {
            context.GetSessionData()->Close();
        } catch (...) {
            // Ignore errors during close
        }
        
        // Recreate the session with a new SSL stream
        client->CreateContextSession();
        
        // Reset the connection timeout timer to give this retry enough time
        client->GetStateTimer().cancel();
        client->GetStateTimer().expires_after(seconds(CLIENT_CONNECT_TIMEOUT_SEC));
        client->GetStateTimer().async_wait(
            boost::bind(&ClientStateStartConnect::TimerTimeout,
                        this,
                        boost::asio::placeholders::error,
                        client));
        
        // Reset iterator to first endpoint for retry
        m_remoteEndpointIterator = m_remoteEndpoint.begin();
        
        // Get the first endpoint
        boost::asio::ip::tcp::endpoint endpoint = m_remoteEndpointIterator->endpoint();
        
        // Reconnect with the NEW session
        context.GetSessionData()->GetSslStream()->lowest_layer().async_connect(
            endpoint,
            boost::bind(&ClientStateStartConnect::HandleConnect,
                        this,
                        boost::asio::placeholders::error,
                        ++m_remoteEndpointIterator,
                        client));
    } else if (ec == boost::asio::error::operation_aborted) {
    }
}

void
ClientStateStartConnect::HandshakeTimeout(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
    
    if (!ec && &client->GetState() == this) {
        
        // Close the hanging SSL connection
        ClientContext &context = client->GetContext();
        if (context.GetSessionData()) {
            try {
                context.GetSessionData()->CloseSocketHandle();
            } catch (...) {
            }
        }
        
        // Trigger retry logic
        if (m_handshakeRetryCount < 1) {
            m_handshakeRetryCount++;
            // Wartezeit: 2s (insgesamt ~10s: 4s + 2s + 4s)
            int delayMs = 2000;
            
            // Reset connection timeout timer
            client->GetStateTimer().cancel();
            client->GetStateTimer().expires_after(seconds(CLIENT_CONNECT_TIMEOUT_SEC));
            client->GetStateTimer().async_wait(
                boost::bind(&ClientStateStartConnect::TimerTimeout,
                            this,
                            boost::asio::placeholders::error,
                            client));
            
            RetryHandshake(client);
        } else {
            // Close the session to clean up async operations
            try {
                context.GetSessionData()->Close();
            } catch (...) {
                // Ignore errors during close
            }
            if (context.GetAddrFamily() == AF_INET6)
                throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_IPV6_FAILED, 0);
            else
                throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_FAILED, 0);
        }
    } else if (ec == boost::asio::error::operation_aborted) {
    }
}

void
ClientStateStartConnect::TimerTimeout(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
    
    if (!ec && &client->GetState() == this) {
        ClientContext &context = client->GetContext();

        if (context.GetSessionData()) {
            try {
                context.GetSessionData()->Close();
            } catch (...) {
                // Ignore errors during close
            }
        }

        if (context.GetAddrFamily() == AF_INET6)
            throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_IPV6_FAILED, 0);
        else
            throw ClientException(__FILE__, __LINE__, ERR_SOCK_CONNECT_FAILED, 0);
    } else {
        if (ec) {
        } else {
        }
    }
}

//-----------------------------------------------------------------------------

AbstractClientStateReceiving::AbstractClientStateReceiving()
{
}

AbstractClientStateReceiving::~AbstractClientStateReceiving()
{
}

void
AbstractClientStateReceiving::HandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_PlayerInfoReplyMessage) {
		const PlayerInfoReplyMessage &infoReply = tmpPacket->GetMsg()->playerinforeplymessage();
		unsigned playerId = infoReply.playerid();
		if (infoReply.has_playerinfodata()) {
			PlayerInfo tmpInfo;
			const PlayerInfoReplyMessage::PlayerInfoData &netInfo = infoReply.playerinfodata();
			tmpInfo.playerName = netInfo.playername();
			tmpInfo.ptype = netInfo.ishuman() ? PLAYER_TYPE_HUMAN : PLAYER_TYPE_COMPUTER;
			tmpInfo.isGuest = netInfo.playerrights() == netPlayerRightsGuest;
			tmpInfo.isAdmin = netInfo.playerrights() == netPlayerRightsAdmin;
			if (netInfo.has_countrycode()) {
				tmpInfo.countryCode = netInfo.countrycode();
			}
			if (netInfo.has_avatardata()) {
				tmpInfo.hasAvatar = true;
				memcpy(tmpInfo.avatar.GetData(), netInfo.avatardata().avatarhash().data(), MD5_DATA_SIZE);
				tmpInfo.avatarType = static_cast<AvatarFileType>(netInfo.avatardata().avatartype());
			}
			client->SetPlayerInfo(
				playerId,
				tmpInfo);
		} else {
			client->SetUnknownPlayer(playerId);
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_RemovedFromGameMessage) {
		const RemovedFromGameMessage &netRemoved = tmpPacket->GetMsg()->removedfromgamemessage();

		client->ClearPlayerDataList();
		// Der Zuschauer-Modus gilt immer nur für das gerade verlassene Spiel.
		client->SetSpectating(false);
		// Resubscribe Lobby messages.
		client->ResubscribeLobbyMsg();
		// Show Lobby.
		client->GetCallback().SignalNetClientWaitDialog();
		int removeReason;
		switch (netRemoved.removedfromgamereason()) {
		case RemovedFromGameMessage::kickedFromGame :
			removeReason = NTF_NET_REMOVED_KICKED;
			break;
		case RemovedFromGameMessage::gameIsFull :
			removeReason = NTF_NET_REMOVED_GAME_FULL;
			break;
		case RemovedFromGameMessage::gameIsRunning :
			removeReason = NTF_NET_REMOVED_ALREADY_RUNNING;
			break;
		case RemovedFromGameMessage::gameTimeout :
			// AFK timeout: the server kicked us.  Do NOT offer rejoin –
			// the server has already cleared the GUID / marked as kicked,
			// so a rejoin attempt would be rejected anyway.
			removeReason = NTF_NET_REMOVED_TIMEOUT;
			break;
		case RemovedFromGameMessage::removedStartFailed :
			removeReason = NTF_NET_REMOVED_START_FAILED;
			break;
		case RemovedFromGameMessage::gameClosed :
			removeReason = NTF_NET_REMOVED_GAME_CLOSED;
			break;
		default :
			removeReason = NTF_NET_REMOVED_ON_REQUEST;
			break;
		}
		client->GetCallback().SignalNetClientRemovedFromGame(removeReason);
		client->SetState(ClientStateWaitJoin::Instance());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GamePlayerLeftMessage) {
		// A player left the game.
		const GamePlayerLeftMessage &netLeft = tmpPacket->GetMsg()->gameplayerleftmessage();

		if (client->GetGame()) {
			boost::shared_ptr<PlayerInterface> tmpPlayer = client->GetGame()->getPlayerByUniqueId(netLeft.playerid());
			if (tmpPlayer) {
				tmpPlayer->setIsKicked(netLeft.gameplayerleftreason() == GamePlayerLeftMessage::leftKicked);
			}
		}
		// Signal to GUI and remove from data list.
		int removeReason;
		switch (netLeft.gameplayerleftreason()) {
		case GamePlayerLeftMessage::leftKicked :
			removeReason = NTF_NET_REMOVED_KICKED;
			break;
		case GamePlayerLeftMessage::leftError :
			// Connection lost / error (server default reason) – keep it distinct
			// from a voluntary leave so the GUI can report "was disconnected".
			removeReason = NTF_NET_INTERNAL;
			break;
		default :
			removeReason = NTF_NET_REMOVED_ON_REQUEST;
			break;
		}
		client->RemovePlayerData(netLeft.playerid(), removeReason);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameAdminChangedMessage) {
		// New admin for the game.
		const GameAdminChangedMessage &netChanged = tmpPacket->GetMsg()->gameadminchangedmessage();

		// Set new game admin and signal to GUI.
		client->SetNewGameAdmin(netChanged.newadminplayerid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GamePlayerJoinedMessage) {
		// Another player joined the network game.
		const GamePlayerJoinedMessage &netPlayerJoined = tmpPacket->GetMsg()->gameplayerjoinedmessage();

		boost::shared_ptr<PlayerData> playerData = client->CreatePlayerData(netPlayerJoined.playerid(), netPlayerJoined.isgameadmin());
		client->AddPlayerData(playerData);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameSpectatorJoinedMessage) {
		// Another spectator joined the network game.
		const GameSpectatorJoinedMessage &netSpectatorJoined = tmpPacket->GetMsg()->gamespectatorjoinedmessage();
		// Request player info if needed.
		PlayerInfo info;
		if (!client->GetCachedPlayerInfo(netSpectatorJoined.playerid(), info)) {
			client->RequestPlayerInfo(netSpectatorJoined.playerid());
		}
		client->ModifyGameInfoAddSpectatorDuringGame(netSpectatorJoined.playerid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameSpectatorLeftMessage) {
		// A spectator left the network game.
		const GameSpectatorLeftMessage &netSpectatorLeft = tmpPacket->GetMsg()->gamespectatorleftmessage();
		// Signal to GUI and remove from data list.
		int removeReason;
		switch (netSpectatorLeft.gamespectatorleftreason()) {
		case GamePlayerLeftMessage::leftKicked :
			removeReason = NTF_NET_REMOVED_KICKED;
			break;
		default :
			removeReason = NTF_NET_REMOVED_ON_REQUEST;
			break;
		}
		client->ModifyGameInfoRemoveSpectatorDuringGame(netSpectatorLeft.playerid(), removeReason);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_TimeoutWarningMessage) {
		const TimeoutWarningMessage &tmpTimeout = tmpPacket->GetMsg()->timeoutwarningmessage();
		client->GetCallback().SignalNetClientShowTimeoutDialog((NetTimeoutReason)tmpTimeout.timeoutreason(), tmpTimeout.remainingseconds());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_YourActionRejectedMessage) {
		// Server hat unsere Aktion abgelehnt – im Client bisher völlig
		// ignoriert (kein Handler-Zweig, kein Log). Genau dieser stille
		// Verlust ist der gesuchte UTG/SB-preflop-Bug-Indikator.
		const YourActionRejectedMessage &rej = tmpPacket->GetMsg()->youractionrejectedmessage();
		qDebug() << "[REJECT] YourActionRejected"
		         << "gamestate=" << (int)rej.gamestate()
		         << "(0=Pre,1=F,2=T,3=R)"
		         << "youraction=" << (int)rej.youraction()
		         << "(1=FOLD,2=CHK,3=CALL,4=BET,5=RAISE,6=ALLIN)"
		         << "yourbet=" << (int)rej.yourrelativebet()
		         << "reason=" << (int)rej.rejectionreason()
		         << "(1=INVALID_STATE,2=NOT_YOUR_TURN,3=NOT_ALLOWED)";
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_ChatMessage) {
		// Chat message - display it in the GUI.
		const ChatMessage &netMessage = tmpPacket->GetMsg()->chatmessage();

		string playerName;
		if (netMessage.chattype() == ChatMessage::chatTypeBroadcast) {
			client->GetCallback().SignalNetClientGameChatMsg("(global notice)", netMessage.chattext());
			client->GetCallback().SignalNetClientLobbyChatMsg("(global notice)", netMessage.chattext());
		} else if (netMessage.chattype() == ChatMessage::chatTypeBot) {
			client->GetCallback().SignalNetClientGameChatMsg("(chat bot)", netMessage.chattext());
			client->GetCallback().SignalNetClientLobbyChatMsg("(chat bot)", netMessage.chattext());
		} else if (netMessage.chattype() == ChatMessage::chatTypeGame) {
			unsigned playerId = netMessage.playerid();
			boost::shared_ptr<PlayerData> tmpPlayer = client->GetPlayerDataByUniqueId(playerId);
			if (tmpPlayer.get()) {
				playerName = tmpPlayer->GetName();
			} else {
				// Zuschauer dürfen mitreden (der Server verteilt ihren Chat an
				// Spieler wie Zuschauer), sitzen aber nicht am Tisch und stehen
				// daher in KEINER Spielerliste. Ihr Name steht in den gecachten
				// PlayerInfos – GameSpectatorJoinedMessage fordert sie an.
				PlayerInfo info;
				if (client->GetCachedPlayerInfo(playerId, info))
					playerName = info.playerName;
				else
					client->RequestPlayerInfo(playerId);
			}
			if (!playerName.empty())
				client->GetCallback().SignalNetClientGameChatMsg(playerName, netMessage.chattext());
		} else if (netMessage.chattype() == ChatMessage::chatTypeLobby) {
			unsigned playerId = netMessage.playerid();
			PlayerInfo info;
			if (client->GetCachedPlayerInfo(playerId, info))
				client->GetCallback().SignalNetClientLobbyChatMsg(info.playerName, netMessage.chattext());
		} else if (netMessage.chattype() == ChatMessage::chatTypePrivate) {
			unsigned playerId = netMessage.playerid();
			PlayerInfo info;
			if (client->GetCachedPlayerInfo(playerId, info))
				client->GetCallback().SignalNetClientPrivateChatMsg(info.playerName, netMessage.chattext());
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_ChatRejectMessage) {
		const ChatRejectMessage &netMessage = tmpPacket->GetMsg()->chatrejectmessage();
		client->GetCallback().SignalNetClientGameChatMsg("(notice)", "Chat rejected: " + netMessage.chattext());
		client->GetCallback().SignalNetClientLobbyChatMsg("(notice)", "Chat rejected: " + netMessage.chattext());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_DialogMessage) {
		// Message box - display it in the GUI.
		const DialogMessage &netDialog = tmpPacket->GetMsg()->dialogmessage();
		client->GetCallback().SignalNetClientMsgBox(netDialog.notificationtext());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_PlayerListMessage) {
		const PlayerListMessage &netPlayerList = tmpPacket->GetMsg()->playerlistmessage();

		if (netPlayerList.playerlistnotification() == PlayerListMessage::playerListNew) {
			client->GetCallback().SignalLobbyPlayerJoined(netPlayerList.playerid(), client->GetPlayerName(netPlayerList.playerid()));
		} else if (netPlayerList.playerlistnotification() == PlayerListMessage::playerListLeft) {
			unsigned leftPlayerId = netPlayerList.playerid();
			client->GetCallback().SignalLobbyPlayerLeft(leftPlayerId);
			client->RemoveCachedPlayerInfo(leftPlayerId);
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListNewMessage) {
		// A new game was created on the server.
		const GameListNewMessage &netListNew = tmpPacket->GetMsg()->gamelistnewmessage();

		// Request player info for players if needed.
		GameInfo tmpInfo;
		list<unsigned> requestList;
		// All players.
		for (int i = 0; i < netListNew.playerids_size(); i++) {
			PlayerInfo info;
			unsigned playerId = netListNew.playerids(i);
			if (!client->GetCachedPlayerInfo(playerId, info)) {
				requestList.push_back(playerId);
			}
			tmpInfo.players.push_back(playerId);
		}
		// All spectators.
		for (int i = 0; i < netListNew.spectatorids_size(); i++) {
			PlayerInfo info;
			unsigned playerId = netListNew.spectatorids(i);
			if (!client->GetCachedPlayerInfo(playerId, info)) {
				requestList.push_back(playerId);
			}
			tmpInfo.spectators.push_back(playerId);
		}
		// Send request for multiple players (will only act if list is non-empty).
		client->RequestPlayerInfo(requestList);

		tmpInfo.adminPlayerId = netListNew.adminplayerid();
		tmpInfo.isPasswordProtected = netListNew.isprivate();
		tmpInfo.mode = static_cast<GameMode>(netListNew.gamemode());
		tmpInfo.name = netListNew.gameinfo().gamename();
		NetPacket::GetGameData(netListNew.gameinfo(), tmpInfo.data);

		client->AddGameInfo(netListNew.gameid(), tmpInfo);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListUpdateMessage) {
		// An existing game was updated on the server.
		const GameListUpdateMessage &netListUpdate = tmpPacket->GetMsg()->gamelistupdatemessage();
		if (netListUpdate.gamemode() == netGameClosed)
			client->RemoveGameInfo(netListUpdate.gameid());
		else
			client->UpdateGameInfoMode(netListUpdate.gameid(), static_cast<GameMode>(netListUpdate.gamemode()));
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListPlayerJoinedMessage) {
		const GameListPlayerJoinedMessage &netListJoined = tmpPacket->GetMsg()->gamelistplayerjoinedmessage();

		client->ModifyGameInfoAddPlayer(netListJoined.gameid(), netListJoined.playerid());
		// Request player info if needed.
		PlayerInfo info;
		if (!client->GetCachedPlayerInfo(netListJoined.playerid(), info)) {
			client->RequestPlayerInfo(netListJoined.playerid());
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListPlayerLeftMessage) {
		const GameListPlayerLeftMessage &netListLeft = tmpPacket->GetMsg()->gamelistplayerleftmessage();

		client->ModifyGameInfoRemovePlayer(netListLeft.gameid(), netListLeft.playerid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListSpectatorJoinedMessage) {
		const GameListSpectatorJoinedMessage &netListJoined = tmpPacket->GetMsg()->gamelistspectatorjoinedmessage();

		client->ModifyGameInfoAddSpectator(netListJoined.gameid(), netListJoined.playerid());
		// Request player info if needed.
		PlayerInfo info;
		if (!client->GetCachedPlayerInfo(netListJoined.playerid(), info)) {
			client->RequestPlayerInfo(netListJoined.playerid());
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListSpectatorLeftMessage) {
		const GameListSpectatorLeftMessage &netListLeft = tmpPacket->GetMsg()->gamelistspectatorleftmessage();

		client->ModifyGameInfoRemoveSpectator(netListLeft.gameid(), netListLeft.playerid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameListAdminChangedMessage) {
		const GameListAdminChangedMessage &netListAdmin = tmpPacket->GetMsg()->gamelistadminchangedmessage();

		client->UpdateGameInfoAdmin(netListAdmin.gameid(), netListAdmin.newadminplayerid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_StartKickPetitionMessage) {
		const StartKickPetitionMessage &netStartPetition = tmpPacket->GetMsg()->startkickpetitionmessage();
		client->StartPetition(netStartPetition.petitionid(), netStartPetition.proposingplayerid(),
							  netStartPetition.kickplayerid(), netStartPetition.kicktimeoutsec(), netStartPetition.numvotesneededtokick());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_KickPetitionUpdateMessage) {
		const KickPetitionUpdateMessage &netPetitionUpdate = tmpPacket->GetMsg()->kickpetitionupdatemessage();
		client->UpdatePetition(netPetitionUpdate.petitionid(), netPetitionUpdate.numvotesagainstkicking(),
							   netPetitionUpdate.numvotesinfavourofkicking(), netPetitionUpdate.numvotesneededtokick());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_EndKickPetitionMessage) {
		const EndKickPetitionMessage &netEndPetition = tmpPacket->GetMsg()->endkickpetitionmessage();
		client->EndPetition(netEndPetition.petitionid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarHeaderMessage) {
		const AvatarHeaderMessage &netAvatarHeader = tmpPacket->GetMsg()->avatarheadermessage();
		client->AddTempAvatarFile(netAvatarHeader.requestid(), netAvatarHeader.avatarsize(), static_cast<AvatarFileType>(netAvatarHeader.avatartype()));
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarDataMessage) {
		const AvatarDataMessage &netAvatarData = tmpPacket->GetMsg()->avatardatamessage();
		vector<unsigned char> fileData(netAvatarData.avatarblock().size());
		memcpy(&fileData[0], netAvatarData.avatarblock().data(), netAvatarData.avatarblock().size());
		client->StoreInTempAvatarFile(netAvatarData.requestid(), fileData);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarEndMessage) {
		const AvatarEndMessage &netAvatarEnd = tmpPacket->GetMsg()->avatarendmessage();
		client->CompleteTempAvatarFile(netAvatarEnd.requestid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_UnknownAvatarMessage) {
		const UnknownAvatarMessage &netUnknownAvatar = tmpPacket->GetMsg()->unknownavatarmessage();
		client->SetUnknownAvatar(netUnknownAvatar.requestid());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_ReportAvatarAckMessage) {
		const ReportAvatarAckMessage &netReportAck = tmpPacket->GetMsg()->reportavatarackmessage();
		unsigned msgCode;
		switch (netReportAck.reportavatarresult()) {
		case ReportAvatarAckMessage::avatarReportAccepted:
			msgCode = MSG_NET_AVATAR_REPORT_ACCEPTED;
			break;
		case ReportAvatarAckMessage::avatarReportDuplicate:
			msgCode = MSG_NET_AVATAR_REPORT_DUP;
			break;
		default:
			msgCode = MSG_NET_AVATAR_REPORT_REJECTED;
			break;
		}
		client->GetCallback().SignalNetClientMsgBox(msgCode);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_ReportGameAckMessage) {
		const ReportGameAckMessage &netReportAck = tmpPacket->GetMsg()->reportgameackmessage();
		unsigned msgCode;
		switch (netReportAck.reportgameresult()) {
		case ReportGameAckMessage::gameReportAccepted:
			msgCode = MSG_NET_GAMENAME_REPORT_ACCEPTED;
			break;
		case ReportGameAckMessage::gameReportDuplicate:
			msgCode = MSG_NET_GAMENAME_REPORT_DUP;
			break;
		default:
			msgCode = MSG_NET_GAMENAME_REPORT_REJECTED;
			break;
		}
		client->GetCallback().SignalNetClientMsgBox(msgCode);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AdminRemoveGameAckMessage) {
		const AdminRemoveGameAckMessage &netRemoveAck = tmpPacket->GetMsg()->adminremovegameackmessage();
		unsigned msgCode;
		switch (netRemoveAck.removegameresult()) {
		case AdminRemoveGameAckMessage::gameRemoveAccepted:
			msgCode = MSG_NET_ADMIN_REMOVE_GAME_ACCEPTED;
			break;
		default:
			msgCode = MSG_NET_ADMIN_REMOVE_GAME_REJECTED;
			break;
		}
		client->GetCallback().SignalNetClientMsgBox(msgCode);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AdminBanPlayerAckMessage) {
		const AdminBanPlayerAckMessage &netBanAck = tmpPacket->GetMsg()->adminbanplayerackmessage();
		unsigned msgCode;
		switch (netBanAck.banplayerresult()) {
		case AdminBanPlayerAckMessage::banPlayerAccepted:
			msgCode = MSG_NET_ADMIN_BAN_PLAYER_ACCEPTED;
			break;
		case AdminBanPlayerAckMessage::banPlayerPending:
			msgCode = MSG_NET_ADMIN_BAN_PLAYER_PENDING;
			break;
		case AdminBanPlayerAckMessage::banPlayerNoDB:
			msgCode = MSG_NET_ADMIN_BAN_PLAYER_NODB;
			break;
		case AdminBanPlayerAckMessage::banPlayerDBError:
			msgCode = MSG_NET_ADMIN_BAN_PLAYER_DBERROR;
			break;
		default:
			msgCode = MSG_NET_ADMIN_BAN_PLAYER_REJECTED;
			break;
		}
		client->GetCallback().SignalNetClientMsgBox(msgCode);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_StatisticsMessage) {
		const StatisticsMessage &netStatistics = tmpPacket->GetMsg()->statisticsmessage();

		unsigned numStats = netStatistics.statisticsdata_size();
		// Request player info for players if needed.
		if (numStats) {
			ServerStats tmpStats;
			for (unsigned i = 0; i < numStats; i++) {
				if (netStatistics.statisticsdata(i).statisticstype() == StatisticsMessage::StatisticsData::statNumberOfPlayers)
					tmpStats.numberOfPlayersOnServer = netStatistics.statisticsdata(i).statisticsvalue();
			}
			client->UpdateStatData(tmpStats);
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_ErrorMessage) {
		// Server reported an error.
		const ErrorMessage &netError = tmpPacket->GetMsg()->errormessage();
		// Show the error.
		throw ClientException(__FILE__, __LINE__, NetPacket::NetErrorToGameError(netError.errorreason()), 0);
	}

	InternalHandlePacket(client, tmpPacket);
}

//-----------------------------------------------------------------------------

ClientStateStartSession &
ClientStateStartSession::Instance()
{
	static ClientStateStartSession state;
	return state;
}

ClientStateStartSession::ClientStateStartSession()
{
}

ClientStateStartSession::~ClientStateStartSession()
{
}

void
ClientStateStartSession::Enter(boost::shared_ptr<ClientThread> client)
{
	// Now we finally start receiving data.
	client->StartAsyncRead();
}

void
ClientStateStartSession::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateStartSession::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AnnounceMessage) {
		// Server has send announcement - check data.
		const AnnounceMessage &netAnnounce = tmpPacket->GetMsg()->announcemessage();
		// Check current game version.
		if (netAnnounce.latestgameversion().majorversion() != POKERTH_VERSION_MAJOR
				|| netAnnounce.latestgameversion().minorversion() != POKERTH_VERSION_MINOR) {
			client->GetCallback().SignalNetClientNotification(NTF_NET_NEW_RELEASE_AVAILABLE);
		} else if (POKERTH_BETA_REVISION && netAnnounce.latestbetarevision() != POKERTH_BETA_REVISION) {
			client->GetCallback().SignalNetClientNotification(NTF_NET_OUTDATED_BETA);
		}
		ClientContext &context = client->GetContext();

		// CASE 1: Authenticated login (username, challenge/response for password).
		if (netAnnounce.servertype() == AnnounceMessage::serverTypeInternetAuth) {
			client->GetCallback().SignalNetClientLoginShow();
			client->SetState(ClientStateWaitEnterLogin::Instance());
		}
		// CASE 2: Unauthenticated login (network game or dedicated server without auth backend).
		else if (netAnnounce.servertype() == AnnounceMessage::serverTypeInternetNoAuth
				 || netAnnounce.servertype() == AnnounceMessage::serverTypeLAN) {
			boost::shared_ptr<NetPacket> init(new NetPacket);
			init->GetMsg()->set_messagetype(PokerTHMessage::Type_InitMessage);
			InitMessage *netInit = init->GetMsg()->mutable_initmessage();
			netInit->mutable_requestedversion()->set_majorversion(NET_VERSION_MAJOR);
			netInit->mutable_requestedversion()->set_minorversion(NET_VERSION_MINOR);
			// Announce the version matching the client type. QML clients must
			// send the QML version (the server's MIN_BUILD_ID_QML check uses it),
			// otherwise a current server rejects them with "version not supported".
			netInit->set_buildid(context.GetClientType() == CLIENT_TYPE_QML
				? MAKE_BUILD_ID(CLIENT_TYPE_QML, QML_VERSION_MAJOR, QML_VERSION_MINOR, QML_VERSION_REVISION)
				: MAKE_BUILD_ID(CLIENT_TYPE_QT_WIDGET, POKERTH_VERSION_MAJOR, POKERTH_VERSION_MINOR, POKERTH_BETA_REVISION));
			if (!context.GetSessionGuid().empty()) {
				netInit->set_mylastsessionid(context.GetSessionGuid());
			}
			// Rejoin-Diagnose: das gesendete mylastsessionid entscheidet, ob der
			// Server ein laufendes Spiel zum Wiedereinstieg anbieten kann. Leer =
			// keine gespeicherte GUID (guid.tmp fehlt/nicht gelesen) => kein Rejoin.
			LOG_MSG("[REJOIN] sending Init (unauth), mylastsessionid size="
					<< context.GetSessionGuid().size());
			if (!context.GetServerPassword().empty()) {
				netInit->set_authserverpassword(context.GetServerPassword());
			}
			netInit->set_login(InitMessage::unauthenticatedLogin);
			netInit->set_nickname(context.GetPlayerName());
			string avatarFile = client->GetQtToolsInterface().stringFromUtf8(context.GetAvatarFile());
			if (!avatarFile.empty()) {
				MD5Buf tmpMD5;
				if (client->GetAvatarManager().GetHashForAvatar(avatarFile, tmpMD5)) {
					// Send MD5 hash of avatar.
					netInit->set_avatarhash(tmpMD5.GetData(), MD5_DATA_SIZE);
				}
			}
			client->GetSender().Send(context.GetSessionData(), init);
			client->SetState(ClientStateWaitSession::Instance());
		}
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitEnterLogin &
ClientStateWaitEnterLogin::Instance()
{
	static ClientStateWaitEnterLogin state;
	return state;
}

ClientStateWaitEnterLogin::ClientStateWaitEnterLogin()
{
}

ClientStateWaitEnterLogin::~ClientStateWaitEnterLogin()
{
}

void
ClientStateWaitEnterLogin::Enter(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().expires_after(milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
	client->GetStateTimer().async_wait(
		boost::bind(
			&ClientStateWaitEnterLogin::TimerLoop, this, boost::asio::placeholders::error, client));
}

void
ClientStateWaitEnterLogin::Exit(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().cancel();
}

void
ClientStateWaitEnterLogin::HandlePacket(boost::shared_ptr<ClientThread> /*client*/, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_ErrorMessage) {
		// Server reported an error.
		const ErrorMessage &netError = tmpPacket->GetMsg()->errormessage();
		// Show the error.
		throw ClientException(__FILE__, __LINE__, NetPacket::NetErrorToGameError(netError.errorreason()), 0);
	}
}

void
ClientStateWaitEnterLogin::TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
    if (!ec && &client->GetState() == this) {
        ClientThread::LoginData loginData;
        if (client->GetLoginData(loginData)) {
            ClientContext &context = client->GetContext();
            boost::shared_ptr<NetPacket> init(new NetPacket);
            init->GetMsg()->set_messagetype(PokerTHMessage::Type_InitMessage);
            InitMessage *netInit = init->GetMsg()->mutable_initmessage();
            netInit->mutable_requestedversion()->set_majorversion(NET_VERSION_MAJOR);
            netInit->mutable_requestedversion()->set_minorversion(NET_VERSION_MINOR);
            // Announce the version matching the client type (see above).
            netInit->set_buildid(context.GetClientType() == CLIENT_TYPE_QML
                ? MAKE_BUILD_ID(CLIENT_TYPE_QML, QML_VERSION_MAJOR, QML_VERSION_MINOR, QML_VERSION_REVISION)
                : MAKE_BUILD_ID(CLIENT_TYPE_QT_WIDGET, POKERTH_VERSION_MAJOR, POKERTH_VERSION_MINOR, POKERTH_BETA_REVISION));
            
            // Include session GUID and server password BEFORE setting login type
            if (!context.GetSessionGuid().empty()) {
                netInit->set_mylastsessionid(context.GetSessionGuid());
            }
            // Rejoin-Diagnose (siehe unauth-Pfad oben): leere mylastsessionid =>
            // kein Wiedereinstieg möglich.
            LOG_MSG("[REJOIN] sending Init (auth), mylastsessionid size="
                    << context.GetSessionGuid().size());
            if (!context.GetServerPassword().empty()) {
                netInit->set_authserverpassword(context.GetServerPassword());
            }

            context.SetPlayerName(loginData.userName);

            // Send avatar hash for all login types (guest and authenticated).
            {
                string avatarFile = client->GetQtToolsInterface().stringFromUtf8(context.GetAvatarFile());
                if (!avatarFile.empty()) {
                    MD5Buf tmpMD5;
                    if (client->GetAvatarManager().GetHashForAvatar(avatarFile, tmpMD5)) {
                        netInit->set_avatarhash(tmpMD5.GetData(), MD5_DATA_SIZE);
                    }
                }
            }

            // Handle guest login first.
            if (loginData.isGuest) {
                context.SetPassword("");
                context.SetPlayerRights(PLAYER_RIGHTS_GUEST);
                netInit->set_login(InitMessage::guestLogin);
                netInit->set_nickname(context.GetPlayerName());

                client->GetSender().Send(context.GetSessionData(), init);
                client->SetState(ClientStateWaitSession::Instance());
            }
            // If the player is not a guest, authenticate.
            else {
                context.SetPassword(loginData.password);
                netInit->set_login(InitMessage::authenticatedLogin);
                netInit->set_nickname(context.GetPlayerName());
                if (!context.GetPassword().empty()) {
                    netInit->set_clientuserdata(context.GetPassword());
                }

                client->GetSender().Send(context.GetSessionData(), init);
                client->SetState(ClientStateWaitSession::Instance());
            }
        } else {
            client->GetStateTimer().expires_after(milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
            client->GetStateTimer().async_wait(
                boost::bind(
                    &ClientStateWaitEnterLogin::TimerLoop, this, boost::asio::placeholders::error, client));
        }
    }
}

//-----------------------------------------------------------------------------

ClientStateWaitAuthChallenge &
ClientStateWaitAuthChallenge::Instance()
{
	static ClientStateWaitAuthChallenge state;
	return state;
}

ClientStateWaitAuthChallenge::ClientStateWaitAuthChallenge()
{
}

ClientStateWaitAuthChallenge::~ClientStateWaitAuthChallenge()
{
}

void
ClientStateWaitAuthChallenge::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitAuthChallenge::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitAuthChallenge::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AuthServerChallengeMessage) {
		const AuthServerChallengeMessage &netAuth = tmpPacket->GetMsg()->authserverchallengemessage();
		string challengeStr(netAuth.serverchallenge());
		boost::shared_ptr<SessionData> tmpSession = client->GetContext().GetSessionData();
		if (!tmpSession->AuthStep(2, challengeStr.c_str()))
			throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD, 0);
		string outUserData(tmpSession->AuthGetNextOutMsg());

		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AuthClientResponseMessage);
		AuthClientResponseMessage *outAuth = packet->GetMsg()->mutable_authclientresponsemessage();
		outAuth->set_clientresponse(outUserData);
		client->GetSender().Send(tmpSession, packet);
		client->SetState(ClientStateWaitAuthVerify::Instance());
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitAuthVerify &
ClientStateWaitAuthVerify::Instance()
{
	static ClientStateWaitAuthVerify state;
	return state;
}

ClientStateWaitAuthVerify::ClientStateWaitAuthVerify()
{
}

ClientStateWaitAuthVerify::~ClientStateWaitAuthVerify()
{
}

void
ClientStateWaitAuthVerify::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitAuthVerify::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitAuthVerify::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AuthServerVerificationMessage) {
		// Check subtype.
		const AuthServerVerificationMessage &netAuth = tmpPacket->GetMsg()->authserververificationmessage();
		string verificationStr(netAuth.serververification());
		boost::shared_ptr<SessionData> tmpSession = client->GetContext().GetSessionData();
		if (!tmpSession->AuthStep(3, verificationStr.c_str()))
			throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PASSWORD, 0);

		client->SetState(ClientStateWaitSession::Instance());
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitSession &
ClientStateWaitSession::Instance()
{
	static ClientStateWaitSession state;
	return state;
}

ClientStateWaitSession::ClientStateWaitSession()
{
}

ClientStateWaitSession::~ClientStateWaitSession()
{
}

void
ClientStateWaitSession::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitSession::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitSession::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_InitAckMessage) {
		// Everything is fine - we are in the lobby.
		const InitAckMessage &netInitAck = tmpPacket->GetMsg()->initackmessage();
		client->SetGuiPlayerId(netInitAck.yourplayerid());

		client->GetContext().SetSessionGuid(netInitAck.yoursessionid());
		client->SetSessionEstablished(true);
		client->GetCallback().SignalNetClientConnect(MSG_SOCK_SESSION_DONE);
		// Rejoin-Diagnose: liegt hier eine rejoingameid an, hält der Server noch
		// unseren Sitz in einem laufenden Spiel bereit und wir bieten den
		// Wiedereinstieg an. Fehlt sie trotz vorher gesendeter mylastsessionid,
		// liegt es serverseitig (Sitz nach 5 min freigegeben, Name/GUID/Cash-
		// Mismatch, oder als "in Lobby" deaktiviert), nicht am Client.
		if (netInitAck.has_rejoingameid()) {
			LOG_MSG("[REJOIN] InitAck offers rejoin, gameId=" << netInitAck.rejoingameid());
			client->GetCallback().SignalNetClientRejoinPossible(netInitAck.rejoingameid());
		} else {
			LOG_MSG("[REJOIN] InitAck without rejoin offer (no running game held for us)");
		}
		client->SetState(ClientStateWaitJoin::Instance());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarRequestMessage) {
		// Before letting us join the lobby, the server requests our avatar.
		const AvatarRequestMessage &netAvatarRequest = tmpPacket->GetMsg()->avatarrequestmessage();

		// TODO compare SHA1.
		NetPacketList tmpList;
		int avatarError = client->GetAvatarManager().AvatarFileToNetPackets(
							  client->GetQtToolsInterface().stringFromUtf8(client->GetContext().GetAvatarFile()),
							  netAvatarRequest.requestid(),
							  tmpList);

		if (!avatarError) {
			client->GetSender().Send(client->GetContext().GetSessionData(), tmpList);
		} else {
			throw ClientException(__FILE__, __LINE__, avatarError, 0);
		}
	} else {
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitJoin &
ClientStateWaitJoin::Instance()
{
	static ClientStateWaitJoin state;
	return state;
}

ClientStateWaitJoin::ClientStateWaitJoin()
{
}

ClientStateWaitJoin::~ClientStateWaitJoin()
{
}

void
ClientStateWaitJoin::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitJoin::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitJoin::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	ClientContext &context = client->GetContext();

	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_JoinGameAckMessage) {
		const JoinGameAckMessage &netJoinAck = tmpPacket->GetMsg()->joingameackmessage();
		// Successfully joined a game.
		client->SetGameId(netJoinAck.gameid());
		GameData tmpData;
		NetPacket::GetGameData(netJoinAck.gameinfo(), tmpData);
		client->SetGameData(tmpData);
		client->ModifyGameInfoClearSpectatorsDuringGame();
		client->SetSpectating(netJoinAck.spectateonly());

		if (netJoinAck.spectateonly()) {
			// Ein Zuschauer belegt keinen Sitz. Legten wir hier PlayerData an,
			// zöge InitGame() daraus einen zusätzlichen Phantom-Sitz am Tisch.
			// Der GUI muss der Beitritt trotzdem gemeldet werden – sonst
			// bekäme sie das sonst über AddPlayerData() ausgelöste
			// SignalNetClientSelfJoined nie zu sehen.
			client->GetCallback().SignalNetClientSelfJoined(
				client->GetGuiPlayerId(), context.GetPlayerName(), false);
		} else {
			// Player number is 0 on init. Will be set when the game starts.
			boost::shared_ptr<PlayerData> playerData(
				new PlayerData(client->GetGuiPlayerId(), 0, PLAYER_TYPE_HUMAN,
							   context.GetPlayerRights(), netJoinAck.areyougameadmin()));
			playerData->SetName(context.GetPlayerName());
			playerData->SetAvatarFile(context.GetAvatarFile());
			client->AddPlayerData(playerData);
		}

		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_JOIN);
		client->SetState(ClientStateWaitGame::Instance());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_JoinGameFailedMessage) {
		// Failed to join a game.
		const JoinGameFailedMessage &netJoinFailed = tmpPacket->GetMsg()->joingamefailedmessage();

		int failureCode;
		switch (netJoinFailed.joingamefailurereason()) {
		case JoinGameFailedMessage::invalidGame :
			failureCode = NTF_NET_JOIN_GAME_INVALID;
			break;
		case JoinGameFailedMessage::gameIsFull :
			failureCode = NTF_NET_JOIN_GAME_FULL;
			break;
		case JoinGameFailedMessage::gameIsRunning :
			failureCode = NTF_NET_JOIN_ALREADY_RUNNING;
			break;
		case JoinGameFailedMessage::invalidPassword :
			failureCode = NTF_NET_JOIN_INVALID_PASSWORD;
			break;
		case JoinGameFailedMessage::notAllowedAsGuest :
			failureCode = NTF_NET_JOIN_GUEST_FORBIDDEN;
			break;
		case JoinGameFailedMessage::notInvited :
			failureCode = NTF_NET_JOIN_NOT_INVITED;
			break;
		case JoinGameFailedMessage::gameNameInUse :
			failureCode = NTF_NET_JOIN_GAME_NAME_IN_USE;
			break;
		case JoinGameFailedMessage::badGameName :
			failureCode = NTF_NET_JOIN_GAME_BAD_NAME;
			break;
		case JoinGameFailedMessage::invalidSettings :
			failureCode = NTF_NET_JOIN_INVALID_SETTINGS;
			break;
		case JoinGameFailedMessage::ipAddressBlocked :
			failureCode = NTF_NET_JOIN_IP_BLOCKED;
			break;
		case JoinGameFailedMessage::rejoinFailed :
			failureCode = NTF_NET_JOIN_REJOIN_FAILED;
			break;
		case JoinGameFailedMessage::noSpectatorsAllowed :
			failureCode = NTF_NET_JOIN_NO_SPECTATORS;
			break;
		default :
			failureCode = NTF_NET_INTERNAL;
			break;
		}

		client->GetCallback().SignalNetClientNotification(failureCode);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_InviteNotifyMessage) {
		const InviteNotifyMessage &netInvNotify = tmpPacket->GetMsg()->invitenotifymessage();
		qDebug() << "[INVITE] ClientStateWaitJoin: InviteNotifyMessage received"
		         << "playeridwho=" << netInvNotify.playeridwho()
		         << "playeridbywhom=" << netInvNotify.playeridbywhom()
		         << "gameid=" << netInvNotify.gameid()
		         << "myPlayerId=" << client->GetGuiPlayerId()
		         << "isSelf=" << (netInvNotify.playeridwho() == client->GetGuiPlayerId());
		if (netInvNotify.playeridwho() == client->GetGuiPlayerId()) {
			client->GetCallback().SignalSelfGameInvitation(netInvNotify.gameid(), netInvNotify.playeridbywhom());
		}
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitGame &
ClientStateWaitGame::Instance()
{
	static ClientStateWaitGame state;
	return state;
}

ClientStateWaitGame::ClientStateWaitGame()
{
}

ClientStateWaitGame::~ClientStateWaitGame()
{
}

void
ClientStateWaitGame::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitGame::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitGame::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_StartEventMessage) {
		const StartEventMessage &netStartEvent = tmpPacket->GetMsg()->starteventmessage();
		if (netStartEvent.starteventtype() == StartEventMessage::rejoinEvent) {
			client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_SYNCREJOIN);
		} else {
			client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_SYNCSTART);
		}
		client->SetState(ClientStateSynchronizeStart::Instance());
	} else if (client->IsSpectating()
			   && tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameStartRejoinMessage) {
		// Einen Zuschauer setzt der Server zu Beginn der nächsten Hand in das
		// laufende Spiel (ServerGameStateHand::InitNewSpectators) und schickt
		// ihm den Spielstand direkt als GameStartRejoinMessage. Es gibt dabei
		// KEIN StartEvent und keine Start-Synchronisation – Zuschauer zählen
		// nicht zu den Sessions, auf deren StartEventAck der Server wartet.
		client->UnsubscribeLobbyMsg();
		client->SetState(ClientStateWaitStart::Instance());
		// Den Spielstart an den nächsten Zustand weiterreichen.
		client->GetState().HandlePacket(client, tmpPacket);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_InviteNotifyMessage) {
		const InviteNotifyMessage &netInvNotify = tmpPacket->GetMsg()->invitenotifymessage();
		qDebug() << "[INVITE] ClientStateWaitGame: InviteNotifyMessage received"
		         << "playeridwho=" << netInvNotify.playeridwho()
		         << "playeridbywhom=" << netInvNotify.playeridbywhom()
		         << "gameid=" << netInvNotify.gameid()
		         << "myPlayerId=" << client->GetGuiPlayerId()
		         << "isSelf=" << (netInvNotify.playeridwho() == client->GetGuiPlayerId());
		if (netInvNotify.playeridwho() == client->GetGuiPlayerId()) {
			client->GetCallback().SignalSelfGameInvitation(netInvNotify.gameid(), netInvNotify.playeridbywhom());
		} else {
			client->GetCallback().SignalPlayerGameInvitation(
				netInvNotify.gameid(),
				netInvNotify.playeridwho(),
				netInvNotify.playeridbywhom());
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_RejectInvNotifyMessage) {
		const RejectInvNotifyMessage &netRejNotify = tmpPacket->GetMsg()->rejectinvnotifymessage();
		client->GetCallback().SignalRejectedGameInvitation(
			netRejNotify.gameid(),
			netRejNotify.playerid(),
			static_cast<DenyGameInvitationReason>(netRejNotify.playerrejectreason()));
	}
}

//-----------------------------------------------------------------------------

ClientStateSynchronizeStart &
ClientStateSynchronizeStart::Instance()
{
	static ClientStateSynchronizeStart state;
	return state;
}

ClientStateSynchronizeStart::ClientStateSynchronizeStart()
{
}

ClientStateSynchronizeStart::~ClientStateSynchronizeStart()
{
}

void
ClientStateSynchronizeStart::Enter(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().expires_after(milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
	client->GetStateTimer().async_wait(
		boost::bind(
			&ClientStateSynchronizeStart::TimerLoop, this, boost::asio::placeholders::error, client));
}

void
ClientStateSynchronizeStart::Exit(boost::shared_ptr<ClientThread> client)
{
	client->GetStateTimer().cancel();
}

void
ClientStateSynchronizeStart::TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client)
{
	if (!ec && &client->GetState() == this) {
		if (client->IsSynchronized()) {
			// Acknowledge start.
			boost::shared_ptr<NetPacket> startAck(new NetPacket);
			startAck->GetMsg()->set_messagetype(PokerTHMessage::Type_StartEventAckMessage);
			StartEventAckMessage *netStartAck = startAck->GetMsg()->mutable_starteventackmessage();
			netStartAck->set_gameid(client->GetGameId());

			client->GetSender().Send(client->GetContext().GetSessionData(), startAck);
			// Unsubscribe lobby messages.
			client->UnsubscribeLobbyMsg();

			client->SetState(ClientStateWaitStart::Instance());
		} else {
			client->GetStateTimer().expires_after(milliseconds(CLIENT_WAIT_TIMEOUT_MSEC));
			client->GetStateTimer().async_wait(
				boost::bind(
					&ClientStateSynchronizeStart::TimerLoop, this, boost::asio::placeholders::error, client));
		}
	}
}

void
ClientStateSynchronizeStart::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameStartInitialMessage) {
		// Try to start anyway. Terminating here is very bad because rejoin is not possible then.
		// Unsubscribe lobby messages.
		client->UnsubscribeLobbyMsg();
		client->SetState(ClientStateWaitStart::Instance());
		// Forward the game start message to the next state.
		client->GetState().HandlePacket(client, tmpPacket);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_InviteNotifyMessage) {
		const InviteNotifyMessage &netInvNotify = tmpPacket->GetMsg()->invitenotifymessage();
		qDebug() << "[INVITE] ClientStateSynchronizeStart: InviteNotifyMessage DROPPED"
		         << "playeridwho=" << netInvNotify.playeridwho()
		         << "gameid=" << netInvNotify.gameid()
		         << "isSelf=" << (netInvNotify.playeridwho() == client->GetGuiPlayerId());
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitStart &
ClientStateWaitStart::Instance()
{
	static ClientStateWaitStart state;
	return state;
}

ClientStateWaitStart::ClientStateWaitStart()
{
}

ClientStateWaitStart::~ClientStateWaitStart()
{
}

void
ClientStateWaitStart::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitStart::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitStart::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameStartInitialMessage
			|| tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameStartRejoinMessage) {
		PlayerIdList tmpPlayerList;
		unsigned tmpHandId = 0;

		if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameStartInitialMessage) {
			// Start the network game as client.
			const GameStartInitialMessage &netStartModeInitial = tmpPacket->GetMsg()->gamestartinitialmessage();

			StartData startData;
			startData.startDealerPlayerId = netStartModeInitial.startdealerplayerid();
			startData.numberOfPlayers = netStartModeInitial.playerseats_size();
			client->SetStartData(startData);

			// Set player numbers using the game start data slots.
			unsigned numPlayers = netStartModeInitial.playerseats_size();
			// Request player info for players if needed.
			if (numPlayers) {
				for (unsigned i = 0; i < numPlayers; i++) {
					unsigned playerId = netStartModeInitial.playerseats(i);
					boost::shared_ptr<PlayerData> tmpPlayer = client->GetPlayerDataByUniqueId(playerId);
					if (!tmpPlayer)
						throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
					tmpPlayer->SetNumber(i);
				}
			} else {
				throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PLAYER_COUNT, 0);
			}
		} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_GameStartRejoinMessage) {
			const GameStartRejoinMessage &netStartModeRejoin = tmpPacket->GetMsg()->gamestartrejoinmessage();

			StartData startData;
			startData.startDealerPlayerId = netStartModeRejoin.startdealerplayerid();
			startData.numberOfPlayers = netStartModeRejoin.rejoinplayerdata_size();
			client->SetStartData(startData);
			tmpHandId = netStartModeRejoin.handnum();

			// Set player numbers using the game start data slots.
			unsigned numPlayers = netStartModeRejoin.rejoinplayerdata_size();
			// Request player info for players if needed.
			if (numPlayers) {
				for (unsigned i = 0; i < numPlayers; i++) {
					const GameStartRejoinMessage::RejoinPlayerData &playerData = netStartModeRejoin.rejoinplayerdata(i);
					boost::shared_ptr<PlayerData> tmpPlayer = client->GetPlayerDataByUniqueId(playerData.playerid());
					if (!tmpPlayer) {
						// If the player is not found: The corresponding session left. We need to create a generic player object.
						// In order to have a complete seat list, we need all players, even those who left.
						tmpPlayer = client->CreatePlayerData(playerData.playerid(), false);
						client->AddPlayerData(tmpPlayer);
						tmpPlayerList.push_back(playerData.playerid());
					}
					tmpPlayer->SetNumber(i);
					tmpPlayer->SetStartCash(playerData.playermoney());
				}
			} else
				throw ClientException(__FILE__, __LINE__, ERR_NET_INVALID_PLAYER_COUNT, 0);
		}
		client->InitGame();
		client->GetGame()->setCurrentHandID(tmpHandId);
		// We need to remove the temporary player data objects after creating the game.
		BOOST_FOREACH(unsigned tmpPlayerId, tmpPlayerList) {
			client->RemovePlayerData(tmpPlayerId, NTF_NET_REMOVED_ON_REQUEST);
		}
		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_START);
		client->SetState(ClientStateWaitHand::Instance());
	}
}

//-----------------------------------------------------------------------------

ClientStateWaitHand &
ClientStateWaitHand::Instance()
{
	static ClientStateWaitHand state;
	return state;
}

ClientStateWaitHand::ClientStateWaitHand()
{
}

ClientStateWaitHand::~ClientStateWaitHand()
{
}

void
ClientStateWaitHand::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitHand::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateWaitHand::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_HandStartMessage) {
		// Hand was started.
		// These are the cards. Good luck.
		const HandStartMessage &netHandStart = tmpPacket->GetMsg()->handstartmessage();
		// -1 = "unbekannte Karte" (Rückseite). Der Server schickt einem
		// Zuschauer weder plainCards noch encryptedCards, so dass unten keiner
		// der beiden Zweige greift.
		int myCards[2] = { -1, -1 };
		string userPassword(client->GetContext().GetPassword());
		if (netHandStart.has_plaincards() && userPassword.empty()) {
			const HandStartMessage::PlainCards &plainCards = netHandStart.plaincards();
			myCards[0] = (int)plainCards.plaincard1();
			myCards[1] = (int)plainCards.plaincard2();
		} else if (netHandStart.has_encryptedcards() && !userPassword.empty()) {
			const string &encryptedCards = netHandStart.encryptedcards();
			string plainCards;
			if (!CryptHelper::AES128Decrypt((const unsigned char *)userPassword.c_str(),
											(unsigned)userPassword.size(),
											(const unsigned char *)encryptedCards.data(),
											(unsigned)encryptedCards.size(),
											plainCards)) {
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			}
			istringstream cardDataStream(plainCards);
			unsigned tmpPlayerId, tmpGameId;
			int tmpHandNum;
			cardDataStream >> tmpPlayerId;
			cardDataStream >> tmpGameId;
			cardDataStream >> tmpHandNum;
			if (tmpPlayerId != client->GetGuiPlayerId()
					|| tmpGameId != client->GetGameId()
					|| tmpHandNum != client->GetGame()->getCurrentHandID() + 1) {
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			}
			cardDataStream >> myCards[0];
			cardDataStream >> myCards[1];
		}
		// Retrieve state for each seat (not based on player id).
		unsigned numPlayers = netHandStart.seatstates_size();

		// IMPORTANT: Save seat states but do NOT apply them yet.
		// We must first stop the GUI animation timers (prepareForNewHand)
		// before modifying any shared player state. Otherwise, a running
		// post-river animation (e.g., All-In showdown) may call refresh
		// functions that read the NEW hand's session states while still
		// displaying the OLD hand, causing players to appear "offline"
		// even though they were active in the previous hand.
		struct SeatStateEntry {
			boost::shared_ptr<PlayerInterface> player;
			NetPlayerState state;
		};
		std::vector<SeatStateEntry> pendingSeatStates;
		pendingSeatStates.reserve(numPlayers);

		for (int i = 0; i < (int)numPlayers; i++) {
			NetPlayerState seatState = netHandStart.seatstates(i);
			int numberDiff = client->GetStartData().numberOfPlayers - client->GetOrigGuiPlayerNum();
			boost::shared_ptr<PlayerInterface> tmpPlayer = client->GetGame()->getPlayerByNumber((i + numberDiff) % client->GetStartData().numberOfPlayers);
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			pendingSeatStates.push_back({tmpPlayer, seatState});
		}

		// Reset all player cards before starting new hand to avoid showing old cards from previous hand
		PlayerListIterator it = client->GetGame()->getSeatsList()->begin();
		PlayerListIterator end = client->GetGame()->getSeatsList()->end();
		int emptyCards[2] = {-1, -1};
		while (it != end) {
			(*it)->setMyCards(emptyCards);
			(*it)->setMyCardsValueInt(0);
			// Also clear sets for all players at hand start to remove stale bet displays
			(*it)->setMySetNull();
			++it;
		}

		// Basic synchronisation before a new hand is started.
		// CRITICAL: Signal the GUI thread to stop all running animation
		// timers BEFORE we modify the shared game state (activePlayerList)
		// in initHand().  The post-river animation chain may still be
		// iterating over activePlayerList in the GUI thread; erasing
		// elements from the network thread would invalidate those
		// iterators and crash – especially on Windows.
		client->GetGui().prepareForNewHand();

		// NOW apply seat states after GUI timers have been stopped.
		// This ensures no running animation can observe the new session states
		// while still rendering the previous hand.
		for (size_t s = 0; s < pendingSeatStates.size(); s++) {
			const SeatStateEntry &entry = pendingSeatStates[s];
			const char* stateStr = (entry.state == netPlayerStateNormal) ? "Normal" :
				(entry.state == netPlayerStateSessionInactive) ? "SessionInactive" :
				(entry.state == netPlayerStateNoMoney) ? "NoMoney" : "Unknown";
			switch (entry.state) {
			case netPlayerStateNormal :
				entry.player->setIsSessionActive(true);
				// CRITICAL FIX: Restore myActiveStatus when server says player is Normal.
				// Without this, a player whose myActiveStatus was set to false (e.g., due
				// to a transient cash=0 state or desync) would remain invisible forever
				// because no other code path restores it -- the "ghost player" bug.
				if (!entry.player->getMyActiveStatus()) {
					entry.player->setMyActiveStatus(true);
					// Re-add to activePlayerList if not already present
					bool found = false;
					PlayerList apl = client->GetGame()->getActivePlayerList();
					for (PlayerListConstIterator aIt = apl->begin(); aIt != apl->end(); ++aIt) {
						if ((*aIt)->getMyUniqueID() == entry.player->getMyUniqueID()) {
							found = true;
							break;
						}
					}
					if (!found) {
						apl->push_back(entry.player);
					}
				}
				break;
			case netPlayerStateSessionInactive :
				entry.player->setIsSessionActive(false);
				break;
			case netPlayerStateNoMoney :
				entry.player->setMyCash(0);
				entry.player->setMySetNull();
				entry.player->setMyActiveStatus(false);
				break;
			}
		}
		// CRITICAL: Refresh Set display BEFORE starting hand to clear stale bets from eliminated players
		client->GetGui().refreshSet();
		// Start new hand. Sitz 0 ist der GUI-Spieler – nur er kennt seine
		// Hole Cards. Als Zuschauer sitzt dort ein fremder Spieler, dessen
		// Karten verdeckt bleiben (bereits oben auf -1 zurückgesetzt).
		if (!client->IsSpectating()) {
			client->GetGame()->getSeatsList()->front()->setMyCards(myCards);
		}
		client->GetGame()->initHand();
		qDebug() << "[ACTDBG] initHand done, new handID=" << client->GetGame()->getCurrentHandID()
		         << "player0Action=" << client->GetGame()->getSeatsList()->front()->getMyAction();
		client->GetGame()->getCurrentHand()->setSmallBlind(netHandStart.smallblind());
		client->GetGame()->getCurrentHand()->getCurrentBeRo()->setMinimumRaise(2 * netHandStart.smallblind());
		client->GetGame()->startHand();
		client->GetGui().dealHoleCards();
		client->GetGui().refreshGameLabels(GAME_STATE_PREFLOP);
		client->GetGui().refreshPot();
		client->GetGui().refreshCash(); // CRITICAL: Update cash display after hand start (fixes Qt6 timing issue)
		client->GetGui().waitForGuiUpdateDone();

		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_HAND_START);
		client->SetState(ClientStateRunHand::Instance());
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_EndOfGameMessage) {
		boost::shared_ptr<Game> curGame = client->GetGame();
		if (curGame) {
			const EndOfGameMessage &netEndOfGame = tmpPacket->GetMsg()->endofgamemessage();

			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(netEndOfGame.winnerplayerid());
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			client->GetGui().logPlayerWinGame(tmpPlayer->getMyName(), curGame->getMyGameID());
			// Flush log to ensure all data is written before leaving game
			client->GetClientLog()->flushLog();
			// CRITICAL: Stop all GUI animation timers before transitioning
			// to the lobby.  The lobby dialog runs a nested Qt event loop
			// (exec()), during which pending timer events would still fire.
			// If the user then closes the lobby dialog, terminateNetworkClient()
			// resets the game object and those timer callbacks would
			// dereference a null pointer -> crash.
			client->GetGui().prepareForNewHand();
			// Resubscribe Lobby messages.
			client->ResubscribeLobbyMsg();
			// Show Lobby dialog.
			client->GetCallback().SignalNetClientWaitDialog();
			client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_END);
			client->SetState(ClientStateWaitGame::Instance());
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AfterHandShowCardsMessage) {
		const AfterHandShowCardsMessage &showCards = tmpPacket->GetMsg()->afterhandshowcardsmessage();
		const PlayerResult &r = showCards.playerresult();

		boost::shared_ptr<PlayerInterface> tmpPlayer = client->GetGame()->getPlayerByUniqueId(r.playerid());
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		int tmpCards[2];
		int bestHandPos[5];
		tmpCards[0] = static_cast<int>(r.resultcard1());
		tmpCards[1] = static_cast<int>(r.resultcard2());
		tmpPlayer->setMyCards(tmpCards);
		for (int num = 0; num < 5; num++) {
			bestHandPos[num] = r.besthandposition(num);
		}
		if (r.cardsvalue()) {
			tmpPlayer->setMyCardsValueInt(r.cardsvalue());
		}
		tmpPlayer->setMyBestHandPosition(bestHandPos);
		tmpPlayer->setMyCash(r.playermoney());
		tmpPlayer->setLastMoneyWon(r.moneywon());

		client->GetCallback().SignalNetClientPostRiverShowCards(r.playerid());
		client->GetClientLog()->logHoleCardsHandName(client->GetGame()->getActivePlayerList(), tmpPlayer, true);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_PlayerIdChangedMessage) {
		boost::shared_ptr<Game> curGame = client->GetGame();
		if (curGame) {
			// Perform Id change.
			const PlayerIdChangedMessage &idChanged = tmpPacket->GetMsg()->playeridchangedmessage();
			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(idChanged.oldplayerid());
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);
			tmpPlayer->setMyUniqueID(idChanged.newplayerid());
			// This player is now active again.
			tmpPlayer->setMyStayOnTableStatus(true);
			// Also update the dealer, if necessary.
			curGame->replaceDealer(idChanged.oldplayerid(), idChanged.newplayerid());
			// Update the player name, if necessary.
			PlayerInfo info;
			if (client->GetCachedPlayerInfo(idChanged.newplayerid(), info)) {
				tmpPlayer->setMyName(info.playerName);
			}
		}
	}
}

//-----------------------------------------------------------------------------

ClientStateRunHand &
ClientStateRunHand::Instance()
{
	static ClientStateRunHand state;
	return state;
}

ClientStateRunHand::ClientStateRunHand()
{
}

ClientStateRunHand::~ClientStateRunHand()
{
}

void
ClientStateRunHand::Enter(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateRunHand::Exit(boost::shared_ptr<ClientThread> /*client*/)
{
}

void
ClientStateRunHand::InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket)
{
	boost::shared_ptr<Game> curGame = client->GetGame();
	if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_PlayersActionDoneMessage) {
		const PlayersActionDoneMessage &netActionDone = tmpPacket->GetMsg()->playersactiondonemessage();

		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(netActionDone.playerid());
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		bool isBigBlind = false;

		if (netActionDone.gamestate() == netStatePreflopSmallBlind) {
			curGame->getCurrentHand()->getCurrentBeRo()->setSmallBlindPositionId(tmpPlayer->getMyUniqueID());
			tmpPlayer->setMyButton(BUTTON_SMALL_BLIND);
		} else if (netActionDone.gamestate() == netStatePreflopBigBlind) {
			curGame->getCurrentHand()->getCurrentBeRo()->setBigBlindPositionId(tmpPlayer->getMyUniqueID());
			tmpPlayer->setMyButton(BUTTON_BIG_BLIND);
			isBigBlind = true;
		} else { // no blind -> log
			if (netActionDone.playeraction()) {
				// Defensive: Clamp totalplayerbet to prevent negative values
				int betAmount = std::max(0, (int)netActionDone.totalplayerbet() - tmpPlayer->getMySet());
				client->GetGui().logPlayerActionMsg(
					tmpPlayer->getMyName(),
					netActionDone.playeraction(),
					betAmount);
				client->GetClientLog()->logPlayerAction(
					tmpPlayer->getMyName(),
					client->GetClientLog()->transformPlayerActionLog(PlayerAction(netActionDone.playeraction())),
					betAmount
				);
				if (tmpPlayer->getMyID() == 0) {
					client->EndPing();
				}
			}
			// Update last players turn only after the blinds.
			curGame->getCurrentHand()->setPreviousPlayerID(tmpPlayer->getMyID());
		}

		tmpPlayer->setMyAction(PlayerAction(netActionDone.playeraction()));
		if (tmpPlayer->getMyID() == 0) {
			auto bero1 = curGame->getCurrentHand()->getCurrentBeRo();
			qDebug() << "[ACTDBG] PAD->setAction(p0) action=" << (int)netActionDone.playeraction()
			         << "padGameState=" << (int)netActionDone.gamestate()
			         << "(0=Pre,1=F,2=T,3=R,4=SB,5=BB)"
			         << "curHandID=" << curGame->getCurrentHandID()
			         << "curRound=" << (int)curGame->getCurrentHand()->getCurrentRound()
			         << "button=" << (int)tmpPlayer->getMyButton()
			         << "bbPosId=" << (bero1 ? (int)bero1->getBigBlindPositionId() : -99)
			         << "prevPlayerId=" << curGame->getCurrentHand()->getPreviousPlayerID()
			         << "firstRound=" << (bero1 ? bero1->getFirstRound() : -1);
		}
		// CRITICAL: Only update set if we're still in the same game state
		// After Flop/Turn/River, collectPot() has already been called and sets were cleared
		// Don't restore sets from stale PlayersActionDoneMessage that arrive after card dealing
		GameState currentRound = curGame->getCurrentHand()->getCurrentRound();
		bool shouldUpdateSet = false;
		
		switch (netActionDone.gamestate()) {
			case netStatePreflopSmallBlind:
			case netStatePreflopBigBlind:
			case netStatePreflop:
				shouldUpdateSet = (currentRound == GAME_STATE_PREFLOP);
				break;
			case netStateFlop:
				shouldUpdateSet = (currentRound == GAME_STATE_FLOP);
				break;
			case netStateTurn:
				shouldUpdateSet = (currentRound == GAME_STATE_TURN);
				break;
			case netStateRiver:
				shouldUpdateSet = (currentRound == GAME_STATE_RIVER);
				break;
			default:
				shouldUpdateSet = true;
				break;
		}
		
		// CRITICAL: If message is from a different game state, ignore it completely
		// (e.g., Preflop All-In action arriving during Flop/Turn/River)
		// This prevents stale values from being processed and displayed
		if (!shouldUpdateSet) {
			// Don't modify any game state for stale messages - just ignore them.
			// Previously this cleared the player's set, which could lose valid bet data
			// if collectPot() hadn't been called yet.
			return;
		}
		
		if (shouldUpdateSet) {
			// totalplayerbet is the player's cumulative bet in the current phase.
			// Always use it when available, regardless of remaining cash.
			// This fixes the bug where all-in bets in post-Preflop rounds (Flop/Turn/River)
			// were silently dropped from the pot because playermoney==0 was used as a filter.
			if (netActionDone.totalplayerbet() > 0) {
				tmpPlayer->setMySetAbsolute(netActionDone.totalplayerbet());
			}
		}
		// Cash from valid (non-stale) messages is always authoritative.
		// Stale messages are already filtered by the shouldUpdateSet check above.
		tmpPlayer->setMyCash(netActionDone.playermoney());
		
		
		curGame->getCurrentHand()->getCurrentBeRo()->setHighestSet(netActionDone.highestset());
		curGame->getCurrentHand()->getCurrentBeRo()->setMinimumRaise(netActionDone.minimumraise());
		
		// Track lastActionPlayerID for showdown card reveal logic (same as server)
		PlayerAction action = PlayerAction(netActionDone.playeraction());
		if (action == PLAYER_ACTION_BET || action == PLAYER_ACTION_RAISE) {
			curGame->getCurrentHand()->setLastActionPlayerID(tmpPlayer->getMyUniqueID());
		} else if (action == PLAYER_ACTION_ALLIN) {
			// All-in counts as last action only if it raises the highest set
			if (tmpPlayer->getMySet() > curGame->getCurrentHand()->getCurrentBeRo()->getHighestSet()) {
				curGame->getCurrentHand()->setLastActionPlayerID(tmpPlayer->getMyUniqueID());
			}
		}
		
		// collectSets() and switchRounds() are always called when shouldUpdateSet is true
		// [RACEDBG] These mutate the shared Hand (player-list iteration) on the
		// NETWORK thread. GameHandler::fold()/doActionDone() mutate the same Hand
		// on the GUI thread with no lock. If a "GameHandler::fold" line from a
		// different thread id appears interleaved between BEGIN and END here in
		// pokerth-debug.log, that overlap is the freeze.
		qDebug() << "[RACEDBG] net: collectSets/switchRounds BEGIN handID="
		         << curGame->getCurrentHandID();
		curGame->getCurrentHand()->getBoard()->collectSets();
		curGame->getCurrentHand()->switchRounds();
		qDebug() << "[RACEDBG] net: collectSets/switchRounds END";

		//log blinds sets after setting bigblind-button
		if (isBigBlind) {
			client->GetGui().logNewBlindsSetsMsg(
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getSmallBlindPositionId())->getMySet(),
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getBigBlindPositionId())->getMySet(),
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getSmallBlindPositionId())->getMyName(),
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getBigBlindPositionId())->getMyName());
			client->GetGui().flushLogAtHand();
			client->GetClientLog()->logNewHandMsg(
				curGame->getCurrentHandID(),
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getDealerPosition())->getMyID()+1,
				curGame->getCurrentHand()->getSmallBlind(),
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getSmallBlindPositionId())->getMyID()+1,
				curGame->getCurrentHand()->getSmallBlind()*2,
				curGame->getPlayerByUniqueId(curGame->getCurrentHand()->getCurrentBeRo()->getBigBlindPositionId())->getMyID()+1,
				curGame->getSeatsList()
			);
		}

		// Stop the timeout for the player.
		client->GetGui().stopTimeoutAnimation(tmpPlayer->getMyID());

		// Unmark last player in GUI.
		client->GetGui().refreshGroupbox(tmpPlayer->getMyID(), 3);

		// Refresh GUI
		if (tmpPlayer->getMyID() == 0) {
			qDebug() << "[PADDBG] PAD-for-me triggers disableMyButtons"
			         << "padAction=" << (int)netActionDone.playeraction()
			         << "padGameState=" << (int)netActionDone.gamestate()
			         << "(0=Pre,1=F,2=T,3=R,4=PreSB,5=PreBB)"
			         << "totalBet=" << (int)netActionDone.totalplayerbet()
			         << "highestSet=" << (int)netActionDone.highestset();
			client->GetGui().disableMyButtons();
		}
		client->GetGui().refreshAction(tmpPlayer->getMyID(), tmpPlayer->getMyAction());
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		client->GetGui().refreshCash();
		client->GetGui().refreshButton();
		client->GetGui().updateMyButtonsState();
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_PlayersTurnMessage) {
		const PlayersTurnMessage &netPlayersTurn = tmpPacket->GetMsg()->playersturnmessage();

		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(netPlayersTurn.playerid());
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		// Set round.
		if (curGame->getCurrentHand()->getCurrentRound() != static_cast<GameState>(netPlayersTurn.gamestate())) {
			ResetPlayerActions(*curGame);
			curGame->getCurrentHand()->setCurrentRound(static_cast<GameState>(netPlayersTurn.gamestate()));
			client->GetClientLog()->setCurrentRound(static_cast<GameState>(netPlayersTurn.gamestate()));
			// Refresh actions.
			client->GetGui().refreshSet();
			client->GetGui().refreshAction();
		}

		// Next player's turn.
		// UNIQUE-ID, nicht Sitz-ID: currentPlayersTurnId ist im gesamten Engine-
		// Code eine Unique-ID (LocalBeRoPreflop::run, ClientHand::switchRounds,
		// Game::getCurrentPlayer() → getPlayerByUniqueId()). Hier stand bisher
		// getMyID() (= Sitznummer), sodass der Wert je nach Schreiber aus zwei
		// verschiedenen ID-Räumen kam und als "wer ist am Zug" unbrauchbar war.
		curGame->getCurrentHand()->getCurrentBeRo()->setCurrentPlayersTurnId(tmpPlayer->getMyUniqueID());

		// Mark current player in GUI.
		int guiStatus = 2;
		if (!tmpPlayer->getMyActiveStatus())
			guiStatus = 0;
		else if (tmpPlayer->getMyAction() == PLAYER_ACTION_FOLD)
			guiStatus = 1;
		client->GetGui().refreshGroupbox(tmpPlayer->getMyID(), guiStatus);
		client->GetGui().refreshAction(tmpPlayer->getMyID(), PLAYER_ACTION_NONE);

		// Start displaying the timeout for the player.
		client->GetGui().startTimeoutAnimation(tmpPlayer->getMyID(), client->GetGameData().playerActionTimeoutSec);

		// Sitz 0 ist der GUI-Spieler – aber nur, wenn wir mitspielen. Als
		// Zuschauer sitzt dort ein fremder Spieler; wir sind nie am Zug.
		if (tmpPlayer->getMyID() == 0 && !client->IsSpectating()) {
			auto bero0 = curGame->getCurrentHand()->getCurrentBeRo();
			qDebug() << "[BBDBG] PlayersTurnMessage seat0"
			         << "msgGameState=" << (int)netPlayersTurn.gamestate()
			         << "curRound=" << (int)curGame->getCurrentHand()->getCurrentRound()
			         << "p0Action=" << (int)tmpPlayer->getMyAction()
			         << "p0Button=" << (int)tmpPlayer->getMyButton()
			         << "(1=D,2=SB,3=BB)"
			         << "bbPosId=" << (bero0 ? (int)bero0->getBigBlindPositionId() : -99)
			         << "prevPlayerId=" << curGame->getCurrentHand()->getPreviousPlayerID()
			         << "firstRound=" << (bero0 ? bero0->getFirstRound() : -1)
			         << "highestSet=" << (bero0 ? bero0->getHighestSet() : -1)
			         << "p0Set=" << tmpPlayer->getMySet();
			// Only allow action if player has cash and is not already All-In
			if (tmpPlayer->getMyCash() > 0 || tmpPlayer->getMyAction() != PLAYER_ACTION_ALLIN) {
				client->GetGui().meInAction();
			}
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_DealFlopCardsMessage) {
		const DealFlopCardsMessage &netDealFlop = tmpPacket->GetMsg()->dealflopcardsmessage();

		int tmpCards[5];
		tmpCards[0] = static_cast<int>(netDealFlop.flopcard1());
		tmpCards[1] = static_cast<int>(netDealFlop.flopcard2());
		tmpCards[2] = static_cast<int>(netDealFlop.flopcard3());
		tmpCards[3] = tmpCards[4] = 0;
		curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
		// collectPot() summiert jetzt selbst die Spieler-Sets und setzt sie zurück
		curGame->getCurrentHand()->getBoard()->collectPot();
		// CRITICAL: Immediately refresh and sync GUI BEFORE any other operations
		// to prevent race condition with stale PlayersActionDoneMessages
		client->GetGui().refreshSet();
		client->GetGui().refreshPot();
		client->GetGui().waitForGuiUpdateDone();
		
		curGame->getCurrentHand()->setPreviousPlayerID(-1);
		ResetPlayerSets(*curGame);

		client->GetGui().logDealBoardCardsMsg(GAME_STATE_FLOP, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
		client->GetClientLog()->setCurrentRound(GAME_STATE_FLOP);
		client->GetClientLog()->logBoardCards(tmpCards);
		client->GetGui().refreshGameLabels(GAME_STATE_FLOP);
		client->GetGui().refreshCash();
		client->GetGui().dealBeRoCards(1);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_DealTurnCardMessage) {
		const DealTurnCardMessage &netDealTurn = tmpPacket->GetMsg()->dealturncardmessage();

		int tmpCards[5];
		curGame->getCurrentHand()->getBoard()->getMyCards(tmpCards);
		tmpCards[3] = static_cast<int>(netDealTurn.turncard());
		curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
		// collectPot() summiert jetzt selbst die Spieler-Sets und setzt sie zurück
		curGame->getCurrentHand()->getBoard()->collectPot();
		// CRITICAL: Immediately refresh and sync GUI BEFORE any other operations
		// to prevent race condition with stale PlayersActionDoneMessages
		client->GetGui().refreshSet();
		client->GetGui().refreshPot();
		client->GetGui().waitForGuiUpdateDone();
		
		curGame->getCurrentHand()->setPreviousPlayerID(-1);
		ResetPlayerSets(*curGame);

		client->GetGui().logDealBoardCardsMsg(GAME_STATE_TURN, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
		client->GetClientLog()->setCurrentRound(GAME_STATE_TURN);
		client->GetClientLog()->logBoardCards(tmpCards);
		client->GetGui().refreshGameLabels(GAME_STATE_TURN);
		client->GetGui().refreshCash();
		client->GetGui().dealBeRoCards(2);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_DealRiverCardMessage) {
		const DealRiverCardMessage &netDealRiver = tmpPacket->GetMsg()->dealrivercardmessage();

		int tmpCards[5];
		curGame->getCurrentHand()->getBoard()->getMyCards(tmpCards);
		tmpCards[4] = static_cast<int>(netDealRiver.rivercard());
		curGame->getCurrentHand()->getBoard()->setMyCards(tmpCards);
		// collectPot() summiert jetzt selbst die Spieler-Sets und setzt sie zurück
		curGame->getCurrentHand()->getBoard()->collectPot();
		// CRITICAL: Immediately refresh and sync GUI BEFORE any other operations
		// to prevent race condition with stale PlayersActionDoneMessages
		client->GetGui().refreshSet();
		client->GetGui().refreshPot();
		client->GetGui().waitForGuiUpdateDone();
		
		curGame->getCurrentHand()->setPreviousPlayerID(-1);
		ResetPlayerSets(*curGame);

		client->GetGui().logDealBoardCardsMsg(GAME_STATE_RIVER, tmpCards[0], tmpCards[1], tmpCards[2], tmpCards[3], tmpCards[4]);
		client->GetClientLog()->setCurrentRound(GAME_STATE_RIVER);
		client->GetClientLog()->logBoardCards(tmpCards);
		client->GetGui().refreshGameLabels(GAME_STATE_RIVER);
		client->GetGui().refreshCash();
		client->GetGui().dealBeRoCards(3);
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_AllInShowCardsMessage) {
		const AllInShowCardsMessage &netAllInShow = tmpPacket->GetMsg()->allinshowcardsmessage();

		curGame->getCurrentHand()->setAllInCondition(true);

		// Set player numbers using the game start data slots.
		unsigned numPlayers = netAllInShow.playersallin_size();
		qDebug() << "[ALLIN] AllInShowCardsMessage received numPlayers=" << numPlayers
		         << "round=" << (int)curGame->getCurrentHand()->getCurrentRound();
		// Request player info for players if needed.
		for (unsigned i = 0; i < numPlayers; i++) {
			const AllInShowCardsMessage::PlayerAllIn &p = netAllInShow.playersallin(i);

			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(p.playerid());
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

			int tmpCards[2];
			tmpCards[0] = static_cast<int>(p.allincard1());
			tmpCards[1] = static_cast<int>(p.allincard2());
			tmpPlayer->setMyCards(tmpCards);
			qDebug() << "[ALLIN]   seatId=" << tmpPlayer->getMyID()
			         << "uniqueId=" << p.playerid()
			         << "cards=" << tmpCards[0] << "/" << tmpCards[1]
			         << "action=" << (int)tmpPlayer->getMyAction();
		}
		qDebug() << "[ALLIN] calling flipHolecardsAllIn()";
		client->GetGui().flipHolecardsAllIn();
		if(curGame->getCurrentHand()->getCurrentRound()<GAME_STATE_RIVER) {
			client->GetClientLog()->logHoleCardsHandName(
				curGame->getActivePlayerList()
			);
		}
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_EndOfHandHideCardsMessage) {
		const EndOfHandHideCardsMessage &hideCards = tmpPacket->GetMsg()->endofhandhidecardsmessage();
		// collectPot() summiert jetzt selbst die Spieler-Sets und setzt sie zurück
		curGame->getCurrentHand()->getBoard()->collectPot();
		// Reset player sets
		ResetPlayerSets(*curGame);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		// Synchronize with GUI.
		client->GetGui().waitForGuiUpdateDone();

		// End of Hand, but keep cards hidden.
		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(hideCards.playerid());
		if (!tmpPlayer)
			throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		tmpPlayer->setMyCash(hideCards.playermoney());
		tmpPlayer->setLastMoneyWon(hideCards.moneywon());
		list<unsigned> winnerList;
		winnerList.push_back(tmpPlayer->getMyUniqueID());

		curGame->getCurrentHand()->getBoard()->setPot(0);
		curGame->getCurrentHand()->getBoard()->setWinners(winnerList);

		// logging
		client->GetClientLog()->logHandWinner(curGame->getActivePlayerList(), tmpPlayer->getMyCardsValueInt(), winnerList);

		client->GetGui().postRiverRunAnimation1();

		// Wait for next Hand.
		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_SERVER_HAND_END);
		client->SetState(ClientStateWaitHand::Instance());

		// logging
		client->GetClientLog()->logPlayerSitsOut(curGame->getActivePlayerList());
		client->GetClientLog()->logGameWinner(curGame->getActivePlayerList());
		client->GetClientLog()->logAfterHand();
	} else if (tmpPacket->GetMsg()->messagetype() == PokerTHMessage::Type_EndOfHandShowCardsMessage) {
		const EndOfHandShowCardsMessage &showCards = tmpPacket->GetMsg()->endofhandshowcardsmessage();


		// collectPot() summiert jetzt selbst die Spieler-Sets und setzt sie zurück
		curGame->getCurrentHand()->getBoard()->collectPot();
		// Reset player sets
		ResetPlayerSets(*curGame);
		client->GetGui().refreshPot();
		client->GetGui().refreshSet();
		// Synchronize with GUI.
		client->GetGui().waitForGuiUpdateDone();

		// End of Hand, show cards.
		list<unsigned> winnerList;
		int highestValueOfCards = 0;
		unsigned numResults = showCards.playerresults_size();
		// Request player info for players if needed.
		for (unsigned i = 0; i < numResults; i++) {
			const PlayerResult &r = showCards.playerresults(i);

			boost::shared_ptr<PlayerInterface> tmpPlayer = curGame->getPlayerByUniqueId(r.playerid());
			if (!tmpPlayer)
				throw ClientException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);


			int tmpCards[2];
			int bestHandPos[5];
			tmpCards[0] = static_cast<int>(r.resultcard1());
			tmpCards[1] = static_cast<int>(r.resultcard2());
			tmpPlayer->setMyCards(tmpCards);
			for (int num = 0; num < 5; num++) {
				bestHandPos[num] = r.besthandposition(num);
			}
			if (r.has_cardsvalue()) {
				tmpPlayer->setMyCardsValueInt(r.cardsvalue());
			}
			tmpPlayer->setMyBestHandPosition(bestHandPos);
			// Only consider non-folded players for highest hand value display
			// (folded players may have higher card values but shouldn't affect the shown hand name)
			if (tmpPlayer->getMyAction() != PLAYER_ACTION_FOLD && tmpPlayer->getMyCardsValueInt() > highestValueOfCards)
				highestValueOfCards = tmpPlayer->getMyCardsValueInt();
			tmpPlayer->setMyCash(r.playermoney());
			tmpPlayer->setLastMoneyWon(r.moneywon());
			
			
			if (r.moneywon())
				winnerList.push_back(r.playerid());
		}

		// Let the client determine which players need to show cards based on poker rules
		// (instead of showing all players from playerresults)
		curGame->getCurrentHand()->getBoard()->determinePlayerNeedToShowCards();
		std::list<unsigned> showList = curGame->getCurrentHand()->getBoard()->getPlayerNeedToShowCards();

		// NOTE: Do NOT call ResetPlayerActions here!
		// The GUI needs the FOLD actions intact to decide which cards to show in postRiverRunAnimation2().
		// Player actions are reset properly in initHand() when the next hand starts.

		curGame->getCurrentHand()->setCurrentRound(GAME_STATE_POST_RIVER);
		client->GetClientLog()->setCurrentRound(GAME_STATE_POST_RIVER);
		curGame->getCurrentHand()->getCurrentBeRo()->setHighestCardsValue(highestValueOfCards);
		curGame->getCurrentHand()->getBoard()->setPot(0);
		curGame->getCurrentHand()->getBoard()->setWinners(winnerList);
		curGame->getCurrentHand()->getBoard()->setPlayerNeedToShowCards(showList);

		// refreshSet() clears stale bet chips; refreshCash() is intentionally
		// deferred to postRiverRunAnimation3() so that the stack update coincides
		// with the Winner badge appearing (not before).
		client->GetGui().refreshSet();
		client->GetGui().waitForGuiUpdateDone();

		// logging
		client->GetClientLog()->logHoleCardsHandName(curGame->getActivePlayerList());
		client->GetClientLog()->logHandWinner(curGame->getActivePlayerList(), highestValueOfCards, winnerList);

		client->GetGui().postRiverRunAnimation1();

		// Wait for next Hand.
		client->GetCallback().SignalNetClientGameInfo(MSG_NET_GAME_CLIENT_HAND_END);
		client->SetState(ClientStateWaitHand::Instance());

		// logging
		client->GetClientLog()->logPlayerSitsOut(curGame->getActivePlayerList());
		client->GetClientLog()->logGameWinner(curGame->getActivePlayerList());
		client->GetClientLog()->logAfterHand();
	}

	// Synchronize with GUI.
	client->GetGui().waitForGuiUpdateDone();
}

void
ClientStateRunHand::ResetPlayerActions(Game &curGame)
{
    PlayerListIterator i = curGame.getActivePlayerList()->begin();
    PlayerListIterator end = curGame.getActivePlayerList()->end();
    while (i != end) {
        // WICHTIG: FOLD-Actions NIEMALS zurücksetzen!
        // Sonst wissen wir beim Showdown nicht mehr wer gefoldet hat.
        if ((*i)->getMyAction() != PLAYER_ACTION_FOLD) {
            (*i)->setMyAction(PLAYER_ACTION_NONE);
        }
        ++i;
    }
}

void
ClientStateRunHand::ResetPlayerSets(Game &curGame)
{
    // CRITICAL: Iterate over ALL players in seats, not just active players
    // Eliminated players (with $0) are not in active list but still need their sets cleared
    PlayerListIterator i = curGame.getSeatsList()->begin();
    PlayerListIterator end = curGame.getSeatsList()->end();
    while (i != end) {
        (*i)->setMySetNull();
        ++i;
    }
}

ClientStateFinal &
ClientStateFinal::Instance()
{
    static ClientStateFinal state;
    return state;
}

