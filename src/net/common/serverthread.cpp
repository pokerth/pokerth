/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <net/serverthread.h>
#include <net/socket_helper.h>


ServerThread::ServerThread()
{
}

ServerThread::~ServerThread()
{
}

void
ServerThread::Init()
{
	if (IsRunning())
		return; // TODO: throw exception
}

void
ServerThread::Main()
{
	while (!ShouldTerminate())
	{
		// Simple hacked server for testing.
		SOCKET sockfd;
		char buf[1024];
		struct sockaddr_storage servaddr, clientaddr;
		int sockaddr_size = sizeof(struct sockaddr_in);
		int addrFamily = AF_INET;
		int addrSize;

		sockfd = socket(addrFamily, SOCK_STREAM, 0);
		bzero(&servaddr, sizeof(servaddr));
		servaddr.ss_family = addrFamily;

		socket_string_to_addr("0.0.0.0", addrFamily, (struct sockaddr *)&servaddr, sockaddr_size);
		socket_set_port(7234, addrFamily, (struct sockaddr *)&servaddr, sockaddr_size);

		bind(sockfd, (const struct sockaddr *)&servaddr, sockaddr_size);
		listen(sockfd, 1);

		bzero(&clientaddr, sizeof(clientaddr));
		addrSize = sockaddr_size;
		SOCKET conn = accept(sockfd, (struct sockaddr *)&clientaddr, &addrSize);
		CLOSESOCKET(sockfd);
		int ret = recv(conn, buf, sizeof(buf), 0);

		send(conn, buf, ret, 0);

		CLOSESOCKET(conn);
	}
}

