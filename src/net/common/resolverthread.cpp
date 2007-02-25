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

#include <net/resolverthread.h>
#include <net/clientdata.h>
#include <net/clientexception.h>

#include <sstream>
#include <cassert>

using namespace std;


ResolverThread::ResolverThread()
: m_retVal(false)
{
	m_data.reset(new ClientData);
}

ResolverThread::~ResolverThread()
{
}

void
ResolverThread::Init(const ClientData &data)
{
	if (IsRunning())
		return; // TODO: throw exception

	m_data->addrFamily = data.addrFamily;
	m_data->serverAddr = data.serverAddr;
	m_data->serverPort = data.serverPort;
}

bool
ResolverThread::GetResult(ClientData &data)
{
	if (IsRunning())
		return false; // TODO: throw exception

	if (m_retVal)
		memcpy(&data.clientAddr, &GetData().clientAddr, GetData().GetServerAddrSize());

	return m_retVal;
}

void
ResolverThread::Main()
{
	const ClientData &data = GetData();

	// Convert the port to a string.
	ostringstream tmpStr;
	tmpStr << data.serverPort;

	// Start the name resolution.
	m_retVal = socket_resolve(
		data.serverAddr.c_str(),
		tmpStr.str().c_str(),
		data.addrFamily,
		SOCK_STREAM,
		0,
		(struct sockaddr *)&data.clientAddr,
		data.GetServerAddrSize());
}

const ClientData &
ResolverThread::GetData() const
{
	assert(m_data.get());
	return *m_data;
}

ClientData &
ResolverThread::GetData()
{
	assert(m_data.get());
	return *m_data;
}

