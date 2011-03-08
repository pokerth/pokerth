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

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <net/socket_startup.h>
#include <core/openssl_wrapper.h>

#ifndef HAVE_OPENSSL

extern "C" {

	int gcry_bthread_init()
	{
		return 0;
	}
	int gcry_bmutex_init(void **obj)
	{
		*obj = (void*)(new boost::mutex);
		return 0;
	}
	int gcry_bmutex_destroy(void **obj)
	{
		delete (boost::mutex *)(*obj);
		return 0;
	}
	int gcry_bmutex_lock(void **obj)
	{
		((boost::mutex *)(*obj))->lock();
		return 0;
	}
	int gcry_bmutex_unlock(void **obj)
	{
		((boost::mutex *)(*obj))->unlock();
		return 0;
	}

	struct gcry_thread_cbs gcry_threads_boost = {
		GCRY_THREAD_OPTION_USER, gcry_bthread_init, gcry_bmutex_init,
		gcry_bmutex_destroy, gcry_bmutex_lock, gcry_bmutex_unlock,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
	};
}
#endif // not HAVE_OPENSSL

bool
socket_startup()
{
#ifdef HAVE_OPENSSL
	return SSL_library_init() == 1;
#else
	gcry_check_version(NULL);
	gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_boost);
	gcry_control(GCRYCTL_ENABLE_QUICK_RANDOM, 0);
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
	return true;
#endif
}

void
socket_cleanup()
{
}


bool
socket_has_sctp()
{
#ifdef IPPROTO_SCTP
	boost::asio::detail::socket_type test = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

	if (test == boost::asio::detail::invalid_socket)
		return false;
#ifdef _WIN32
	closesocket(test);
#else
	close(test);
#endif
	return true;
#else
	return false;
#endif
}

bool
socket_has_ipv6()
{
	boost::asio::detail::socket_type test = socket(AF_INET6, SOCK_STREAM, 0);

	if (test == boost::asio::detail::invalid_socket)
		return false;
#ifdef _WIN32
	closesocket(test);
#else
	close(test);
#endif
	return true;
}

bool
socket_has_dual_stack()
{
	bool retVal = false;
	boost::asio::detail::socket_type test = socket(AF_INET6, SOCK_STREAM, 0);

	if (test != boost::asio::detail::invalid_socket) {
		int ipv6only = 0;
		if (setsockopt(test, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&ipv6only, sizeof(ipv6only)) == 0)
			retVal = true;
#ifdef _WIN32
		closesocket(test);
#else
		close(test);
#endif
	}
	return retVal;
}

