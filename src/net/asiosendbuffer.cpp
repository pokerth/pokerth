/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2013 Felix Hammer, Florian Thauer, Lothar May          *
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

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/ssl.hpp>

#include <net/asiosendbuffer.h>
#include <net/sessiondata.h>
#include <net/netpacket.h>
#include <core/loghelper.h>
#if BOOST_VERSION >= 108400
#include <boost/core/invoke_swap.hpp>
#else
#include <boost/swap.hpp>
#endif
#include <cstring> // memcpy

using namespace std;


AsioSendBuffer::AsioSendBuffer()
	: sendBuf(NULL), curWriteBuf(NULL), sendBufAllocated(0), sendBufUsed(0),
	  curWriteBufAllocated(0), curWriteBufUsed(0), closeAfterSend(false)
{
}

AsioSendBuffer::~AsioSendBuffer()
{
	free(sendBuf);
	free(curWriteBuf);
}

void
AsioSendBuffer::SetCloseAfterSend()
{
	closeAfterSend = true;
}

void
AsioSendBuffer::HandleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const boost::system::error_code &error)
{
	try {
		// Prüfe ob Socket noch gültig ist
		if (!socket) {
			boost::mutex::scoped_lock lock(dataMutex);
			curWriteBufUsed = 0;
			sendBufUsed = 0;
			closeAfterSend = false;
			return;
		}
		
		if (!error) {
			// Successfully sent the data.
			boost::mutex::scoped_lock lock(dataMutex);
			curWriteBufUsed = 0;
			// Send more data, if available.
			AsyncSendNextPacket(socket);
		} else if (error == boost::asio::error::operation_aborted) {
			// Operation abgebrochen - Puffer leeren, nicht mehr senden
			boost::mutex::scoped_lock lock(dataMutex);
			curWriteBufUsed = 0;
			sendBufUsed = 0;
			closeAfterSend = false;
		} else {
			// Write error - log and close socket
			LOG_ERROR("HandleWrite error: " << error.message());
			boost::mutex::scoped_lock lock(dataMutex);
			curWriteBufUsed = 0;
			sendBufUsed = 0;  // Discard pending data
			closeAfterSend = false;
			try {
				if (socket->is_open()) {
					boost::system::error_code ec;
					socket->close(ec);
				}
			} catch (...) {}
		}
	} catch (const std::exception& e) {
		LOG_ERROR("Exception in HandleWrite: " << e.what());
		boost::mutex::scoped_lock lock(dataMutex);
		curWriteBufUsed = 0;
		sendBufUsed = 0;
		closeAfterSend = false;
	} catch (...) {
		LOG_ERROR("Unknown exception in HandleWrite");
		boost::mutex::scoped_lock lock(dataMutex);
		curWriteBufUsed = 0;
		sendBufUsed = 0;
		closeAfterSend = false;
	}
}

