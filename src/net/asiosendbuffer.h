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
/* Buffer for sending network data. */

#ifndef _ASIOSENDBUFFER_H_
#define _ASIOSENDBUFFER_H_

#include <net/sendbuffer.h>
#include <cstdlib>
#include <boost/asio/ssl.hpp>
#include <boost/weak_ptr.hpp>


#define SEND_BUF_FIRST_ALLOC_CHUNKSIZE		4096
#define MAX_SEND_BUF_SIZE					SEND_BUF_FIRST_ALLOC_CHUNKSIZE * 256


class AsioSendBuffer : public SendBuffer
{
public:
    AsioSendBuffer();
    virtual ~AsioSendBuffer();

    virtual void HandleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const boost::system::error_code &error);
    virtual void HandleWriteSsl(boost::shared_ptr<boost::asio::ssl::stream<boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::any_io_executor>>> sslStream, const boost::system::error_code &error);

    virtual void AsyncSendNextPacket(boost::shared_ptr<SessionData> session);
    void AsyncSendNextPacket(boost::shared_ptr<boost::asio::ip::tcp::socket> socket);
    void AsyncSendNextPacketSsl(boost::shared_ptr<boost::asio::ssl::stream<boost::asio::basic_stream_socket<boost::asio::ip::tcp, boost::asio::any_io_executor>>> sslStream);

    virtual void InternalStorePacket(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
    int EncodeToBuf(const void *data, size_t size);

    virtual size_t GetPendingBytes() const;
    virtual bool CheckAndClearOverflow();

    virtual void SetCloseAfterSend();

protected:
    size_t GetSendBufLeft() const;
    bool ReallocSendBuf();
    void AppendToSendBufWithoutCheck(const char *data, size_t size);
    // Close the owning session after a write error (returns false if the
    // session is no longer reachable and the caller must close the raw
    // socket itself). May throw PokerTHException on the client (intended).
    bool CloseSessionOnWriteError(boost::shared_ptr<SessionData> session);

private:
    // Owning session (weak to avoid the SessionData <-> SendBuffer cycle);
    // needed to report write errors as a proper session close instead of
    // silently closing the socket handle.
    boost::weak_ptr<SessionData> m_session;
    char *sendBuf;
    char *curWriteBuf;
    size_t sendBufAllocated;
    size_t sendBufUsed;
    size_t curWriteBufAllocated;
    size_t curWriteBufUsed;
    bool closeAfterSend;
    // Set when a packet did not fit into the send queue any more. Read and
    // cleared by CheckAndClearOverflow().
    bool sendBufOverflow;
};

#endif

