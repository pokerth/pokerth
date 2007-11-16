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
#include <net/clientcontext.h>
#include <net/clientexception.h>

#include <sstream>
#include <cassert>

using namespace std;


ResolverThread::ResolverThread()
: m_retVal(false)
{
	m_context.reset(new ClientContext);
}

ResolverThread::~ResolverThread()
{
}

void
ResolverThread::Init(const ClientContext &context)
{
	if (IsRunning())
		return; // TODO: throw exception

	GetContext().SetAddrFamily(context.GetAddrFamily());
	GetContext().SetServerAddr(context.GetServerAddr());
	GetContext().SetServerPort(context.GetServerPort());
}

bool
ResolverThread::GetResult(ClientContext &context) const
{
	if (IsRunning())
		return false; // TODO: throw exception

	if (m_retVal)
		memcpy(context.GetClientSockaddr(), GetContext().GetClientSockaddr(), GetContext().GetClientSockaddrSize());

	return m_retVal;
}

void
ResolverThread::Main()
{
	const ClientContext &context = GetContext();

	// Convert the port to a string.
	ostringstream tmpStr;
	tmpStr << context.GetServerPort();

	// Start the name resolution.
	m_retVal = socket_resolve(
		context.GetServerAddr().c_str(),
		tmpStr.str().c_str(),
		context.GetAddrFamily(),
		SOCK_STREAM,
		context.GetProtocol(),
		(struct sockaddr *)context.GetClientSockaddr(),
		context.GetClientSockaddrSize());
}

const ClientContext &
ResolverThread::GetContext() const
{
	assert(m_context.get());
	return *m_context;
}

ClientContext &
ResolverThread::GetContext()
{
	assert(m_context.get());
	return *m_context;
}