// Implementierung mit exakt passender Signatur (any_io_executor)
void
AsioSendBuffer::HandleWriteSsl(boost::shared_ptr<boost::asio::ssl::stream<boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::any_io_executor>>> sslStream, const boost::system::error_code &error)
{
    try {
        // Prüfe ob SSL-Stream noch gültig ist
        if (!sslStream) {
            boost::mutex::scoped_lock lock(dataMutex);
            curWriteBufUsed = 0;
            sendBufUsed = 0;
            closeAfterSend = false;
            return;
        }
        
        if (!error) {
            boost::mutex::scoped_lock lock(dataMutex);
            curWriteBufUsed = 0;
            // Weiter senden (ruft die passende SSL-Send-Funktion)
            AsyncSendNextPacketSsl(sslStream);
        } else if (error == boost::asio::error::operation_aborted) {
            // Operation abgebrochen - Puffer leeren, nicht mehr senden
            boost::mutex::scoped_lock lock(dataMutex);
            curWriteBufUsed = 0;
            sendBufUsed = 0;
            closeAfterSend = false;
        } else {
            // Write error - log and close socket
            LOG_ERROR("HandleWriteSsl error: " << error.message());
            boost::mutex::scoped_lock lock(dataMutex);
            curWriteBufUsed = 0;
            sendBufUsed = 0;  // Discard pending data
            closeAfterSend = false;
            try {
                if (sslStream->lowest_layer().is_open()) {
                    boost::system::error_code ec;
                    sslStream->lowest_layer().close(ec);
                }
            } catch (...) {}
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in HandleWriteSsl: " << e.what());
        boost::mutex::scoped_lock lock(dataMutex);
        curWriteBufUsed = 0;
        sendBufUsed = 0;
        closeAfterSend = false;
    } catch (...) {
        LOG_ERROR("Unknown exception in HandleWriteSsl");
        boost::mutex::scoped_lock lock(dataMutex);
        curWriteBufUsed = 0;
        sendBufUsed = 0;
        closeAfterSend = false;
    }
}

void
AsioSendBuffer::AsyncSendNextPacket(boost::shared_ptr<SessionData> session)
{
    try {
        if (!session) return;
        
        // Prüfe ob Session noch offen ist
        if (session->GetState() == SessionData::Closed) {
            // Session bereits geschlossen, Puffer leeren
            curWriteBufUsed = 0;
            sendBufUsed = 0;
            closeAfterSend = false;
            return;
        }
        
        if (session->IsSsl()) {
            auto sslStream = session->GetSslStream();
            if (sslStream && sslStream->lowest_layer().is_open()) {
                AsyncSendNextPacketSsl(sslStream);
            } else {
                // Socket geschlossen, Puffer leeren
                curWriteBufUsed = 0;
                sendBufUsed = 0;
                closeAfterSend = false;
            }
        } else {
            auto socket = session->GetAsioSocket();
            if (socket && socket->is_open()) {
                AsyncSendNextPacket(socket);
            } else {
                // Socket geschlossen, Puffer leeren
                curWriteBufUsed = 0;
                sendBufUsed = 0;
                closeAfterSend = false;
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in AsyncSendNextPacket: " << e.what());
        curWriteBufUsed = 0;
        sendBufUsed = 0;
        closeAfterSend = false;
    } catch (...) {
        LOG_ERROR("Unknown exception in AsyncSendNextPacket");
        curWriteBufUsed = 0;
        sendBufUsed = 0;
        closeAfterSend = false;
    }
}

void
AsioSendBuffer::AsyncSendNextPacket(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    // KRITISCH: Prüfe ob Socket gültig und offen ist BEVOR wir async_write starten
    if (!socket) {
        curWriteBufUsed = 0;
        sendBufUsed = 0;
        closeAfterSend = false;
        return;
    }
    
    try {
        if (!socket->is_open()) {
            // Socket bereits geschlossen - Puffer leeren und beenden
            curWriteBufUsed = 0;
            sendBufUsed = 0;
            closeAfterSend = false;
            return;
        }
    } catch (...) {
        // Exception beim Prüfen - Socket ist wahrscheinlich ungültig
        curWriteBufUsed = 0;
        sendBufUsed = 0;
        closeAfterSend = false;
        return;
    }
    
    if (!curWriteBufUsed) {
        // Swap buffers and send data.
#if BOOST_VERSION >= 108400
		boost::core::invoke_swap(curWriteBuf, sendBuf);
		boost::core::invoke_swap(curWriteBufAllocated, sendBufAllocated);
		boost::core::invoke_swap(curWriteBufUsed, sendBufUsed);
#else
		boost::swap(curWriteBuf, sendBuf);
		boost::swap(curWriteBufAllocated, sendBufAllocated);
		boost::swap(curWriteBufUsed, sendBufUsed);
#endif
        if (curWriteBufUsed) {
            try {
                // WICHTIG: Erneut prüfen ob Socket noch offen (Race Condition vermeiden)
                if (!socket->is_open()) {
                    curWriteBufUsed = 0;
                    return;
                }
                
                boost::asio::async_write(
					*socket,
					boost::asio::buffer(curWriteBuf, curWriteBufUsed),
					boost::bind(&SendBuffer::HandleWrite,
								shared_from_this(),
								socket,
								boost::asio::placeholders::error));
            } catch (const std::exception& e) {
                LOG_ERROR("Exception starting async_write TCP: " << e.what());
                curWriteBufUsed = 0;
            } catch (...) {
                LOG_ERROR("Unknown exception starting async_write TCP");
                curWriteBufUsed = 0;
            }
        } else if (closeAfterSend) {
            try {
                boost::system::error_code ec;
                socket->close(ec);
            } catch (...) {}
            closeAfterSend = false;
        }
    }
}

// AsyncSendNextPacketSsl (angepasste Signatur, falls noch nicht exakt so vorhanden)
void
AsioSendBuffer::AsyncSendNextPacketSsl(boost::shared_ptr<boost::asio::ssl::stream<boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::any_io_executor>>> sslStream)
{
    // KRITISCH: Prüfe ob Stream gültig und Socket offen ist BEVOR wir async_write starten
    if (!sslStream) {
        curWriteBufUsed = 0;
        sendBufUsed = 0;
        closeAfterSend = false;
        return;
    }
    
    try {
        if (!sslStream->lowest_layer().is_open()) {
            // Socket bereits geschlossen - Puffer leeren und beenden
            curWriteBufUsed = 0;
            sendBufUsed = 0;
            closeAfterSend = false;
            return;
        }
    } catch (...) {
        // Exception beim Prüfen - Socket ist wahrscheinlich ungültig
        curWriteBufUsed = 0;
        sendBufUsed = 0;
        closeAfterSend = false;
        return;
    }
    
    if (!curWriteBufUsed) {
#if BOOST_VERSION >= 108400
        boost::core::invoke_swap(curWriteBuf, sendBuf);
        boost::core::invoke_swap(curWriteBufAllocated, sendBufAllocated);
        boost::core::invoke_swap(curWriteBufUsed, sendBufUsed);
#else
        boost::swap(curWriteBuf, sendBuf);
        boost::swap(curWriteBufAllocated, sendBufAllocated);
        boost::swap(curWriteBufUsed, sendBufUsed);
#endif
        if (curWriteBufUsed) {
            try {
                // WICHTIG: Erneut prüfen ob Socket noch offen (Race Condition vermeiden)
                if (!sslStream->lowest_layer().is_open()) {
                    curWriteBufUsed = 0;
                    return;
                }
                
                // shared_from_this() verwenden um sicherzustellen dass das Objekt nicht
                // während des async_write zerstört wird
                boost::asio::async_write(
                    *sslStream,
                    boost::asio::buffer(curWriteBuf, curWriteBufUsed),
                    boost::bind(&AsioSendBuffer::HandleWriteSsl,
                                boost::static_pointer_cast<AsioSendBuffer>(shared_from_this()),
                                sslStream,
                                boost::asio::placeholders::error));
            } catch (const std::exception& e) {
                LOG_ERROR("Exception starting async_write SSL: " << e.what());
                curWriteBufUsed = 0;
            } catch (...) {
                LOG_ERROR("Unknown exception starting async_write SSL");
                curWriteBufUsed = 0;
            }
        } else if (closeAfterSend) {
            try {
                boost::system::error_code ec;
                sslStream->lowest_layer().close(ec);
            } catch (...) {}
            closeAfterSend = false;
        }
    }
}

void
AsioSendBuffer::InternalStorePacket(boost::shared_ptr<SessionData> /*session*/, boost::shared_ptr<NetPacket> packet)
{
	uint32_t packetSize = packet->GetMsg()->ByteSizeLong();
	google::protobuf::uint8 *buf = new google::protobuf::uint8[packetSize + NET_HEADER_SIZE];
	*((uint32_t *)buf) = htonl(packetSize);
	packet->GetMsg()->SerializeWithCachedSizesToArray(&buf[NET_HEADER_SIZE]);
	EncodeToBuf(buf, packetSize + NET_HEADER_SIZE);
	delete[] buf;
}

int
AsioSendBuffer::EncodeToBuf(const void *data, size_t size)
{
    // Realloc buffer if necessary.
    while (GetSendBufLeft() < size) {
        if (!ReallocSendBuf()) {
            return -1;
        }
    }

    AppendToSendBufWithoutCheck((const char*)data, size);

    return 0;
}

// --- Buffer helper implementations ----------------------------------------
size_t
AsioSendBuffer::GetSendBufLeft() const
{
    return (sendBufAllocated > sendBufUsed) ? (sendBufAllocated - sendBufUsed) : 0;
}

bool
AsioSendBuffer::ReallocSendBuf()
{
    // Grow strategy: double until MAX_SEND_BUF_SIZE
    size_t newSize = sendBufAllocated ? sendBufAllocated * 2 : SEND_BUF_FIRST_ALLOC_CHUNKSIZE;
    if (newSize > MAX_SEND_BUF_SIZE)
        newSize = MAX_SEND_BUF_SIZE;
    // If already at max or cannot grow further
    if (newSize <= sendBufAllocated)
        return false;

    char *newBuf = (char*)realloc(sendBuf, newSize);
    if (!newBuf)
        return false;

    sendBuf = newBuf;
    sendBufAllocated = newSize;
    return true;
}

void
AsioSendBuffer::AppendToSendBufWithoutCheck(const char *data, size_t size)
{
    // Caller guarantees enough space.
    memcpy(sendBuf + sendBufUsed, data, size);
    sendBufUsed += size;
}

