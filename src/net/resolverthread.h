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
/* Name resolution thread. */

#ifndef _RESOLVERTHREAD_H_
#define _RESOLVERTHREAD_H_

#include <core/thread.h>
#include <string>
#include <boost/shared_ptr.hpp>

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

