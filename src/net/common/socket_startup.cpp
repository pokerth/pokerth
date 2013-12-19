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

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <net/socket_startup.h>
#include <core/openssl_wrapper.h>

#ifndef HAVE_OPENSSL

// With libgcrypt 1.6.0, it is no longer supported to provide thread callbacks.
// Use the default thread implementation instead, and cross fingers that it works with boost thread...
#if GCRYPT_VERSION_NUMBER < 0x010600

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

	static struct gcry_thread_cbs gcry_threads_boost = {
		GCRY_THREAD_OPTION_USER, gcry_bthread_init, gcry_bmutex_init,
		gcry_bmutex_destroy, gcry_bmutex_lock, gcry_bmutex_unlock,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
	};
}

#endif // GCRYPT_VERSION_NUMBER < 0x010600

#endif // not HAVE_OPENSSL

bool
socket_startup()
{
#ifdef HAVE_OPENSSL
	return SSL_library_init() == 1;
#else
#if GCRYPT_VERSION_NUMBER < 0x010600
	gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_boost);
#endif
	gcry_check_version(NULL);
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

