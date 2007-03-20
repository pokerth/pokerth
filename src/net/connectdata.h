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
/* Connection data. */

#ifndef _CONNECTDATA_H_
#define _CONNECTDATA_H_

#include <net/socket_helper.h>

class ConnectData
{
public:
	ConnectData();
	~ConnectData();

	SOCKET GetSocket() const
	{return m_sockfd;}
	void SetSocket(SOCKET sockfd)
	{m_sockfd = sockfd;}

	SOCKET ReleaseSocket();

private:
	SOCKET				m_sockfd;
};

#endif
