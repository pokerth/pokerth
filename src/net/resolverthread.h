/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/
/* Name resolution thread. */

#ifndef _RESOLVERTHREAD_H_
#define _RESOLVERTHREAD_H_

#include <string>

#include <core/thread.h>

class ClientContext;

class ResolverThread : public Thread
{
public:
	ResolverThread();
	virtual ~ResolverThread();

	// Set the parameters. Does not do any error checking.
	// To prevent access faults if this thread cannot be
	// terminated, the data is not modified.
	void Init(const ClientContext &context);

	// Retrieve the result of the name resolution.
	// ONLY CALL THIS FUNCTION AFTER THE THREAD TERMINATED.
	// You have been warned...
	bool GetResult(ClientContext &context) const;

protected:

	// Main function of the thread.
	virtual void Main();

	const ClientContext &GetContext() const;
	ClientContext &GetContext();

private:

	boost::shared_ptr<ClientContext> m_context;
	bool m_retVal;
};

#endif

